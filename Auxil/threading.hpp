#ifndef THREADING_HPP
#define THREADING_HPP

namespace Auxil {




    class WorkerThread {
        std::queue<std::function<void()>> tasks;
        std::thread thread;
        atomic_bool running;

        std::mutex mtx;
        std::condition_variable cv;
        std::condition_variable cv_thread_ready;

        std::shared_ptr<std::atomic_bool> failsafe{std::make_shared<std::atomic_bool>(true)};

        void run() {
            //this loop is somewhat similar to the one from the WorkerThread class
            while (failsafe->load(std::memory_order::acquire)) {
                std::unique_lock lk(mtx);
                cv.wait(lk, [this]{ return !failsafe->load(std::memory_order::acquire) || !tasks.empty(); });

                if (failsafe->load(std::memory_order::acquire)) {
                    //now we own the lock so we'll start draining

                    //store true in the running flag
                    running.store(true, std::memory_order::release);
                    while (!tasks.empty() && failsafe->load(std::memory_order::acquire)) {
                        auto task = std::move(tasks.front());
                        tasks.pop();
                        //release the lock
                        lk.unlock();

                        //run the task
                        task();
                        //acquire the lock again to read again
                        lk.lock();
                    }
                    //store false in the running flag
                    running.store(false, std::memory_order::release);
                    //notify anyone waiting on this thread
                    cv_thread_ready.notify_all();
                    lk.unlock();

                } else break;
            }
        }

    public:
        ~WorkerThread() {
            failsafe->store(false, std::memory_order::release);
            cv.notify_one();
            thread.join();
        }

        WorkerThread() : thread(&WorkerThread::run, this) {}


        //this queues a task to be run
        template<typename Func, typename... Args>
        auto queue_task(Func &&func, Args &&... args) -> std::future<std::invoke_result_t<Func, Args...> > {
            using result_t = std::invoke_result_t<Func, Args...>;
            //acquire a lock onto the queue
            auto task = std::make_shared<std::packaged_task<result_t()> >(
                [f = std::forward<Func>(func), ...args = std::forward<Args>(args)]() mutable {
                    return std::invoke(std::move(f), std::move(args)...);
                }
            );

            std::future<result_t> fut = task->get_future(); {
                std::lock_guard lk(mtx);
                tasks.push([task] { (*task)(); });
            }
            cv.notify_one();
            return fut;
        }

        //waits on the thread to finish
        void wait() {

            if (running.load(std::memory_order::acquire) || !tasks.empty()) {
                std::unique_lock lk(mtx);
                //waits until the thread is finished
                cv_thread_ready.wait(lk, [this]{ return !running.load(std::memory_order::acquire) && tasks.empty(); });
            }
        }
    };




    //can execute many tasks simultaneously
    class Executor {
        struct Worker {
            std::thread first;
            std::unique_ptr<std::atomic_bool> second = std::make_unique<std::atomic_bool>(false);

            Worker(std::thread t) : first(std::move(t)) {}
        };
        std::vector<Worker> threads;

        std::queue<std::function<void()>> tasks;

        std::mutex mtx;
        std::condition_variable cv;

        std::condition_variable cv_thread_ready;

        std::shared_ptr<std::atomic_bool> failsafe{std::make_shared<std::atomic_bool>(true)};

        void init_threads(const u32 n) {
            if (n == 0) throw Exception("Error, attempt to create Executor with 0 threads\n"
                                        "Note: there are {} logical processors available on the current machine",
                                        std::thread::hardware_concurrency());


            for (u32 i = 0; i < n; i++) {
                threads.push_back({std::thread(&Executor::run, this, i)});
            }
        }

        void run(const u32 ind) {
            //this loop is somewhat similar to the one from the WorkerThread class
            while (failsafe->load(std::memory_order::acquire)) {
                std::unique_lock lk(mtx);
                cv.wait(lk, [this]{ return !failsafe->load(std::memory_order::acquire) || !tasks.empty(); });

                if (failsafe->load(std::memory_order::acquire)) {
                    //now we own the lock so we'll start draining

                    //store true in the running flag
                    threads[ind].second->store(true, std::memory_order::release);
                    while (!tasks.empty() && failsafe->load(std::memory_order::acquire)) {
                        auto task = std::move(tasks.front());
                        tasks.pop();
                        //allow more tasks to be added
                        lk.unlock();

                        //run the task
                        task();
                        //prepare for the next task
                        lk.lock();
                    }
                    //store false in the running flag
                    threads[ind].second->store(false, std::memory_order::release);
                    //notify anyone waiting on this thread
                    lk.unlock();
                    cv_thread_ready.notify_all();

                } else break;
            }
        }



    public:

        ~Executor() {
            failsafe->store(false, std::memory_order::release);
            cv.notify_all();
            for (u32 i = 0; i < threads.size(); i++) {
                threads[i].first.join();
            }
        }


        Executor()  {
            init_threads(std::thread::hardware_concurrency());
        }

        explicit Executor(const u32 num_threads)  {
            init_threads(num_threads);
        }


        template<typename Func, typename... Args>
        auto execute(Func &&func, Args &&... args) -> std::future<std::invoke_result_t<Func, Args...>> {
            if (threads.size() == 0) throw Exception("Cannot run task because there are no threads to run on\n"
                                                  "Note: there are {} logical processors available on the current machine",
                                                  std::thread::hardware_concurrency());

            using result_t = std::invoke_result_t<Func, Args...>;

            auto task = std::make_shared<std::packaged_task<result_t()> >(
                [f = std::forward<Func>(func), ...args = std::forward<Args>(args)]() mutable {
                    return std::invoke(std::move(f), std::move(args)...);
                }
            );

            std::future<result_t> fut = task->get_future(); {
                std::lock_guard lk(mtx);
                tasks.push([task] { (*task)(); });
            }
            //notify a waiting thread

            cv.notify_one();
            return fut;
        }

        //returns the number of worker threads
        u32 executor_count() const {
            return threads.size();
        }

        //waits for a single thread to be running for a new task
        void wait_for_single() {
            //initial check
            bool ready = false;
            for (auto &[t, running_flag]: threads) {
                if (!running_flag->load(std::memory_order::acquire)) { ready = true; break; }
            }

            //if we aren't running:
            if (!ready) {
                std::unique_lock lk(mtx);
                cv_thread_ready.wait(lk, [&] {
                    for (auto &[t, running_flag]: threads) {
                        if (!running_flag->load(std::memory_order::acquire)) { ready = true; break; }
                    }
                    return ready;
                });
            }
        }

        //waits for all threads to be finished
        void wait() {
            bool ready = false;
            //initial check
            ready = true;
            for (auto &[t, running_flag]: threads) {
                if (running_flag->load(std::memory_order::acquire)) { ready = false; break; }
            }

            if (!ready) {
                std::unique_lock lk(mtx);
                cv_thread_ready.wait(lk, [&] {
                    ready = true;
                    for (auto &[t, running_flag]: threads) {
                        if (running_flag->load(std::memory_order::acquire)) { ready = false; break; }
                    }
                    return ready;
                });
            }
        }


        //waits a little bit and returns if tasks are done
        template<typename Rep, typename Dur>
        bool wait(std::chrono::duration<Rep, Dur> duration) {
            bool ready = false;
            //initial check
            ready = true;
            for (auto &[t, running_flag]: threads) {
                if (running_flag->load(std::memory_order::acquire)) { ready = false; break; }
            }

            if (!ready) {
                std::unique_lock lk(mtx);
                std::this_thread::sleep_for(duration);

                ready = true;
                for (auto &[t, running_flag]: threads) {
                    if (running_flag->load(std::memory_order::acquire)) { ready = false; break; }
                }
            }

            return ready;
        }
    };
}


#endif
