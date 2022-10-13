/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lib/mac/mac_ul/mac_ul_processor.h"
#include "mac_ctrl_test_dummies.h"
#include "srsgnb/scheduler/scheduler_feedback_handler.h"
#include "srsgnb/support/executors/manual_task_worker.h"
#include <gtest/gtest.h>

using namespace srsgnb;

// Implement dummy scheduler feedback handler, only used for this test.
class dummy_scheduler_feedback_handler : public scheduler_feedback_handler
{
public:
  dummy_scheduler_feedback_handler() = default;
  void handle_sr_indication(const sr_indication_message& sr) override { last_shed_req_msg = sr; };
  void handle_ul_bsr_indication(const ul_bsr_indication_message& bsr) override { last_bsr_msg = bsr; };
  void handle_crc_indication(const ul_crc_indication& crc) override {}
  void handle_uci_indication(const uci_indication& msg) override {}
  void handle_dl_mac_ce_indication(const dl_mac_ce_indication& ce) override {}

  // Compare last_shed_req_msg with a test message passed to the function.
  bool verify_sched_req_msg(const sr_indication_message& sr)
  {
    bool test_msg = last_shed_req_msg.cell_index == sr.cell_index && last_shed_req_msg.ue_index == sr.ue_index &&
                    last_shed_req_msg.crnti == sr.crnti && last_shed_req_msg.sr_payload == sr.sr_payload;

    return test_msg;
  }

  // Compare last_bsr_msg with a test message passed to the function.
  bool verify_bsr_msg(const ul_bsr_indication_message& test_usr_msg)
  {
    bool test_msg = last_bsr_msg.cell_index == test_usr_msg.cell_index &&
                    last_bsr_msg.ue_index == test_usr_msg.ue_index && last_bsr_msg.crnti == test_usr_msg.crnti &&
                    last_bsr_msg.type == test_usr_msg.type;
    for (size_t n = 0; n < test_usr_msg.reported_lcgs.size(); ++n) {
      test_msg = test_msg && last_bsr_msg.reported_lcgs[n].lcg_id == test_usr_msg.reported_lcgs[n].lcg_id &&
                 last_bsr_msg.reported_lcgs[n].nof_bytes == test_usr_msg.reported_lcgs[n].nof_bytes;
    }

    return test_msg;
  }

private:
  ul_bsr_indication_message last_bsr_msg;
  sr_indication_message     last_shed_req_msg;
};

// Helper struct that creates a MAC UL to test the correct processing of RX indication messages.
struct test_bench {
  test_bench(rnti_t rnti, du_ue_index_t du_ue_idx, du_cell_index_t cell_idx_) :
    task_exec(128),
    ul_exec_mapper(task_exec),
    dl_exec_mapper({&task_exec}),
    cfg(du_mng_notifier, ul_exec_mapper, dl_exec_mapper, task_exec, phy_notifier),
    ue_rnti(rnti),
    cell_idx(cell_idx_),
    mac_ul(cfg, sched_feedback, rnti_table)
  {
    rnti_table.add_ue(rnti, du_ue_idx);
    rx_msg_sbsr.cell_index = cell_idx;
    rx_msg_sbsr.sl_rx      = slot_point(0, 1);

    srslog::fetch_basic_logger("MAC").set_level(srslog::basic_levels::debug);
    srslog::init();
  }

  // Add a UE to the RNTI table.
  void add_ue(rnti_t rnti, du_ue_index_t du_ue_idx)
  {
    if (rnti_table.has_rnti(rnti)) {
      return;
    }
    rnti_table.add_ue(rnti, du_ue_idx);
  }

  // Add a PDU to the list of PDUs that will be included in the RX indication message.
  void enqueue_pdu(byte_buffer& pdu_payload)
  {
    // Create PDU content.
    mac_rx_pdu pdu{.rnti = ue_rnti, .rapid = 1, .harq_id = 0};
    pdu.pdu.append(pdu_payload);

    // Add PDU to the list in the RX indication message.
    rx_msg_sbsr.pdus.push_back(pdu);
  }

  // Send a RX indication message to the MAC UL and call the manual task executor.
  void send_rx_indication_msg(byte_buffer& pdu_payload)
  {
    // Add PDU to the list of PDUs to be included in the RX indication message.
    enqueue_pdu(pdu_payload);

    // Send RX data indication to MAC UL.
    mac_ul.handle_rx_data_indication(rx_msg_sbsr);

    // Call task executor manually.
    while (task_exec.has_pending_tasks()) {
      task_exec.try_run_next();
    }
  }

  // Call the dummy scheduler to compare the SR indication with a benchmark message.
  bool verify_sched_req_notification(const sr_indication_message& test_msg)
  {
    return sched_feedback.verify_sched_req_msg(test_msg);
  }

  // Call the dummy scheduler to compare the BSR indication with a benchmark message.
  bool verify_sched_bsr_notification(const ul_bsr_indication_message& test_msg)
  {
    return sched_feedback.verify_bsr_msg(test_msg);
  }

  // Call the dummy DU notifier to compare the UL CCCH indication with a benchmark message.
  bool verify_du_ul_ccch_msg(const ul_ccch_indication_message& test_msg)
  {
    return du_mng_notifier.verify_ul_ccch_msg(test_msg);
  }

private:
  manual_task_worker               task_exec;
  dummy_ue_executor_mapper         ul_exec_mapper;
  dummy_dl_executor_mapper         dl_exec_mapper;
  dummy_mac_result_notifier        phy_notifier;
  dummy_mac_event_indicator        du_mng_notifier;
  mac_common_config_t              cfg;
  du_rnti_table                    rnti_table;
  dummy_scheduler_feedback_handler sched_feedback;
  // This is the RNTI of the UE that appears in the mac_rx_pdu created by send_rx_indication_msg()
  rnti_t                 ue_rnti;
  du_cell_index_t        cell_idx;
  mac_ul_processor       mac_ul;
  mac_rx_data_indication rx_msg_sbsr;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Test UL MAC processing of RX indication message with MAC PDU for UL CCCH 48 bits.
TEST(mac_ul_processor, decode_ul_ccch_48bit)
{
  // Define UE and create test_bench.
  rnti_t          ue1_rnti = to_rnti(0x4601);
  du_ue_index_t   ue1_idx  = to_du_ue_index(1U);
  du_cell_index_t cell_idx = to_du_cell_index(1U);
  test_bench      t_bench(ue1_rnti, ue1_idx, cell_idx);

  // Create PDU content.
  // R/LCID MAC subheader | MAC SDU (UL CCCH 48 bits)
  // { 0x34  | 0x1e, 0x4f, 0xc0, 0x04, 0xa6, 0x06}  (Random 6B sequence)
  byte_buffer pdu({0x34, 0x1e, 0x4f, 0xc0, 0x04, 0xa6, 0x06});

  // Send RX data indication to MAC UL.
  t_bench.send_rx_indication_msg(pdu);

  // Create UL CCCH indication msg to verify MAC processing of PDU.
  struct ul_ccch_indication_message ul_ccch_msg {};
  ul_ccch_msg.cell_index = cell_idx;
  ul_ccch_msg.slot_rx    = slot_point{0, 1};
  ul_ccch_msg.crnti      = ue1_rnti;
  // Remove R/R/LCID header (0x34) from PDU
  ul_ccch_msg.subpdu.append({0x1e, 0x4f, 0xc0, 0x04, 0xa6, 0x06});

  // Test if notification sent to DU manager has been received and it is correct.
  ASSERT_TRUE(t_bench.verify_du_ul_ccch_msg(ul_ccch_msg));
}

// Test UL MAC processing of RX indication message with MAC PDU for UL CCCH 64 bits.
TEST(mac_ul_processor, decode_ul_ccch_64bit)
{
  // Define UE and create test_bench.
  rnti_t          ue1_rnti = to_rnti(0x4601);
  du_ue_index_t   ue1_idx  = to_du_ue_index(1U);
  du_cell_index_t cell_idx = to_du_cell_index(1U);
  test_bench      t_bench(ue1_rnti, ue1_idx, cell_idx);

  // Create PDU content.
  // R/LCID MAC subheader | MAC SDU (UL CCCH 64 bits)
  // { 0x00  | 0x1e, 0x4f, 0xc0, 0x04, 0xa6, 0x06, 0x13, 0x54}  (Random 8B sequence)
  byte_buffer pdu({0x00, 0x1e, 0x4f, 0xc0, 0x04, 0xa6, 0x06, 0x13, 0x54});

  // Send RX data indication to MAC UL.
  t_bench.send_rx_indication_msg(pdu);

  // Create UL CCCH indication msg to verify MAC processing of PDU.
  struct ul_ccch_indication_message ul_ccch_msg {};
  ul_ccch_msg.cell_index = cell_idx;
  ul_ccch_msg.slot_rx    = slot_point{0, 1};
  ul_ccch_msg.crnti      = ue1_rnti;
  // Remove R/R/LCID header (0x00) from PDU
  ul_ccch_msg.subpdu.append({0x1e, 0x4f, 0xc0, 0x04, 0xa6, 0x06, 0x13, 0x54});

  // Test if notification sent to DU manager has been received and it is correct.
  ASSERT_TRUE(t_bench.verify_du_ul_ccch_msg(ul_ccch_msg));
}

// Test UL MAC processing of RX indication message with MAC PDU for MAC CE Short BSR.
TEST(mac_ul_processor, decode_short_bsr)
{
  // Define UE and create test_bench.
  rnti_t          ue1_rnti = to_rnti(0x4601);
  du_ue_index_t   ue1_idx  = to_du_ue_index(1U);
  du_cell_index_t cell_idx = to_du_cell_index(1U);
  test_bench      t_bench(ue1_rnti, ue1_idx, cell_idx);

  // Create PDU content.
  // R/LCID MAC subheader | MAC CE Short BSR
  // { 0x3d | 0x59}
  byte_buffer pdu({0x3d, 0x59});

  // Send RX data indication to MAC UL
  t_bench.send_rx_indication_msg(pdu);

  // Create UL BSR indication  message to compare with one passed to the scheduler.
  ul_bsr_indication_message ul_bsr_ind{};
  ul_bsr_ind.cell_index = cell_idx;
  ul_bsr_ind.ue_index   = ue1_idx;
  ul_bsr_ind.crnti      = ue1_rnti;
  ul_bsr_ind.type       = bsr_format::SHORT_BSR;
  ul_bsr_lcg_report sbsr_report{.lcg_id = uint_to_lcg_id(2U), .nof_bytes = 28581};
  ul_bsr_ind.reported_lcgs.push_back(sbsr_report);
  // Test if notification sent to Scheduler has been received and it is correct.
  ASSERT_TRUE(t_bench.verify_sched_bsr_notification(ul_bsr_ind));
}

// Test UL MAC processing of RX indication message with MAC PDU for MAC CE Short Truncated BSR.
TEST(mac_ul_processor, decode_short_trunc_bsr)
{
  // Define UE and create test_bench.
  rnti_t          ue1_rnti = to_rnti(0x4601);
  du_ue_index_t   ue1_idx  = to_du_ue_index(1U);
  du_cell_index_t cell_idx = to_du_cell_index(1U);
  test_bench      t_bench(ue1_rnti, ue1_idx, cell_idx);

  // Create PDU content.
  // R/LCID MAC subheader | MAC CE Short Truncated BSR
  // { 0x3b | 0xae}
  byte_buffer pdu({0x3b, 0xae});

  // Send RX data indication to MAC UL
  t_bench.send_rx_indication_msg(pdu);

  // Create UL BSR indication  message to compare with one passed to the scheduler.
  ul_bsr_indication_message ul_bsr_ind{};
  ul_bsr_ind.cell_index = cell_idx;
  ul_bsr_ind.ue_index   = ue1_idx;
  ul_bsr_ind.crnti      = ue1_rnti;
  ul_bsr_ind.type       = bsr_format::SHORT_TRUNC_BSR;
  ul_bsr_lcg_report sbsr_report{.lcg_id = uint_to_lcg_id(5U), .nof_bytes = 745};
  ul_bsr_ind.reported_lcgs.push_back(sbsr_report);

  // Test if notification sent to Scheduler has been received and it is correct.
  ASSERT_TRUE(t_bench.verify_sched_bsr_notification(ul_bsr_ind));
}

// Test UL MAC processing of RX indication message with MAC PDU for MAC CE Long BSR.
TEST(mac_ul_processor, decode_long_bsr)
{
  // Define UE and create test_bench.
  rnti_t          ue1_rnti = to_rnti(0x4601);
  du_ue_index_t   ue1_idx  = to_du_ue_index(1U);
  du_cell_index_t cell_idx = to_du_cell_index(1U);
  test_bench      t_bench(ue1_rnti, ue1_idx, cell_idx);

  // Create PDU content.
  // R/F/LCID/L MAC subheader | MAC CE Short BSR
  // { 0x3e, 0x03 | 0x81, 0xd9, 0xab }
  byte_buffer pdu({0x3e, 0x03, 0x81, 0xd9, 0xab});

  // Send RX data indication to MAC UL.
  t_bench.send_rx_indication_msg(pdu);

  // Create UL BSR indication  message to compare with one passed to the scheduler.
  ul_bsr_indication_message ul_bsr_ind{};
  ul_bsr_ind.cell_index = to_du_cell_index(1);
  ul_bsr_ind.ue_index   = ue1_idx;
  ul_bsr_ind.crnti      = ue1_rnti;
  ul_bsr_ind.type       = bsr_format::LONG_BSR;
  ul_bsr_lcg_report bsr_report_lcg0{.lcg_id = uint_to_lcg_id(0U), .nof_bytes = 8453028U};
  ul_bsr_ind.reported_lcgs.push_back(bsr_report_lcg0);
  ul_bsr_lcg_report bsr_report_lcg7{.lcg_id = uint_to_lcg_id(7U), .nof_bytes = 468377U};
  ul_bsr_ind.reported_lcgs.push_back(bsr_report_lcg7);

  // Test if notification sent to Scheduler has been received and it is correct.
  ASSERT_TRUE(t_bench.verify_sched_bsr_notification(ul_bsr_ind));
}

// Test UL MAC processing of RX indication message with MAC PDU for MAC CE C-RNTI.
TEST(mac_ul_processor, decode_crnti_ce)
{
  // Define UE and create test_bench.
  rnti_t          ue1_rnti = to_rnti(0x4602);
  du_ue_index_t   ue1_idx  = to_du_ue_index(2U);
  du_cell_index_t cell_idx = to_du_cell_index(1U);
  test_bench      t_bench(ue1_rnti, ue1_idx, cell_idx);
  // Add a UE. This RNTI (0x4601) is the one carried by the MAC CE C-RNTI and should be used in the sr_ind{} below.
  t_bench.add_ue(to_rnti(0x4601), to_du_ue_index(1U));

  // Create PDU content.
  // R/LCID MAC subheader | MAC CE C-RNTI
  // { 0x3a | 0x46, 0x01 }
  byte_buffer pdu({0x3a, 0x46, 0x01});

  // Send RX data indication to MAC UL.
  t_bench.send_rx_indication_msg(pdu);

  // Create UL BSR indication  message to compare with one passed to the scheduler.
  sr_indication_message sr_ind{};
  sr_ind.cell_index = cell_idx;
  sr_ind.ue_index   = to_du_ue_index(1U);
  sr_ind.crnti      = to_rnti(0x4601);
  sr_ind.sr_payload.resize(1);
  sr_ind.sr_payload.set(0);
  // Test if notification sent to Scheduler has been received and it is correct.
  ASSERT_TRUE(t_bench.verify_sched_req_notification(sr_ind));
}

// Test UL MAC processing of RX indication message with MAC PDU for multiple subPDUs (MAC CE C-RNTI, MAC CE Short BSR).
TEST(mac_ul_processor, decode_crnti_ce_and_sbsr)
{
  // Define UE and create test_bench.
  rnti_t          ue1_rnti = to_rnti(0x4602);
  du_ue_index_t   ue1_idx  = to_du_ue_index(2U);
  du_cell_index_t cell_idx = to_du_cell_index(1U);
  test_bench      t_bench(ue1_rnti, ue1_idx, cell_idx);
  // Add a UE. This RNTI (0x4601) is the one carried by the MAC CE C-RNTI and should be used in the sr_ind{} below
  t_bench.add_ue(to_rnti(0x4601), to_du_ue_index(1U));

  // Create subPDU content.
  // R/LCID MAC subheader | MAC CE C-RNTI
  // { 0x3a | 0x46, 0x01 }
  byte_buffer pdu_ce_crnti({0x3a, 0x46, 0x01});
  t_bench.enqueue_pdu(pdu_ce_crnti);

  // Create subPDU content.
  // R/LCID MAC subheader | MAC CE Short BSR
  // { 0x3d | 0x59}
  byte_buffer pdu_sbsr({0x3d, 0x59});
  // Send RX data indication to MAC UL
  t_bench.send_rx_indication_msg(pdu_sbsr);

  // Create UL Sched Req indication message (generated by MAC CE C-RNTI) to compare with one passed to the scheduler.
  sr_indication_message sr_ind{};
  sr_ind.cell_index = cell_idx;
  sr_ind.ue_index   = to_du_ue_index(1U);
  sr_ind.crnti      = to_rnti(0x4601);
  sr_ind.sr_payload.resize(1);
  sr_ind.sr_payload.set(0);
  // Test if notification sent to Scheduler has been received and it is correct.
  ASSERT_TRUE(t_bench.verify_sched_req_notification(sr_ind));

  // Create UL BSR indication message to compare with one passed to the scheduler.
  ul_bsr_indication_message ul_bsr_ind{};
  ul_bsr_ind.cell_index = cell_idx;
  ul_bsr_ind.ue_index   = ue1_idx;
  ul_bsr_ind.crnti      = ue1_rnti;
  ul_bsr_ind.type       = bsr_format::SHORT_BSR;
  ul_bsr_lcg_report sbsr_report{.lcg_id = uint_to_lcg_id(2U), .nof_bytes = 28581};
  ul_bsr_ind.reported_lcgs.push_back(sbsr_report);
  ASSERT_TRUE(t_bench.verify_sched_bsr_notification(ul_bsr_ind));
}
