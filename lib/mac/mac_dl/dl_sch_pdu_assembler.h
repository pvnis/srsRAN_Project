/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "mac_dl_ue_manager.h"
#include "srsgnb/mac/lcid_dl_sch.h"
#include "srsgnb/mac/mac_pdu_format.h"
#include "srsgnb/scheduler/harq_id.h"
#include "srsgnb/scheduler/scheduler_slot_handler.h"
#include "srsgnb/support/memory_pool/ring_buffer_pool.h"

namespace srsgnb {

class byte_buffer_slice_chain;

/// \brief This class represents and encodes a MAC DL-SCH PDU that may contain multiple subPDUs.
/// Each subPDU is composed of a MAC subheader and MAC CE or MAC SDU payload.
class dl_sch_pdu
{
public:
  /// Maximum size for a MAC PDU (implementation-defined).
  static constexpr size_t MAX_PDU_LENGTH = 32768;

  explicit dl_sch_pdu(span<uint8_t> pdu_buffer_) : pdu(pdu_buffer_) {}

  /// Adds a MAC SDU as a subPDU.
  unsigned add_sdu(lcid_t lcid_, byte_buffer_slice_chain&& sdu);

  /// Adds a UE Contention Resolution CE as a subPDU.
  void add_ue_con_res_id(const ue_con_res_id_t& con_res_payload);

  /// Adds a padding CE as a subPDU.
  void add_padding(unsigned len);

  /// Number of bytes of the MAC PDU.
  unsigned nof_bytes() const { return byte_offset; }

  /// Remaining space in number of bytes in the PDU.
  unsigned nof_empty_bytes() const { return pdu.size() - byte_offset; }

  /// Gets the held MAC PDU bytes.
  span<uint8_t> get() { return pdu.first(byte_offset); }

private:
  void encode_subheader(bool F_bit, lcid_dl_sch_t lcid, unsigned header_len, unsigned payload_len);

  span<uint8_t> pdu;
  unsigned      byte_offset = 0;
};

/// \brief Class that manages the encoding of DL-SCH MAC PDUs that will be stored in Transport Blocks.
class dl_sch_pdu_assembler
{
public:
  explicit dl_sch_pdu_assembler(mac_dl_ue_manager& ue_mng_, ticking_ring_buffer_pool& pool_);

  /// \brief Encodes a MAC DL-SCH PDU with the provided scheduler information.
  /// \param rnti RNTI for which the MAC PDU was allocated.
  /// \param h_id HARQ-Id of the HARQ process used for this PDU transmission.
  /// \param tb_idx Transport block index of the HARQ process used for this PDU transmission.
  /// \param tb_info The information relative to the transport block allocated by the scheduler. This class contains
  /// a list of LCIDs of the subPDUs to allocated together with how many bytes each subPDU should take.
  /// \param tb_size_bytes Number of bytes allocated for the Transport Block.
  /// \return Byte container with assembled PDU. This container length should be lower or equal to \c tb_size_bytes.
  span<const uint8_t> assemble_newtx_pdu(rnti_t                rnti,
                                         harq_id_t             h_id,
                                         unsigned              tb_idx,
                                         const dl_msg_tb_info& tb_info,
                                         unsigned              tb_size_bytes);

  /// \brief Fetches and assembles MAC DL-SCH PDU that corresponds to a HARQ retransmission.
  /// \param rnti RNTI for which the MAC PDU was allocated.
  /// \param h_id HARQ-Id of the HARQ process used for this PDU transmission.
  /// \param tb_idx Transport block index of the HARQ process used for this PDU transmission.
  /// \return Byte container with assembled PDU.
  span<const uint8_t> assemble_retx_pdu(rnti_t rnti, harq_id_t harq_id, unsigned tb_idx);

private:
  class dl_sch_pdu_logger;

  /// Assemble MAC SDUs for a given LCID.
  void assemble_sdus(dl_sch_pdu& ue_pdu, rnti_t rnti, const dl_msg_lc_info& subpdu, dl_sch_pdu_logger& pdu_logger);

  /// Assemble MAC subPDU with a CE.
  void assemble_ce(dl_sch_pdu& ue_pdu, rnti_t rnti, const dl_msg_lc_info& subpdu, dl_sch_pdu_logger& pdu_logger);

  mac_dl_ue_manager&        ue_mng;
  ticking_ring_buffer_pool& pdu_pool;
  srslog::basic_logger&     logger;
};

} // namespace srsgnb
