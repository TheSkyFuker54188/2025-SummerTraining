#include "EightPuzzle.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

// 定义目标状态
const std::vector<std::vector<int>> GOAL_STATE = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 0}
};

// 定义六种不同的初始状态
const std::vector<std::vector<std::vector<int>>> INITIAL_STATES = {
    // 状态1：比较简单的状态，只需要少量移动
    {
        {1, 2, 3},
        {4, 5, 6},
        {0, 7, 8}
    },
    // 状态2：中等难度
    {
        {1, 2, 3},
        {4, 0, 6},
        {7, 5, 8}
    },
    // 状态3：中等难度
    {
        {1, 2, 3},
        {0, 4, 6},
        {7, 5, 8}
    },
    // 状态4：较难
    {
        {1, 2, 0},
        {4, 5, 3},
        {7, 8, 6}
    },
    // 状态5：较难
    {
        {1, 0, 3},
        {4, 2, 6},
        {7, 5, 8}
    },
    // 状态6：非常难
    {
        {0, 1, 3},
        {4, 2, 5},
        {7, 8, 6}
    }
};

// 显示带颜色的状态
void displayColorState(const EightPuzzleState& state) {
    const std::vector<std::vector<int>> board = state.getBoard();
    
    // 定义一些ANSI颜色代码
    const std::string COLOR_RESET = "\033[0m";
    const std::string COLOR_RED = "\033[31m";
    const std::string COLOR_GREEN = "\033[32m";
    const std::string COLOR_YELLOW = "\033[33m";
    const std::string COLOR_BLUE = "\033[34m";
    const std::string COLOR_MAGENTA = "\033[35m";
    const std::string COLOR_CYAN = "\033[36m";
    const std::string COLOR_WHITE = "\033[37m";
    
    const std::string colors[] = {
        COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, 
        COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE, COLOR_RED
    };
    
    std::cout << "┌───┬───┬───┐" << std::endl;
    
    for (int i = 0; i < 3; ++i) {
        std::cout << "│";
        for (int j = 0; j < 3; ++j) {
            if (board[i][j] == 0) {
                std::cout << "   │"; // 空格
            } else {
                std::cout << " " << colors[board[i][j] - 1] << board[i][j] << COLOR_RESET << " │";
            }
        }
        std::cout << std::endl;
        
        if (i < 2) {
            std::cout << "├───┼───┼───┤" << std::endl;
        }
    }
    
    std::cout << "└───┴───┴───┘" << std::endl;
}

// 可视化解决方案
void visualizeSolution(const std::vector<EightPuzzleState*>& solution) {
    if (solution.empty()) {
        std::cout << "无解决方案！" << std::endl;
        return;
    }
    
    std::cout << "解决方案步数：" << solution.size() - 1 << std::endl;
    
    for (size_t i = 0; i < solution.size(); ++i) {
        std::cout << "Step " << i << ":" << std::endl;
        displayColorState(*solution[i]);
        
        if (i < solution.size() - 1) {
            std::cout << "动作：" << solution[i + 1]->getAction() << std::endl;
        }
        
        std::cout << "------------------------" << std::endl;
        
        // 每步暂停一下，便于观察
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

// 在visualizeSolution函数后添加一个新函数，用于验证解决方案

// 验证解决方案的正确性
bool verifySolution(const std::vector<EightPuzzleState*>& solution) {
    if (solution.empty()) {
        std::cout << "无解决方案可验证！" << std::endl;
        return false;
    }
    
    // 检查每一步是否是合法移动
    for (size_t i = 1; i < solution.size(); ++i) {
        EightPuzzleState* prev = solution[i-1];
        EightPuzzleState* curr = solution[i];
        
        // 找到空格在prev中的位置
        int emptyRow = -1, emptyCol = -1;
        auto prevBoard = prev->getBoard();
        
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                if (prevBoard[r][c] == 0) {
                    emptyRow = r;
                    emptyCol = c;
                    break;
                }
            }
            if (emptyRow != -1) break;
        }
        
        // 获取当前动作
        std::string action = curr->getAction();
        
        // 基于动作计算期望的新状态
        std::vector<std::vector<int>> expectedBoard = prevBoard;
        if (action == "上" && emptyRow > 0) {
            // 空格向上移动（数字向下移动）
            expectedBoard[emptyRow][emptyCol] = expectedBoard[emptyRow-1][emptyCol];
            expectedBoard[emptyRow-1][emptyCol] = 0;
        } else if (action == "下" && emptyRow < 2) {
            // 空格向下移动（数字向上移动）
            expectedBoard[emptyRow][emptyCol] = expectedBoard[emptyRow+1][emptyCol];
            expectedBoard[emptyRow+1][emptyCol] = 0;
        } else if (action == "左" && emptyCol > 0) {
            // 空格向左移动（数字向右移动）
            expectedBoard[emptyRow][emptyCol] = expectedBoard[emptyRow][emptyCol-1];
            expectedBoard[emptyRow][emptyCol-1] = 0;
        } else if (action == "右" && emptyCol < 2) {
            // 空格向右移动（数字向左移动）
            expectedBoard[emptyRow][emptyCol] = expectedBoard[emptyRow][emptyCol+1];
            expectedBoard[emptyRow][emptyCol+1] = 0;
        } else {
            std::cout << "无效动作：" << action << " 在位置 [" << emptyRow << "," << emptyCol << "]" << std::endl;
            return false;
        }
        
        // 比较期望的新状态与实际状态
        auto currBoard = curr->getBoard();
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                if (expectedBoard[r][c] != currBoard[r][c]) {
                    std::cout << "状态不匹配 在步骤 " << i << std::endl;
                    std::cout << "期望：" << std::endl;
                    for (int r2 = 0; r2 < 3; ++r2) {
                        for (int c2 = 0; c2 < 3; ++c2) {
                            std::cout << expectedBoard[r2][c2] << " ";
                        }
                        std::cout << std::endl;
                    }
                    
                    std::cout << "实际：" << std::endl;
                    for (int r2 = 0; r2 < 3; ++r2) {
                        for (int c2 = 0; c2 < 3; ++c2) {
                            std::cout << currBoard[r2][c2] << " ";
                        }
                        std::cout << std::endl;
                    }
                    
                    return false;
                }
            }
        }
    }
    
    // 验证最终状态是否为目标状态
    if (!solution.back()->isGoal()) {
        std::cout << "最终状态不是目标状态！" << std::endl;
        return false;
    }
    
    return true;
}

// 运行BFS或DFS算法
void runAlgorithm(const std::vector<std::vector<int>>& initialState, const std::string& algorithm) {
    std::cout << "--------------------------------" << std::endl;
    std::cout << "使用" << algorithm << "算法解决八数码问题" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    
    std::cout << "初始状态：" << std::endl;
    EightPuzzleState initialStateObj(initialState);
    displayColorState(initialStateObj);
    
    std::cout << "目标状态：" << std::endl;
    EightPuzzleState goalStateObj(GOAL_STATE);
    displayColorState(goalStateObj);
    
    EightPuzzleSolver solver(initialState, GOAL_STATE);
    
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<EightPuzzleState*> solution;
    
    if (algorithm == "BFS") {
        solution = solver.solveBFS();
    } else if (algorithm == "DFS") {
        solution = solver.solveDFS();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "求解耗时：" << duration.count() << " ms" << std::endl;
    
    visualizeSolution(solution);
    
    // 验证解决方案的正确性
    bool isValid = verifySolution(solution);
    std::cout << "解决方案验证结果: " << (isValid ? "正确" : "错误") << std::endl;
}

int main() {
    // 设置控制台输出
    std::ios_base::sync_with_stdio(false);
    std::cout << std::setprecision(3) << std::fixed;
    
    std::cout << "八数码问题求解器" << std::endl;
    std::cout << "==================" << std::endl;
    
    // 依次测试所有6种初始状态
    std::cout << "依次测试所有6种初始状态（使用BFS算法）" << std::endl;
    
    for (int i = 0; i < 6; ++i) {
        std::cout << "\n\n测试状态 " << (i + 1) << "：" << std::endl;
        std::cout << "=======================" << std::endl;
        
        // 获取初始状态
        const std::vector<std::vector<int>>& initialState = INITIAL_STATES[i];
        
        // 使用BFS算法解决
        runAlgorithm(initialState, "BFS");
        
        std::cout << "\n按任意键继续测试下一个状态..." << std::endl;
        std::cin.get(); // 等待用户按键
    }
    
    std::cout << "\n所有6种状态测试完成！" << std::endl;
    
    return 0;
} 