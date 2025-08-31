#include <iostream>
#include <vector>
#include <limits>
#include <algorithm>
#include <string>
#include <sstream>

// 第二版本：大模型帮助实现所有子函数

// 井字棋游戏状态
class TicTacToeState {
public:
    char board[3][3]; // 棋盘
    char currentPlayer; // 当前玩家 ('X' 或 'O')

    // 构造函数，初始化空棋盘
    TicTacToeState() {
        // 初始化棋盘
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                board[i][j] = ' ';
            }
        }
        currentPlayer = 'X'; // X先手
    }

    // 复制构造函数
    TicTacToeState(const TicTacToeState& other) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                board[i][j] = other.board[i][j];
            }
        }
        currentPlayer = other.currentPlayer;
    }

    // 判断是否是终止状态
    bool isTerminal() const {
        // 检查是否有玩家获胜
        if (checkWin('X') || checkWin('O')) {
            return true;
        }
        
        // 检查是否平局（棋盘已满）
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (board[i][j] == ' ') {
                    return false; // 还有空位，游戏没有结束
                }
            }
        }
        
        return true; // 棋盘已满，平局
    }

    // 检查玩家是否获胜
    bool checkWin(char player) const {
        // 检查行
        for (int i = 0; i < 3; i++) {
            if (board[i][0] == player && board[i][1] == player && board[i][2] == player) {
                return true;
            }
        }
        
        // 检查列
        for (int j = 0; j < 3; j++) {
            if (board[0][j] == player && board[1][j] == player && board[2][j] == player) {
                return true;
            }
        }
        
        // 检查对角线
        if (board[0][0] == player && board[1][1] == player && board[2][2] == player) {
            return true;
        }
        
        if (board[0][2] == player && board[1][1] == player && board[2][0] == player) {
            return true;
        }
        
        return false;
    }

    // 获取状态的评估值
    int evaluate() const {
        if (checkWin('X')) {
            return 10; // X获胜，返回正分
        } else if (checkWin('O')) {
            return -10; // O获胜，返回负分
        } else {
            return 0; // 平局或者游戏未结束
        }
    }

    // 获取所有可能的后继状态
    std::vector<TicTacToeState> getSuccessors() const {
        std::vector<TicTacToeState> successors;
        
        // 遍历棋盘，找到所有空位
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (board[i][j] == ' ') {
                    // 创建新状态
                    TicTacToeState newState(*this);
                    newState.board[i][j] = currentPlayer;
                    newState.currentPlayer = (currentPlayer == 'X') ? 'O' : 'X'; // 切换玩家
                    successors.push_back(newState);
                }
            }
        }
        
        return successors;
    }

    // 执行移动并返回移动是否有效
    bool makeMove(int row, int col) {
        if (row >= 0 && row < 3 && col >= 0 && col < 3 && board[row][col] == ' ') {
            board[row][col] = currentPlayer;
            currentPlayer = (currentPlayer == 'X') ? 'O' : 'X'; // 切换玩家
            return true;
        }
        return false;
    }

    // 打印棋盘
    void printBoard() const {
        std::cout << "-------------" << std::endl;
        for (int i = 0; i < 3; i++) {
            std::cout << "| ";
            for (int j = 0; j < 3; j++) {
                std::cout << board[i][j] << " | ";
            }
            std::cout << std::endl << "-------------" << std::endl;
        }
    }
};

// Minimax核心算法
int minimax(const TicTacToeState& state, int depth, bool maximizingPlayer) {
    // 如果到达终止状态或深度限制，返回评估值
    if (depth == 0 || state.isTerminal()) {
        return state.evaluate();
    }

    if (maximizingPlayer) {
        // 最大化玩家
        int bestValue = std::numeric_limits<int>::min();
        std::vector<TicTacToeState> successors = state.getSuccessors();
        
        for (auto& child : successors) {
            int v = minimax(child, depth - 1, false);
            bestValue = std::max(bestValue, v);
        }
        
        return bestValue;
    } else {
        // 最小化玩家
        int bestValue = std::numeric_limits<int>::max();
        std::vector<TicTacToeState> successors = state.getSuccessors();
        
        for (auto& child : successors) {
            int v = minimax(child, depth - 1, true);
            bestValue = std::min(bestValue, v);
        }
        
        return bestValue;
    }
}

// 带Alpha-Beta剪枝的Minimax算法
int minimaxAlphaBeta(const TicTacToeState& state, int depth, int alpha, int beta, bool maximizingPlayer) {
    // 如果到达终止状态或深度限制，返回评估值
    if (depth == 0 || state.isTerminal()) {
        return state.evaluate();
    }

    if (maximizingPlayer) {
        // 最大化玩家
        int bestValue = std::numeric_limits<int>::min();
        std::vector<TicTacToeState> successors = state.getSuccessors();
        
        for (auto& child : successors) {
            int v = minimaxAlphaBeta(child, depth - 1, alpha, beta, false);
            bestValue = std::max(bestValue, v);
            alpha = std::max(alpha, bestValue);
            if (beta <= alpha) {
                break; // Beta剪枝
            }
        }
        
        return bestValue;
    } else {
        // 最小化玩家
        int bestValue = std::numeric_limits<int>::max();
        std::vector<TicTacToeState> successors = state.getSuccessors();
        
        for (auto& child : successors) {
            int v = minimaxAlphaBeta(child, depth - 1, alpha, beta, true);
            bestValue = std::min(bestValue, v);
            beta = std::min(beta, bestValue);
            if (beta <= alpha) {
                break; // Alpha剪枝
            }
        }
        
        return bestValue;
    }
}

// 找到最佳移动的位置（行、列）
std::pair<int, int> findBestMove(TicTacToeState& state, int depth) {
    int bestRow = -1;
    int bestCol = -1;
    int bestValue = std::numeric_limits<int>::min();
    
    // 遍历所有可能的移动
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (state.board[i][j] == ' ') {
                // 尝试这个移动
                TicTacToeState newState(state);
                newState.board[i][j] = state.currentPlayer;
                newState.currentPlayer = (state.currentPlayer == 'X') ? 'O' : 'X';
                
                // 评估这个移动
                int moveValue = minimaxAlphaBeta(newState, depth - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), false);
                
                // 如果这个移动比之前找到的最佳移动更好，则更新最佳移动
                if (moveValue > bestValue) {
                    bestValue = moveValue;
                    bestRow = i;
                    bestCol = j;
                }
            }
        }
    }
    
    return std::make_pair(bestRow, bestCol);
}

// 解析用户输入的坐标
bool parseUserInput(std::string input, int& row, int& col) {
    // 移除所有括号和多余空格
    input.erase(std::remove(input.begin(), input.end(), '('), input.end());
    input.erase(std::remove(input.begin(), input.end(), ')'), input.end());
    
    // 解析坐标
    std::istringstream iss(input);
    if (iss >> row >> col) {
        return row >= 0 && row < 3 && col >= 0 && col < 3;
    }
    return false;
}

int main() {
    TicTacToeState gameState;
    bool gameOver = false;
    int row, col;
    std::string input;
    
    std::cout << "Tic-Tac-Toe Game V2 - Complete Implementation" << std::endl;
    std::cout << "You are player O, AI is player X" << std::endl;
    std::cout << "Enter position as \"row col\" (both 0-2), e.g., \"1 2\"" << std::endl;
    std::cout << "(You can also use format like \"(1 2)\" if preferred)" << std::endl;
    
    while (!gameOver) {
        // AI turn (X)
        if (gameState.currentPlayer == 'X') {
            std::cout << "AI thinking..." << std::endl;
            auto [bestRow, bestCol] = findBestMove(gameState, 9);
            gameState.makeMove(bestRow, bestCol);
            std::cout << "AI chose position: " << bestRow << " " << bestCol << std::endl;
        }
        // Player turn (O)
        else {
            gameState.printBoard();
            
            bool validInput = false;
            while (!validInput) {
                std::cout << "Your turn (row col): ";
                std::getline(std::cin, input);
                
                if (parseUserInput(input, row, col)) {
                    if (gameState.makeMove(row, col)) {
                        validInput = true;
                    } else {
                        std::cout << "Position " << row << " " << col << " is already taken or out of bounds!" << std::endl;
                    }
                } else {
                    std::cout << "Invalid input! Please enter two numbers from 0-2." << std::endl;
                }
            }
        }
        
        // Check if game is over
        if (gameState.isTerminal()) {
            gameState.printBoard();
            if (gameState.checkWin('X')) {
                std::cout << "AI won!" << std::endl;
            } else if (gameState.checkWin('O')) {
                std::cout << "Congratulations, you won!" << std::endl;
            } else {
                std::cout << "It's a draw!" << std::endl;
            }
            gameOver = true;
        }
    }
    
    return 0;
} 