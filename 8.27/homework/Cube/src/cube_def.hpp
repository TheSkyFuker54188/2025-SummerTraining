#pragma once
#include <stdexcept>
#include <string>

// 魔方基本参数定义
constexpr int CUBE_SIZE = 3;           // 3阶魔方
constexpr int EDGE_COUNT = 8;          // 一个面边缘位置数量
constexpr int FACE_COUNT = 6;          // 魔方的面数量

// 魔方面枚举
enum FaceType { 
    FACE_FRONT,    
    FACE_BACK,
    FACE_LEFT,
    FACE_RIGHT,
    FACE_TOP,
    FACE_BOTTOM 
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

// 面与旋转偏移量组合结构
struct FaceWithOffset {
    FaceType face;
    int offset;  // 正值表示顺时针偏移

    FaceWithOffset(FaceType f, int off)
        : face(f), offset(off) {}
};

// 颜色枚举
enum class ColorType { 
    WHITE,
    YELLOW,
    RED,
    PINK,
    BLUE,
    GREEN 
};

// 相对位置枚举
enum class FacePosition { 
    UPPER,
    LOWER,
    LSIDE,
    RSIDE 
};

// 旋转方向
enum class RotateDir { 
    CLOCKWISE,
    COUNTER_CLOCKWISE 
};

// 魔方操作结构
struct MoveAction {
    bool is_middle;
    FaceType face;
    RotateDir dir;
};

// 标准格式的魔方操作
struct StandardAction {
    int action_index;
    bool is_positive;
};

// 18种标准操作定义
constexpr StandardAction ALL_ACTIONS[] = {
    {0, true}, {1, true}, {2, true},
    {3, true}, {4, true}, {5, true},
    {6, true}, {7, true}, {8, true},
    {0, false}, {1, false}, {2, false},
    {3, false}, {4, false}, {5, false},
    {6, false}, {7, false}, {8, false},
};

// 面名称转换为枚举
inline FaceType StringToFace(const std::string& name) {
    FaceType result;
    
    switch (name[0]) {
        case 'u': result = FaceType::FACE_TOP; break;
        case 'd': result = FaceType::FACE_BOTTOM; break;
        case 'f': result = FaceType::FACE_FRONT; break;
        case 'b': result = FaceType::FACE_BACK; break;
        case 'l': result = FaceType::FACE_LEFT; break;
        case 'r': result = FaceType::FACE_RIGHT; break;
        default:
            throw std::invalid_argument("未知面名称: " + name);
    }
    
    return result;
}

// 面枚举转换为名称字符串
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

// 颜色名称转换为枚举
inline ColorType CharToColor(const std::string& code) {
    ColorType result;
    
    switch (code[0]) {
        case 'w': result = ColorType::WHITE; break;
        case 'y': result = ColorType::YELLOW; break;
        case 'r': result = ColorType::RED; break;
        case 'p': result = ColorType::PINK; break;
        case 'b': result = ColorType::BLUE; break;
        case 'g': result = ColorType::GREEN; break;
        default:
            throw std::invalid_argument("未知颜色码: " + code);
    }
    
    return result;
}

// 颜色枚举转换为字符
inline char ColorToChar(ColorType color) {
    switch (color) {
        case ColorType::WHITE: return 'w';
        case ColorType::YELLOW: return 'y';
        case ColorType::RED: return 'r';
        case ColorType::PINK: return 'p';
        case ColorType::BLUE: return 'b';
        case ColorType::GREEN: return 'g';
    }
}

// 位置索引增加
inline PositionIdx NextPosition(PositionIdx& pos) {
    pos = static_cast<PositionIdx>((pos + 1) % EDGE_COUNT);
    return pos;
}

// 位置索引加上偏移
inline PositionIdx OffsetPosition(PositionIdx pos, int offset) {
    if (offset < 0) {
        // 处理负偏移，确保结果为正
        offset = offset + (1 + (-offset) / EDGE_COUNT) * EDGE_COUNT;
    }
    return static_cast<PositionIdx>(((int)pos + offset * 2) % EDGE_COUNT);
}

// 位置索引减去偏移
inline PositionIdx BackPosition(PositionIdx pos, int offset) {
    return OffsetPosition(pos, -offset);
}

// 获取给定面的相对面及其旋转偏移
inline FaceWithOffset GetRelativeFace(FaceType face, FacePosition pos) {
    switch (face) {
        case FaceType::FACE_FRONT:
            switch (pos) {
                case FacePosition::UPPER: return FaceWithOffset(FaceType::FACE_TOP, 0);
                case FacePosition::LOWER: return FaceWithOffset(FaceType::FACE_BOTTOM, 0);
                case FacePosition::LSIDE: return FaceWithOffset(FaceType::FACE_LEFT, -1);
                case FacePosition::RSIDE: return FaceWithOffset(FaceType::FACE_RIGHT, +1);
            }
        case FaceType::FACE_BACK:
            switch (pos) {
                case FacePosition::UPPER: return FaceWithOffset(FaceType::FACE_BOTTOM, 0);
                case FacePosition::LOWER: return FaceWithOffset(FaceType::FACE_TOP, 0);
                case FacePosition::LSIDE: return FaceWithOffset(FaceType::FACE_LEFT, +1);
                case FacePosition::RSIDE: return FaceWithOffset(FaceType::FACE_RIGHT, -1);
            }
        case FaceType::FACE_LEFT:
            switch (pos) {
                case FacePosition::UPPER: return FaceWithOffset(FaceType::FACE_BACK, -1);
                case FacePosition::LOWER: return FaceWithOffset(FaceType::FACE_FRONT, +1);
                case FacePosition::LSIDE: return FaceWithOffset(FaceType::FACE_BOTTOM, +2);
                case FacePosition::RSIDE: return FaceWithOffset(FaceType::FACE_TOP, 0);
            }
        case FaceType::FACE_RIGHT:
            switch (pos) {
                case FacePosition::UPPER: return FaceWithOffset(FaceType::FACE_BACK, +1);
                case FacePosition::LOWER: return FaceWithOffset(FaceType::FACE_FRONT, -1);
                case FacePosition::LSIDE: return FaceWithOffset(FaceType::FACE_TOP, 0);
                case FacePosition::RSIDE: return FaceWithOffset(FaceType::FACE_BOTTOM, -2);
            }
        case FaceType::FACE_TOP:
            switch (pos) {
                case FacePosition::UPPER: return FaceWithOffset(FaceType::FACE_BACK, 0);
                case FacePosition::LOWER: return FaceWithOffset(FaceType::FACE_FRONT, 0);
                case FacePosition::LSIDE: return FaceWithOffset(FaceType::FACE_LEFT, 0);
                case FacePosition::RSIDE: return FaceWithOffset(FaceType::FACE_RIGHT, 0);
            }
        case FaceType::FACE_BOTTOM:
            switch (pos) {
                case FacePosition::UPPER: return FaceWithOffset(FaceType::FACE_FRONT, 0);
                case FacePosition::LOWER: return FaceWithOffset(FaceType::FACE_BACK, 0);
                case FacePosition::LSIDE: return FaceWithOffset(FaceType::FACE_LEFT, -2);
                case FacePosition::RSIDE: return FaceWithOffset(FaceType::FACE_RIGHT, +2);
            }
        default:
            throw std::invalid_argument("错误的面索引");
    }
}

// 常规操作转换为标准操作格式
inline StandardAction ConvertToStandard(MoveAction move) {
    StandardAction std_action;
    
    if (move.is_middle) {
        switch (move.face) {
            case FaceType::FACE_LEFT:
                std_action.action_index = 1;
                std_action.is_positive = move.dir == RotateDir::CLOCKWISE;
                break;
            case FaceType::FACE_RIGHT:
                std_action.action_index = 1;
                std_action.is_positive = move.dir == RotateDir::COUNTER_CLOCKWISE;
                break;
            case FaceType::FACE_TOP:
                std_action.action_index = 4;
                std_action.is_positive = move.dir == RotateDir::CLOCKWISE;
                break;
            case FaceType::FACE_BOTTOM:
                std_action.action_index = 4;
                std_action.is_positive = move.dir == RotateDir::COUNTER_CLOCKWISE;
                break;
            case FaceType::FACE_FRONT:
                std_action.action_index = 7;
                std_action.is_positive = move.dir == RotateDir::CLOCKWISE;
                break;
            case FaceType::FACE_BACK:
                std_action.action_index = 7;
                std_action.is_positive = move.dir == RotateDir::COUNTER_CLOCKWISE;
                break;
            default:
                throw std::runtime_error("无效的面");
        }
    } else {
        switch (move.face) {
            case FaceType::FACE_LEFT:
                std_action.action_index = 0;
                std_action.is_positive = move.dir == RotateDir::CLOCKWISE;
                break;
            case FaceType::FACE_RIGHT:
                std_action.action_index = 2;
                std_action.is_positive = move.dir == RotateDir::COUNTER_CLOCKWISE;
                break;
            case FaceType::FACE_TOP:
                std_action.action_index = 5;
                std_action.is_positive = move.dir == RotateDir::CLOCKWISE;
                break;
            case FaceType::FACE_BOTTOM:
                std_action.action_index = 3;
                std_action.is_positive = move.dir == RotateDir::COUNTER_CLOCKWISE;
                break;
            case FaceType::FACE_FRONT:
                std_action.action_index = 6;
                std_action.is_positive = move.dir == RotateDir::CLOCKWISE;
                break;
            case FaceType::FACE_BACK:
                std_action.action_index = 8;
                std_action.is_positive = move.dir == RotateDir::COUNTER_CLOCKWISE;
                break;
            default:
                throw std::runtime_error("无效的面");
        }
    }
    
    return std_action;
}

// 标准操作格式转换为常规操作
inline MoveAction ConvertToMove(StandardAction std_action) {
    MoveAction move;
    
    // 确定是否为中层旋转
    move.is_middle = (std_action.action_index == 1 || 
                      std_action.action_index == 4 || 
                      std_action.action_index == 7);
    
    // 根据索引确定面和方向
    switch (std_action.action_index) {
        case 0:
            move.face = FaceType::FACE_LEFT;
            move.dir = std_action.is_positive ? RotateDir::CLOCKWISE : RotateDir::COUNTER_CLOCKWISE;
            break;
        case 1:
            move.face = FaceType::FACE_LEFT;
            move.dir = std_action.is_positive ? RotateDir::CLOCKWISE : RotateDir::COUNTER_CLOCKWISE;
            break;
        case 2:
            move.face = FaceType::FACE_RIGHT;
            move.dir = std_action.is_positive ? RotateDir::COUNTER_CLOCKWISE : RotateDir::CLOCKWISE;
            break;
        case 3:
            move.face = FaceType::FACE_BOTTOM;
            move.dir = std_action.is_positive ? RotateDir::COUNTER_CLOCKWISE : RotateDir::CLOCKWISE;
            break;
        case 4:
            move.face = FaceType::FACE_TOP;
            move.dir = std_action.is_positive ? RotateDir::CLOCKWISE : RotateDir::COUNTER_CLOCKWISE;
            break;
        case 5:
            move.face = FaceType::FACE_TOP;
            move.dir = std_action.is_positive ? RotateDir::CLOCKWISE : RotateDir::COUNTER_CLOCKWISE;
            break;
        case 6:
            move.face = FaceType::FACE_FRONT;
            move.dir = std_action.is_positive ? RotateDir::CLOCKWISE : RotateDir::COUNTER_CLOCKWISE;
            break;
        case 7:
            move.face = FaceType::FACE_FRONT;
            move.dir = std_action.is_positive ? RotateDir::CLOCKWISE : RotateDir::COUNTER_CLOCKWISE;
            break;
        case 8:
            move.face = FaceType::FACE_BACK;
            move.dir = std_action.is_positive ? RotateDir::COUNTER_CLOCKWISE : RotateDir::CLOCKWISE;
            break;
        default:
            throw std::runtime_error("无效的操作索引");
    }
    
    return move;
} 