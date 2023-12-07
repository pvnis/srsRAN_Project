#pragma once

#include "dl_logical_channel_manager.h"
#include "ta_manager.h"
#include "ue_cell.h"
#include "ul_logical_channel_manager.h"
#include "srsran/ran/du_types.h"
#include "srsran/scheduler/mac_scheduler.h"

namespace srsran {

class slice
{
public:
  // MCC
  int MCC;
  // MNC
  int MNC;
  // SST
  int SST;
  // SD
  int SD;
};
    
} // namespace srsran