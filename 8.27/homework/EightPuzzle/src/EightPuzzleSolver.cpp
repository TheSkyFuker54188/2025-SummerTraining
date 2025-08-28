#include "EightPuzzle.h"
#include <iostream>
#include <queue>
#include <stack>
#include <unordered_set>
#include <algorithm>

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
        
        // 打印当前状态
        std::cout << "当前状态：" << std::endl;
        current->printState();
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

// 深度优先搜索，添加最大深度限制
std::vector<EightPuzzleState*> EightPuzzleSolver::solveDFS() {
    std::vector<EightPuzzleState*> solution;
    
    // 检查问题是否有解
    if (!isSolvable(*initialState)) {
        std::cout << "此八数码问题无解！" << std::endl;
        return solution;
    }
    
    // 使用栈进行DFS
    std::stack<std::pair<EightPuzzleState*, int>> stack; // 使用pair存储状态和深度
    std::unordered_set<std::string> visited;
    
    // 最大深度限制
    const int MAX_DEPTH = 20; // 设置合理的深度限制
    
    // 将初始状态加入栈，深度为0
    stack.push({initialState, 0});
    visited.insert(initialState->getHashCode());
    
    // 记录用于释放内存的状态
    std::vector<EightPuzzleState*> allStates;
    
    // 记录搜索的节点数
    int nodesExplored = 0;
    
    // 开始DFS
    while (!stack.empty() && nodesExplored < 10000) { // 添加节点探索上限
        auto [current, depth] = stack.top();
        stack.pop();
        nodesExplored++;
        
        // 打印当前状态
        std::cout << "当前状态 (深度: " << depth << ")：" << std::endl;
        current->printState();
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
            for (EightPuzzleState* state : allStates) {
                if (std::find(solution.begin(), solution.end(), state) == solution.end()) {
                    delete state;
                }
            }
            
            return solution;
        }
        
        // 检查是否超过最大深度
        if (depth >= MAX_DEPTH) {
            continue;
        }
        
        // 获取下一步可能的状态
        std::vector<EightPuzzleState*> nextStates = current->getNextStates();
        // 为了DFS，我们需要从右向左遍历，因为栈是后进先出的
        for (int i = nextStates.size() - 1; i >= 0; --i) {
            EightPuzzleState* next = nextStates[i];
            if (visited.find(next->getHashCode()) == visited.end()) {
                stack.push({next, depth + 1});
                visited.insert(next->getHashCode());
                allStates.push_back(next);
            } else {
                // 如果状态已经访问过，释放内存
                delete next;
            }
        }
    }
    
    std::cout << "搜索超出限制，未找到解决方案！" << std::endl;
    
    // 释放所有创建的状态
    for (EightPuzzleState* state : allStates) {
        delete state;
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
        solution[i]->printState();
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