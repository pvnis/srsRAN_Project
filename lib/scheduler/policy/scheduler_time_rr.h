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
#include "../ue_scheduling/ue_cell.h"
#include "../ue_scheduling/ue_pdsch_param_candidate_searcher.h"

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

  const uint32_t& get_s_needs() const override { return s_quota.needs; }

  const uint32_t& get_s_quota() const override { return s_quota.quota; }

  const uint32_t& get_s_leftover() const override { return s_quota.leftover; }

  void set_s_nssaiQuota(const uint32_t& newQuota) override { s_quota.quota = newQuota; }

  // void set_s_nssaiRemaining(const uint32_t& newRem) override { s_quota.remaining = newRem; }

  void set_s_nssaiLeftOver(const uint32_t& newLO) override { s_quota.leftover = newLO; }

  void poll_quota(std::vector<std::shared_ptr<ue>> ues_slice,const ue_resource_grid_view& res_grid) override {
    u_int32_t prbs = 0;
    u_int32_t prbs_ue = 0;
    for (auto& ue : ues_slice) {
      for (unsigned i = 0; i != ue->nof_cells(); ++i) {
        const ue_cell&   ue_cc      = ue->get_cell(to_ue_cell_index(i));
        const slot_point pdcch_slot = res_grid.get_pdcch_slot(ue_cc.cell_index);
        ue_pdsch_param_candidate_searcher candidates{*ue, to_ue_cell_index(i), false, pdcch_slot};
        // only use the first candidate to calculate the PRBs
        if (candidates.begin() != candidates.end()){
          const ue_pdsch_param_candidate_searcher::candidate& param_candidate = *candidates.begin();
          const pdsch_time_domain_resource_allocation& pdsch = param_candidate.pdsch_td_res();
          const dci_dl_rnti_config_type dci_type = param_candidate.dci_dl_rnti_cfg_type();
          prbs_ue = ue_cc.required_dl_prbs(pdsch, ue->pending_dl_newtx_bytes(), dci_type).n_prbs;
          logger.debug("Poll: UE {} in slice sst={} sd={} needs {} PRBs", ue_cc.rnti(), s_nssai.sst, s_nssai.sd, prbs_ue);
          prbs += prbs_ue;
        }
        // for (const ue_pdsch_param_candidate_searcher::candidate& param_candidate : candidates) {
        //   const pdsch_time_domain_resource_allocation& pdsch = param_candidate.pdsch_td_res();
        //   const dci_dl_rnti_config_type dci_type = param_candidate.dci_dl_rnti_cfg_type();
        //   prbs += ue_cc.required_dl_prbs(pdsch, ue->pending_dl_newtx_bytes(), dci_type).n_prbs;
        //   break;
        // }
        // logger.debug("Poll: UE {} in slice sst={} sd={} needs {} PRBs", ue_cc.rnti(), s_nssai.sst, s_nssai.sd, prbs);
      }
    }
    s_quota.needs = prbs;
  };

private:
  s_nssai_t             s_nssai;
  s_quota_t             s_quota;
  srslog::basic_logger& logger;
  du_ue_index_t         next_ul_ue_index;
};

} // namespace srsran
