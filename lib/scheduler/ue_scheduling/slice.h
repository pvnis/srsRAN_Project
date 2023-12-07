#pragma once

#include "dl_logical_channel_manager.h"
#include "ta_manager.h"
#include "ue_cell.h"
#include "ul_logical_channel_manager.h"
#include "srsran/ran/du_types.h"
#include "srsran/scheduler/mac_scheduler.h"
#include "include/srsran/ran/s_nssai.h"
#include "asn1/f1ap/f1ap_common.h"

namespace srsran {

// Duplicate of include/srsran/asn1/f1ap/f1ap_ies.h:6836 ?
class slice
{
public:
    // PLMNID
    fixed_octstring<3, true> plmn_id;
    // NSSAI
    s_nssai_t nssai;
};
        
} // namespace srsran