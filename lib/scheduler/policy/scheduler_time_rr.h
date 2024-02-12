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

#include "scheduler_policy.h"
#include "srsran/ran/s_nssai.h"
#include "srsran/ran/slice.h"

namespace srsran {

class scheduler_time_rr : public scheduler_policy
{
public:
  scheduler_time_rr(s_nssai_t nssai, srslog::basic_logger& logger);

  void
  dl_sched(ue_pdsch_allocator& pdsch_alloc, const ue_resource_grid_view& res_grid, const ue_repository& ues) override;

  void
  ul_sched(ue_pusch_allocator& pusch_alloc, const ue_resource_grid_view& res_grid, const ue_repository& ues) override;

  const s_nssai_t& get_s_nssai() const override { return s_nssai; }

  const uint32_t& get_s_quota() const override { return s_quota.quota; }

  const uint32_t& get_s_leftover() const override { return s_quota.leftover; }

  void set_s_nssaiQuota(const uint32_t& newQuota) override { s_quota.quota = newQuota; }

  // void set_s_nssaiRemaining(const uint32_t& newRem) override { s_quota.remaining = newRem; }

  void set_s_nssaiLeftOver(const uint32_t& newLO) override { s_quota.leftover = newLO; }


private:
  s_nssai_t             s_nssai;
  s_quota_t             s_quota;
  srslog::basic_logger& logger;
  du_ue_index_t         next_dl_ue_index, next_ul_ue_index;
};

} // namespace srsran
