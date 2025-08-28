#pragma once

#include <iostream>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <string>
#include "cube.hpp"
#include "handle_task.hpp"

/**
 * 魔方任务结构
 * 包含魔方当前状态和到达该状态的操作序列
 */
typedef struct MagicCubeTask {
    Cube currentState;
    std::vector<CubeAction> operationSequence;

    MagicCubeTask(Cube&& state, std::vector<CubeAction>&& operations)
        : currentState(state), operationSequence(operations) {}
}* PMagicCubeTask;

/**
 * 魔方求解器
 * 实现魔方BFS搜索求解
 */
class MagicCubeSolver : public TaskProcessor<PMagicCubeTask> {
   private:
    // 常量配置
    const int maxSearchDepth;                               // 最大搜索深度
    int availableActions = sizeof(StandardActions) / sizeof(StandardActions[0]);  // 可用操作数量
    CubeAction actionSet[sizeof(StandardActions) / sizeof(StandardActions[0])];   // 操作集合
    bool enableDebug;                                       // 是否启用调试输出
    bool stopOnFirstSolution;                               // 是否找到第一个解就停止

    // 状态追踪
    std::mutex solutionMutex;                               // 解决方案互斥锁
    bool solutionFound = false;                             // 是否找到解决方案
    std::string solutionSteps;                              // 解决方案步骤

    // 已访问状态缓存，避免重复搜索
    std::unordered_map<std::string, bool> visitedStates;

   public:
    /**
     * 构造函数
     * @param depth 最大搜索深度
     * @param debug 是否启用调试输出
     * @param stopOnFirst 是否找到第一个解就停止
     */
    MagicCubeSolver(int depth, bool debug, bool stopOnFirst)
        : maxSearchDepth(depth), enableDebug(debug), stopOnFirstSolution(stopOnFirst) {
        // 初始化动作集
        for (int i = 0; i < availableActions; i++) {
            actionSet[i] = ConvertCubeActionFormat(StandardActions[i]);
        }
    }

    /**
     * 处理魔方任务
     * @param task 当前任务
     * @param enqueuer 任务队列
     */
    void ProcessTask(PMagicCubeTask& task, TaskEnqueuer<PMagicCubeTask>& enqueuer) override {
        // 首先检查任务的初始状态是否已经是解决状态
        if (task->operationSequence.size() == 0) {
            if (task->currentState.IsSolved()) {
                std::lock_guard<std::mutex> lock(solutionMutex);
                solutionSteps = "已经是解决状态，无需操作";
                solutionFound = true;
                delete task;
                return;
            }
        }
        
        // 检查当前状态是否已解决
        if (task->currentState.IsSolved()) {
            std::lock_guard<std::mutex> lock(solutionMutex);
            // 只记录最短的解决方案
            if (!solutionFound || task->operationSequence.size() < solutionSteps.size()) {
                solutionFound = true;
                solutionSteps = "";
                
                // 构建解决方案字符串
                for (auto action : task->operationSequence) {
                    CubeActionStandard stdAction = ConvertCubeActionFormat(action);
                    solutionSteps += std::to_string(stdAction.standard_surface_index);
                    solutionSteps += stdAction.is_positive_direction ? "+" : "-";
                    solutionSteps += " ";
                }
                
                if (!solutionSteps.empty()) {
                    solutionSteps.pop_back();  // 移除末尾空格
                }
            }
        } else {
            // 未解决且未超过最大深度，继续搜索
            if ((!solutionFound || !stopOnFirstSolution) && 
                task->operationSequence.size() < (unsigned int)maxSearchDepth) {
                // 生成当前状态的唯一键，用于去重
                std::string stateKey = task->currentState.ToString();
                
                // 检查状态是否已访问过
                if (visitedStates.find(stateKey) == visitedStates.end()) {
                    visitedStates[stateKey] = true;
                    
                    // 尝试所有可能的操作
                    for (int i = 0; i < availableActions; i++) {
                        CubeAction nextAction = actionSet[i];
                        Cube nextState = task->currentState.DoRotation(nextAction);
                        std::vector<CubeAction> nextSequence(task->operationSequence);
                        nextSequence.push_back(nextAction);
                        
                        // 输出调试信息
                        if (enableDebug) {
                            std::cout << "----------------------------" << std::endl;
                            std::cout << "深度: " << nextSequence.size() << std::endl;
                            std::cout << "操作序列: ";
                            for (auto op : nextSequence) {
                                CubeActionStandard stdAction = ConvertCubeActionFormat(op);
                                std::cout << stdAction.standard_surface_index
                                          << (stdAction.is_positive_direction ? "+" : "-") << " ";
                            }
                            std::cout << std::endl << nextState.ToString() << std::endl;
                        }
                        
                        // 将新任务加入队列
                        enqueuer.AddTask(new MagicCubeTask(std::move(nextState), std::move(nextSequence)));
                    }
                }
            }
        }
        
        // 释放当前任务
        delete task;
    }

    /**
     * 是否找到解决方案
     */
    bool HasSolution() const {
        return solutionFound;
    }

    /**
     * 获取解决方案步骤
     */
    std::string GetSolutionSteps() const {
        return solutionSteps;
    }
}; 