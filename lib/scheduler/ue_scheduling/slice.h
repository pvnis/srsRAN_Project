#pragma once

#include "ue_cell.h"
#include "../../../include/srsran/ran/s_nssai.h"

namespace srsran {

// Duplicate of include/srsran/asn1/f1ap/f1ap_ies.h:6836 ?
class slice
{
public:
    // PLMNID
    int MCC = 001;
    int MNC = 01;
    int MNC_len = 2;
    int plmn_id = MCC * pow(10,MNC_len) + MNC;
    // NSSAI
    s_nssai_t nssai;
    // index
    int slice_index;
};
        
} // namespace srsran