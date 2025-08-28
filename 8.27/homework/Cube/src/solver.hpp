#pragma once

#include <iostream>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <string>
#include "cube.hpp"
#include "handle_task.hpp"

/**
 * 魔方求解任务
 * 包含魔方当前状态和操作序列
 */
typedef struct CubeTask {
    // 魔方当前状态
    Cube cubeState;
    
    // 到达当前状态的操作序列
    std::vector<MoveAction> moveHistory;

    // 构造函数
    CubeTask(Cube&& state, std::vector<MoveAction>&& moves)
        : cubeState(state), moveHistory(moves) {}
        
}* PCubeTask;

/**
 * 魔方求解器
 * 使用BFS搜索算法寻找魔方解法
 */
class CubeSolver : public TaskProcessor<PCubeTask> {
private:
    // 搜索深度限制
    const int depthLimit;
    
    // 可用操作数量和操作集合
    int actionCount;
    MoveAction moveSet[18];
    
    // 控制参数
    bool showDebug;           // 是否显示调试信息
    bool stopAtFirstSolution; // 是否找到第一个解就停止
    
    // 求解状态
    std::mutex solutionLock;
    bool hasSolution = false;
    std::string solution;
    
    // 已访问状态集合（用于去重）
    std::unordered_map<std::string, bool> visitedStates;
    
public:
    /**
     * 构造函数
     */
    CubeSolver(int maxDepth, bool debug, bool stopFirst)
        : depthLimit(maxDepth), showDebug(debug), stopAtFirstSolution(stopFirst) {
        
        // 初始化可用操作集合
        actionCount = sizeof(ALL_ACTIONS) / sizeof(ALL_ACTIONS[0]);
        for (int i = 0; i < actionCount; i++) {
            moveSet[i] = ConvertToMove(ALL_ACTIONS[i]);
        }
    }
    
    /**
     * 处理一个魔方任务
     */
    void ProcessTask(PCubeTask& task, TaskEnqueuer<PCubeTask>& enqueuer) override {
        // 检查初始状态是否已解决
        if (task->moveHistory.empty() && task->cubeState.IsSolved()) {
            std::lock_guard<std::mutex> lock(solutionLock);
            solution = "初始状态已解决，无需操作";
            hasSolution = true;
            delete task;
            return;
        }
        
        // 检查当前状态是否已解决
        if (task->cubeState.IsSolved()) {
            std::lock_guard<std::mutex> lock(solutionLock);
            
            // 检查是否为更短的解法
            if (!hasSolution || task->moveHistory.size() < solution.size()) {
                hasSolution = true;
                
                // 构建解决方案字符串
                solution.clear();
                for (const auto& move : task->moveHistory) {
                    StandardAction stdMove = ConvertToStandard(move);
                    solution += std::to_string(stdMove.action_index);
                    solution += stdMove.is_positive ? "+" : "-";
                    solution += " ";
                }
                
                // 移除末尾空格
                if (!solution.empty()) {
                    solution.pop_back();
                }
            }
        } 
        else if ((!hasSolution || !stopAtFirstSolution) && 
                 task->moveHistory.size() < static_cast<size_t>(depthLimit)) {
            
            // 计算当前状态的哈希值，用于去重
            std::string stateKey = task->cubeState.ToString();
            
            // 检查状态是否已访问
            if (visitedStates.find(stateKey) == visitedStates.end()) {
                // 标记状态为已访问
                visitedStates[stateKey] = true;
                
                // 尝试所有可能的操作
                for (int i = 0; i < actionCount; i++) {
                    MoveAction nextMove = moveSet[i];
                    
                    // 执行操作，获取新状态
                    Cube nextState = task->cubeState.DoRotation(nextMove);
                    
                    // 构建新的操作序列
                    std::vector<MoveAction> nextHistory = task->moveHistory;
                    nextHistory.push_back(nextMove);
                    
                    // 输出调试信息
                    if (showDebug) {
                        std::cout << "===== 调试信息 =====" << std::endl;
                        std::cout << "搜索深度: " << nextHistory.size() << std::endl;
                        std::cout << "操作序列: ";
                        
                        for (const auto& move : nextHistory) {
                            StandardAction stdMove = ConvertToStandard(move);
                            std::cout << stdMove.action_index
                                    << (stdMove.is_positive ? "+" : "-") << " ";
                        }
                        
                        std::cout << std::endl;
                        std::cout << nextState.ToString() << std::endl;
                    }
                    
                    // 创建新任务并添加到队列
                    enqueuer.AddTask(new CubeTask(std::move(nextState), std::move(nextHistory)));
                }
            }
        }
        
        // 释放当前任务
        delete task;
    }
    
    /**
     * 判断是否找到解决方案
     */
    bool HasSolution() const {
        return hasSolution;
    }
    
    /**
     * 获取解决方案
     */
    std::string GetSolution() const {
        return solution;
    }
}; 