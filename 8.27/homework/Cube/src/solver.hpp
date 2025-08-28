#ifndef CUBE_SOLVER_HPP
#define CUBE_SOLVER_HPP

/*=============================================
 * 引入必要的头文件
 *=============================================*/
#include <iostream>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>  // 新增
#include <iomanip>    // 新增
#include "cube.hpp"
#include "handle_task.hpp"

namespace cube {

class SolverException : public MagicCubeException {
public:
    SolverException(const std::string& msg) 
        : MagicCubeException("求解器错误: " + msg) {}
};

/*=============================================
 * 魔方任务结构体定义
 *=============================================*/
/**
 * 魔方求解任务结构体
 * 包含魔方状态和操作历史
 */
struct CubeTaskData {
    // 魔方当前状态
    Cube cubeState;
    
    // 到达当前状态的操作序列
    std::vector<MoveAction> moveHistory;
    
    // 构造函数
    CubeTaskData(Cube&& state, std::vector<MoveAction>&& moves)
        : cubeState(state), moveHistory(moves) {}
};

// 定义魔方任务指针类型
typedef CubeTaskData* PCubeTask;

/*==============================================
 * 魔方求解器类
 *==============================================*/
/**
 * 魔方求解器
 * 使用广度优先搜索求解魔方
 */
class CubeSolver : public TaskProcessor<PCubeTask> {
    //=================================
    // 内部类型定义
    //=================================
public:
    /**
     * 求解统计信息结构体
     */
    struct Statistics {
        size_t nodesExplored;      // 探索的节点数
        size_t statesGenerated;    // 生成的状态数
        size_t duplicatesSkipped;  // 跳过的重复状态数
        size_t maxQueueSize;       // 队列最大长度
        
        // 构造函数，初始化所有计数为0
        Statistics() : nodesExplored(0), statesGenerated(0), 
                       duplicatesSkipped(0), maxQueueSize(0) {}
                       
        // 转换为字符串表示
        std::string toString() const {
            std::stringstream ss;
            ss << "探索节点数: " << nodesExplored << std::endl
               << "生成状态数: " << statesGenerated << std::endl
               << "跳过重复数: " << duplicatesSkipped << std::endl
               << "队列最大长: " << maxQueueSize;
            return ss.str();
        }
    };

    //=================================
    // 成员变量
    //=================================
private:
    /* 搜索控制参数 */
    const int maxDepthLimit;        // 最大搜索深度
    bool debugModeEnabled;          // 是否启用调试模式
    bool stopAfterFirstSolution;    // 是否找到一个解就停止
    
    /* 标准动作集合 */
    MoveAction availableMoves[18];  // 可用的动作数组
    int moveCount;                  // 动作数量
    
    /* 求解状态 */
    std::mutex solutionGuard;       // 解决方案互斥锁
    bool solutionExists;            // 是否找到解决方案
    std::string solutionPath;       // 解决方案路径
    Statistics stats;               // 统计信息
    
    /* 状态缓存 */
    std::unordered_map<std::string, bool> visitedStatesMap;  // 已访问状态的映射表
    
    //=================================
    // 公共接口
    //=================================
public:
    /**
     * 构造函数
     * @param depth 最大搜索深度
     * @param debug 是否启用调试
     * @param stopOnFirst 是否找到第一个解就停止
     */
    CubeSolver(int depth, bool debug, bool stopOnFirst);
    
    /**
     * 析构函数
     */
    virtual ~CubeSolver() {}
    
    /**
     * 处理魔方任务
     * @param task 任务指针
     * @param enqueuer 任务队列
     */
    virtual void ProcessTask(PCubeTask& task, TaskEnqueuer<PCubeTask>& enqueuer) override;
    
    /**
     * 判断是否找到解决方案
     * @return 如果找到解决方案则返回true
     */
    bool HasSolution() const {
        return solutionExists;
    }
    
    /**
     * 获取解决方案字符串
     * @return 解决方案步骤的字符串表示
     */
    std::string GetSolution() const {
        return solutionPath;
    }
    
    /**
     * 获取求解统计信息
     * @return 统计信息对象
     */
    const Statistics& GetStatistics() const {
        return stats;
    }
    
    //=================================
    // 私有辅助方法
    //=================================
private:
    /**
     * 初始化可用动作集
     */
    void InitializeActions();
    
    /**
     * 生成解决方案描述字符串
     * @param actions 操作序列
     * @return 格式化的解决方案字符串
     */
    std::string GenerateSolutionString(const std::vector<MoveAction>& actions) const;
    
    /**
     * 打印调试信息
     * @param task 当前任务
     * @param newState 新状态
     * @param nextMove 下一个动作
     */
    void PrintDebugInfo(const PCubeTask& task, const Cube& newState, MoveAction nextMove) const;
};

/*==============================================
 * 魔方求解器方法实现
 *==============================================*/

/**
 * 构造函数
 */
CubeSolver::CubeSolver(int depth, bool debug, bool stopOnFirst)
    : maxDepthLimit(depth), debugModeEnabled(debug), stopAfterFirstSolution(stopOnFirst),
      solutionExists(false) {
    // 初始化统计信息
    stats = Statistics();
    
    // 初始化动作集合
    InitializeActions();
}

/**
 * 初始化可用动作集
 */
void CubeSolver::InitializeActions() {
    moveCount = sizeof(ALL_ACTIONS) / sizeof(ALL_ACTIONS[0]);
    for (int i = 0; i < moveCount; i++) {
        try {
            availableMoves[i] = ConvertToMove(ALL_ACTIONS[i]);
        }
        catch (const MagicCubeException& e) {
            throw SolverException("初始化动作时出错: " + std::string(e.what()));
        }
    }
}

/**
 * 处理魔方任务
 */
void CubeSolver::ProcessTask(PCubeTask& task, TaskEnqueuer<PCubeTask>& enqueuer) {
    try {
        stats.nodesExplored++;
        
        // 检查任务有效性
        if (!task) {
            throw SolverException("任务指针为空");
        }
        
        // 检查初始状态是否已解决
        if (task->moveHistory.empty()) {
            if (task->cubeState.IsSolved()) {
                std::lock_guard<std::mutex> lock(solutionGuard);
                solutionPath = "初始状态已解决，无需操作";
                solutionExists = true;
                delete task;
                return;
            }
        }
        
        // 检查当前状态是否已解决
        if (task->cubeState.IsSolved()) {
            std::lock_guard<std::mutex> lock(solutionGuard);
            
            // 如果找到更短的解决方案，或者这是第一个解决方案
            if (!solutionExists || task->moveHistory.size() < solutionPath.length() / 2) {
                solutionExists = true;
                solutionPath = GenerateSolutionString(task->moveHistory);
            }
        } 
        // 如果未解决且未超过最大深度，则继续搜索
        else if ((!solutionExists || !stopAfterFirstSolution) && 
                 task->moveHistory.size() < static_cast<size_t>(maxDepthLimit)) {
                 
            // 生成状态唯一标识
            std::string stateId = task->cubeState.ToString();
            
            // 检查是否已访问此状态
            if (visitedStatesMap.find(stateId) == visitedStatesMap.end()) {
                // 标记状态为已访问
                visitedStatesMap[stateId] = true;
                
                // 尝试所有可能的动作
                for (int i = 0; i < moveCount; i++) {
                    // 更新统计信息
                    stats.statesGenerated++;
                    
                    MoveAction nextMove = availableMoves[i];
                    
                    // 执行操作，获取新状态
                    Cube newState = task->cubeState.DoRotation(nextMove);
                    
                    // 构建新的操作序列
                    std::vector<MoveAction> nextHistory = task->moveHistory;
                    nextHistory.push_back(nextMove);
                    
                    // 如果启用了调试输出，打印调试信息
                    if (debugModeEnabled) {
                        PrintDebugInfo(task, newState, nextMove);
                    }
                    
                    // 创建新任务并加入队列
                    enqueuer.AddTask(new CubeTaskData(std::move(newState), std::move(nextHistory)));
                }
            }
            else {
                // 更新统计信息 - 跳过重复状态
                stats.duplicatesSkipped++;
            }
        }
        
        // 释放当前任务
        delete task;
    }
    catch (const std::exception& e) {
        // 确保任务被释放
        if (task) {
            delete task;
        }
        throw SolverException(std::string("处理任务失败: ") + e.what());
    }
}

/**
 * 生成解决方案描述字符串
 */
std::string CubeSolver::GenerateSolutionString(const std::vector<MoveAction>& actions) const {
    std::stringstream result;
    
    // 格式化输出操作序列
    for (const auto& action : actions) {
        StandardAction stdAction = ConvertToStandard(action);
        result << stdAction.action_index << (stdAction.is_positive ? "+" : "-") << " ";
    }
    
    // 获取结果字符串并移除末尾空格
    std::string finalResult = result.str();
    if (!finalResult.empty()) {
        finalResult.pop_back();
    }
    
    return finalResult;
}

/**
 * 打印调试信息
 */
void CubeSolver::PrintDebugInfo(const PCubeTask& task, const Cube& newState, MoveAction nextMove) const {
    std::cout << "===== 调试信息 =====" << std::endl;
    std::cout << "当前深度: " << (task->moveHistory.size() + 1) << std::endl;
    std::cout << "已探索节点: " << stats.nodesExplored << std::endl;
    
    // 输出当前操作序列
    std::cout << "操作历史: ";
    for (const auto& move : task->moveHistory) {
        StandardAction stdAction = ConvertToStandard(move);
        std::cout << stdAction.action_index << (stdAction.is_positive ? "+" : "-") << " ";
    }
    
    // 输出下一个操作
    StandardAction nextStdAction = ConvertToStandard(nextMove);
    std::cout << "-> " << nextStdAction.action_index << (nextStdAction.is_positive ? "+" : "-") << std::endl;
    
    // 输出新状态
    std::cout << newState.ToString() << std::endl;
    std::cout << "===================" << std::endl;
}

} // namespace cube

#endif // CUBE_SOLVER_HPP 