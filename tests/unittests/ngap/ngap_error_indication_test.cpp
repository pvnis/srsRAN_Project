/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ngap_test_helpers.h"
#include "srsran/support/test_utils.h"
#include <gtest/gtest.h>

using namespace srsran;
using namespace srs_cu_cp;

class ngap_error_indication_test : public ngap_test
{
protected:
  void start_procedure(const ue_index_t ue_index)
  {
    ASSERT_EQ(ngap->get_nof_ues(), 0);
    create_ue(ue_index);

    // Inject DL NAS transport message from AMF
    run_dl_nas_transport(ue_index);

    // Inject UL NAS transport message from RRC
    run_ul_nas_transport(ue_index);

    // Inject Initial Context Setup Request
    run_inital_context_setup(ue_index);
  }

  bool was_error_indication_sent() const
  {
    return msg_notifier.last_ngap_msg.pdu.init_msg().value.type() ==
           asn1::ngap::ngap_elem_procs_o::init_msg_c::types_opts::error_ind;
  }
};

// Note that this is currently a manual test without asserts, as we currently don't handle the error indication

/// Test handling of error indication message for inexisting ue
TEST_F(ngap_error_indication_test,
       when_error_indication_message_for_inexisting_ue_received_message_is_dropped_and_error_indication_is_sent)
{
  // Inject error indication message
  ngap_message error_indication_msg = generate_error_indication_message(uint_to_amf_ue_id(10), uint_to_ran_ue_id(0));
  ngap->handle_message(error_indication_msg);

  // Check that Error Indication has been sent to AMF
  ASSERT_TRUE(was_error_indication_sent());
}

/// Test handling of error indication message for existing ue
TEST_F(ngap_error_indication_test, when_error_indication_message_for_existing_ue_received_message_is_logged)
{
  // Test preamble
  ue_index_t ue_index = uint_to_ue_index(
      test_rgen::uniform_int<uint64_t>(ue_index_to_uint(ue_index_t::min), ue_index_to_uint(ue_index_t::max)));
  this->start_procedure(ue_index);

  auto& ue = test_ues.at(ue_index);

  // Inject error indication message
  ngap_message error_indication_msg = generate_error_indication_message(ue.amf_ue_id.value(), ue.ran_ue_id.value());
  ngap->handle_message(error_indication_msg);
}
