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

#include "srsran/f1u/du/f1u_config.h"
#include "srsran/mac/mac_lc_config.h"
#include "srsran/rlc/rlc_config.h"

namespace srsran {

/// \brief QoS Configuration, i.e. 5QI and the associated RLC configuration for DRBs
struct du_qos_config {
  rlc_config         rlc;
  srs_du::f1u_config f1u;
  mac_lc_config      mac;
};

} // namespace srsran
