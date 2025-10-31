#ifndef THREADING_HPP
#define THREADING_HPP "Auxil/threading.hpp"
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>

#include "misc.hpp"
namespace Auxil {
    using namespace Primitives;



    class Executor {
        std::mutex qmtx;
        std::condition_variable cv;
        std::condition_variable cv_wait;
        std::atomic_bool running{true};
        std::atomic_uint32_t active_tasks;

        std::queue<std::move_only_function<void()>> tasks;

        std::vector<std::thread> threads;

        void thread_runner() {
            //this ensures we finish all the tasks before exiting, that way when a task is
            //executed its guaranteed to run
            while (running.load(std::memory_order_acquire) || !tasks.empty()) {
                std::move_only_function<void()> task;
                bool has_task{false};

                auto run_task = [&] {
                    active_tasks.fetch_add(1, std::memory_order::acquire);
                    task();
                    active_tasks.fetch_sub(1, std::memory_order::acquire);

                    //notify waiters
                    cv.notify_one();
                    cv_wait.notify_one();
                };

                {
                    std::lock_guard lk(qmtx);
                    if (!tasks.empty()) {
                        task = std::move(tasks.front());
                        tasks.pop();
                        has_task = true;
                    }
                }

                if (has_task) {
                    run_task();
                } else {
                    // tweak iteration count
                    for (int i = 0; i < 1000; ++i) {
                        {
                            std::lock_guard lk(qmtx);
                            if (!tasks.empty()) {
                                task = std::move(tasks.front());
                                tasks.pop();
                                has_task = true;
                                break;
                            }
                        }
                        std::this_thread::yield(); // allow other threads to run
                    }
                    if (has_task) {
                        run_task();
                    }
                }

                //if there's more tasks, lets just get the next one
                if (tasks.empty()) {
                    std::unique_lock lk(qmtx);
                    cv.wait(lk, [this]{ return !tasks.empty() || !running; });
                }
            }
        }

        void init_threads(const u32 n) {
            threads.reserve(n);
            for (u32 i = 0; i < n; i++) {
                threads.emplace_back(&Executor::thread_runner, this);
            }
        }
    public:
        ~Executor() {
            running.store(false, std::memory_order_release);
            cv.notify_all();

            for (auto& t: threads) {
                t.join();
            }
        }

        Executor() { init_threads(std::thread::hardware_concurrency()); }
        explicit Executor(const u32 n_threads) { init_threads(n_threads); }

        template<typename F, typename... Args>
        requires Invocable<F, Args...>
        auto execute_task(F&& func, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
            //prepare the task
            using result_t = std::invoke_result_t<F, Args...>;

            auto task = std::packaged_task<result_t()>(
                [func = std::forward<F>(func), ...args = std::forward<Args>(args)]() mutable {
                    return std::invoke(std::forward<F>(func), std::forward<Args>(args)...);
                }
            );

            std::future<result_t> fut = task.get_future();


            {
                std::lock_guard lk(qmtx);
                tasks.emplace([task = std::move(task)]() mutable { task(); });
            }

            cv.notify_one();
            return fut;
        }

        //blocks until all tasks are finished
        void wait() {
            if (tasks.empty() && !active_tasks.load(std::memory_order::acquire)) return;
            std::unique_lock lk(qmtx);
            cv_wait.wait(lk, [this] { return tasks.empty() && active_tasks.load(std::memory_order::acquire) == 0; });
        }

        //the number of currently active tasks
        int active() const {
            return active_tasks.load();
        }

        //returns the number of threads
        [[nodiscard]] u32 size() const {
            return threads.size();
        }
    };
}

#endif
