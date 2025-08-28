/*
 *  Description:    任务系统定义，用于搜索算法
 */

#pragma once
#include <chrono>
#include <memory>
#include <queue>

/// @brief 任务入队接口
/// @tparam TTask 任务类型
template <typename TTask>
class IEnqueuer {
   public:
    virtual ~IEnqueuer() {}
    virtual void Enqueue(TTask task) = 0;
};

/// @brief 任务处理接口
/// @tparam TTask 任务类型
template <typename TTask>
class IHandler {
   public:
    virtual ~IHandler() {}
    virtual void Handle(TTask& task, IEnqueuer<TTask>& enqueuer) = 0;
};

/// @brief 任务系统接口
/// @tparam TTask 任务类型
template <typename TTask>
class IHandleQueueSystem {
   public:
    virtual ~IHandleQueueSystem() {}
    virtual void Solve(TTask& initial_task, IHandler<TTask>& handler) = 0;
};

/// @brief 单线程任务系统实现
/// @tparam TTask 任务类型
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
