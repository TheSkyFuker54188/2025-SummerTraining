#ifndef EIGHT_PUZZLE_H
#define EIGHT_PUZZLE_H

#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <unordered_set>
#include <functional>

// 八数码问题的状态类
class EightPuzzleState {
private:
    std::vector<std::vector<int>> board; // 3x3的棋盘
    int emptyRow; // 空格的行位置
    int emptyCol; // 空格的列位置
    EightPuzzleState* parent; // 父状态
    std::string action; // 从父状态到当前状态的动作
    int gValue; // 从初始状态到当前状态的实际步数
    int hValue; // 启发式函数值：从当前状态到目标状态的估计步数

public:
    // 构造函数
    EightPuzzleState();
    EightPuzzleState(const std::vector<std::vector<int>>& board);
    
    // 复制构造函数
    EightPuzzleState(const EightPuzzleState& other);
    
    // 析构函数
    ~EightPuzzleState();
    
    // 获取棋盘状态
    std::vector<std::vector<int>> getBoard() const;
    
    // 设置父状态
    void setParent(EightPuzzleState* parent);
    
    // 获取父状态
    EightPuzzleState* getParent() const;
    
    // 设置动作
    void setAction(const std::string& action);
    
    // 获取动作
    std::string getAction() const;
    
    // 设置和获取g值（从初始状态到当前状态的实际步数）
    void setGValue(int g);
    int getGValue() const;
    
    // 计算曼哈顿距离（从当前状态到目标状态每个数字需要移动的最小步数之和）
    int calculateManhattanDistance() const;
    
    // 计算不在位数（当前状态中有多少数字不在其目标位置上）
    int calculateMisplacedTiles() const;
    
    // 设置和获取h值（启发式函数值）
    void setHValue(int h);
    int getHValue() const;
    
    // 获取f值 (f = g + h)，用于A*和贪心算法
    int getFValue() const;
    
    // 判断是否为目标状态
    bool isGoal() const;
    
    // 获取可能的下一步状态
    std::vector<EightPuzzleState*> getNextStates();
    
    // 打印当前状态
    void printState() const;
    
    // 获取状态的哈希值，用于判重
    std::string getHashCode() const;
    
    // 判断两个状态是否相等
    bool equals(const EightPuzzleState& other) const;

private:
    // 初始化空格位置
    void findEmptyPosition();
};

// 哈希函数，用于EightPuzzleState的哈希集合
namespace std {
    template <>
    struct hash<EightPuzzleState> {
        std::size_t operator()(const EightPuzzleState& state) const {
            return std::hash<std::string>()(state.getHashCode());
        }
    };
}

// 用于A*和贪心算法的状态比较函数
struct StateComparison {
    bool operator()(const EightPuzzleState* lhs, const EightPuzzleState* rhs) const {
        // 对于A*: f = g + h，对于贪心: f = h
        return lhs->getFValue() > rhs->getFValue();
    }
};

// 八数码问题求解器
class EightPuzzleSolver {
private:
    EightPuzzleState* initialState;
    EightPuzzleState* goalState;
    
public:
    // 构造函数
    EightPuzzleSolver(const std::vector<std::vector<int>>& initial, const std::vector<std::vector<int>>& goal);
    
    // 析构函数
    ~EightPuzzleSolver();
    
    // 广度优先搜索
    std::vector<EightPuzzleState*> solveBFS();
    
    // A*搜索算法，使用曼哈顿距离作为启发式函数
    std::vector<EightPuzzleState*> solveAStar();
    
    // 贪心最佳优先搜索，使用曼哈顿距离作为启发式函数
    std::vector<EightPuzzleState*> solveGreedy();
    
    // 打印解决方案
    void printSolution(const std::vector<EightPuzzleState*>& solution);
    
    // 检查问题是否有解
    bool isSolvable(const EightPuzzleState& initial) const;
};

#endif // EIGHT_PUZZLE_H 