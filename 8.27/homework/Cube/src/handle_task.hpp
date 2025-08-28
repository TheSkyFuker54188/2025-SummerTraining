#pragma once
#include <chrono>
#include <memory>
#include <queue>

/**
 * 任务队列接口
 * 负责将任务加入队列
 */
template <typename TTask>
class TaskEnqueuer {
   public:
    virtual ~TaskEnqueuer() {}
    virtual void AddTask(TTask task) = 0;
};

/**
 * 任务处理接口
 * 负责处理具体任务
 */
template <typename TTask>
class TaskProcessor {
   public:
    virtual ~TaskProcessor() {}
    virtual void ProcessTask(TTask& task, TaskEnqueuer<TTask>& enqueuer) = 0;
};

/**
 * 任务管理系统接口
 * 管理任务的执行流程
 */
template <typename TTask>
class TaskSystem {
   public:
    virtual ~TaskSystem() {}
    virtual void Execute(TTask& initialTask, TaskProcessor<TTask>& processor) = 0;
};

/**
 * 单线程任务系统实现
 * 采用简单队列实现任务管理
 */
template <typename TTask>
class SingleThreadTaskSystem : public TaskSystem<TTask> {
   private:
    // 内部任务队列管理类
    class QueueEnqueuer : public TaskEnqueuer<TTask> {
       private:
        std::queue<TTask>& taskQueue;

       public:
        QueueEnqueuer(std::queue<TTask>& queue) : taskQueue(queue) {}
        void AddTask(TTask task) override { taskQueue.push(task); }
    };

   public:
    /**
     * 执行初始任务并处理所有后续任务
     * @param initialTask 初始任务
     * @param processor 任务处理器
     */
    void Execute(TTask& initialTask, TaskProcessor<TTask>& processor) override {
        std::queue<TTask> taskQueue;
        QueueEnqueuer enqueuer(taskQueue);
        
        // 将初始任务加入队列
        enqueuer.AddTask(initialTask);
        
        // 处理队列中所有任务直到队列为空
        while (!taskQueue.empty()) {
            TTask currentTask = taskQueue.front();
            taskQueue.pop();
            processor.ProcessTask(currentTask, enqueuer);
        }
    }
}; 