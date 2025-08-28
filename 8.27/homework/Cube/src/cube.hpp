#ifndef MAGIC_CUBE_H
#define MAGIC_CUBE_H

#include <sstream>
#include "cube_def.hpp"

namespace cube {

// 前置声明
class InvalidCubeOperationException;

/*******************************************************
 * 魔方类
 * 表示魔方的状态和基本操作
 *******************************************************/
class Cube {
public:
    /******************************************
     * 构造与析构
     ******************************************/
    
    /**
     * 从文本描述构造魔方状态
     */
    explicit Cube(std::string description);
    
    /**
     * 拷贝构造函数
     */
    Cube(const Cube& other);
    
    /**
     * 虚析构函数
     */
    virtual ~Cube() {}

    /******************************************
     * 公开方法
     ******************************************/

    /**
     * 将魔方状态转换为字符串表示
     * @return 格式化的魔方状态字符串
     */
    std::string ToString() const;
    
    /**
     * 判断魔方是否处于已解决状态
     * @return 如果魔方已解决返回true，否则返回false
     */
    bool IsSolved() const;
    
    /**
     * 执行一次旋转操作并返回新的魔方状态
     * @param action 要执行的操作
     * @return 执行操作后的新魔方状态
     */
    Cube DoRotation(MoveAction action) const;

private:
    /******************************************
     * 数据成员
     ******************************************/
    
    /* 中心块颜色 */
    ColorType centerColor[FACE_COUNT];
    
    /* 周围块颜色 */
    ColorType borderColor[FACE_COUNT][EDGE_COUNT];
    
    /******************************************
     * 私有辅助方法
     ******************************************/
    
    /**
     * 旋转一个面上的所有边缘块
     * @param newCube 新魔方状态
     * @param face 要旋转的面
     * @param isClockwise 是否顺时针旋转
     */
    void RotateFaceEdges(Cube& newCube, FaceType face, bool isClockwise) const;
    
    /**
     * 执行相邻面块的交换
     * @param newCube 新魔方状态
     * @param face 当前旋转的面
     * @param isClockwise 是否顺时针旋转
     */
    void SwapAdjacentEdges(Cube& newCube, FaceType face, bool isClockwise) const;
    
    /**
     * 执行中层旋转操作
     * @param newCube 新魔方状态
     * @param face 当前旋转的面
     * @param isClockwise 是否顺时针旋转
     */
    void RotateMiddleLayer(Cube& newCube, FaceType face, bool isClockwise) const;
};

/**
 * 无效魔方操作异常
 */
class InvalidCubeOperationException : public MagicCubeException {
public:
    InvalidCubeOperationException(const std::string& msg) 
        : MagicCubeException("无效的魔方操作: " + msg) {}
};

/*******************************************************
 * Cube类方法实现
 *******************************************************/

/**
 * 从文本描述构造魔方状态
 */
Cube::Cube(std::string description) {
    std::stringstream ss(description);
    
    // 读取六个面的描述
    for (int i = 0; i < FACE_COUNT; i++) {
        try {
            // 读取面的名称
            std::string faceName;
            ss >> faceName;
            
            if (faceName.empty()) {
                throw MagicCubeException("魔方描述格式错误");
            }
            
            // 将面名称转换为枚举
            FaceType face = StringToFace(faceName);
            
            // 读取面的颜色
            ColorType colorMatrix[CUBE_SIZE * CUBE_SIZE];
            
            for (int j = 0; j < CUBE_SIZE * CUBE_SIZE; j++) {
                std::string colorCode;
                ss >> colorCode;
                
                if (colorCode.empty()) {
                    throw MagicCubeException("魔方颜色描述格式错误");
                }
                
                colorMatrix[j] = CharToColor(colorCode);
            }
            
            // 设置中心块颜色
            centerColor[face] = colorMatrix[4]; // 3x3的中心位置
            
            // 设置周围块颜色，按顺时针排列
            borderColor[face][POS_TOP_LEFT] = colorMatrix[0];
            borderColor[face][POS_TOP_MID] = colorMatrix[1];
            borderColor[face][POS_TOP_RIGHT] = colorMatrix[2];
            borderColor[face][POS_MID_RIGHT] = colorMatrix[5];
            borderColor[face][POS_BOT_RIGHT] = colorMatrix[8];
            borderColor[face][POS_BOT_MID] = colorMatrix[7];
            borderColor[face][POS_BOT_LEFT] = colorMatrix[6];
            borderColor[face][POS_MID_LEFT] = colorMatrix[3];
        }
        catch (const MagicCubeException& e) {
            throw MagicCubeException(std::string("解析第") + std::to_string(i+1) + "个面时出错: " + e.what());
        }
    }
}

/**
 * 拷贝构造函数
 */
Cube::Cube(const Cube& other) {
    for (int f = 0; f < FACE_COUNT; f++) {
        // 复制中心块颜色
        centerColor[f] = other.centerColor[f];
        
        // 复制周围块颜色
        for (int e = 0; e < EDGE_COUNT; e++) {
            borderColor[f][e] = other.borderColor[f][e];
        }
    }
}

/**
 * 将魔方状态转换为字符串表示
 */
std::string Cube::ToString() const {
    std::stringstream result;
    
    // 处理每一个面
    for (int f = 0; f < FACE_COUNT; f++) {
        FaceType face = static_cast<FaceType>(f);
        
        // 添加面名称
        result << FaceToString(face) << ":\n";
        
        // 将边缘数组映射回3x3矩阵
        ColorType grid[CUBE_SIZE][CUBE_SIZE];
        
        // 从内部表示重建3x3网格
        grid[0][0] = borderColor[f][POS_TOP_LEFT];
        grid[0][1] = borderColor[f][POS_TOP_MID];
        grid[0][2] = borderColor[f][POS_TOP_RIGHT];
        grid[1][0] = borderColor[f][POS_MID_LEFT];
        grid[1][1] = centerColor[f];
        grid[1][2] = borderColor[f][POS_MID_RIGHT];
        grid[2][0] = borderColor[f][POS_BOT_LEFT];
        grid[2][1] = borderColor[f][POS_BOT_MID];
        grid[2][2] = borderColor[f][POS_BOT_RIGHT];
        
        // 按行输出颜色
        for (int r = 0; r < CUBE_SIZE; r++) {
            for (int c = 0; c < CUBE_SIZE; c++) {
                result << ColorToChar(grid[r][c]);
                
                // 除了每行最后一个元素，都添加空格
                if (c < CUBE_SIZE - 1) {
                    result << " ";
                }
            }
            result << "\n";
        }
        result << "\n";
    }
    
    // 转换为字符串并移除最后一个换行符
    std::string finalResult = result.str();
    if (!finalResult.empty()) {
        finalResult.pop_back();
    }
    
    return finalResult;
}

/**
 * 判断魔方是否处于已解决状态
 */
bool Cube::IsSolved() const {
    // 检查每个面
    for (int f = 0; f < FACE_COUNT; f++) {
        ColorType center = centerColor[f];
        
        // 检查该面的所有边缘块
        for (int e = 0; e < EDGE_COUNT; e++) {
            if (borderColor[f][e] != center) {
                return false; // 发现不同颜色，魔方未解决
            }
        }
    }
    
    return true; // 所有面都是同一颜色
}

/**
 * 旋转一个面上的所有边缘块
 */
void Cube::RotateFaceEdges(Cube& newCube, FaceType face, bool isClockwise) const {
    for (int i = 0; i < EDGE_COUNT; i++) {
        PositionIdx newPos = isClockwise 
            ? OffsetPosition(static_cast<PositionIdx>(i), 1)
            : OffsetPosition(static_cast<PositionIdx>(i), -1);
        
        newCube.borderColor[face][newPos] = this->borderColor[face][i];
    }
}

/**
 * 执行相邻面块的交换 - 外层旋转
 */
void Cube::SwapAdjacentEdges(Cube& newCube, FaceType face, bool isClockwise) const {
    // 获取相邻面
    FaceWithOffset topInfo = GetRelativeFace(face, FacePosition::UPPER);
    FaceWithOffset botInfo = GetRelativeFace(face, FacePosition::LOWER);
    FaceWithOffset leftInfo = GetRelativeFace(face, FacePosition::LSIDE);
    FaceWithOffset rightInfo = GetRelativeFace(face, FacePosition::RSIDE);
    
    FaceType topFace = topInfo.face;
    FaceType botFace = botInfo.face;
    FaceType leftFace = leftInfo.face;
    FaceType rightFace = rightInfo.face;
    
    int topOffset = topInfo.offset;
    int botOffset = botInfo.offset;
    int leftOffset = leftInfo.offset;
    int rightOffset = rightInfo.offset;
    
    if (isClockwise) {
        // 顶面 <- 左面
        newCube.borderColor[topFace][BackPosition(POS_BOT_LEFT, topOffset)] = 
            this->borderColor[leftFace][BackPosition(POS_BOT_RIGHT, leftOffset)];
        newCube.borderColor[topFace][BackPosition(POS_BOT_MID, topOffset)] = 
            this->borderColor[leftFace][BackPosition(POS_MID_RIGHT, leftOffset)];
        newCube.borderColor[topFace][BackPosition(POS_BOT_RIGHT, topOffset)] = 
            this->borderColor[leftFace][BackPosition(POS_TOP_RIGHT, leftOffset)];
        
        // 底面 <- 右面
        newCube.borderColor[botFace][BackPosition(POS_TOP_LEFT, botOffset)] = 
            this->borderColor[rightFace][BackPosition(POS_BOT_LEFT, rightOffset)];
        newCube.borderColor[botFace][BackPosition(POS_TOP_MID, botOffset)] = 
            this->borderColor[rightFace][BackPosition(POS_MID_LEFT, rightOffset)];
        newCube.borderColor[botFace][BackPosition(POS_TOP_RIGHT, botOffset)] = 
            this->borderColor[rightFace][BackPosition(POS_TOP_LEFT, rightOffset)];
        
        // 左面 <- 底面
        newCube.borderColor[leftFace][BackPosition(POS_TOP_RIGHT, leftOffset)] = 
            this->borderColor[botFace][BackPosition(POS_TOP_LEFT, botOffset)];
        newCube.borderColor[leftFace][BackPosition(POS_MID_RIGHT, leftOffset)] = 
            this->borderColor[botFace][BackPosition(POS_TOP_MID, botOffset)];
        newCube.borderColor[leftFace][BackPosition(POS_BOT_RIGHT, leftOffset)] = 
            this->borderColor[botFace][BackPosition(POS_TOP_RIGHT, botOffset)];
        
        // 右面 <- 顶面
        newCube.borderColor[rightFace][BackPosition(POS_TOP_LEFT, rightOffset)] = 
            this->borderColor[topFace][BackPosition(POS_BOT_LEFT, topOffset)];
        newCube.borderColor[rightFace][BackPosition(POS_MID_LEFT, rightOffset)] = 
            this->borderColor[topFace][BackPosition(POS_BOT_MID, topOffset)];
        newCube.borderColor[rightFace][BackPosition(POS_BOT_LEFT, rightOffset)] = 
            this->borderColor[topFace][BackPosition(POS_BOT_RIGHT, topOffset)];
    }
    else { // 逆时针旋转
        // 顶面 <- 右面
        newCube.borderColor[topFace][BackPosition(POS_BOT_LEFT, topOffset)] = 
            this->borderColor[rightFace][BackPosition(POS_TOP_LEFT, rightOffset)];
        newCube.borderColor[topFace][BackPosition(POS_BOT_MID, topOffset)] = 
            this->borderColor[rightFace][BackPosition(POS_MID_LEFT, rightOffset)];
        newCube.borderColor[topFace][BackPosition(POS_BOT_RIGHT, topOffset)] = 
            this->borderColor[rightFace][BackPosition(POS_BOT_LEFT, rightOffset)];
        
        // 底面 <- 左面
        newCube.borderColor[botFace][BackPosition(POS_TOP_LEFT, botOffset)] = 
            this->borderColor[leftFace][BackPosition(POS_TOP_RIGHT, leftOffset)];
        newCube.borderColor[botFace][BackPosition(POS_TOP_MID, botOffset)] = 
            this->borderColor[leftFace][BackPosition(POS_MID_RIGHT, leftOffset)];
        newCube.borderColor[botFace][BackPosition(POS_TOP_RIGHT, botOffset)] = 
            this->borderColor[leftFace][BackPosition(POS_BOT_RIGHT, leftOffset)];
        
        // 左面 <- 顶面
        newCube.borderColor[leftFace][BackPosition(POS_TOP_RIGHT, leftOffset)] = 
            this->borderColor[topFace][BackPosition(POS_BOT_RIGHT, topOffset)];
        newCube.borderColor[leftFace][BackPosition(POS_MID_RIGHT, leftOffset)] = 
            this->borderColor[topFace][BackPosition(POS_BOT_MID, topOffset)];
        newCube.borderColor[leftFace][BackPosition(POS_BOT_RIGHT, leftOffset)] = 
            this->borderColor[topFace][BackPosition(POS_BOT_LEFT, topOffset)];
        
        // 右面 <- 底面
        newCube.borderColor[rightFace][BackPosition(POS_TOP_LEFT, rightOffset)] = 
            this->borderColor[botFace][BackPosition(POS_TOP_RIGHT, botOffset)];
        newCube.borderColor[rightFace][BackPosition(POS_MID_LEFT, rightOffset)] = 
            this->borderColor[botFace][BackPosition(POS_TOP_MID, botOffset)];
        newCube.borderColor[rightFace][BackPosition(POS_BOT_LEFT, rightOffset)] = 
            this->borderColor[botFace][BackPosition(POS_TOP_LEFT, botOffset)];
    }
}

/**
 * 执行中层旋转操作
 */
void Cube::RotateMiddleLayer(Cube& newCube, FaceType face, bool isClockwise) const {
    // 获取相邻面
    FaceWithOffset topInfo = GetRelativeFace(face, FacePosition::UPPER);
    FaceWithOffset botInfo = GetRelativeFace(face, FacePosition::LOWER);
    FaceWithOffset leftInfo = GetRelativeFace(face, FacePosition::LSIDE);
    FaceWithOffset rightInfo = GetRelativeFace(face, FacePosition::RSIDE);
    
    FaceType topFace = topInfo.face;
    FaceType botFace = botInfo.face;
    FaceType leftFace = leftInfo.face;
    FaceType rightFace = rightInfo.face;
    
    int topOffset = topInfo.offset;
    int botOffset = botInfo.offset;
    int leftOffset = leftInfo.offset;
    int rightOffset = rightInfo.offset;
    
    if (isClockwise) {
        // 顶面 <- 左面
        newCube.centerColor[topFace] = this->centerColor[leftFace];
        newCube.borderColor[topFace][BackPosition(POS_MID_LEFT, topOffset)] = 
            this->borderColor[leftFace][BackPosition(POS_BOT_MID, leftOffset)];
        newCube.borderColor[topFace][BackPosition(POS_MID_RIGHT, topOffset)] = 
            this->borderColor[leftFace][BackPosition(POS_TOP_MID, leftOffset)];
        
        // 底面 <- 右面
        newCube.centerColor[botFace] = this->centerColor[rightFace];
        newCube.borderColor[botFace][BackPosition(POS_MID_RIGHT, botOffset)] = 
            this->borderColor[rightFace][BackPosition(POS_TOP_MID, rightOffset)];
        newCube.borderColor[botFace][BackPosition(POS_MID_LEFT, botOffset)] = 
            this->borderColor[rightFace][BackPosition(POS_BOT_MID, rightOffset)];
        
        // 左面 <- 底面
        newCube.centerColor[leftFace] = this->centerColor[botFace];
        newCube.borderColor[leftFace][BackPosition(POS_BOT_MID, leftOffset)] = 
            this->borderColor[botFace][BackPosition(POS_MID_RIGHT, botOffset)];
        newCube.borderColor[leftFace][BackPosition(POS_TOP_MID, leftOffset)] = 
            this->borderColor[botFace][BackPosition(POS_MID_LEFT, botOffset)];
        
        // 右面 <- 顶面
        newCube.centerColor[rightFace] = this->centerColor[topFace];
        newCube.borderColor[rightFace][BackPosition(POS_BOT_MID, rightOffset)] = 
            this->borderColor[topFace][BackPosition(POS_MID_RIGHT, topOffset)];
        newCube.borderColor[rightFace][BackPosition(POS_TOP_MID, rightOffset)] = 
            this->borderColor[topFace][BackPosition(POS_MID_LEFT, topOffset)];
    }
    else { // 逆时针旋转
        // 顶面 <- 右面
        newCube.centerColor[topFace] = this->centerColor[rightFace];
        newCube.borderColor[topFace][BackPosition(POS_MID_LEFT, topOffset)] = 
            this->borderColor[rightFace][BackPosition(POS_TOP_MID, rightOffset)];
        newCube.borderColor[topFace][BackPosition(POS_MID_RIGHT, topOffset)] = 
            this->borderColor[rightFace][BackPosition(POS_BOT_MID, rightOffset)];
        
        // 底面 <- 左面
        newCube.centerColor[botFace] = this->centerColor[leftFace];
        newCube.borderColor[botFace][BackPosition(POS_MID_LEFT, botOffset)] = 
            this->borderColor[leftFace][BackPosition(POS_TOP_MID, leftOffset)];
        newCube.borderColor[botFace][BackPosition(POS_MID_RIGHT, botOffset)] = 
            this->borderColor[leftFace][BackPosition(POS_BOT_MID, leftOffset)];
        
        // 左面 <- 顶面
        newCube.centerColor[leftFace] = this->centerColor[topFace];
        newCube.borderColor[leftFace][BackPosition(POS_TOP_MID, leftOffset)] = 
            this->borderColor[topFace][BackPosition(POS_MID_RIGHT, topOffset)];
        newCube.borderColor[leftFace][BackPosition(POS_BOT_MID, leftOffset)] = 
            this->borderColor[topFace][BackPosition(POS_MID_LEFT, topOffset)];
        
        // 右面 <- 底面
        newCube.centerColor[rightFace] = this->centerColor[botFace];
        newCube.borderColor[rightFace][BackPosition(POS_TOP_MID, rightOffset)] = 
            this->borderColor[botFace][BackPosition(POS_MID_RIGHT, botOffset)];
        newCube.borderColor[rightFace][BackPosition(POS_BOT_MID, rightOffset)] = 
            this->borderColor[botFace][BackPosition(POS_MID_LEFT, botOffset)];
    }
}

/**
 * 执行一次旋转操作并返回新的魔方状态
 */
Cube Cube::DoRotation(MoveAction action) const {
    try {
        FaceType face = action.face;
        bool isClockwise = (action.dir == RotateDir::CLOCKWISE);
        
        // 创建新的魔方状态（拷贝当前状态）
        Cube newCube(*this);
        
        if (!action.is_middle) {
            // 外层旋转
            
            // 1. 旋转当前面的边缘块
            RotateFaceEdges(newCube, face, isClockwise);
            
            // 2. 交换相邻面的边缘块
            SwapAdjacentEdges(newCube, face, isClockwise);
        }
        else {
            // 中层旋转
            RotateMiddleLayer(newCube, face, isClockwise);
        }
        
        return newCube;
    }
    catch (const MagicCubeException& e) {
        throw InvalidCubeOperationException("执行旋转操作时出错: " + std::string(e.what()));
    }
}

} // namespace cube

#endif // MAGIC_CUBE_H