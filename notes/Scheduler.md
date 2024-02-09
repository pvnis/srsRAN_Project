# Scheduler
## Downlink
1. `run_sched_strategy()` loops over the slices and calls `dl_sched()`
2. `dl_sched()` calls `round_robin_apply()` for the re-transmission first and then the new transmission
3. `round_robin_apply()` checks if UE is in the slice, calls `alloc_dl_ue()` and stores the outcome in `alloc_result()`
4. `alloc_dl_ue()` iterates over the candidates, computes their used CRBs (if they are all used, it skips the slot)
5. `alloc_dl_ue()` computes the PRBs needed by the UE using `required_dl_prbs()`
6. `alloc_dl_ue()` uses `find_empty_interval_of_length()` to find an interval (first interval satisfying the request OR the largest interval found)
7. If `find_empty_interval_of_length()` found a valid interval, `alloc_dl_ue()` allocates a grant using `allocate_dl_grant()` and returns the outcome
5. If `alloc_result()` is successfull, `round_robin_apply()` calls `to_du_ue_index()`