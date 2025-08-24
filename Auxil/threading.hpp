#ifndef THREADING_HPP
#define THREADING_HPP

namespace Auxil {




    class WorkerThread {
        std::queue<std::function<void()> > tasks;
        std::thread thread;

        std::mutex mtx;
        std::condition_variable cv;

        std::shared_ptr<std::atomic_bool> failsafe{std::make_shared<std::atomic_bool>(true)};

        void run() {
            while (failsafe->load(std::memory_order::acquire)) {
                std::unique_lock lk(mtx);
                cv.wait(lk, [this] { return !failsafe->load(std::memory_order::acquire) || !tasks.empty(); });
                if (failsafe->load(std::memory_order::acquire)) {
                    //now we own the lock, so we can read the data we need, and pop it
                    auto task = tasks.front();
                    tasks.pop();

                    //unlock the lock since we dont need to read from the queue anymore
                    lk.unlock();

                    task();

                } else break;
            }
        }

    public:
        ~WorkerThread() {
            failsafe->store(false, std::memory_order::release);
            cv.notify_one();
            thread.join();
        }

        WorkerThread() : thread(&WorkerThread::run, this) {
        }


        //this queues a task to be run
        template<typename Func, typename... Args>
        auto queue_task(Func &&func, Args &&... args) -> std::future<std::invoke_result_t<Func, Args...> > {
            using result_t = std::invoke_result_t<Func, Args...>;
            //aquire a lock onto the queue
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
    };


    //can execute many tasks simultaneously
    class Executor {
        std::thread* threads{nullptr};
        u32 num_threads{0};

        std::queue<std::function<void()>> tasks;

        std::mutex mtx;
        std::condition_variable cv;

        std::shared_ptr<std::atomic_bool> failsafe{std::make_shared<std::atomic_bool>(true)};

        void init_threads(const u32 n) {
            if (n == 0) throw Exception("Error, attempt to create Executor with 0 threads\n"
                                        "Note: there are {} logical processors available on the current machine",
                                        std::thread::hardware_concurrency());

            threads = new std::thread[n];
            num_threads = n;
            for (u32 i = 0; i < n; i++) {
                threads[i] = std::thread(&Executor::run, this);
            }
        }

        void run() {
            //this loop is somewhat similar to the one from the WorkerThread class
            while (failsafe->load(std::memory_order::acquire)) {
                std::unique_lock lk(mtx);
                cv.wait(lk, [this]{ return !failsafe->load(std::memory_order::acquire) || !tasks.empty(); });

                if (failsafe->load(std::memory_order::acquire)) {
                    //now we own the lock so well get the task
                    std::function<void()> task = std::move(tasks.front());
                    tasks.pop();

                    //unlock the lock since we no longer need to read anything
                    lk.unlock();

                    task();
                } else break;
            }
        }

    public:

        ~Executor() {
            failsafe->store(false, std::memory_order::release);
            cv.notify_all();
            for (u32 i = 0; i < num_threads; i++) {
                threads[i].join();
            }
            delete[] threads;
            threads = nullptr;
            num_threads = 0;
        }


        Executor() {
            init_threads(std::thread::hardware_concurrency());
        }

        explicit Executor(const u32 num_threads) {
            init_threads(num_threads);
        }


        template<typename Func, typename... Args>
        auto execute(Func &&func, Args &&... args) -> std::future<std::invoke_result_t<Func, Args...>> {
            if (num_threads == 0) throw Exception("Cannot run task because there are no threads to run on\n"
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
            return num_threads;
        }
    };
}


#endif
