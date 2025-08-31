#ifndef CUBE_DEFINITION_H
#define CUBE_DEFINITION_H

/* 
 * 引入标准库
 * ==============================================
 */
#include <stdexcept>
#include <string>
#include <functional>

namespace cube {

// 自定义异常类型
class MagicCubeException : public std::exception {
private:
    std::string msg;
public:
    MagicCubeException(const std::string& message) : msg(message) {}
    virtual const char* what() const noexcept override { return msg.c_str(); }
};

class InvalidFaceException : public MagicCubeException {
public:
    InvalidFaceException() : MagicCubeException("无效的魔方面") {}
};

class InvalidPositionException : public MagicCubeException {
public:
    InvalidPositionException() : MagicCubeException("无效的相对位置") {}
};

class InvalidColorException : public MagicCubeException {
public:
    InvalidColorException(const std::string& code) : MagicCubeException("未知颜色码: " + code) {}
};

// 魔方的基本常量
/*--------------------------------------------*/
constexpr int EDGE_COUNT = 8;          // 一个面边缘位置数量
constexpr int FACE_COUNT = 6;          // 魔方的面数量
constexpr int CUBE_SIZE = 3;           // 3阶魔方
/*--------------------------------------------*/

/* 
 * 旋转方向枚举
 * 顺时针和逆时针
 */
enum class RotateDir { 
    COUNTER_CLOCKWISE,
    CLOCKWISE
};

/*
 * 相对位置枚举
 * 表示面的相对方向 
 */
enum class FacePosition { 
    RSIDE,
    LSIDE,
    UPPER,
    LOWER
};

// 颜色枚举定义
enum class ColorType { 
    GREEN,
    BLUE,
    PINK,
    RED,
    YELLOW,
    WHITE
};

/*
 * 魔方的六个面枚举
 * 按照国际标准定义
 */
enum FaceType { 
    FACE_RIGHT,
    FACE_LEFT,
    FACE_BACK,
    FACE_FRONT,
    FACE_BOTTOM,
    FACE_TOP
};

// 一个面上的8个位置编号
//  0    1    2
//  7   中心   3
//  6    5    4
enum PositionIdx { 
    POS_TOP_LEFT = 0,
    POS_TOP_MID,
    POS_TOP_RIGHT,
    POS_MID_RIGHT,
    POS_BOT_RIGHT,
    POS_BOT_MID,
    POS_BOT_LEFT,
    POS_MID_LEFT = 7 
};

/*
 * 面与旋转偏移量的组合结构
 */
struct FaceWithOffset {
    FaceType face;
    int offset;  // 正值表示顺时针偏移

    FaceWithOffset(FaceType f, int off)
        : face(f), offset(off) {}
};

// 魔方操作结构
struct MoveAction {
    RotateDir dir;
    FaceType face;
    bool is_middle;
};

// 标准格式的魔方操作
struct StandardAction {
    bool is_positive;
    int action_index;
};

// 18种标准操作定义
constexpr StandardAction ALL_ACTIONS[] = {
    {true, 0}, {true, 1}, {true, 2},
    {true, 3}, {true, 4}, {true, 5},
    {true, 6}, {true, 7}, {true, 8},
    {false, 0}, {false, 1}, {false, 2},
    {false, 3}, {false, 4}, {false, 5},
    {false, 6}, {false, 7}, {false, 8},
};

/*=============================================
 * 位置索引操作相关函数
 *=============================================*/

/**
 * 位置索引加上偏移
 * @param pos 初始位置
 * @param offset 偏移量
 * @return 新位置
 */
inline PositionIdx OffsetPosition(PositionIdx pos, int offset) {
    // 处理负偏移，确保结果为正
    if (offset < 0) {
        offset = offset + (1 + (-offset) / EDGE_COUNT) * EDGE_COUNT;
    }
    return static_cast<PositionIdx>(((int)pos + offset * 2) % EDGE_COUNT);
}

/**
 * 位置索引减去偏移
 */
inline PositionIdx BackPosition(PositionIdx pos, int offset) {
    return OffsetPosition(pos, -offset);
}

/**
 * 位置索引增加
 */
inline PositionIdx NextPosition(PositionIdx& pos) {
    pos = static_cast<PositionIdx>((pos + 1) % EDGE_COUNT);
    return pos;
}

/*=============================================
 * 名称与枚举转换相关函数
 *=============================================*/

/**
 * 颜色枚举转换为字符
 */
inline char ColorToChar(ColorType color) {
    char result;
    switch (color) {
        case ColorType::WHITE: result = 'w'; break;
        case ColorType::YELLOW: result = 'y'; break;
        case ColorType::RED: result = 'r'; break;
        case ColorType::PINK: result = 'p'; break;
        case ColorType::BLUE: result = 'b'; break;
        case ColorType::GREEN: result = 'g'; break;
        default: throw InvalidColorException("未知颜色值");
    }
    return result;
}

/**
 * 颜色名称转换为枚举
 */
inline ColorType CharToColor(const std::string& code) {
    if (code.empty()) {
        throw InvalidColorException("空颜色码");
    }
    
    switch (code[0]) {
        case 'w': return ColorType::WHITE;
        case 'y': return ColorType::YELLOW; 
        case 'r': return ColorType::RED;
        case 'p': return ColorType::PINK;
        case 'b': return ColorType::BLUE;
        case 'g': return ColorType::GREEN;
        default: throw InvalidColorException(code);
    }
}

/**
 * 面枚举转换为名称字符串
 */
inline std::string FaceToString(FaceType face) {
    switch (face) {
        case FaceType::FACE_TOP: return "up";
        case FaceType::FACE_BOTTOM: return "down";
        case FaceType::FACE_FRONT: return "front";
        case FaceType::FACE_BACK: return "back";
        case FaceType::FACE_LEFT: return "left";
        case FaceType::FACE_RIGHT: return "right";
        default: return "unknown";
    }
}

/**
 * 面名称转换为枚举
 */
inline FaceType StringToFace(const std::string& name) {
    if (name.empty()) {
        throw InvalidFaceException();
    }
    
    switch (name[0]) {
        case 'u': return FaceType::FACE_TOP;
        case 'd': return FaceType::FACE_BOTTOM;
        case 'f': return FaceType::FACE_FRONT;
        case 'b': return FaceType::FACE_BACK;
        case 'l': return FaceType::FACE_LEFT;
        case 'r': return FaceType::FACE_RIGHT;
        default: throw InvalidFaceException();
    }
}

/*=============================================
 * 魔方面关系和操作相关函数
 *=============================================*/

/**
 * 获取给定面的相对面及其旋转偏移
 */
inline FaceWithOffset GetRelativeFace(FaceType face, FacePosition pos) {
    try {
        switch (face) {
            case FaceType::FACE_FRONT:
                switch (pos) {
                    case FacePosition::UPPER: return FaceWithOffset(FaceType::FACE_TOP, 0);
                    case FacePosition::LOWER: return FaceWithOffset(FaceType::FACE_BOTTOM, 0);
                    case FacePosition::LSIDE: return FaceWithOffset(FaceType::FACE_LEFT, -1);
                    case FacePosition::RSIDE: return FaceWithOffset(FaceType::FACE_RIGHT, +1);
                }
                break;
                
            case FaceType::FACE_BACK:
                switch (pos) {
                    case FacePosition::UPPER: return FaceWithOffset(FaceType::FACE_BOTTOM, 0);
                    case FacePosition::LOWER: return FaceWithOffset(FaceType::FACE_TOP, 0);
                    case FacePosition::LSIDE: return FaceWithOffset(FaceType::FACE_LEFT, +1);
                    case FacePosition::RSIDE: return FaceWithOffset(FaceType::FACE_RIGHT, -1);
                }
                break;
                
            case FaceType::FACE_LEFT:
                switch (pos) {
                    case FacePosition::UPPER: return FaceWithOffset(FaceType::FACE_BACK, -1);
                    case FacePosition::LOWER: return FaceWithOffset(FaceType::FACE_FRONT, +1);
                    case FacePosition::LSIDE: return FaceWithOffset(FaceType::FACE_BOTTOM, +2);
                    case FacePosition::RSIDE: return FaceWithOffset(FaceType::FACE_TOP, 0);
                }
                break;
                
            case FaceType::FACE_RIGHT:
                switch (pos) {
                    case FacePosition::UPPER: return FaceWithOffset(FaceType::FACE_BACK, +1);
                    case FacePosition::LOWER: return FaceWithOffset(FaceType::FACE_FRONT, -1);
                    case FacePosition::LSIDE: return FaceWithOffset(FaceType::FACE_TOP, 0);
                    case FacePosition::RSIDE: return FaceWithOffset(FaceType::FACE_BOTTOM, -2);
                }
                break;
                
            case FaceType::FACE_TOP:
                switch (pos) {
                    case FacePosition::UPPER: return FaceWithOffset(FaceType::FACE_BACK, 0);
                    case FacePosition::LOWER: return FaceWithOffset(FaceType::FACE_FRONT, 0);
                    case FacePosition::LSIDE: return FaceWithOffset(FaceType::FACE_LEFT, 0);
                    case FacePosition::RSIDE: return FaceWithOffset(FaceType::FACE_RIGHT, 0);
                }
                break;
                
            case FaceType::FACE_BOTTOM:
                switch (pos) {
                    case FacePosition::UPPER: return FaceWithOffset(FaceType::FACE_FRONT, 0);
                    case FacePosition::LOWER: return FaceWithOffset(FaceType::FACE_BACK, 0);
                    case FacePosition::LSIDE: return FaceWithOffset(FaceType::FACE_LEFT, -2);
                    case FacePosition::RSIDE: return FaceWithOffset(FaceType::FACE_RIGHT, +2);
                }
                break;
        }
        
        // 若未找到匹配项，抛出异常
        throw InvalidPositionException();
    }
    catch (...) {
        throw InvalidFaceException();
    }
}

/**
 * 常规操作转换为标准操作格式
 */
inline StandardAction ConvertToStandard(MoveAction move) {
    StandardAction result;
    
    try {
        if (move.is_middle) {
            switch (move.face) {
                case FaceType::FACE_LEFT:
                    result.action_index = 1;
                    result.is_positive = move.dir == RotateDir::CLOCKWISE;
                    break;
                case FaceType::FACE_RIGHT:
                    result.action_index = 1;
                    result.is_positive = move.dir == RotateDir::COUNTER_CLOCKWISE;
                    break;
                case FaceType::FACE_TOP:
                    result.action_index = 4;
                    result.is_positive = move.dir == RotateDir::CLOCKWISE;
                    break;
                case FaceType::FACE_BOTTOM:
                    result.action_index = 4;
                    result.is_positive = move.dir == RotateDir::COUNTER_CLOCKWISE;
                    break;
                case FaceType::FACE_FRONT:
                    result.action_index = 7;
                    result.is_positive = move.dir == RotateDir::CLOCKWISE;
                    break;
                case FaceType::FACE_BACK:
                    result.action_index = 7;
                    result.is_positive = move.dir == RotateDir::COUNTER_CLOCKWISE;
                    break;
                default:
                    throw InvalidFaceException();
            }
        } 
        else {
            switch (move.face) {
                case FaceType::FACE_LEFT:
                    result.action_index = 0;
                    result.is_positive = move.dir == RotateDir::CLOCKWISE;
                    break;
                case FaceType::FACE_RIGHT:
                    result.action_index = 2;
                    result.is_positive = move.dir == RotateDir::COUNTER_CLOCKWISE;
                    break;
                case FaceType::FACE_TOP:
                    result.action_index = 5;
                    result.is_positive = move.dir == RotateDir::CLOCKWISE;
                    break;
                case FaceType::FACE_BOTTOM:
                    result.action_index = 3;
                    result.is_positive = move.dir == RotateDir::COUNTER_CLOCKWISE;
                    break;
                case FaceType::FACE_FRONT:
                    result.action_index = 6;
                    result.is_positive = move.dir == RotateDir::CLOCKWISE;
                    break;
                case FaceType::FACE_BACK:
                    result.action_index = 8;
                    result.is_positive = move.dir == RotateDir::COUNTER_CLOCKWISE;
                    break;
                default:
                    throw InvalidFaceException();
            }
        }
    }
    catch (...) {
        throw MagicCubeException("转换操作格式失败");
    }
    
    return result;
}

/**
 * 标准操作格式转换为常规操作
 */
inline MoveAction ConvertToMove(StandardAction std_action) {
    MoveAction result;
    
    try {
        // 确定是否为中层旋转
        result.is_middle = (std_action.action_index == 1 || 
                         std_action.action_index == 4 || 
                         std_action.action_index == 7);
        
        // 根据索引确定面和方向
        switch (std_action.action_index) {
            case 0:
                result.face = FaceType::FACE_LEFT;
                result.dir = std_action.is_positive ? RotateDir::CLOCKWISE : RotateDir::COUNTER_CLOCKWISE;
                break;
            case 1:
                result.face = FaceType::FACE_LEFT;
                result.dir = std_action.is_positive ? RotateDir::CLOCKWISE : RotateDir::COUNTER_CLOCKWISE;
                break;
            case 2:
                result.face = FaceType::FACE_RIGHT;
                result.dir = std_action.is_positive ? RotateDir::COUNTER_CLOCKWISE : RotateDir::CLOCKWISE;
                break;
            case 3:
                result.face = FaceType::FACE_BOTTOM;
                result.dir = std_action.is_positive ? RotateDir::COUNTER_CLOCKWISE : RotateDir::CLOCKWISE;
                break;
            case 4:
                result.face = FaceType::FACE_TOP;
                result.dir = std_action.is_positive ? RotateDir::CLOCKWISE : RotateDir::COUNTER_CLOCKWISE;
                break;
            case 5:
                result.face = FaceType::FACE_TOP;
                result.dir = std_action.is_positive ? RotateDir::CLOCKWISE : RotateDir::COUNTER_CLOCKWISE;
                break;
            case 6:
                result.face = FaceType::FACE_FRONT;
                result.dir = std_action.is_positive ? RotateDir::CLOCKWISE : RotateDir::COUNTER_CLOCKWISE;
                break;
            case 7:
                result.face = FaceType::FACE_FRONT;
                result.dir = std_action.is_positive ? RotateDir::CLOCKWISE : RotateDir::COUNTER_CLOCKWISE;
                break;
            case 8:
                result.face = FaceType::FACE_BACK;
                result.dir = std_action.is_positive ? RotateDir::COUNTER_CLOCKWISE : RotateDir::CLOCKWISE;
                break;
            default:
                throw MagicCubeException("无效的操作索引");
        }
    }
    catch (...) {
        throw MagicCubeException("转换操作格式失败");
    }
    
    return result;
}

} // namespace cube

#endif // CUBE_DEFINITION_H 