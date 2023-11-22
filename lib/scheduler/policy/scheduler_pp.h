#pragma once

#include "scheduler_policy.h"
#include <chrono>

namespace srsran {

class scheduler_pp : public scheduler_policy
{
public:
  scheduler_pp();

  void
  dl_sched(ue_pdsch_allocator& pdsch_alloc, const ue_resource_grid_view& res_grid, const ue_repository& ues, bool is_it_ul_slot, std::chrono::nanoseconds delta) override;

  void
  ul_sched(ue_pusch_allocator& pusch_alloc, const ue_resource_grid_view& res_grid, const ue_repository& ues, bool is_it_ul_slot, std::chrono::nanoseconds delta) override;

private:
  srslog::basic_logger& logger;
  du_ue_index_t         next_dl_ue_index, next_ul_ue_index;
};

} // namespace srsran
