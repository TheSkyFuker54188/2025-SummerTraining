#ifndef TASK_HANDLER_H_INCLUDED
#define TASK_HANDLER_H_INCLUDED

/*==============================================
 * 任务处理系统
 * 实现通用任务队列与处理框架
 *==============================================*/

// 系统头文件
#include <chrono>
#include <memory>
#include <queue>
#include <functional>
#include <stdexcept>

namespace cube {

// 前置声明
template <typename T>
class TaskEnqueuer;

template <typename T>
class TaskProcessor;

template <typename T>
class TaskSystem;

// 任务处理相关异常
class TaskException : public std::runtime_error {
public:
    explicit TaskException(const std::string& message) : std::runtime_error(message) {}
};

class TaskQueueEmptyException : public TaskException {
public:
    TaskQueueEmptyException() : TaskException("任务队列为空") {}
};

class TaskProcessingException : public TaskException {
public:
    explicit TaskProcessingException(const std::string& message) 
        : TaskException("任务处理错误: " + message) {}
};

/**
 * 任务入队接口
 * 负责将任务加入处理队列
 */
template <typename T>
class TaskEnqueuer {
public:
    /**
     * 虚析构函数
     */
    virtual ~TaskEnqueuer() = default;
    
    /**
     * 将任务添加到队列
     * @param task 要添加的任务
     */
    virtual void AddTask(T task) = 0;
    
    /**
     * 获取队列中当前任务数量
     * @return 队列中的任务数
     */
    virtual size_t GetQueueSize() const = 0;
};

/**
 * 任务处理器接口
 * 负责处理具体任务逻辑
 */
template <typename T>
class TaskProcessor {
public:
    /**
     * 虚析构函数
     */
    virtual ~TaskProcessor() = default;
    
    /**
     * 处理单个任务
     * @param task 待处理的任务
     * @param enqueuer 任务队列操作接口
     */
    virtual void ProcessTask(T& task, TaskEnqueuer<T>& enqueuer) = 0;
};

/**
 * 任务系统接口
 * 管理任务的执行流程
 */
template <typename T>
class TaskSystem {
public:
    /**
     * 虚析构函数
     */
    virtual ~TaskSystem() = default;
    
    /**
     * 启动任务系统并执行
     * @param initialTask 初始任务
     * @param processor 任务处理器
     */
    virtual void Execute(T& initialTask, TaskProcessor<T>& processor) = 0;
};

/**
 * 单线程任务系统实现
 * 通过队列实现任务调度
 */
template <typename T>
class SingleThreadTaskSystem : public TaskSystem<T> {
private:
    /**
     * 内部队列包装类
     */
    class QueueWrapper : public TaskEnqueuer<T> {
    private:
        std::queue<T>& taskQueue;  // 引用任务队列
        size_t maxSize;            // 记录队列历史最大长度

    public:
        /**
         * 构造函数
         * @param queue 任务队列引用
         */
        explicit QueueWrapper(std::queue<T>& queue) 
            : taskQueue(queue), maxSize(0) {}

        /**
         * 添加任务到队列
         * @param task 要添加的任务
         */
        void AddTask(T task) override { 
            taskQueue.push(task); 
            
            // 更新最大队列长度
            if (taskQueue.size() > maxSize) {
                maxSize = taskQueue.size();
            }
        }
        
        /**
         * 获取当前队列大小
         */
        size_t GetQueueSize() const override {
            return taskQueue.size();
        }
        
        /**
         * 获取历史最大队列大小
         */
        size_t GetMaxQueueSize() const {
            return maxSize;
        }
    };

public:
    /**
     * 构造函数
     */
    SingleThreadTaskSystem() = default;
    
    /**
     * 析构函数
     */
    ~SingleThreadTaskSystem() override = default;
    
    /**
     * 执行任务处理流程
     * @param initialTask 初始任务
     * @param processor 任务处理器
     * @throws TaskException 如果处理过程中出现错误
     */
    void Execute(T& initialTask, TaskProcessor<T>& processor) override {
        try {
            // 创建任务队列
            std::queue<T> taskQueue;
            
            // 创建队列包装器
            QueueWrapper enqueuer(taskQueue);
            
            // 添加初始任务
            enqueuer.AddTask(initialTask);
            
            // 处理队列中的所有任务
            while (!taskQueue.empty()) {
                // 获取队首任务
                T currentTask = taskQueue.front();
                taskQueue.pop();
                
                // 处理当前任务
                processor.ProcessTask(currentTask, enqueuer);
            }
        }
        catch (const std::exception& e) {
            // 转换为任务处理异常并重新抛出
            throw TaskProcessingException(e.what());
        }
    }
};

} // namespace cube

#endif // TASK_HANDLER_H_INCLUDED 