#pragma once
#include <sstream>
#include "cube_def.hpp"

/**
 * 魔方类
 * 表示3阶魔方状态和基本操作
 */
class Cube {
private:
    // 中心块颜色
    ColorType centerColor[FACE_COUNT];
    
    // 周围块颜色
    ColorType borderColor[FACE_COUNT][EDGE_COUNT];

public:
    /**
     * 从文本描述构造魔方状态
     */
    Cube(std::string description) {
        std::stringstream ss(description);
        
        // 读取六个面的描述
        for (int i = 0; i < FACE_COUNT; i++) {
            // 读取面的名称
            std::string faceName;
            ss >> faceName;
            
            if (faceName.empty()) {
                throw std::invalid_argument("魔方描述格式错误");
            }
            
            // 将面名称转换为枚举
            FaceType face = StringToFace(faceName);
            
            // 读取面的颜色
            ColorType colorMatrix[CUBE_SIZE * CUBE_SIZE];
            
            for (int j = 0; j < CUBE_SIZE * CUBE_SIZE; j++) {
                std::string colorCode;
                ss >> colorCode;
                
                if (colorCode.empty()) {
                    throw std::invalid_argument("魔方颜色描述格式错误");
                }
                
                colorMatrix[j] = CharToColor(colorCode);
            }
            
            // 设置中心块颜色
            centerColor[face] = colorMatrix[4]; // 3x3的中心位置
            
            // 设置周围块颜色，按顺时针排列
            // 将3x3矩阵的边缘映射到边缘数组
            borderColor[face][POS_TOP_LEFT] = colorMatrix[0];
            borderColor[face][POS_TOP_MID] = colorMatrix[1];
            borderColor[face][POS_TOP_RIGHT] = colorMatrix[2];
            borderColor[face][POS_MID_RIGHT] = colorMatrix[5];
            borderColor[face][POS_BOT_RIGHT] = colorMatrix[8];
            borderColor[face][POS_BOT_MID] = colorMatrix[7];
            borderColor[face][POS_BOT_LEFT] = colorMatrix[6];
            borderColor[face][POS_MID_LEFT] = colorMatrix[3];
        }
    }
    
    /**
     * 拷贝构造函数
     */
    Cube(const Cube& other) {
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
     * 将魔方状态转换为字符串
     */
    std::string ToString() const {
        std::string result;
        
        // 处理每一个面
        for (int f = 0; f < FACE_COUNT; f++) {
            FaceType face = static_cast<FaceType>(f);
            
            // 添加面名称
            result += FaceToString(face) + ":\n";
            
            // 将边缘数组映射回3x3矩阵
            ColorType grid[CUBE_SIZE][CUBE_SIZE];
            
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
                    result += ColorToChar(grid[r][c]);
                    result += " ";
                }
                result = result.substr(0, result.length() - 1); // 移除末尾空格
                result += "\n";
            }
            result += "\n";
        }
        
        // 移除最后一个换行符
        if (!result.empty()) {
            result.pop_back();
        }
        
        return result;
    }
    
    /**
     * 判断魔方是否已解决(每个面都是同一种颜色)
     */
    bool IsSolved() const {
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
     * 执行一次魔方旋转操作
     */
    Cube DoRotation(MoveAction action) const {
        FaceType face = action.face;
        
        // 计算相对面及其旋转偏移
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
        
        // 创建新的魔方状态
        Cube newCube(*this);
        
        // 根据操作类型执行旋转
        if (!action.is_middle) {
            // 外层旋转
            if (action.dir == RotateDir::CLOCKWISE) {
                // 旋转当前面的边缘块
                for (int i = 0; i < EDGE_COUNT; i++) {
                    PositionIdx newPos = OffsetPosition(static_cast<PositionIdx>(i), 1);
                    newCube.borderColor[face][newPos] = this->borderColor[face][i];
                }
                
                // 处理四个相邻面的边缘块交换
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
                // 旋转当前面的边缘块
                for (int i = 0; i < EDGE_COUNT; i++) {
                    PositionIdx newPos = OffsetPosition(static_cast<PositionIdx>(i), -1);
                    newCube.borderColor[face][newPos] = this->borderColor[face][i];
                }
                
                // 处理四个相邻面的边缘块交换，交换方向与顺时针旋转相反
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
        else { // 中间层旋转
            if (action.dir == RotateDir::CLOCKWISE) {
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
        
        return newCube;
    }
};