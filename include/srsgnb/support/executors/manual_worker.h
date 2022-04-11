
#ifndef SRSGNB_MANUAL_WORKER_H
#define SRSGNB_MANUAL_WORKER_H

#include "srsgnb/adt/circular_buffer.h"
#include "task_executor.h"

namespace srsgnb {

/// \brief Task worker that implements the executor interface and requires manual calls to run pending deferred tasks.
/// Useful for unit testing.
class manual_worker : public task_executor
{
public:
  manual_worker(size_t q_size) : pending_tasks(q_size) {}

  std::thread::id get_thread_id() const { return t_id; }

  void execute(unique_task task) override
  {
    if (std::this_thread::get_id() == t_id) {
      task();
    } else {
      defer(std::move(task));
    }
  }

  void defer(unique_task task) override { pending_tasks.push_blocking(std::move(task)); }

  bool has_pending_tasks() const { return not pending_tasks.empty(); }

  bool is_stopped() const { return pending_tasks.is_stopped(); }

  void stop()
  {
    if (not is_stopped()) {
      pending_tasks.stop();
    }
  }

  void request_stop()
  {
    defer([this]() { stop(); });
  }

  /// Run all pending tasks until queue is emptied.
  bool run_pending_tasks()
  {
    set_thread_id();
    bool ret = false, success = false;
    do {
      unique_task t;
      success = pending_tasks.try_pop(t);
      if (success) {
        t();
        ret = true;
      }
    } while (success);
    return ret;
  }

  /// Run next pending task if it is enqueued.
  bool try_run_next()
  {
    set_thread_id();
    unique_task t;
    bool        success = pending_tasks.try_pop(t);
    if (not success) {
      return false;
    }
    t();
    return true;
  }

  /// Run next pending task once it is enqueued.
  bool run_next_blocking()
  {
    set_thread_id();
    bool        success = false;
    unique_task t       = pending_tasks.pop_blocking(&success);
    if (not success) {
      return false;
    }
    t();
    return true;
  }

private:
  bool has_thread_id() const { return t_id != std::thread::id{}; }

  void set_thread_id()
  {
    if (not has_thread_id()) {
      // if not initialized.
      t_id = std::this_thread::get_id();
    } else {
      srsran_assert(t_id == std::this_thread::get_id(), "run() caller thread should not change.");
    }
  }

  std::thread::id                 t_id;
  dyn_blocking_queue<unique_task> pending_tasks;
};

} // namespace srsgnb

#endif // SRSGNB_MANUAL_WORKER_H
