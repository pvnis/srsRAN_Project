/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
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

#include "srsran/adt/optional.h"

namespace srsran {

struct s_quota_t {
  uint32_t needs;   // RB needed by the slice
  uint32_t quota;   // Number of RB allocated to the slice (max 275)
  // optional<uint32_t> remaining;   // Number of RB allocated to the slice (max 275)
  uint32_t leftover = 0;   // Number of RB allocated to the slice (max 275)
  // s_nssai_t s_nssai;
};

} // namespace srsran
