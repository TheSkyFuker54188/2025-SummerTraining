#include "EightPuzzle.h"
#include <iostream>
#include <queue>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

// 添加彩色显示状态的前向声明（函数在main.cpp中定义）
void displayColorState(const EightPuzzleState& state);

// 构造函数
EightPuzzleSolver::EightPuzzleSolver(const std::vector<std::vector<int>>& initial, const std::vector<std::vector<int>>& goal) {
    initialState = new EightPuzzleState(initial);
    goalState = new EightPuzzleState(goal);
}

// 析构函数
EightPuzzleSolver::~EightPuzzleSolver() {
    delete initialState;
    delete goalState;
}

// 广度优先搜索
std::vector<EightPuzzleState*> EightPuzzleSolver::solveBFS() {
    std::vector<EightPuzzleState*> solution;
    
    // 检查问题是否有解
    if (!isSolvable(*initialState)) {
        std::cout << "此八数码问题无解！" << std::endl;
        return solution;
    }
    
    // 使用队列进行BFS
    std::queue<EightPuzzleState*> queue;
    std::unordered_set<std::string> visited;
    
    // 将初始状态加入队列
    queue.push(initialState);
    visited.insert(initialState->getHashCode());
    
    // 记录搜索的节点数
    int nodesExplored = 0;
    
    // 开始BFS
    while (!queue.empty() && nodesExplored < 100000) { // 添加节点探索上限
        EightPuzzleState* current = queue.front();
        queue.pop();
        nodesExplored++;
        
        // 打印当前状态，使用彩色显示
        std::cout << "当前状态：" << std::endl;
        displayColorState(*current); // 使用彩色显示替代printState
        std::cout << "动作：" << current->getAction() << std::endl;
        std::cout << "已探索节点数：" << nodesExplored << std::endl;
        std::cout << "------------------------" << std::endl;
        
        // 判断是否达到目标状态
        if (current->isGoal()) {
            // 构建解决方案路径
            EightPuzzleState* state = current;
            while (state != nullptr) {
                solution.push_back(state);
                state = state->getParent();
            }
            // 反转路径，从初始状态到目标状态
            std::reverse(solution.begin(), solution.end());
            return solution;
        }
        
        // 获取下一步可能的状态
        std::vector<EightPuzzleState*> nextStates = current->getNextStates();
        for (EightPuzzleState* next : nextStates) {
            if (visited.find(next->getHashCode()) == visited.end()) {
                queue.push(next);
                visited.insert(next->getHashCode());
            } else {
                // 如果状态已经访问过，释放内存
                delete next;
            }
        }
    }
    
    std::cout << "搜索超出限制，未找到解决方案！" << std::endl;
    return solution; // 如果找不到解决方案，返回空向量
}

// A*搜索算法，使用曼哈顿距离作为启发式函数
std::vector<EightPuzzleState*> EightPuzzleSolver::solveAStar() {
    std::vector<EightPuzzleState*> solution;
    
    // 检查问题是否有解
    if (!isSolvable(*initialState)) {
        std::cout << "此八数码问题无解！" << std::endl;
        return solution;
    }
    
    // 使用优先队列实现A*搜索，优先队列按F值(G+H)排序
    std::priority_queue<EightPuzzleState*, std::vector<EightPuzzleState*>, StateComparison> openList;
    std::unordered_set<std::string> closedList; // 记录已访问的状态
    std::unordered_map<std::string, EightPuzzleState*> allStates; // 记录所有创建的状态，用于内存管理
    
    // 初始状态的g值为0，计算h值
    initialState->setGValue(0);
    initialState->setHValue(initialState->calculateManhattanDistance());
    
    // 将初始状态加入开放列表
    openList.push(initialState);
    allStates[initialState->getHashCode()] = initialState;
    
    // 记录搜索的节点数
    int nodesExplored = 0;
    
    // 开始A*搜索
    while (!openList.empty() && nodesExplored < 100000) { // 添加节点探索上限
        // 获取F值最小的状态
        EightPuzzleState* current = openList.top();
        openList.pop();
        
        // 如果当前状态已经在闭集中，继续下一个
        if (closedList.find(current->getHashCode()) != closedList.end()) {
            continue;
        }
        
        // 将当前状态加入闭集
        closedList.insert(current->getHashCode());
        nodesExplored++;
        
        // 打印当前状态，使用彩色显示
        std::cout << "当前状态 (g=" << current->getGValue() << ", h=" << current->getHValue() 
                  << ", f=" << current->getFValue() << ")：" << std::endl;
        displayColorState(*current); // 使用彩色显示替代printState
        std::cout << "动作：" << current->getAction() << std::endl;
        std::cout << "已探索节点数：" << nodesExplored << std::endl;
        std::cout << "------------------------" << std::endl;
        
        // 判断是否达到目标状态
        if (current->isGoal()) {
            // 构建解决方案路径
            EightPuzzleState* state = current;
            while (state != nullptr) {
                solution.push_back(state);
                state = state->getParent();
            }
            // 反转路径，从初始状态到目标状态
            std::reverse(solution.begin(), solution.end());
            
            // 释放内存（除了解决方案路径中的状态）
            for (auto& pair : allStates) {
                if (std::find(solution.begin(), solution.end(), pair.second) == solution.end()) {
                    delete pair.second;
                }
            }
            
            return solution;
        }
        
        // 获取下一步可能的状态
        std::vector<EightPuzzleState*> nextStates = current->getNextStates();
        for (EightPuzzleState* next : nextStates) {
            std::string hashCode = next->getHashCode();
            
            // 如果该状态已经在闭集中，则释放内存并继续
            if (closedList.find(hashCode) != closedList.end()) {
                delete next;
                continue;
            }
            
            // 计算启发式函数值
            next->setHValue(next->calculateManhattanDistance());
            
            // 检查是否已经在开放列表中有相同的状态
            if (allStates.find(hashCode) != allStates.end()) {
                // 比较G值，如果新的路径更短，则更新
                EightPuzzleState* existingState = allStates[hashCode];
                if (next->getGValue() < existingState->getGValue()) {
                    // 更新现有状态的G值和父状态
                    existingState->setGValue(next->getGValue());
                    existingState->setParent(current);
                    existingState->setAction(next->getAction());
                }
                // 释放新创建的状态，因为我们已经有相同的状态
                delete next;
            } else {
                // 加入新状态到开放列表和所有状态映射
                openList.push(next);
                allStates[hashCode] = next;
            }
        }
    }
    
    std::cout << "搜索超出限制，未找到解决方案！" << std::endl;
    
    // 释放所有创建的状态
    for (auto& pair : allStates) {
        delete pair.second;
    }
    
    return solution; // 如果找不到解决方案，返回空向量
}

// 贪心最佳优先搜索，使用曼哈顿距离作为启发式函数
std::vector<EightPuzzleState*> EightPuzzleSolver::solveGreedy() {
    std::vector<EightPuzzleState*> solution;
    
    // 检查问题是否有解
    if (!isSolvable(*initialState)) {
        std::cout << "此八数码问题无解！" << std::endl;
        return solution;
    }
    
    // 使用优先队列实现贪心最佳优先搜索，优先队列仅按H值排序
    std::priority_queue<EightPuzzleState*, std::vector<EightPuzzleState*>, StateComparison> openList;
    std::unordered_set<std::string> closedList; // 记录已访问的状态
    std::unordered_map<std::string, EightPuzzleState*> allStates; // 记录所有创建的状态，用于内存管理
    
    // 初始状态的g值为0，计算h值
    initialState->setGValue(0);
    // 在贪心算法中，我们只关心h值，但是我们仍然使用F值进行排序
    // 为了贪心算法，我们将h值设置为曼哈顿距离，g值设为0（在状态比较时忽略g值）
    initialState->setHValue(initialState->calculateManhattanDistance());
    
    // 将初始状态加入开放列表
    openList.push(initialState);
    allStates[initialState->getHashCode()] = initialState;
    
    // 记录搜索的节点数
    int nodesExplored = 0;
    
    // 开始贪心搜索
    while (!openList.empty() && nodesExplored < 100000) { // 添加节点探索上限
        // 获取H值最小的状态
        EightPuzzleState* current = openList.top();
        openList.pop();
        
        // 如果当前状态已经在闭集中，继续下一个
        if (closedList.find(current->getHashCode()) != closedList.end()) {
            continue;
        }
        
        // 将当前状态加入闭集
        closedList.insert(current->getHashCode());
        nodesExplored++;
        
        // 打印当前状态，使用彩色显示
        std::cout << "当前状态 (g=" << current->getGValue() << ", h=" << current->getHValue() 
                  << ", f=" << current->getFValue() << ")：" << std::endl;
        displayColorState(*current); // 使用彩色显示替代printState
        std::cout << "动作：" << current->getAction() << std::endl;
        std::cout << "已探索节点数：" << nodesExplored << std::endl;
        std::cout << "------------------------" << std::endl;
        
        // 判断是否达到目标状态
        if (current->isGoal()) {
            // 构建解决方案路径
            EightPuzzleState* state = current;
            while (state != nullptr) {
                solution.push_back(state);
                state = state->getParent();
            }
            // 反转路径，从初始状态到目标状态
            std::reverse(solution.begin(), solution.end());
            
            // 释放内存（除了解决方案路径中的状态）
            for (auto& pair : allStates) {
                if (std::find(solution.begin(), solution.end(), pair.second) == solution.end()) {
                    delete pair.second;
                }
            }
            
            return solution;
        }
        
        // 获取下一步可能的状态
        std::vector<EightPuzzleState*> nextStates = current->getNextStates();
        for (EightPuzzleState* next : nextStates) {
            std::string hashCode = next->getHashCode();
            
            // 如果该状态已经在闭集中，则释放内存并继续
            if (closedList.find(hashCode) != closedList.end()) {
                delete next;
                continue;
            }
            
            // 计算启发式函数值（曼哈顿距离）
            next->setHValue(next->calculateManhattanDistance());
            
            // 在贪心算法中，我们忽略g值，只考虑h值
            // 为了重用StateComparison结构，我们将g值设为0，这样比较时就只看h值
            next->setGValue(0);
            
            // 检查是否已经在开放列表中有相同的状态
            if (allStates.find(hashCode) != allStates.end()) {
                // 在贪心算法中，我们不关心路径长度，只关心启发式估计
                // 如果新状态的h值更小，则更新
                EightPuzzleState* existingState = allStates[hashCode];
                if (next->getHValue() < existingState->getHValue()) {
                    // 更新现有状态的父状态和动作
                    existingState->setParent(current);
                    existingState->setAction(next->getAction());
                    existingState->setHValue(next->getHValue());
                }
                // 释放新创建的状态，因为我们已经有相同的状态
                delete next;
            } else {
                // 加入新状态到开放列表和所有状态映射
                openList.push(next);
                allStates[hashCode] = next;
            }
        }
    }
    
    std::cout << "搜索超出限制，未找到解决方案！" << std::endl;
    
    // 释放所有创建的状态
    for (auto& pair : allStates) {
        delete pair.second;
    }
    
    return solution; // 如果找不到解决方案，返回空向量
}

// 打印解决方案
void EightPuzzleSolver::printSolution(const std::vector<EightPuzzleState*>& solution) {
    if (solution.empty()) {
        std::cout << "无解决方案！" << std::endl;
        return;
    }
    
    std::cout << "解决方案步数：" << solution.size() - 1 << std::endl;
    
    for (size_t i = 0; i < solution.size(); ++i) {
        std::cout << "Step " << i << ":" << std::endl;
        displayColorState(*solution[i]); // 使用彩色显示替代printState
        if (i < solution.size() - 1) {
            std::cout << "动作：" << solution[i + 1]->getAction() << std::endl;
        }
        std::cout << "------------------------" << std::endl;
    }
}

// 检查问题是否有解
// 对于八数码问题，只有逆序数对数量的奇偶性相同的两个状态才能相互到达
bool EightPuzzleSolver::isSolvable(const EightPuzzleState& state) const {
    std::vector<std::vector<int>> board = state.getBoard();
    std::vector<int> flatBoard;
    
    // 将二维数组展平为一维数组，去掉空格
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (board[i][j] != 0) {
                flatBoard.push_back(board[i][j]);
            }
        }
    }
    
    // 计算逆序数
    int inversions = 0;
    for (size_t i = 0; i < flatBoard.size(); ++i) {
        for (size_t j = i + 1; j < flatBoard.size(); ++j) {
            if (flatBoard[i] > flatBoard[j]) {
                inversions++;
            }
        }
    }
    
    // 同理，计算目标状态的逆序数
    std::vector<std::vector<int>> goalBoard = goalState->getBoard();
    std::vector<int> flatGoalBoard;
    
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (goalBoard[i][j] != 0) {
                flatGoalBoard.push_back(goalBoard[i][j]);
            }
        }
    }
    
    int goalInversions = 0;
    for (size_t i = 0; i < flatGoalBoard.size(); ++i) {
        for (size_t j = i + 1; j < flatGoalBoard.size(); ++j) {
            if (flatGoalBoard[i] > flatGoalBoard[j]) {
                goalInversions++;
            }
        }
    }
    
    // 判断两个状态的逆序数奇偶性是否相同
    return (inversions % 2) == (goalInversions % 2);
} 