#pragma once
#include <wasmedge/wasmedge.h>
#include "scheduler_policy.h"
#include "lib/scheduler/policy/ue_allocator.h"

namespace srsran {

class scheduler_policy_adapter : public scheduler_policy
{
public:
  scheduler_policy_adapter(ue_cell_grid_allocator &ue_alloc);

  void
  dl_sched(ue_pdsch_allocator& pdsch_alloc, const ue_resource_grid_view& res_grid, const ue_repository& ues) override;

  void
  ul_sched(ue_pusch_allocator& pusch_alloc, const ue_resource_grid_view& res_grid, const ue_repository& ues) override;

private:
  srslog::basic_logger& logger;
  WasmEdge_VMContext *vm_cxt;

  ue_cell_grid_allocator &ue_alloc;
};

} // namespace srsran
