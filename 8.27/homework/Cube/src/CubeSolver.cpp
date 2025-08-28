#include "Cube.h"
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <chrono>
#include <queue>
#include <functional>

// 构造函数
CubeSolver::CubeSolver(const CubeState& initial) {
    initialState = new CubeState(initial);
}

// 析构函数
CubeSolver::~CubeSolver() {
    delete initialState;
}

// 简单的启发式函数：计算有多少块不在正确位置上
int heuristic(const CubeState& state) {
    auto cubeState = state.getState();
    int misplaced = 0;
    
    // 对于每个面的中心块颜色
    for (int face = 0; face < 6; ++face) {
        Color centerColor = cubeState[face][1][1];
        
        // 检查该面的每个块
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (cubeState[face][i][j] != centerColor) {
                    misplaced++;
                }
            }
        }
    }
    
    return misplaced / 9; // 归一化为面数
}

// 使用A*搜索算法的迭代加深搜索
std::vector<CubeState*> CubeSolver::solveIDDFS(int maxDepth) {
    std::vector<CubeState*> solution;
    
    // 在有限深度内搜索
    for (int depth = 1; depth <= maxDepth; ++depth) {
        std::cout << "尝试深度：" << depth << std::endl;
        
        // 使用优先队列，基于A*算法的f值（g+h）排序
        // g是当前深度，h是启发式函数值
        auto compare = [](const std::pair<CubeState*, int>& a, const std::pair<CubeState*, int>& b) {
            int fa = a.second + heuristic(*a.first);
            int fb = b.second + heuristic(*b.first);
            return fa > fb; // 最小堆
        };
        
        std::priority_queue<std::pair<CubeState*, int>, 
                          std::vector<std::pair<CubeState*, int>>, 
                          decltype(compare)> pq(compare);
        
        std::unordered_set<std::string> visited;
        
        // 记录所有创建的状态，用于内存管理
        std::vector<CubeState*> allStates;
        
        // 初始状态入队
        pq.push({new CubeState(*initialState), 0});
        visited.insert(initialState->getHashCode());
        allStates.push_back(pq.top().first);
        
        int expandedNodes = 0;
        const int MAX_NODES = 50000; // 限制节点扩展数量
        
        while (!pq.empty() && expandedNodes < MAX_NODES) {
            auto [current, g] = pq.top();
            pq.pop();
            expandedNodes++;
            
            // 如果找到目标状态
            if (current->isGoal()) {
                // 构建解决方案路径
                CubeState* state = current;
                while (state != nullptr) {
                    solution.push_back(new CubeState(*state));
                    state = state->getParent();
                }
                
                // 反转路径，从初始状态到目标状态
                std::reverse(solution.begin(), solution.end());
                
                // 释放内存
                for (CubeState* state : allStates) {
                    bool inSolution = false;
                    for (CubeState* solState : solution) {
                        if (state->equals(*solState)) {
                            inSolution = true;
                            break;
                        }
                    }
                    if (!inSolution) {
                        delete state;
                    }
                }
                
                return solution;
            }
            
            // 如果深度未达到限制
            if (g < depth) {
                // 获取下一步可能的状态
                std::vector<CubeState*> nextStates = current->getNextStates();
                
                for (CubeState* next : nextStates) {
                    if (visited.find(next->getHashCode()) == visited.end()) {
                        pq.push({next, g + 1});
                        visited.insert(next->getHashCode());
                        allStates.push_back(next);
                    } else {
                        delete next;
                    }
                }
            }
            
            // 每处理1000个节点输出一次进度
            if (expandedNodes % 1000 == 0) {
                std::cout << "已扩展节点数: " << expandedNodes << ", 队列大小: " << pq.size() << std::endl;
            }
        }
        
        // 清理当前深度的内存
        for (CubeState* state : allStates) {
            delete state;
        }
        
        std::cout << "深度 " << depth << " 搜索完成，扩展了 " << expandedNodes << " 个节点" << std::endl;
    }
    
    std::cout << "在最大深度 " << maxDepth << " 内没有找到解决方案。" << std::endl;
    return solution;
}

// 打印解决方案
void CubeSolver::printSolution(const std::vector<CubeState*>& solution) {
    if (solution.empty()) {
        std::cout << "无解决方案！" << std::endl;
        return;
    }
    
    std::cout << "解决方案步数：" << solution.size() - 1 << std::endl;
    
    // 打印初始状态
    std::cout << "初始状态：" << std::endl;
    solution[0]->printState();
    
    // 打印每一步操作和状态
    for (size_t i = 1; i < solution.size(); ++i) {
        std::cout << "步骤 " << i << "：" << solution[i]->getActionString() << std::endl;
        solution[i]->printState();
    }
    
    // 打印操作序列
    std::cout << "操作序列：";
    for (size_t i = 1; i < solution.size(); ++i) {
        std::cout << solution[i]->getActionString() << " ";
    }
    std::cout << std::endl;
}

// 打印简短解决方案（符合文档要求）
void CubeSolver::printSolutionShortNotation(const std::vector<CubeState*>& solution) {
    if (solution.empty()) {
        std::cout << "无解决方案！" << std::endl;
        return;
    }
    
    // 打印格式化的解决方案序列
    for (size_t i = 1; i < solution.size(); ++i) {
        std::cout << solution[i]->getActionShortNotation() << " ";
    }
    std::cout << std::endl;
} 