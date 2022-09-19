/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

/// \file
/// \brief PUCCH detector unit test.
///
/// The test reads received symbols and channel coefficients from a test vector, detects a PUCCH Format 1 transmission
/// and compares the resulting bits (SR or HARQ-ACK) with the expected ones.

#include "pucch_detector_test_data.h"
#include "srsgnb/phy/upper/channel_processors/channel_processor_factories.h"

#include <gtest/gtest.h>

using namespace srsgnb;

/// \cond
namespace srsgnb {

std::ostream& operator<<(std::ostream& os, const test_case_t& tc)
{
  std::string hops = (tc.cfg.second_hop_prb.has_value() ? "intraslot frequency hopping" : "no frequency hopping");
  return os << fmt::format("Numerology {}, {}, symbol allocation [{}, {}], {} HARQ-ACK bit(s), {} SR bit(s).",
                           tc.cfg.slot.numerology(),
                           hops,
                           tc.cfg.start_symbol_index,
                           tc.cfg.nof_symbols,
                           tc.cfg.nof_harq_ack,
                           tc.sr_bit.size());
}

} // namespace srsgnb

namespace {

class PUCCHDetectFixture : public ::testing::TestWithParam<test_case_t>
{
protected:
  static void SetUpTestSuite()
  {
    if (!detector_factory) {
      std::shared_ptr<low_papr_sequence_generator_factory> low_papr_gen =
          create_low_papr_sequence_generator_sw_factory();
      std::shared_ptr<low_papr_sequence_collection_factory> low_papr_col =
          create_low_papr_sequence_collection_sw_factory(low_papr_gen);
      std::shared_ptr<pseudo_random_generator_factory> pseudorandom = create_pseudo_random_generator_sw_factory();
      detector_factory = create_pucch_detector_factory_sw(low_papr_col, pseudorandom);
    }
    ASSERT_NE(detector_factory, nullptr);
  }

  void SetUp() override
  {
    // Assert factories again for compatibility with GTest < 1.11.
    ASSERT_NE(detector_factory, nullptr);

    detector_test = detector_factory->create();
    ASSERT_NE(detector_test, nullptr);
  }

  static std::shared_ptr<pucch_detector_factory> detector_factory;
  std::unique_ptr<pucch_detector>                detector_test;
};

std::shared_ptr<pucch_detector_factory> PUCCHDetectFixture::detector_factory = nullptr;

void fill_ch_estimate(channel_estimate& ch_est, const std::vector<resource_grid_reader_spy::expected_entry_t>& entries)
{
  for (const auto& entry : entries) {
    ch_est.set_ch_estimate(entry.value, entry.subcarrier, entry.symbol, entry.port);
  }
}

TEST_P(PUCCHDetectFixture, Format1Test)
{
  test_case_t test_data = GetParam();

  pucch_detector::format1_configuration config = test_data.cfg;

  unsigned                                                nof_res      = config.nof_symbols / 2 * NRE;
  std::vector<resource_grid_reader_spy::expected_entry_t> grid_entries = test_data.received_symbols.read();
  ASSERT_EQ(grid_entries.size(), nof_res) << "The number of grid entries and the number of PUCCH REs do not match";

  resource_grid_reader_spy grid;

  grid.write(grid_entries);

  std::vector<resource_grid_reader_spy::expected_entry_t> channel_entries = test_data.ch_estimates.read();
  ASSERT_EQ(channel_entries.size(), nof_res)
      << "The number of channel estimates and the number of PUCCH REs do not match";

  channel_estimate                              csi;
  channel_estimate::channel_estimate_dimensions ch_dims;
  ch_dims.nof_tx_layers = 1;
  ch_dims.nof_rx_ports  = 1;
  ch_dims.nof_symbols   = MAX_NSYMB_PER_SLOT;
  ch_dims.nof_prb       = MAX_RB;
  csi.resize(ch_dims);

  fill_ch_estimate(csi, channel_entries);

  csi.set_noise_variance(test_data.noise_var, 0);

  std::unique_ptr<pucch_detector> pucch_det = detector_factory->create();

  pucch_uci_message msg = pucch_det->detect(grid, csi, test_data.cfg);

  if (test_data.cfg.nof_harq_ack == 0) {
    if (test_data.sr_bit.empty()) {
      ASSERT_EQ(msg.status, uci_status::invalid) << "An empty PUCCH occasion should return an 'invalid' UCI.";
      return;
    }
    if (test_data.sr_bit[0] == 1) {
      ASSERT_EQ(msg.status, uci_status::valid) << "A positive SR-only PUCCH occasion should return a 'valid' UCI.";
      return;
    }
    ASSERT_EQ(msg.status, uci_status::invalid) << "A negative SR-only PUCCH occasion should return an 'invalid' UCI.";
    return;
  }

  ASSERT_EQ(msg.status, uci_status::valid);

  ASSERT_EQ(msg.harq_ack.size(), test_data.ack_bits.size()) << "Wrong number of HARQ-ACK bits.";
  ASSERT_TRUE(std::equal(msg.harq_ack.begin(), msg.harq_ack.end(), test_data.ack_bits.begin()))
      << "The HARQ-ACK bits do not match.";
}

INSTANTIATE_TEST_SUITE_P(PUCCHDetectorSuite, PUCCHDetectFixture, ::testing::ValuesIn(pucch_detector_test_data));
} // namespace

/// \endcond