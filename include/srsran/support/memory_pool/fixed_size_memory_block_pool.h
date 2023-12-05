/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#pragma once

#include "memory_block_list.h"
#include "srsran/adt/static_vector.h"
#include "srsran/support/error_handling.h"
#include "srsran/support/srsran_assert.h"
#include <mutex>
#include <thread>
#include <vector>

namespace srsran {

/**
 * Concurrent memory pool of memory blocks of equal size. This pool is thread-safe.
 * Each worker keeps a separate thread-local memory block cache that it uses for fast, non-contended allocation and
 * deallocation. When accessing a thread-local cache, no locks are required.
 *
 * When the local cache gets depleted, the worker tries to obtain a batch of segments from a central memory block cache.
 * If the central cache is also depleted, the allocation fails.
 *
 * Since there is no stealing of segments between workers' local caches, it is possible that a worker cannot allocate
 * while another worker still has blocks in its own cache. To minimize the impact of this event, an upper bound is
 * placed on a worker local cache size. Once a worker reaches that upper bound, it sends half of its stored blocks to
 * the central cache.
 *
 * Note1: Notice that the same memory block might be allocated and deallocated in different workers. So, it can happen
 * that a worker is allocating many blocks and another worker just deallocating. The latter worker has to keep
 * returning blocks back to the central cache every time its local cache grows beyond a given upper bound.
 *
 * Note2: Taking into account the usage of thread_local, this class is made a singleton. To be able to instantiate
 *       different pools, the user should use different IdTag types.
 *
 * Note3: No considerations were made regarding false sharing between workers. It is assumed that the blocks are big
 *        enough to fill a cache line.
 *
 * \tparam IdTag We use a ID type tag to be able to intantiate different pool objects.
 * \tparam DebugSanitizeAddress when set to true, the pool verifies that the addresses allocated and deallocated are
 * valid.
 */
template <typename IdTag, bool DebugSanitizeAddress = false>
class fixed_size_memory_block_pool
{
  using pool_type = fixed_size_memory_block_pool<IdTag, DebugSanitizeAddress>;

  /// The number of blocks in batch that the worker can steal from the central cache.
  constexpr static size_t block_batch_size = 32U;

  /// The number of batches of blocks that a worker can store in its own thread for non-contended access.
  constexpr static size_t MAX_LOCAL_BATCH_CAPACITY = 64U;

  using local_cache_type = static_vector<free_memory_block_list, MAX_LOCAL_BATCH_CAPACITY>;

  /// Ctor of the memory pool. It is set as private because the class works as a singleton.
  explicit fixed_size_memory_block_pool(size_t nof_blocks_, size_t memory_block_size_) :
    mblock_size(align_next(memory_block_size_, alignof(std::max_align_t))),
    nof_blocks(nof_blocks_),
    max_local_batches(
        std::max(std::min((size_t)MAX_LOCAL_BATCH_CAPACITY, static_cast<size_t>(nof_blocks / block_batch_size / 32U)),
                 static_cast<size_t>(2U)))
  {
    srsran_assert(nof_blocks > max_local_cache_size(),
                  "The number of segments in the pool must be much larger than the thread cache size ({} <= {})",
                  nof_blocks,
                  max_local_cache_size());
    srsran_assert(mblock_size > free_memory_block_list::min_memory_block_align(),
                  "Segment size is too small ({} <= {})",
                  mblock_size,
                  free_memory_block_list::min_memory_block_align());

    // Allocate the required memory for the given number of segments and segment size.
    const size_t total_mem = mblock_size * nof_blocks;
    allocated_memory.resize(total_mem);

    // Push all segments to the central cache.
    const size_t nof_batches = ceil(nof_blocks / (double)block_batch_size);
    central_mem_cache.resize(nof_batches);
    for (unsigned i = 0; i != nof_blocks; ++i) {
      const unsigned batch_idx = i / block_batch_size;
      const unsigned offset    = i * mblock_size;
      central_mem_cache[batch_idx].push(allocated_memory.data() + offset);
    }
  }

public:
  fixed_size_memory_block_pool(const fixed_size_memory_block_pool&)            = delete;
  fixed_size_memory_block_pool(fixed_size_memory_block_pool&&)                 = delete;
  fixed_size_memory_block_pool& operator=(const fixed_size_memory_block_pool&) = delete;
  fixed_size_memory_block_pool& operator=(fixed_size_memory_block_pool&&)      = delete;

  ~fixed_size_memory_block_pool()
  {
    std::lock_guard<std::mutex> lock(central_cache_mutex);
    allocated_memory.clear();
  }

  /// \brief Get instance of a memory pool singleton.
  static fixed_size_memory_block_pool<IdTag, DebugSanitizeAddress>& get_instance(size_t nof_blocks     = 0,
                                                                                 size_t mem_block_size = 0)
  {
    static fixed_size_memory_block_pool<IdTag, DebugSanitizeAddress> pool(nof_blocks, mem_block_size);
    return pool;
  }

  /// Memory block size in bytes.
  size_t memory_block_size() const { return mblock_size; }

  /// Number of memory blocks contained in this memory pool.
  size_t nof_memory_blocks() const { return nof_blocks; }

  /// Maximum number of blocks that can be stored in the thread-local memory block cache.
  size_t max_local_cache_size() const { return max_local_batches * block_batch_size; }

  /// Allocate a node from the memory pool with the maximum size.
  void* allocate_node() noexcept { return allocate_node(memory_block_size()); }

  /// Allocate a node from the memory pool with the provided size.
  void* allocate_node(size_t sz) noexcept
  {
    srsran_assert(sz <= mblock_size, "Allocated node size={} exceeds max object size={}", sz, mblock_size);
    worker_ctxt* w_ctx = get_worker_cache();

    // Attempt memory block pop from local cache.
    void* node = nullptr;
    while (not w_ctx->local_cache.empty()) {
      node = w_ctx->local_cache.back().try_pop();
      if (node != nullptr) {
        return node;
      }
      w_ctx->local_cache.pop_back();
    }

    // Local cache is empty. Attempt memory block pop from central cache.
    std::lock_guard<std::mutex> lock(central_cache_mutex);
    if (not central_mem_cache.empty()) {
      w_ctx->local_cache.push_back(central_mem_cache.back());
      central_mem_cache.pop_back();
      node = w_ctx->local_cache.back().try_pop();
    }

    return node;
  }

  /// Deallocate node by returning it back to the memory pool.
  void deallocate_node(void* p)
  {
    srsran_assert(p != nullptr, "Deallocated nodes must have valid address");

    worker_ctxt* w_ctx = get_worker_cache();

    if (DebugSanitizeAddress) {
      // For debug purposes.
      std::lock_guard<std::mutex> lock(debug_mutex);
      bool                        found = false;
      for (unsigned i = 0; i != nof_blocks; ++i) {
        if (allocated_memory.data() + i * mblock_size == static_cast<uint8_t*>(p)) {
          found = true;
        }
      }
      if (not found) {
        report_fatal_error("Error dealloccating block with address {:#x}", p);
      }
    }

    // Verify if new batch needs to be created in local cache.
    if (w_ctx->local_cache.empty() or w_ctx->local_cache.back().size() >= block_batch_size) {
      w_ctx->local_cache.emplace_back();
    }

    // Push block to local cache.
    w_ctx->local_cache.back().push(p);

    if (w_ctx->local_cache.size() >= max_local_batches and w_ctx->local_cache.back().size() >= block_batch_size) {
      // Local cache is full. Rebalance by sending batches of blocks to central cache.
      // We leave one batch in the local cache.
      std::lock_guard<std::mutex> lock(central_cache_mutex);
      for (unsigned i = 0; i != max_local_batches - 1; ++i) {
        central_mem_cache.push_back(w_ctx->local_cache.back());
        w_ctx->local_cache.pop_back();
      }
    }
  }

  void print_all_buffers()
  {
    auto*    worker = get_worker_cache();
    unsigned count  = 0;
    for (const auto& l : worker->local_cache) {
      count += l.size();
    }

    fmt::print("There are {}/{} buffers in central memory block cache. This thread contains {} in its local cache.\n",
               central_mem_cache.size(),
               nof_memory_blocks(),
               count);
  }

private:
  struct worker_ctxt {
    std::thread::id  id;
    local_cache_type local_cache;

    worker_ctxt() : id(std::this_thread::get_id()) {}
    ~worker_ctxt()
    {
      pool_type&                  pool = pool_type::get_instance();
      std::lock_guard<std::mutex> lock(pool.central_cache_mutex);
      while (not local_cache.empty()) {
        pool.central_mem_cache.push_back(local_cache.back());
        local_cache.pop_back();
      }
    }
  };

  worker_ctxt* get_worker_cache()
  {
    thread_local worker_ctxt worker_cache;
    return &worker_cache;
  }

  const size_t mblock_size;
  const size_t nof_blocks;
  const size_t max_local_batches;

  std::mutex                          central_cache_mutex;
  std::vector<free_memory_block_list> central_mem_cache;

  std::mutex           debug_mutex;
  std::vector<uint8_t> allocated_memory;
};

} // namespace srsran
