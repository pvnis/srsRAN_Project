#pragma once

#include "ue.h"
#include "srsran/adt/ring_buffer.h"

namespace srsran {

/// Container that stores all scheduler UEs.
class slice_repository
{
  using slice_list = slotted_id_table<du_ue_index_t, std::unique_ptr<slice>, MAX_NOF_SLICES>;

private:
  srslog::basic_logger&         logger;

};

} // namespace srsran
