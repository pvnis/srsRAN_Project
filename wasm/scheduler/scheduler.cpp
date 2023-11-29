#include "schedule.h"

int main() {
    // Create VM context


    // struct ue_pdsch_grant {
    // const ue*         user;
    // du_cell_index_t   cell_index;
    // harq_id_t         h_id;
    // search_space_id   ss_id;
    // unsigned          time_res_index;
    // crb_interval      crbs;
    // aggregation_level aggr_lvl = aggregation_level::n4;
    // sch_mcs_index     mcs;
    // };

    //  Call grant allocator
    enum WasmEdge_ValType ParamList[8] = {};
    enum WasmEdge_ValType ReturnList[1] = {};
    /* Create a function type: {i32, i32} -> {i32}. */
    WasmEdge_FunctionTypeContext *GrantAllocFType = WasmEdge_FunctionTypeCreate(ParamList, 2, ReturnList, 1);
    /*
    * Create a function context with the function type and host function body.
    * The `Cost` parameter can be 0 if developers do not need the cost
    * measuring.
    */
    WasmEdge_FunctionInstanceContext *HostFunc = WasmEdge_FunctionInstanceCreate(GrantAllocFType, Add, NULL, 0);
    /*
    * The third parameter is the pointer to the additional data.
    * Developers should guarantee the life cycle of the data, and it can be NULL if the external data is not needed.
    */
    
    /* Delete function */
    WasmEdge_FunctionTypeDelete(HostType);
    return EXIT_SUCCESS;
}
