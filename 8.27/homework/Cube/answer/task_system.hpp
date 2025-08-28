/*
 *  Description:    A generic task system for DFS-like algorithms.
 *                  This file defines the interfaces of the task system,
 *                  and provides two implementations: single-thread and multi-thread.
 *
 *  Author(s):      Nictheboy Li <nictheboy@outlook.com>
 *
 *  Last Updated:   2024-06-25
 *
 */

#pragma once
#include <concurrentqueue.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <queue>
#include <thread>

/// @brief The interface of a task enqueuer.
/// @tparam TTask The type of the task.
template <typename TTask>
class IEnqueuer {
   public:
    virtual ~IEnqueuer() {}
    virtual void Enqueue(TTask task) = 0;
};

/// @brief The interface of a task handler.
/// @tparam TTask The type of the task.
template <typename TTask>
class IHandler {
   public:
    virtual ~IHandler() {}
    virtual void Handle(TTask& task, IEnqueuer<TTask>& enqueuer) = 0;
};

/// @brief The interface of a task system.
/// @tparam TTask The type of the task.
template <typename TTask>
class IHandleQueueSystem {
   public:
    virtual ~IHandleQueueSystem() {}
    virtual void Solve(TTask& initial_task, IHandler<TTask>& handler) = 0;
};

/// @brief The implementation of a task system using a single thread.
/// @tparam TTask The type of the task.
template <typename TTask>
class HandleQueueSystemSingleThread : public IHandleQueueSystem<TTask> {
   private:
    class Enqueuer : public IEnqueuer<TTask> {
       private:
        std::queue<TTask>& queue;

       public:
        Enqueuer(std::queue<TTask>& queue)
            : queue(queue) {}
        void Enqueue(TTask task) override { queue.push(task); }
    };

   public:
    void Solve(TTask& initial_task, IHandler<TTask>& handler) override {
        std::queue<TTask> queue;
        Enqueuer enqueuer(queue);
        enqueuer.Enqueue(initial_task);
        while (!queue.empty()) {
            TTask task = queue.front();
            queue.pop();
            handler.Handle(task, enqueuer);
        }
    }
};

/// @brief The implementation of a task system using multiple threads.
/// @tparam TTask The type of the task.
template <typename TTask>
class HandleQueueSystemMultiThread : public IHandleQueueSystem<TTask> {
    const int concurrency;

   private:
    class Enqueuer : public IEnqueuer<TTask> {
       private:
        moodycamel::ConcurrentQueue<TTask>& queue;

       public:
        Enqueuer(moodycamel::ConcurrentQueue<TTask>& queue)
            : queue(queue) {}
        void Enqueue(TTask task) override {
            queue.enqueue(task);
        }
    };

   public:
    HandleQueueSystemMultiThread(int concurrency = std::thread::hardware_concurrency())
        : concurrency(concurrency) {}

    void Solve(TTask& initial_task, IHandler<TTask>& handler) override {
        moodycamel::ConcurrentQueue<TTask> queue;
        std::atomic_int run_counter = 0;
        Enqueuer enqueuer(queue);
        enqueuer.Enqueue(initial_task);
        std::unique_ptr<std::unique_ptr<std::thread>[]> thread_pool =
            std::make_unique<std::unique_ptr<std::thread>[]>(concurrency);
        for (int i = 0; i < concurrency; i++) {
            thread_pool[i] = std::make_unique<std::thread>([&] {
                while (true) {
                    TTask task;
                    bool request_quit = false;
                    while (!queue.try_dequeue(task)) {
                        if (run_counter == 0) {
                            request_quit = true;
                            break;
                        }
                        std::this_thread::yield();
                    }
                    if (request_quit)
                        break;
                    try {
                        run_counter++;
                        handler.Handle(task, enqueuer);
                        run_counter--;
                    } catch (...) {
                        printf(
                            "HandleQueueSystemMultiThread<CubeTask*>::Solve(CubeTask*&, IHandler<CubeTask*>&): "
                            "Exception caught\n");
                    }
                }
            });
        }
        for (int i = 0; i < concurrency; i++) {
            thread_pool[i]->join();
        }
    }
};
