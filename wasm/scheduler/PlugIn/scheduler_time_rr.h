#pragma once

#include "scheduler_policy.h"
#include "lib/scheduler/policy/ue_allocator.h"

namespace srsran {

class scheduler_time_rr : public scheduler_policy
{
public:
  scheduler_time_rr();

  void
  dl_sched(ue_pdsch_allocator& pdsch_alloc, const ue_resource_grid_view& res_grid, const ue_repository& ues) override;

  void
  ul_sched(ue_pusch_allocator& pusch_alloc, const ue_resource_grid_view& res_grid, const ue_repository& ues) override;

private:
  srslog::basic_logger& logger;
  du_ue_index_t         next_dl_ue_index, next_ul_ue_index;
};

} // namespace srsran

