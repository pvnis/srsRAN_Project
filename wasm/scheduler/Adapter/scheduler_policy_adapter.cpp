#pragma once

#include "scheduler_policy_adapter.h"

using namespace srsran;

WasmEdge_Result xxx(void *Data,
                          const WasmEdge_CallingFrameContext *CallingFrameCxt,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {

  ue* ue = (ue *) WasmEdge_ValueGetExternRef(In[0]);
  auto cell_index = WasmEdge_ValueGetI32(In[1]);
  auto harq_process_id = WasmEdge_ValueGetI32(In[2]);
  auto ss_id = WasmEdge_ValueGetI32(In[3]);
  auto pdsch_td_res_index = WasmEdge_ValueGetI64(In[4]);
  auto ue_grant_crbs_start = WasmEdge_ValueGetI32(In[5]);
  auto ue_grant_crbs_end = WasmEdge_ValueGetI32(In[6]);
  auto aggr_lvl = WasmEdge_ValueGetI32(In[7]);
  auto mcs = WasmEdge_ValueGetI32(In[8]);

  crb_interval crb_interval{ue_grant_crbs_start, ue_grant_crbs_end};
  ue_pdsch_grant grant{ue, cell_index, harq_process_id, ss_id, pdsch_td_res_index, crb_interval, aggr_lvl, mcs};

  //TODO:
  
}

WasmEdge_Result yyy(void *Data,
                          const WasmEdge_CallingFrameContext *CallingFrameCxt,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) {

  ue* ue = (ue *) WasmEdge_ValueGetExternRef(In[0]);
  auto cell_index = WasmEdge_ValueGetI32(In[1]);
  auto harq_process_id = WasmEdge_ValueGetI32(In[2]);
  auto crbs_start = WasmEdge_ValueGetI32(In[3]);
  auto crbs_end = WasmEdge_ValueGetI32(In[4]);
  auto symbols_start = WasmEdge_ValueGetI32(In[5]);
  auto symbols_end = WasmEdge_ValueGetI32(In[6]);
  auto pusch_td_res_index = WasmEdge_ValueGetI64(In[7]);
  auto ss_id = WasmEdge_ValueGetI32(In[8]);
  auto aggr_lvl = WasmEdge_ValueGetI32(In[9]);
  auto mcs = WasmEdge_ValueGetI32(In[10]);

}

scheduler_policy_adapter::scheduler_policy_adapter() :
  logger(srslog::fetch_basic_logger("SCHED"))
{
  /* Create the configure context and add the WASI support. */
  /* This step is not necessary unless you need WASI support. */
  WasmEdge_ConfigureContext *ConfCxt = WasmEdge_ConfigureCreate();
  WasmEdge_ConfigureAddHostRegistration(ConfCxt,
                                        WasmEdge_HostRegistration_Wasi);
  /* The configure and store context to the VM creation can be NULL. */
  vm_cxt = WasmEdge_VMCreate(ConfCxt, NULL);

  WasmEdge_String ModName = WasmEdge_StringCreateByCString("extern_module");
  WasmEdge_ModuleInstanceContext *HostMod =
      WasmEdge_ModuleInstanceCreate(ModName);

  // Declare an anonymous function
  auto alloc_dl_grant = [this](void *Data,
                          const WasmEdge_CallingFrameContext *CallingFrameCxt,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) -> WasmEdge_Result {
    ue* ue = (ue *) WasmEdge_ValueGetExternRef(In[0]);
    auto cell_index = WasmEdge_ValueGetI32(In[1]);
    auto harq_process_id = WasmEdge_ValueGetI32(In[2]);
    auto ss_id = WasmEdge_ValueGetI32(In[3]);
    auto pdsch_td_res_index = WasmEdge_ValueGetI64(In[4]);
    auto ue_grant_crbs_start = WasmEdge_ValueGetI32(In[5]);
    auto ue_grant_crbs_end = WasmEdge_ValueGetI32(In[6]);
    auto aggr_lvl = WasmEdge_ValueGetI32(In[7]);
    auto mcs = WasmEdge_ValueGetI32(In[8]);

    crb_interval crb_interval{ue_grant_crbs_start, ue_grant_crbs_end};
    ue_pdsch_grant grant{ue, cell_index, harq_process_id, ss_id, pdsch_td_res_index, crb_interval, aggr_lvl, mcs};

    //make call to ue_pdsch_allocator::allocate_grant
    ue_alloc.allocate_dl_grant(grant);
  };

  enum WasmEdge_ValType P[9], R[1];
  P[0] = WasmEdge_ValType_ExternRef;
  P[1] = WasmEdge_ValType_I32;
  P[2] = WasmEdge_ValType_I32;
  P[3] = WasmEdge_ValType_I32;
  P[4] = WasmEdge_ValType_I64;
  P[5] = WasmEdge_ValType_I32;
  P[6] = WasmEdge_ValType_I32;
  P[7] = WasmEdge_ValType_I32;
  P[8] = WasmEdge_ValType_I32;
  R[0] = WasmEdge_ValType_I32;
  WasmEdge_FunctionTypeContext *HostFType = WasmEdge_FunctionTypeCreate(P, 9, R, 0);
  WasmEdge_FunctionInstanceContext *HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, alloc_dl_grant, NULL, 0);
  WasmEdge_String HostName = WasmEdge_StringCreateByCString("alloc_dl_grant");

  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);

  auto alloc_ul_grant = [this](void *Data,
                          const WasmEdge_CallingFrameContext *CallingFrameCxt,
                          const WasmEdge_Value *In, WasmEdge_Value *Out) -> WasmEdge_Result {

    ue* ue = (ue *) WasmEdge_ValueGetExternRef(In[0]);
    auto cell_index = WasmEdge_ValueGetI32(In[1]);
    auto harq_process_id = WasmEdge_ValueGetI32(In[2]);
    auto crbs_start = WasmEdge_ValueGetI32(In[3]);
    auto crbs_end = WasmEdge_ValueGetI32(In[4]);
    auto symbols_start = WasmEdge_ValueGetI32(In[5]);
    auto symbols_end = WasmEdge_ValueGetI32(In[6]);
    auto pusch_td_res_index = WasmEdge_ValueGetI64(In[7]);
    auto ss_id = WasmEdge_ValueGetI32(In[8]);
    auto aggr_lvl = WasmEdge_ValueGetI32(In[9]);
    auto mcs = WasmEdge_ValueGetI32(In[10]);

    crb_interval crb_interval{crbs_start, crbs_end};
    ofdm_symbol_range symbol_interval{symbols_start, symbols_end};
    ue_pusch_grant grant{ue, cell_index, harq_process_id, crb_interval, symbol_interval, pusch_td_res_index, ss_id, aggr_lvl, mcs};

    //make call to ue_pusch_allocator::allocate_grant
    ue_alloc.allocate_ul_grant(grant);
  };

  enum WasmEdge_ValType P[11], R[1];
  P[0] = WasmEdge_ValType_ExternRef;
  P[1] = WasmEdge_ValType_I32;
  P[2] = WasmEdge_ValType_I32;
  P[3] = WasmEdge_ValType_I32;
  P[4] = WasmEdge_ValType_I32;
  P[5] = WasmEdge_ValType_I32;
  P[6] = WasmEdge_ValType_I32;
  P[7] = WasmEdge_ValType_I64;
  P[8] = WasmEdge_ValType_I32;
  P[9] = WasmEdge_ValType_I32;
  P[10] = WasmEdge_ValType_I32;
  R[0] = WasmEdge_ValType_I32;
  WasmEdge_FunctionTypeContext *HostFType = WasmEdge_FunctionTypeCreate(P, 11, R, 0);
  WasmEdge_FunctionInstanceContext *HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, alloc_dl_grant, NULL, 0);
  WasmEdge_String HostName = WasmEdge_StringCreateByCString("alloc_ul_grant");

  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);

  WasmEdge_Result Res = WasmEdge_VMRegisterModuleFromImport(vm_cxt, HostMod);
  if (!WasmEdge_ResultOK(Res)) {
    printf("Host module instance registration failed\n");
    return -1;
  }

  /* Load from file*/
  Res = WasmEdge_VMLoadWasmFromFile(vm_cxt, "policy_rr.so");
  if (!WasmEdge_ResultOK(Res)) {
    WasmEdge_VMDelete(vm_cxt);
    printf("Load test.wasm error: %s\n", WasmEdge_ResultGetMessage(Res));
    return -1;
  }
  Res = WasmEdge_VMValidate(vm_cxt);
  if (!WasmEdge_ResultOK(Res)) {
    WasmEdge_VMDelete(vm_cxt);
    printf("Validate test.wasm error: %s\n",
           WasmEdge_ResultGetMessage(Res));
    return -1;
  }
  Res = WasmEdge_VMInstantiate(vm_cxt);
  if (!WasmEdge_ResultOK(Res)) {
    WasmEdge_VMDelete(vm_cxt);
    printf("Instantiate test.wasm error: %s\n",
           WasmEdge_ResultGetMessage(Res));
    return -1;
  }
}

void scheduler_policy_adapter::dl_sched(ue_pdsch_allocator&   pdsch_alloc,
                                 const ue_resource_grid_view& res_grid,
                                 const ue_repository&         ues)
{
  const slot_point pdcch_slot = res_grid.get_pdcch_slot();
  
  //TODO: serialize arguments
  /* The parameters and returns arrays. */
  WasmEdge_Value Params[1] = {WasmEdge_ValueGenExternRef(test_str)};
  WasmEdge_Value Returns[1];

  /* Function name. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("dl_sched");

  WasmEdge_Result Res = WasmEdge_VMExecute(vm_cxt, FuncName, Params, 1, Returns, 1);

  if (WasmEdge_ResultOK(Res)) {
    printf("Get result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
  } else {
    printf("Error message: %s\n", WasmEdge_ResultGetMessage(Res));
  }
}

void scheduler_policy_adapter::ul_sched(ue_pusch_allocator&   pusch_alloc,
                                 const ue_resource_grid_view& res_grid,
                                 const ue_repository&         ues)
{
  //TODO: serialize arguments
  /* The parameters and returns arrays. */
  WasmEdge_Value Params[1] = {WasmEdge_ValueGenExternRef(test_str)};
  WasmEdge_Value Returns[1];

  /* Function name. */
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("ul_sched");

  WasmEdge_Result Res = WasmEdge_VMExecute(vm_cxt, FuncName, Params, 1, Returns, 1);

  if (WasmEdge_ResultOK(Res)) {
    printf("Get result: %d\n", WasmEdge_ValueGetI32(Returns[0]));
  } else {
    printf("Error message: %s\n", WasmEdge_ResultGetMessage(Res));
  }
}
