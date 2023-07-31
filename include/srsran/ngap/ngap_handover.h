/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "srsran/cu_cp/cu_cp_types.h"
#include "srsran/security/security.h"

namespace srsran {
namespace srs_cu_cp {

struct ngap_ue_source_handover_context {
  std::vector<pdu_session_id_t> pdu_sessions;
  byte_buffer                   transparent_container;
};

struct ngap_handover_preparation_request {
  ue_index_t   ue_index = ue_index_t::invalid;
  unsigned     gnb_id;
  nr_cell_id_t nci;
};

struct ngap_handover_preparation_response {
  // Place-holder for possible return values.
  bool success = false;
};

enum class ngap_handov_type { intra5gs = 0, fivegs_to_eps, eps_to_5gs, fivegs_to_utran };

struct ngap_ue_aggr_max_bit_rate {
  uint64_t ue_aggr_max_bit_rate_dl;
  uint64_t ue_aggr_max_bit_rate_ul;
};

struct ngap_qos_flow_info_item {
  qos_flow_id_t  qos_flow_id = qos_flow_id_t::invalid;
  optional<bool> dl_forwarding;
};

struct ngap_drbs_to_qos_flows_map_item {
  drb_id_t                               drb_id = drb_id_t::invalid;
  std::vector<cu_cp_associated_qos_flow> associated_qos_flow_list;
};

struct ngap_pdu_session_res_info_item {
  pdu_session_id_t                             pdu_session_id = pdu_session_id_t::invalid;
  std::vector<ngap_qos_flow_info_item>         qos_flow_info_list;
  std::vector<ngap_drbs_to_qos_flows_map_item> drbs_to_qos_flows_map_list;
};

struct ngap_erab_info_item {
  uint8_t        erab_id;
  optional<bool> dl_forwarding;
};

enum class ngap_cell_size { verysmall = 0, small, medium, large };

struct ngap_cell_type {
  ngap_cell_size cell_size;
};

struct ngap_last_visited_ngran_cell_info {
  nr_cell_global_id_t global_cell_id;
  ngap_cell_type      cell_type;
  uint16_t            time_ue_stayed_in_cell;
  optional<uint16_t>  time_ue_stayed_in_cell_enhanced_granularity;
  optional<cause_t>   ho_cause_value;
};

struct ngap_last_visited_cell_item {
  ngap_last_visited_ngran_cell_info last_visited_cell_info;
};

struct ngap_source_ngran_node_to_target_ngran_node_transparent_container {
  byte_buffer                                 rrc_container;
  std::vector<ngap_pdu_session_res_info_item> pdu_session_res_info_list;
  std::vector<ngap_erab_info_item>            erab_info_list;
  nr_cell_global_id_t                         target_cell_id;
  optional<uint16_t>                          idx_to_rfsp;
  std::vector<ngap_last_visited_cell_item>    ue_history_info;
};

struct ngap_handover_request {
  ue_index_t                ue_index = ue_index_t::invalid;
  ngap_handov_type          handov_type;
  cause_t                   cause;
  ngap_ue_aggr_max_bit_rate ue_aggr_max_bit_rate;
  // TODO: Add optional core_network_assist_info_for_inactive
  security::security_context                                            security_context;
  optional<bool>                                                        new_security_context_ind;
  byte_buffer                                                           nasc;
  slotted_id_vector<pdu_session_id_t, cu_cp_pdu_session_res_setup_item> pdu_session_res_setup_list_ho_req;
  std::vector<s_nssai_t>                                                allowed_nssai;
  // TODO: Add optional trace_activation
  optional<uint64_t>                                                masked_imeisv;
  ngap_source_ngran_node_to_target_ngran_node_transparent_container source_to_target_transparent_container;
  // TODO: Add optional mob_restrict_list
  // TODO: Add optional location_report_request_type
  // TODO: Add optional rrc_inactive_transition_report_request
  guami_t guami;
  // TODO: Add optional redirection_voice_fallback
  // TODO: Add optional cn_assisted_ran_tuning
};

} // namespace srs_cu_cp
} // namespace srsran
