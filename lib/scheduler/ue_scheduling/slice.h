#pragma once

#include "ue_cell.h"
#include "../../../include/srsran/ran/s_nssai.h"

namespace srsran {

// Duplicate of include/srsran/asn1/f1ap/f1ap_ies.h:6836 ?
class slice
{
public:
    slice(const int MCC_, const int MNC_, const int MNC_len_, const s_nssai_t& nssai_, const int slice_index_) :
	    MCC(MCC_), MNC(MNC_), MNC_len(MNC_len_), nssai(nssai_), slice_index(slice_index_) {
    	plmn_id = MCC * pow(10,MNC_len) + MNC;
    }

    slice(const slice&)            = delete;
    slice(slice&&)                 = delete;

    // PLMNID
    int MCC = 001;
    int MNC = 01;
    int MNC_len = 2;
    int plmn_id;
    // NSSAI
    s_nssai_t nssai;
    // index
    int slice_index;
};
        
} // namespace srsran
