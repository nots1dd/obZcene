#pragma once

/**
 * @file Threading.hpp
 * @brief Small thread fan-out helper shared by the render pipeline
 *        (namespace obzcene::threading).
 *
 * `parallel_run` invokes a callable once per worker index on that many
 * threads (worker 0 runs on the caller). It is the only place in the engine
 * that spawns threads, so rasterization code can stay declarative about
 * *what* runs in parallel without owning the *how*.
 */

#include <cstddef>
#include <thread>
#include <vector>

namespace obzcene::threading
{

/**
 * @brief Invoke `fn(worker_index, worker_count)` for workers 0..count-1.
 *
 * Worker 0 runs on the calling thread; the rest run on fresh threads that
 * are all joined before this returns. `fn` must be thread-safe for the
 * ranges it is given (the engine partitions disjoint data per worker).
 * With `workers <= 1` the call is fully inline — no thread is created.
 */
template <typename F> inline auto parallel_run(int workers, F&& fn) -> void
{
  if (workers <= 1)
  {
    fn(0, 1);
    return;
  }
  std::vector<std::thread> threads;
  threads.reserve(static_cast<std::size_t>(workers - 1));
  for (auto w = 1; w < workers; ++w)
  {
    threads.emplace_back([&, w] { fn(w, workers); });
  }
  fn(0, workers);
  for (auto& thread : threads)
  {
    thread.join();
  }
}

} // namespace obzcene::threading
