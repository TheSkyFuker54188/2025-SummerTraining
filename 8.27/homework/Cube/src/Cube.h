#ifndef CUBE_H
#define CUBE_H

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <functional>

// 定义魔方的六个面
enum Face {
    UP = 0,   // 上面
    DOWN = 1, // 下面
    LEFT = 2, // 左面
    RIGHT = 3,// 右面
    FRONT = 4,// 前面
    BACK = 5  // 后面
};

// 定义魔方的颜色
enum Color {
    WHITE = 0, // 白色
    YELLOW = 1,// 黄色
    RED = 2,   // 红色
    ORANGE = 3,// 橙色
    GREEN = 4, // 绿色
    BLUE = 5,  // 蓝色
    PURPLE = 6,// 紫色
    BLACK = 7  // 黑色
};

// 魔方操作
enum MoveType {
    // 定义操作类型，与文档中要求的输出格式对应
    U, U_PRIME,   // 1+, 1- (上层)
    D, D_PRIME,   // 2+, 2- (下层)
    L, L_PRIME,   // 3+, 3- (左层)
    R, R_PRIME,   // 4+, 4- (右层)
    F, F_PRIME,   // 5+, 5- (前层)
    B, B_PRIME,   // 6+, 6- (后层)
    M, M_PRIME,   // 7+, 7- (中层)
    E, E_PRIME,   // 8+, 8- (赤道层)
    S, S_PRIME    // 9+, 9- (前中层)
};

// 魔方状态类
class CubeState {
private:
    // 使用3D数组表示魔方状态，第一个索引表示面，后两个索引表示行和列
    std::vector<std::vector<std::vector<Color>>> cube;
    CubeState* parent; // 父状态
    MoveType action; // 从父状态到当前状态的动作
    
public:
    // 构造函数
    CubeState();
    CubeState(const std::vector<std::vector<std::vector<Color>>>& state);
    
    // 复制构造函数
    CubeState(const CubeState& other);
    
    // 析构函数
    ~CubeState();
    
    // 获取魔方状态
    std::vector<std::vector<std::vector<Color>>> getState() const;
    
    // 设置父状态
    void setParent(CubeState* parent);
    
    // 获取父状态
    CubeState* getParent() const;
    
    // 设置动作
    void setAction(MoveType action);
    
    // 获取动作
    MoveType getAction() const;
    
    // 获取动作的字符串表示（标准表示法）
    std::string getActionString() const;
    
    // 获取动作的简短表示（符合文档要求）
    std::string getActionShortNotation() const;
    
    // 判断是否为目标状态（每个面都是相同颜色）
    bool isGoal() const;
    
    // 执行魔方操作
    void move(MoveType move);
    
    // 获取可能的下一步状态
    std::vector<CubeState*> getNextStates();
    
    // 打印当前状态
    void printState() const;
    
    // 获取状态的哈希值，用于判重
    std::string getHashCode() const;
    
    // 判断两个状态是否相等
    bool equals(const CubeState& other) const;
    
    // 从字符串解析魔方状态
    static CubeState fromString(const std::string& str);
    
    // 将颜色字符转换为颜色枚举
    static Color charToColor(char c);
};

// 哈希函数，用于CubeState的哈希集合
namespace std {
    template <>
    struct hash<CubeState> {
        std::size_t operator()(const CubeState& state) const {
            return std::hash<std::string>()(state.getHashCode());
        }
    };
}

// 魔方求解器
class CubeSolver {
private:
    CubeState* initialState;
    
public:
    // 构造函数
    CubeSolver(const CubeState& initial);
    
    // 析构函数
    ~CubeSolver();
    
    // 使用迭代加深搜索（IDDFS）求解魔方
    std::vector<CubeState*> solveIDDFS(int maxDepth);
    
    // 打印解决方案
    void printSolution(const std::vector<CubeState*>& solution);
    
    // 打印简短解决方案（符合文档要求）
    void printSolutionShortNotation(const std::vector<CubeState*>& solution);
};

#endif // CUBE_H 