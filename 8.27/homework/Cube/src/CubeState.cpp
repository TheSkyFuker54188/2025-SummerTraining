#include "Cube.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>

// 颜色名称映射，用于输入输出
static const std::map<char, Color> charToColor = {
    {'w', WHITE},
    {'y', YELLOW},
    {'r', RED},
    {'o', ORANGE},
    {'g', GREEN},
    {'b', BLUE},
    {'p', PURPLE},
    {'k', BLACK}
};

static const std::map<Color, char> colorToChar = {
    {WHITE, 'w'},
    {YELLOW, 'y'},
    {RED, 'r'},
    {ORANGE, 'o'},
    {GREEN, 'g'},
    {BLUE, 'b'},
    {PURPLE, 'p'},
    {BLACK, 'k'}
};

// 颜色名称映射，用于显示
static const std::map<Color, std::string> colorNames = {
    {WHITE, "白色"},
    {YELLOW, "黄色"},
    {RED, "红色"},
    {ORANGE, "橙色"},
    {GREEN, "绿色"},
    {BLUE, "蓝色"},
    {PURPLE, "紫色"},
    {BLACK, "黑色"}
};

// ANSI颜色代码，用于彩色输出
static const std::map<Color, std::string> colorCodes = {
    {WHITE, "\033[47m"},   // 白色背景
    {YELLOW, "\033[43m"},  // 黄色背景
    {RED, "\033[41m"},     // 红色背景
    {ORANGE, "\033[45m"},  // 紫红色背景（用于表示橙色，终端有限）
    {GREEN, "\033[42m"},   // 绿色背景
    {BLUE, "\033[44m"},    // 蓝色背景
    {PURPLE, "\033[45m"},  // 紫红色背景
    {BLACK, "\033[40m"}    // 黑色背景
};

// 动作字符串映射
static const std::map<MoveType, std::string> moveToString = {
    {U, "U"},
    {U_PRIME, "U'"},
    {D, "D"},
    {D_PRIME, "D'"},
    {L, "L"},
    {L_PRIME, "L'"},
    {R, "R"},
    {R_PRIME, "R'"},
    {F, "F"},
    {F_PRIME, "F'"},
    {B, "B"},
    {B_PRIME, "B'"},
    {M, "M"},
    {M_PRIME, "M'"},
    {E, "E"},
    {E_PRIME, "E'"},
    {S, "S"},
    {S_PRIME, "S'"}
};

// 将字符映射到颜色枚举
Color CubeState::charToColor(char c) {
    switch (c) {
        case 'w': return WHITE;
        case 'y': return YELLOW;
        case 'r': return RED;
        case 'o': return ORANGE;
        case 'g': return GREEN;
        case 'b': return BLUE;
        case 'p': return PURPLE;
        case 'k': return BLACK;
        default: return WHITE; // 默认为白色
    }
}

// 将动作映射到简短格式（如 "3-"、"6+"）
std::string CubeState::getActionShortNotation() const {
    std::string notation;
    
    switch (action) {
        case U: notation = "1+"; break;
        case U_PRIME: notation = "1-"; break;
        case D: notation = "2+"; break;
        case D_PRIME: notation = "2-"; break;
        case L: notation = "3+"; break;
        case L_PRIME: notation = "3-"; break;
        case R: notation = "4+"; break;
        case R_PRIME: notation = "4-"; break;
        case F: notation = "5+"; break;
        case F_PRIME: notation = "5-"; break;
        case B: notation = "6+"; break;
        case B_PRIME: notation = "6-"; break;
        case M: notation = "7+"; break;
        case M_PRIME: notation = "7-"; break;
        case E: notation = "8+"; break;
        case E_PRIME: notation = "8-"; break;
        case S: notation = "9+"; break;
        case S_PRIME: notation = "9-"; break;
        default: notation = "";
    }
    
    return notation;
}

// 修改从字符串解析魔方状态的方法
CubeState CubeState::fromString(const std::string& str) {
    std::vector<std::vector<std::vector<Color>>> cubeState(6, std::vector<std::vector<Color>>(3, std::vector<Color>(3)));
    
    std::istringstream ss(str);
    std::string line;
    
    // 面的映射，将输入的面名称映射到Face枚举
    std::map<std::string, Face> faceMap = {
        {"back", BACK},
        {"down", DOWN},
        {"front", FRONT},
        {"left", LEFT},
        {"right", RIGHT},
        {"up", UP}
    };
    
    Face currentFace = UP; // 默认面
    int row = 0;
    
    while (std::getline(ss, line)) {
        // 跳过空行
        if (line.empty()) {
            continue;
        }
        
        // 检查是否是面的开始
        if (line.find(':') != std::string::npos) {
            std::string faceName = line.substr(0, line.find(':'));
            
            // 查找面名称
            if (faceMap.find(faceName) != faceMap.end()) {
                currentFace = faceMap[faceName];
                row = 0;
            }
            
            continue;
        }
        
        // 解析一行颜色
        std::istringstream lineStream(line);
        char c;
        int col = 0;
        
        while (lineStream >> c && col < 3) {
            if (c != ' ') {
                cubeState[currentFace][row][col++] = charToColor(c);
            }
        }
        
        if (col > 0) {
            row++; // 只有在成功解析了颜色之后才增加行号
        }
        
        if (row >= 3) {
            row = 0; // 防止越界
        }
    }
    
    return CubeState(cubeState);
}

// 默认构造函数，创建一个已还原的魔方
CubeState::CubeState() {
    // 初始化魔方，6个面，每个面3x3
    cube.resize(6, std::vector<std::vector<Color>>(3, std::vector<Color>(3)));
    
    // 初始化各个面为相同颜色
    for (int face = 0; face < 6; ++face) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                cube[face][i][j] = static_cast<Color>(face);
            }
        }
    }
    
    parent = nullptr;
    action = U; // 默认动作
}

// 带参数的构造函数
CubeState::CubeState(const std::vector<std::vector<std::vector<Color>>>& state) {
    cube = state;
    parent = nullptr;
    action = U; // 默认动作
}

// 复制构造函数
CubeState::CubeState(const CubeState& other) {
    cube = other.cube;
    parent = other.parent;
    action = other.action;
}

// 析构函数
CubeState::~CubeState() {
    // 不需要在这里释放parent，因为parent是由搜索算法管理的
}

// 获取魔方状态
std::vector<std::vector<std::vector<Color>>> CubeState::getState() const {
    return cube;
}

// 设置父状态
void CubeState::setParent(CubeState* p) {
    parent = p;
}

// 获取父状态
CubeState* CubeState::getParent() const {
    return parent;
}

// 设置动作
void CubeState::setAction(MoveType a) {
    action = a;
}

// 获取动作
MoveType CubeState::getAction() const {
    return action;
}

// 获取动作的字符串表示
std::string CubeState::getActionString() const {
    return moveToString.at(action);
}

// 判断是否为目标状态（每个面都是相同颜色）
bool CubeState::isGoal() const {
    for (int face = 0; face < 6; ++face) {
        Color centerColor = cube[face][1][1]; // 中心块的颜色
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (cube[face][i][j] != centerColor) {
                    return false;
                }
            }
        }
    }
    return true;
}

// 执行魔方操作
void CubeState::move(MoveType move) {
    std::vector<std::vector<std::vector<Color>>> newCube = cube;
    
    switch (move) {
        case U: // 上层顺时针
            // 旋转上层
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    newCube[UP][i][j] = cube[UP][2-j][i];
                }
            }
            // 更新四周
            for (int i = 0; i < 3; ++i) {
                newCube[FRONT][0][i] = cube[RIGHT][0][i];
                newCube[RIGHT][0][i] = cube[BACK][0][i];
                newCube[BACK][0][i] = cube[LEFT][0][i];
                newCube[LEFT][0][i] = cube[FRONT][0][i];
            }
            break;
        case U_PRIME: // 上层逆时针
            // 旋转上层
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    newCube[UP][i][j] = cube[UP][j][2-i];
                }
            }
            // 更新四周
            for (int i = 0; i < 3; ++i) {
                newCube[FRONT][0][i] = cube[LEFT][0][i];
                newCube[LEFT][0][i] = cube[BACK][0][i];
                newCube[BACK][0][i] = cube[RIGHT][0][i];
                newCube[RIGHT][0][i] = cube[FRONT][0][i];
            }
            break;
        case D: // 下层顺时针
            // 旋转下层
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    newCube[DOWN][i][j] = cube[DOWN][2-j][i];
                }
            }
            // 更新四周
            for (int i = 0; i < 3; ++i) {
                newCube[FRONT][2][i] = cube[LEFT][2][i];
                newCube[LEFT][2][i] = cube[BACK][2][i];
                newCube[BACK][2][i] = cube[RIGHT][2][i];
                newCube[RIGHT][2][i] = cube[FRONT][2][i];
            }
            break;
        case D_PRIME: // 下层逆时针
            // 旋转下层
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    newCube[DOWN][i][j] = cube[DOWN][j][2-i];
                }
            }
            // 更新四周
            for (int i = 0; i < 3; ++i) {
                newCube[FRONT][2][i] = cube[RIGHT][2][i];
                newCube[RIGHT][2][i] = cube[BACK][2][i];
                newCube[BACK][2][i] = cube[LEFT][2][i];
                newCube[LEFT][2][i] = cube[FRONT][2][i];
            }
            break;
        case L: // 左层顺时针
            // 旋转左层
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    newCube[LEFT][i][j] = cube[LEFT][2-j][i];
                }
            }
            // 更新四周
            for (int i = 0; i < 3; ++i) {
                newCube[UP][i][0] = cube[BACK][2-i][2];
                newCube[FRONT][i][0] = cube[UP][i][0];
                newCube[DOWN][i][0] = cube[FRONT][i][0];
                newCube[BACK][i][2] = cube[DOWN][2-i][0];
            }
            break;
        case L_PRIME: // 左层逆时针
            // 旋转左层
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    newCube[LEFT][i][j] = cube[LEFT][j][2-i];
                }
            }
            // 更新四周
            for (int i = 0; i < 3; ++i) {
                newCube[UP][i][0] = cube[FRONT][i][0];
                newCube[FRONT][i][0] = cube[DOWN][i][0];
                newCube[DOWN][i][0] = cube[BACK][2-i][2];
                newCube[BACK][i][2] = cube[UP][2-i][0];
            }
            break;
        case R: // 右层顺时针
            // 旋转右层
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    newCube[RIGHT][i][j] = cube[RIGHT][2-j][i];
                }
            }
            // 更新四周
            for (int i = 0; i < 3; ++i) {
                newCube[UP][i][2] = cube[FRONT][i][2];
                newCube[FRONT][i][2] = cube[DOWN][i][2];
                newCube[DOWN][i][2] = cube[BACK][2-i][0];
                newCube[BACK][i][0] = cube[UP][2-i][2];
            }
            break;
        case R_PRIME: // 右层逆时针
            // 旋转右层
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    newCube[RIGHT][i][j] = cube[RIGHT][j][2-i];
                }
            }
            // 更新四周
            for (int i = 0; i < 3; ++i) {
                newCube[UP][i][2] = cube[BACK][2-i][0];
                newCube[FRONT][i][2] = cube[UP][i][2];
                newCube[DOWN][i][2] = cube[FRONT][i][2];
                newCube[BACK][i][0] = cube[DOWN][2-i][2];
            }
            break;
        case F: // 前层顺时针
            // 旋转前层
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    newCube[FRONT][i][j] = cube[FRONT][2-j][i];
                }
            }
            // 更新四周
            for (int i = 0; i < 3; ++i) {
                newCube[UP][2][i] = cube[LEFT][2-i][2];
                newCube[RIGHT][i][0] = cube[UP][2][i];
                newCube[DOWN][0][2-i] = cube[RIGHT][i][0];
                newCube[LEFT][i][2] = cube[DOWN][0][i];
            }
            break;
        case F_PRIME: // 前层逆时针
            // 旋转前层
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    newCube[FRONT][i][j] = cube[FRONT][j][2-i];
                }
            }
            // 更新四周
            for (int i = 0; i < 3; ++i) {
                newCube[UP][2][i] = cube[RIGHT][i][0];
                newCube[RIGHT][i][0] = cube[DOWN][0][2-i];
                newCube[DOWN][0][i] = cube[LEFT][2-i][2];
                newCube[LEFT][i][2] = cube[UP][2][2-i];
            }
            break;
        case B: // 后层顺时针
            // 旋转后层
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    newCube[BACK][i][j] = cube[BACK][2-j][i];
                }
            }
            // 更新四周
            for (int i = 0; i < 3; ++i) {
                newCube[UP][0][i] = cube[RIGHT][i][2];
                newCube[RIGHT][i][2] = cube[DOWN][2][2-i];
                newCube[DOWN][2][i] = cube[LEFT][2-i][0];
                newCube[LEFT][i][0] = cube[UP][0][2-i];
            }
            break;
        case B_PRIME: // 后层逆时针
            // 旋转后层
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    newCube[BACK][i][j] = cube[BACK][j][2-i];
                }
            }
            // 更新四周
            for (int i = 0; i < 3; ++i) {
                newCube[UP][0][i] = cube[LEFT][i][0];
                newCube[RIGHT][i][2] = cube[UP][0][i];
                newCube[DOWN][2][2-i] = cube[RIGHT][i][2];
                newCube[LEFT][i][0] = cube[DOWN][2][i];
            }
            break;
        // 处理其他新增操作，保持简单实现
        case M:
        case M_PRIME:
        case E:
        case E_PRIME:
        case S:
        case S_PRIME:
            // 暂时不实现这些复杂操作，但保留枚举，避免警告
            break;
    }
    
    cube = newCube;
}

// 获取可能的下一步状态
std::vector<CubeState*> CubeState::getNextStates() {
    std::vector<CubeState*> nextStates;
    
    // 基本操作 - 我们只考虑常用的基本操作，不考虑中层操作，以减少状态空间
    MoveType moves[] = {
        U, U_PRIME, D, D_PRIME, L, L_PRIME, 
        R, R_PRIME, F, F_PRIME, B, B_PRIME
    };
    
    for (MoveType move : moves) {
        CubeState* newState = new CubeState(*this);
        newState->move(move);
        newState->setParent(this);
        newState->setAction(move);
        nextStates.push_back(newState);
    }
    
    return nextStates;
}

// 打印当前状态
void CubeState::printState() const {
    const std::string RESET = "\033[0m";
    
    // 面的名称
    const std::string faceNames[] = {"上面", "下面", "左面", "右面", "前面", "后面"};
    
    for (int face = 0; face < 6; ++face) {
        std::cout << faceNames[face] << ":" << std::endl;
        
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                Color color = cube[face][i][j];
                std::cout << colorCodes.at(color) << "  " << RESET;
            }
            std::cout << std::endl;
        }
        
        std::cout << std::endl;
    }
}

// 获取状态的哈希值，用于判重
std::string CubeState::getHashCode() const {
    std::stringstream ss;
    
    for (int face = 0; face < 6; ++face) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                ss << static_cast<int>(cube[face][i][j]);
            }
        }
    }
    
    return ss.str();
}

// 判断两个状态是否相等
bool CubeState::equals(const CubeState& other) const {
    return getHashCode() == other.getHashCode();
} 