#ifndef THREADING_HPP
#define THREADING_HPP
#include <x86intrin.h>

namespace Auxil {

    struct SafeAtomicBoolFlag {
        std::atomic_bool _bool;
        ~SafeAtomicBoolFlag() {
            _bool.store(false, std::memory_order::release);
        }
    };

    //similar to LinkedList, but much smaller, made somewhat specifically
    //for worker thread operations (WorkerThread, Executor)
    template<std::semiregular T>
    class AtomicQueue {
    public:
        static constexpr usize npos = std::numeric_limits<usize>::max();
        struct Node {
            //if this node is valid
            bool valid{false};
            T value{};
            //indices into whatever is next or previous
            usize prev{npos};
            usize next{npos};

            void reset() {
                valid = false;
                value = T();
                prev = npos;
                next = npos;
            }
        };


    private:

        Array<Node> nodes;
        std::atomic<usize> length;
        std::atomic<usize> cur_index{};
        SafeAtomicBoolFlag modifying_flag{false};
        u64 max_spin_cycles{30000000};

        void swap(usize i1, usize i2) {


            if (cur_index == i1) cur_index = i2;
            else if (cur_index == i2) cur_index = i1;

            Node& one = nodes[i1];
            Node& two = nodes[i2];

            bool change2_prev{true};
            bool change2_next{true};


            if (one.next != npos) {
                if (one.next == i2) {
                    nodes[one.next].prev = i2;
                    one.next = i1;
                    change2_prev = false;
                } else {
                    nodes[one.next].prev = i2;
                }
            }
            if (one.prev != npos) {
                if (one.prev == i2) {
                    nodes[one.prev].next = i2;
                    one.prev = i1;
                    change2_next = false;
                } else {
                    nodes[one.prev].next = i2;
                }
            }

            if (two.next != npos && change2_next) nodes[two.next].prev = i1;
            if (two.prev != npos && change2_prev) nodes[two.prev].next = i1;

            std::swap(one, two);
        }

        void _reserve(usize new_size) {
            Array new_allocation = Array<Node>(new_size);

            auto rng = nodes | std::views::take(new_size);
            std::ranges::move(rng, new_allocation.begin());
            length = std::min(new_size, length.load(std::memory_order::acquire));

            nodes = std::move(new_allocation);
        }
    public:

        AtomicQueue() = default;

        AtomicQueue& reserve(usize new_size) {
            const u64 start = __rdtsc();
            while (modifying_flag._bool.exchange(true, std::memory_order_acquire)) {
                if (__rdtsc() - start > max_spin_cycles) std::this_thread::yield();
            }

            _reserve(new_size);

            modifying_flag._bool.store(false, std::memory_order::release);
            return *this;
        }

        T& push_back(const T& value) {
            const u64 start = __rdtsc();
            while (modifying_flag._bool.exchange(true, std::memory_order_acquire)) {
                if (__rdtsc() - start > max_spin_cycles) std::this_thread::yield();
            }
            if (length == nodes.size()) {
                //ensures we get at least enough space for the new element
                _reserve(nodes.size()*2+1);
            }
            //if the array is empty, there is no 'back'
            //if the array has just 1 element, then the back is also the front, so the zero'th element,
            //if the array has 2 or more elements, then the back should be the first element so long as
            //nothing has gone wrong
            usize back_ind = length > 0 /*If empty*/ ? (length >= 2 /*If it has 2 or more elements*/ ? 1:0) : npos;
            Node& n = nodes.emplace_at(length++, true, value, back_ind, npos);

            //set it to 1 because we swap its current position with 1 later, if this is the only element currently added
            //then the npos check will stop anything from happening,
            if (back_ind != npos) nodes[back_ind].next = length-1;

            //this is to make sure we keep the last element in the second index
            //this also will fix the fact that the back index which we pointed to previously has now changed and all
            //references should stay consistent
            if (length > 2) swap(1, length-1);

            if (cur_index == npos) cur_index = 0;

            modifying_flag._bool.store(false, std::memory_order::release);
            //return the new value
            return n.value;
        }

        bool try_pop_front(T& out) {
            const u64 start = __rdtsc();
            while (modifying_flag._bool.exchange(true, std::memory_order_acquire)) {
                if (__rdtsc() - start > max_spin_cycles) std::this_thread::yield();
            }
            if (length.load(std::memory_order::acquire) == 0) {
                modifying_flag._bool.store(false, std::memory_order::release);
                return false;
            }

            Node* front{nullptr};
            front = &nodes[0];
            T res = std::move(front->value);

            if (cur_index == 0) cur_index = front->next;

            //move this front to the end
            swap(0, length-1);
            front = &nodes[length-1];

            //move its previous node to the front
            if (front->next != npos) swap(front->next, 0);

            front->reset();

            --length;
            modifying_flag._bool.store(false, std::memory_order::release);
            out = res;
            return true;
        }

        void set_max_spin_cycles(const u64 amt) {
            max_spin_cycles = amt;
        }

        u64 get_spins() const {
            return max_spin_cycles;
        }

        usize size() const {
            return length.load(std::memory_order::acquire);
        }
    };

    class WorkerThread {
        AtomicQueue<std::function<void()>> tasks;
        std::thread thread;
        SafeAtomicBoolFlag running;


        std::shared_ptr<std::atomic_bool> failsafe{std::make_shared<std::atomic_bool>(true)};

        void run() {
            //this loop is somewhat similar to the one from the WorkerThread class
            u64 start = __rdtsc();
            while (failsafe->load(std::memory_order::acquire)) {
                std::function<void()> task;
                if (tasks.try_pop_front(task)) {
                    start = __rdtsc();
                    //now we own the lock so we'll start draining

                    //store true in the running flag
                    running._bool.store(true, std::memory_order::release);
                    do {
                        task();
                    } while (tasks.try_pop_front(task));
                    //store false in the running flag
                    running._bool.store(false, std::memory_order::release);



                } else if (__rdtsc()-start > tasks.get_spins()) {
                    std::this_thread::yield();
                }
            }
        }

    public:
        ~WorkerThread() {
            failsafe->store(false, std::memory_order::release);
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
                tasks.push_back([task] { (*task)(); });
            }
            return fut;
        }

        //waits on the thread to finish
        void wait() const {
            bool ready = false;
            //initial check
            ready = !running._bool.load(std::memory_order::acquire) && tasks.size() == 0;

            if (!ready) {
                const u64 start = __rdtsc();
                while (!ready) {
                    ready = !running._bool.load(std::memory_order::acquire) && tasks.size() == 0;
                    if (!ready && __rdtsc()-start > tasks.get_spins()) {
                        std::this_thread::yield();
                    }
                }
            }
        }
    };

    //can execute many tasks simultaneously
    class Executor {
        struct Worker {
            std::thread first;
            std::unique_ptr<SafeAtomicBoolFlag> second = std::make_unique<SafeAtomicBoolFlag>(false);

            Worker(std::thread t) : first(std::move(t)) {}
        };
        std::vector<Worker> threads;

        AtomicQueue<std::function<void()>> tasks;

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
            u64 start = __rdtsc();
            while (failsafe->load(std::memory_order::acquire)) {
                std::function<void()> task;
                if (tasks.try_pop_front(task)) {
                    start = __rdtsc();
                    //now we own the lock so we'll start draining

                    //store true in the running flag
                    threads[ind].second->_bool.store(true, std::memory_order::release);
                    do {
                        task();
                    } while (tasks.try_pop_front(task));
                    //store false in the running flag
                    threads[ind].second->_bool.store(false, std::memory_order::release);



                } else if (__rdtsc()-start > tasks.get_spins()) {
                    std::this_thread::yield();
                }
            }
        }



    public:

        ~Executor() {
            failsafe->store(false, std::memory_order::release);
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

        void set_max_spin_cycles(u64 spins) {
            tasks.set_max_spin_cycles(spins);
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

            std::future<result_t> fut = task->get_future();

            tasks.push_back([task] { (*task)(); });

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
                if (!running_flag->_bool.load(std::memory_order::acquire)) { ready = true; break; }
            }

            //if we aren't running:
            if (!ready) {
                const u64 start = __rdtsc();
                while (!ready) {

                    for (auto &[t, running_flag]: threads) {
                        if (!running_flag->_bool.load(std::memory_order::acquire)) { ready = true; break; }
                    }
                    if (!ready && __rdtsc()-start > tasks.get_spins()) {
                        std::this_thread::yield();
                    }
                }
            }
        }

        //waits for all threads to be finished
        void wait() {
            bool ready = false;
            //initial check
            ready = true;
            for (auto &[t, running_flag]: threads) {
                if (running_flag->_bool.load(std::memory_order::acquire)) { ready = false; break; }
            }

            if (!ready) {
                const u64 start = __rdtsc();
                while (!ready) {
                    ready = true;
                    for (auto &[t, running_flag]: threads) {
                        if (running_flag->_bool.load(std::memory_order::acquire)) { ready = false; break; }
                    }
                    if (!ready && __rdtsc()-start > tasks.get_spins()) {
                        std::this_thread::yield();
                    }
                }

            }
        }


        //waits a little bit and returns if tasks are done
        template<typename Rep, typename Dur>
        bool wait(std::chrono::duration<Rep, Dur> duration) {
            bool ready = false;
            //initial check
            ready = true;
            for (auto &[t, running_flag]: threads) {
                if (running_flag->_bool.load(std::memory_order::acquire)) { ready = false; break; }
            }

            if (!ready) {
                std::this_thread::sleep_for(duration);
                ready = true;
                for (auto &[t, running_flag]: threads) {
                    if (running_flag->_bool.load(std::memory_order::acquire)) { ready = false; break; }
                }
            }

            return ready;
        }
    };
}


#endif
