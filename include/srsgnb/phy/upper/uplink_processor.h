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
/// \brief Uplink processor interface.

#pragma once

#include "srsgnb/phy/upper/channel_processors/prach_detector.h"
#include "srsgnb/phy/upper/channel_processors/pusch_processor.h"

namespace srsgnb {

class prach_buffer;
struct prach_buffer_context;
class slot_point;

/// \brief Uplink processor interface.
///
/// The uplink processor is in charge of handling incoming requests to process the physical uplink channels within a
/// certain slot.
///
/// Requests are dispatched asynchronously as they get enqueued for execution, and generate an event through the \ref
/// upper_phy_rx_results_notifier interface upon completion.
class uplink_processor
{
public:
  /// Defines the intermediate struct that contains all the data of the PUSCH PDU.
  struct pusch_pdu {
    /// HARQ process number.
    unsigned harq_id;
    /// Transport block size in bytes.
    unsigned tb_size;
    /// PUSCH processor PDU.
    pusch_processor::pdu_t pdu;
  };

  virtual ~uplink_processor() = default;

  /// \brief Processes the PRACH using the given configuration and context.
  ///
  /// The PRACH detection results will be notified by the upper_phy_rx_results_notifier with event \ref
  /// upper_phy_rx_results_notifier::on_new_prach_results "on_new_prach_results".
  ///
  /// \param[in] buffer        Channel symbols the PRACH detection is performed on.
  /// \param[in] context       Context used by the underlying PRACH detector.
  virtual void process_prach(const prach_buffer& buffer, const prach_buffer_context& context) = 0;

  /// \brief Processes a PUSCH transmission.
  ///
  /// The size of each transport block is determined by <tt> data[TB index].size() </tt>.
  ///
  /// \param[out] data Provides the transport block to receive.
  /// \param[in,out] softbuffer Provides the data reception softbuffer.
  /// \param[in] grid Provides the destination resource grid.
  /// \param[in] pdu Provides the necessary parameters to process the PUSCH transmission.
  virtual void process_pusch(span<uint8_t>                      data,
                             rx_softbuffer&                     softbuffer,
                             const resource_grid_reader&        grid,
                             const uplink_processor::pusch_pdu& pdu) = 0;
};

/// \brief Pool of uplink processors.
///
/// This interface manages the access to the available uplink processors.
class uplink_processor_pool
{
public:
  virtual ~uplink_processor_pool() = default;

  /// \brief Returns an uplink processor for the given slot and sector.
  ///
  /// \param slot[in]      Slot point.
  /// \param sector_id[in] Sector identifier.
  /// \return An uplink processor.
  virtual uplink_processor& get_processor(slot_point slot, unsigned sector_id) = 0;
};

} // namespace srsgnb
