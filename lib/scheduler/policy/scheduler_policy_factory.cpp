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

#include "scheduler_policy_factory.h"
#include "scheduler_time_rr.h"
#include "scheduler_pp.h"

using namespace srsran;

std::unique_ptr<scheduler_policy> srsran::create_scheduler_strategy(const scheduler_strategy_params& params)
{
  
  if (params.strategy == "rr_time") {
    return std::make_unique<scheduler_time_rr>();
  }
  
  if (params.strategy == "pp") {
    return std::make_unique<scheduler_pp>();
  }
  
  else {
    return std::make_unique<scheduler_time_rr>();
  }
  //return std::make_unique<scheduler_time_rr>();
}
