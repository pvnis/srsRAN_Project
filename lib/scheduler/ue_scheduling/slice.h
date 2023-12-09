#pragma once

#include "ue_cell.h"
#include "../../../include/srsran/ran/s_nssai.h"

namespace srsran {

// Duplicate of include/srsran/asn1/f1ap/f1ap_ies.h:6836 ?
class slice
{
public:
    slice(const s_nssai_t& nssai_, const bool ll_, const int latency_slo_nanos_) :
	    nssai(nssai_), low_latency(ll_), latency_slo_nanos(latency_slo_nanos_) {
    }

    slice(const slice&)            = delete;
    slice(slice&&)                 = delete;

    // NSSAI
    const s_nssai_t nssai;
    // index
    const int slice_index;

    enum class slice_type {
        RESERVED,
        EMERGENCY,
        CRITICAL_COMMUNICATION,
        INTERNET_OF_THINGS,
        MISSION_CRITICAL_COMMUNICATION,
        MAX_SLICE_TYPE
    };

    const bool low_latency;
    const int latency_slo_nanos;

};
        
} // namespace srsran
