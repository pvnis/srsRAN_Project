/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#pragma once

#include "tx_buffer_impl.h"
#include "srsran/adt/ring_buffer.h"
#include "srsran/adt/static_vector.h"
#include "srsran/phy/upper/codeblock_metadata.h"
#include "srsran/phy/upper/tx_buffer_pool.h"
#include "srsran/phy/upper/unique_tx_buffer.h"
#include "srsran/support/error_handling.h"
#include "srsran/support/math_utils.h"
#include <mutex>

namespace srsran {

/// Describes a receiver buffer pool.
class tx_buffer_pool_impl : public tx_buffer_pool
{
public:
  /// \brief Creates a generic receiver buffer pool.
  /// \param[in] config Provides the pool required parameters.
  tx_buffer_pool_impl(const tx_buffer_pool_config& config) :
    logger(srslog::fetch_basic_logger("PHY", true)),
    codeblock_pool(config.nof_codeblocks, config.max_codeblock_size, config.external_soft_bits),
    available_buffers(config.nof_buffers),
    reserved_buffers(config.nof_buffers),
    expire_timeout_slots(config.expire_timeout_slots)
  {
    for (unsigned i = 0, i_end = config.nof_buffers; i != i_end; ++i) {
      buffer_pool.emplace_back(std::make_unique<tx_buffer_impl>(codeblock_pool));
      available_buffers.push(i);
    }
  }

  // See interface for documentation.
  unique_tx_buffer
  reserve_buffer(const slot_point& slot, const tx_buffer_identifier& id, unsigned nof_codeblocks) override;

  // See interface for documentation.
  unique_tx_buffer reserve_buffer(const slot_point& slot, unsigned nof_codeblocks) override;

  // See interface for documentation.
  void run_slot(const slot_point& slot) override;

private:
  /// Buffer identifier for non specified RNTI and HARQ.
  const tx_buffer_identifier unknown_id = {0, 16};

  /// Protects methods from concurrent calls.
  std::mutex mutex;
  /// General physical layer logger.
  srslog::basic_logger& logger;
  /// Codeblock buffer pool.
  tx_buffer_codeblock_pool codeblock_pool;
  /// Actual buffer pool.
  std::vector<std::unique_ptr<tx_buffer_impl>> buffer_pool;
  /// Storage of available buffer identifier.
  ring_buffer<unsigned> available_buffers;
  /// Storage of reserved buffer identifier.
  ring_buffer<unsigned> reserved_buffers;
  /// Indicates the lifetime of a buffer reservation as a number of slots.
  unsigned expire_timeout_slots;
};

} // namespace srsran
