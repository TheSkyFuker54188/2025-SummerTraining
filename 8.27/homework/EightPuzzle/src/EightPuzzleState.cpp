#include "EightPuzzle.h"
#include <iostream>
#include <sstream>

// 默认构造函数
EightPuzzleState::EightPuzzleState() {
    // 默认创建有序的目标状态 1-8 和一个空格
    board = std::vector<std::vector<int>>(3, std::vector<int>(3));
    int num = 1;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (i == 2 && j == 2) {
                board[i][j] = 0; // 空格用0表示
            } else {
                board[i][j] = num++;
            }
        }
    }
    emptyRow = 2;
    emptyCol = 2;
    parent = nullptr;
    action = "";
}

// 带参数的构造函数
EightPuzzleState::EightPuzzleState(const std::vector<std::vector<int>>& b) {
    board = b;
    findEmptyPosition();
    parent = nullptr;
    action = "";
}

// 复制构造函数
EightPuzzleState::EightPuzzleState(const EightPuzzleState& other) {
    board = other.board;
    emptyRow = other.emptyRow;
    emptyCol = other.emptyCol;
    parent = other.parent;
    action = other.action;
}

// 析构函数
EightPuzzleState::~EightPuzzleState() {
    // 不需要在这里释放parent，因为parent是由搜索算法管理的
}

// 获取棋盘状态
std::vector<std::vector<int>> EightPuzzleState::getBoard() const {
    return board;
}

// 设置父状态
void EightPuzzleState::setParent(EightPuzzleState* p) {
    parent = p;
}

// 获取父状态
EightPuzzleState* EightPuzzleState::getParent() const {
    return parent;
}

// 设置动作
void EightPuzzleState::setAction(const std::string& a) {
    action = a;
}

// 获取动作
std::string EightPuzzleState::getAction() const {
    return action;
}

// 判断是否为目标状态（1-8按顺序排列，空格在右下角）
bool EightPuzzleState::isGoal() const {
    int num = 1;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (i == 2 && j == 2) {
                if (board[i][j] != 0) {
                    return false;
                }
            } else if (board[i][j] != num++) {
                return false;
            }
        }
    }
    return true;
}

// 获取可能的下一步状态
std::vector<EightPuzzleState*> EightPuzzleState::getNextStates() {
    std::vector<EightPuzzleState*> nextStates;
    
    // 空格可能的移动方向：上、下、左、右
    const int dr[] = {-1, 1, 0, 0};
    const int dc[] = {0, 0, -1, 1};
    const std::string actions[] = {"上", "下", "左", "右"};
    
    for (int i = 0; i < 4; ++i) {
        int newRow = emptyRow + dr[i];
        int newCol = emptyCol + dc[i];
        
        // 检查移动是否有效（不越界）
        if (newRow >= 0 && newRow < 3 && newCol >= 0 && newCol < 3) {
            // 创建新状态
            EightPuzzleState* newState = new EightPuzzleState(*this);
            
            // 交换空格和数字
            newState->board[emptyRow][emptyCol] = newState->board[newRow][newCol];
            newState->board[newRow][newCol] = 0;
            
            // 更新空格位置
            newState->emptyRow = newRow;
            newState->emptyCol = newCol;
            
            // 设置父状态和动作
            newState->setParent(this);
            newState->setAction(actions[i]);
            
            nextStates.push_back(newState);
        }
    }
    
    return nextStates;
}

// 打印当前状态
void EightPuzzleState::printState() const {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (board[i][j] == 0) {
                std::cout << "  "; // 空格显示为两个空格
            } else {
                std::cout << board[i][j] << " ";
            }
        }
        std::cout << std::endl;
    }
}

// 获取状态的哈希值，用于判重
std::string EightPuzzleState::getHashCode() const {
    std::stringstream ss;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            ss << board[i][j];
        }
    }
    return ss.str();
}

// 判断两个状态是否相等
bool EightPuzzleState::equals(const EightPuzzleState& other) const {
    return getHashCode() == other.getHashCode();
}

// 初始化空格位置
void EightPuzzleState::findEmptyPosition() {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (board[i][j] == 0) {
                emptyRow = i;
                emptyCol = j;
                return;
            }
        }
    }
    // 如果找不到空格，设置为默认位置
    emptyRow = 2;
    emptyCol = 2;
} 