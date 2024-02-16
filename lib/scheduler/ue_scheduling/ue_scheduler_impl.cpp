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

#include "ue_scheduler_impl.h"
#include "../policy/scheduler_policy_factory.h"
#include <string>
#include "srsran/ran/subcarrier_spacing.h"

using namespace srsran;

ue_scheduler_impl::ue_scheduler_impl(const scheduler_ue_expert_config& expert_cfg_,
                                     sched_configuration_notifier&     mac_notif,
                                     scheduler_metrics_handler&        metric_handler,
                                     scheduler_event_logger&           sched_ev_logger) :
  expert_cfg(expert_cfg_),
  //sched_strategy(create_scheduler_strategy(scheduler_strategy_params{"time_rr", &srslog::fetch_basic_logger("SCHED")})),
  ue_alloc(expert_cfg, ue_db, srslog::fetch_basic_logger("SCHED")),
  event_mng(ue_db, metric_handler, sched_ev_logger),
  logger(srslog::fetch_basic_logger("SCHED"))
{
  // create scheduler for each slice
  for (const auto& slice : expert_cfg.slice_cfg) {
    slices.push_back(create_scheduler_strategy(scheduler_strategy_params{"time_rr", slice, srslog::fetch_basic_logger("SCHED")}));
  }

  // sort the slices in descending order
  std::sort(slices.begin(), slices.end(), [](const auto& a, const auto& b) { return a->get_s_nssai().sd.value() > b->get_s_nssai().sd.value(); });

  // create default slice
  slices.push_back(create_scheduler_strategy(scheduler_strategy_params{"time_rr", s_nssai_t{0}, srslog::fetch_basic_logger("SCHED")}));
}

void ue_scheduler_impl::add_cell(const ue_scheduler_cell_params& params)
{
  ue_res_grid_view.add_cell(*params.cell_res_alloc);
  cells[params.cell_index] = std::make_unique<cell>(expert_cfg, params, ue_db);
  event_mng.add_cell(*params.cell_res_alloc, cells[params.cell_index]->srb0_sched);
  ue_alloc.add_cell(params.cell_index, *params.pdcch_sched, *params.uci_alloc, *params.cell_res_alloc);
}

void ue_scheduler_impl::run_sched_strategy(slot_point slot_tx, du_cell_index_t cell_index)
{
  // Print resource grid for debugging purposes.
  uint8_t k0 = 0;
  const cell_slot_resource_grid& grid = ue_res_grid_view.get_pdsch_grid(cell_index,k0);
  crb_interval dl_crb_lims{0,51};
  ofdm_symbol_range symbols_lims{1,14};
  //const crb_bitmap used_crbs = grid.used_crbs(subcarrier_spacing::kHz30, dl_crb_lims, symbols_lims);
  //logger.debug("cell={}, slot={}: used CRBs befor scheduling: {}", cell_index, slot_tx, used_crbs);
  logger.debug("cell={}, slot={}: res grid before scheduling: {}", cell_index, slot_tx, grid);
  

  // Update all UEs state.
  ue_db.slot_indication(slot_tx);

  if (not ue_res_grid_view.get_cell_cfg_common(cell_index).is_dl_enabled(slot_tx)) {
    // This slot is inactive for PDCCH in this cell. We therefore, can skip the scheduling strategy.
    // Note: we are currently assuming that all cells have the same TDD pattern and that the scheduling strategy
    // only allocates PDCCHs for the current slot_tx.
    return;
  }

  // Poll slices for desired quotas
  u_int32_t prbs_tot = 0;
  for (const auto& slice : slices) {
    // get sub-list of UE in slice
    std::vector<std::shared_ptr<ue>> ues_slice;
    std::copy_if(ue_db.begin(), ue_db.end(), std::back_inserter(ues_slice), [slice](const std::shared_ptr<ue>& u) {
      return u->s_nssai.sst == slice->get_s_nssai().sst and u->s_nssai.sd == slice->get_s_nssai().sd;
    });
    // Poll
    slice->poll_quota(ues_slice, ue_res_grid_view);
    // If there are UEs in the slice, allocate at least 2 RBs
    if (ues_slice.size() > 0) {
      slice->set_s_nssaiQuota(std::max(slice->get_s_needs(), (u_int32_t) 10));
    }
    logger.debug("Slice sst={} sd={} needs {} RBs", slice->get_s_nssai().sst, slice->get_s_nssai().sd, slice->get_s_needs());
    prbs_tot += slice->get_s_needs();
  }

  // Compute the total available RBs without PDCCH 
  uint32_t nrb = grid.get_carrier_res_grid(subcarrier_spacing::kHz30).nof_rbs() - grid.used_crbs(subcarrier_spacing::kHz30, dl_crb_lims, symbols_lims).count();
  logger.debug("Available RBs {}", nrb);

  // Set quotas for each slice
  // if (prbs_tot <= nrb){
  //   logger.debug("Total needs: {} RBs, lower than available RBs: {}", prbs_tot, nrb);
  //   for (const auto& slice : slices) {
  //     if (slice->get_s_nssai().sst == 0) { // Currently allocating all remaining RBs to the default slice
  //       slice->set_s_nssaiQuota(slice->get_s_needs() + nrb - prbs_tot);
  //       logger.debug("Slice sst={} sd={} receiving {} RBs", slice->get_s_nssai().sst, slice->get_s_nssai().sd, nrb - prbs_tot);
  //     } else {
  //       slice->set_s_nssaiQuota(slice->get_s_needs());
  //       logger.debug("Slice sst={} sd={} receiving {} RBs", slice->get_s_nssai().sst, slice->get_s_nssai().sd, slice->get_s_needs());
  //     }
  //   }
  // } else { // Temporary solution, need to implement a better way to distribute the RBs
  //   logger.debug("Total needs: {} RBs, higher than available RBs: {}", prbs_tot, nrb);
  //   for (const auto& slice : slices) {
  //     slice->set_s_nssaiQuota((int) nrb * slice->get_s_needs() / prbs_tot);
  //     logger.debug("Slice sst={} sd={} receiving {} RBs", slice->get_s_nssai().sst, slice->get_s_nssai().sd, (int) nrb * slice->get_s_needs() / prbs_tot);
  //   }
  // }
  for (const auto& slice : slices) {
    slice->set_s_nssaiQuota((int) nrb / slices.size());
    logger.debug("Slice sst={} sd={} receiving {} RBs", slice->get_s_nssai().sst, slice->get_s_nssai().sd, (int) nrb / slices.size());
  }

  // Run the scheduling strategy for each slice
  for (const auto& slice : slices) {
    
    if (expert_cfg.enable_csi_rs_pdsch_multiplexing or (*cells[cell_index]->cell_res_alloc)[0].result.dl.csi_rs.empty()) {
      slice->dl_sched(ue_alloc, ue_res_grid_view, ue_db);
    }

    // Print resource grid after each slice is scheduled for debugging purposes.
    logger.debug("cell={}, slot={}: res grid after scheduling slice sst={} sd={}: {}", cell_index, slot_tx, slice->get_s_nssai().sst, slice->get_s_nssai().sd, grid);

    // Print CRBs after scheduling for debugging purposes.
    const crb_bitmap used_crbs_after = grid.used_crbs(subcarrier_spacing::kHz30, dl_crb_lims, symbols_lims);
    logger.debug("cell={}, slot={}: used_crbs 1D after scheduling: \n{}", cell_index, slot_tx, used_crbs_after);

    slice->ul_sched(ue_alloc, ue_res_grid_view, ue_db);
  }
}

void ue_scheduler_impl::update_harq_pucch_counter(cell_resource_allocator& cell_alloc)
{
  // We need to update the PUCCH counter after the SR/CSI scheduler because the allocation of CSI/SR can add/remove
  // PUCCH grants.
  const unsigned HARQ_SLOT_DELAY = 0;
  const auto&    slot_alloc      = cell_alloc[HARQ_SLOT_DELAY];

  // Spans through the PUCCH grant list and update the HARQ-ACK PUCCH grant counter for the corresponding RNTI and HARQ
  // process id.
  for (const auto& pucch : slot_alloc.result.ul.pucchs) {
    if ((pucch.format == pucch_format::FORMAT_1 and pucch.format_1.harq_ack_nof_bits > 0) or
        (pucch.format == pucch_format::FORMAT_2 and pucch.format_2.harq_ack_nof_bits > 0)) {
      ue* user = ue_db.find_by_rnti(pucch.crnti);
      // This is to handle the case of a UE that gets removed after the PUCCH gets allocated and before this PUCCH is
      // expected to be sent.
      if (user == nullptr) {
        logger.warning(
            "rnti={}: No user with such RNTI found in the ue scheduler database. Skipping PUCCH grant counter",
            pucch.crnti,
            slot_alloc.slot);
        continue;
      }
      srsran_assert(pucch.format == pucch_format::FORMAT_1 or pucch.format == pucch_format::FORMAT_2,
                    "rnti={}: Only PUCCH format 1 and format 2 are supported",
                    pucch.crnti);
      const unsigned nof_harqs_per_rnti_per_slot =
          pucch.format == pucch_format::FORMAT_1 ? pucch.format_1.harq_ack_nof_bits : pucch.format_2.harq_ack_nof_bits;
      // Each PUCCH grants can potentially carry ACKs for different HARQ processes (as many as the harq_ack_nof_bits)
      // expecting to be acknowledged on the same slot.
      for (unsigned harq_bit_idx = 0; harq_bit_idx != nof_harqs_per_rnti_per_slot; ++harq_bit_idx) {
        dl_harq_process* h_dl = user->get_pcell().harqs.find_dl_harq_waiting_ack_slot(slot_alloc.slot, harq_bit_idx);
        if (h_dl == nullptr) {
          logger.warning(
              "ue={} rnti={}: No DL HARQ process with state waiting-for-ack found at slot={} for harq-bit-index={}",
              user->ue_index,
              user->crnti,
              slot_alloc.slot,
              harq_bit_idx);
          continue;
        };
        h_dl->increment_pucch_counter();
      }
    }
  }
}

void ue_scheduler_impl::puxch_grant_sanitizer(cell_resource_allocator& cell_alloc)
{
  const unsigned HARQ_SLOT_DELAY = 0;
  const auto&    slot_alloc      = cell_alloc[HARQ_SLOT_DELAY];

  if (not cell_alloc.cfg.is_ul_enabled(slot_alloc.slot)) {
    return;
  }

  // Spans through the PUCCH grant list and check if there is any PUCCH grant scheduled for a UE that has a PUSCH.
  for (const auto& pucch : slot_alloc.result.ul.pucchs) {
    const auto* pusch_grant =
        std::find_if(slot_alloc.result.ul.puschs.begin(),
                     slot_alloc.result.ul.puschs.end(),
                     [&pucch](const ul_sched_info& pusch) { return pusch.pusch_cfg.rnti == pucch.crnti; });

    if (pusch_grant != slot_alloc.result.ul.puschs.end()) {
      unsigned harq_bits = 0;
      unsigned csi_bits  = 0;
      unsigned sr_bits   = 0;
      if (pucch.format == pucch_format::FORMAT_1) {
        harq_bits = pucch.format_1.harq_ack_nof_bits;
        sr_bits   = sr_nof_bits_to_uint(pucch.format_1.sr_bits);
      } else if (pucch.format == pucch_format::FORMAT_2) {
        harq_bits = pucch.format_2.harq_ack_nof_bits;
        csi_bits  = pucch.format_2.csi_part1_bits;
        sr_bits   = sr_nof_bits_to_uint(pucch.format_2.sr_bits);
      }
      logger.error("rnti={}: has both PUCCH and PUSCH grants scheduled at slot {}, PUCCH  format={} with nof "
                   "harq-bits={} csi-1-bits={} sr-bits={}",
                   pucch.crnti,
                   slot_alloc.slot,
                   static_cast<unsigned>(pucch.format),
                   harq_bits,
                   csi_bits,
                   sr_bits);
    }
  }
}

void ue_scheduler_impl::run_slot(slot_point slot_tx, du_cell_index_t cell_index)
{
  // Process any pending events that are directed at UEs.
  event_mng.run(slot_tx, cell_index);

  // Mark the start of a new slot in the UE grid allocator.
  ue_alloc.slot_indication();

  // Schedule periodic UCI (SR and CSI) before any UL grants.
  cells[cell_index]->uci_sched.run_slot(*cells[cell_index]->cell_res_alloc, slot_tx);

  // Run cell-specific SRB0 scheduler.
  cells[cell_index]->srb0_sched.run_slot(*cells[cell_index]->cell_res_alloc);

  // Synchronize all carriers. Last thread to reach this synchronization point, runs UE scheduling strategy.
  sync_point.wait(
      slot_tx, ue_alloc.nof_cells(), [this, slot_tx, cell_index]() { run_sched_strategy(slot_tx, cell_index); });

  // Update the PUCCH counter after the UE DL and UL scheduler.
  update_harq_pucch_counter(*cells[cell_index]->cell_res_alloc);

  // TODO: remove this.
  puxch_grant_sanitizer(*cells[cell_index]->cell_res_alloc);
}
