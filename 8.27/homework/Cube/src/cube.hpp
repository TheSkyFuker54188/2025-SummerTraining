#pragma once
#include <sstream>
#include "cube_def.hpp"

/**
 * 魔方类
 * 包含魔方状态和旋转操作
 */
class Cube {
   private:
    // 中心块和周围块的颜色表示
    Color centralColors[SurfaceCount];
    Color peripheralColors[SurfaceCount][RoundCount];

   public:
    // 从文本描述构造魔方
    Cube(std::string cubeDesc) {
        std::stringstream inputStream(cubeDesc);
        
        for (int surfaceIdx = 0; surfaceIdx < SurfaceCount; surfaceIdx++) {
            std::string surfaceLabel;
            inputStream >> surfaceLabel;
            
            if (surfaceLabel.empty())
                throw std::invalid_argument("魔方描述格式无效");
                
            Surface currentSurface = ToSurface(surfaceLabel);
            Color colorGrid[Rank * Rank];
            
            for (int colorIdx = 0; colorIdx < Rank * Rank; colorIdx++) {
                std::string colorLabel;
                inputStream >> colorLabel;
                
                if (colorLabel.empty())
                    throw std::invalid_argument("魔方描述格式无效");
                    
                colorGrid[colorIdx] = ToColor(colorLabel);
            }
            
            // 中心块
            centralColors[currentSurface] = colorGrid[4];
            
            // 周围块 - 按顺时针排列
            peripheralColors[currentSurface][0] = colorGrid[0];
            peripheralColors[currentSurface][1] = colorGrid[1];
            peripheralColors[currentSurface][2] = colorGrid[2];
            peripheralColors[currentSurface][3] = colorGrid[5];
            peripheralColors[currentSurface][4] = colorGrid[8];
            peripheralColors[currentSurface][5] = colorGrid[7];
            peripheralColors[currentSurface][6] = colorGrid[6];
            peripheralColors[currentSurface][7] = colorGrid[3];
        }
    }

    // 拷贝构造函数
    Cube(const Cube& other) {
        for (int i = 0; i < SurfaceCount; i++) {
            centralColors[i] = other.centralColors[i];
            for (int j = 0; j < RoundCount; j++) {
                peripheralColors[i][j] = other.peripheralColors[i][j];
            }
        }
    }

    // 转为字符串描述
    std::string ToString() const {
        std::string result;
        
        for (int surfaceIdx = 0; surfaceIdx < SurfaceCount; surfaceIdx++) {
            result += ToSurfaceName(static_cast<Surface>(surfaceIdx));
            result += ":\n";
            
            Color faceGrid[Rank][Rank];
            
            // 从内部表示重建3x3网格
            faceGrid[0][0] = peripheralColors[surfaceIdx][0];
            faceGrid[0][1] = peripheralColors[surfaceIdx][1];
            faceGrid[0][2] = peripheralColors[surfaceIdx][2];
            faceGrid[1][0] = peripheralColors[surfaceIdx][7];
            faceGrid[1][1] = centralColors[surfaceIdx];
            faceGrid[1][2] = peripheralColors[surfaceIdx][3];
            faceGrid[2][0] = peripheralColors[surfaceIdx][6];
            faceGrid[2][1] = peripheralColors[surfaceIdx][5];
            faceGrid[2][2] = peripheralColors[surfaceIdx][4];
            
            for (int row = 0; row < Rank; row++) {
                for (int col = 0; col < Rank; col++) {
                    result += ToColorName(faceGrid[row][col]);
                    result += " ";
                }
                result.pop_back();  // 移除末尾空格
                result += "\n";
            }
            result += "\n";
        }
        
        result.pop_back();  // 移除最后的换行符
        return result;
    }

    // 检查魔方是否还原完成
    bool IsSolved() const {
        for (int surfaceIdx = 0; surfaceIdx < SurfaceCount; surfaceIdx++) {
            Color centerColor = centralColors[surfaceIdx];
            for (int posIdx = 0; posIdx < RoundCount; posIdx++) {
                if (peripheralColors[surfaceIdx][posIdx] != centerColor)
                    return false;
            }
        }
        return true;
    }

    // 执行魔方旋转操作
    Cube DoRotation(CubeAction action) const {
        Surface targetSurface = action.surface;
        
        // 计算四个相邻面及其旋转偏移
        SurfaceWithRotationOffset topWithOffset = targetSurface + RelativePosition::Upside;
        SurfaceWithRotationOffset bottomWithOffset = targetSurface + RelativePosition::Downside;
        SurfaceWithRotationOffset leftWithOffset = targetSurface + RelativePosition::Leftside;
        SurfaceWithRotationOffset rightWithOffset = targetSurface + RelativePosition::Rightside;
        
        Surface topSurface = topWithOffset.surface;
        Surface bottomSurface = bottomWithOffset.surface;
        Surface leftSurface = leftWithOffset.surface;
        Surface rightSurface = rightWithOffset.surface;
        
        int topOffset = topWithOffset.rotation_offset;
        int bottomOffset = bottomWithOffset.rotation_offset;
        int leftOffset = leftWithOffset.rotation_offset;
        int rightOffset = rightWithOffset.rotation_offset;
        
        Cube rotatedCube(*this);
        
        // 根据旋转类型和方向进行操作
        if (!action.is_middle_layer) {
            // 外层旋转
            if (action.direction == Direction::Clockwise) {
                // 当前面顺时针旋转
                for (int i = 0; i < RoundCount; i++) {
                    rotatedCube.peripheralColors[targetSurface][(RotationPosition)i + 1] = 
                        this->peripheralColors[targetSurface][i];
                }
                
                // 相邻面的边缘块移动
                // 顶部面
                rotatedCube.peripheralColors[topSurface][RotationPosition::DownLeft - topOffset] =
                    this->peripheralColors[leftSurface][RotationPosition::DownRight - leftOffset];
                rotatedCube.peripheralColors[topSurface][RotationPosition::DownMiddle - topOffset] =
                    this->peripheralColors[leftSurface][RotationPosition::MiddleRight - leftOffset];
                rotatedCube.peripheralColors[topSurface][RotationPosition::DownRight - topOffset] =
                    this->peripheralColors[leftSurface][RotationPosition::UpRight - leftOffset];
                
                // 底部面
                rotatedCube.peripheralColors[bottomSurface][RotationPosition::UpLeft - bottomOffset] =
                    this->peripheralColors[rightSurface][RotationPosition::DownLeft - rightOffset];
                rotatedCube.peripheralColors[bottomSurface][RotationPosition::UpMiddle - bottomOffset] =
                    this->peripheralColors[rightSurface][RotationPosition::MiddleLeft - rightOffset];
                rotatedCube.peripheralColors[bottomSurface][RotationPosition::UpRight - bottomOffset] =
                    this->peripheralColors[rightSurface][RotationPosition::UpLeft - rightOffset];
                
                // 左侧面
                rotatedCube.peripheralColors[leftSurface][RotationPosition::UpRight - leftOffset] =
                    this->peripheralColors[bottomSurface][RotationPosition::UpLeft - bottomOffset];
                rotatedCube.peripheralColors[leftSurface][RotationPosition::MiddleRight - leftOffset] =
                    this->peripheralColors[bottomSurface][RotationPosition::UpMiddle - bottomOffset];
                rotatedCube.peripheralColors[leftSurface][RotationPosition::DownRight - leftOffset] =
                    this->peripheralColors[bottomSurface][RotationPosition::UpRight - bottomOffset];
                
                // 右侧面
                rotatedCube.peripheralColors[rightSurface][RotationPosition::UpLeft - rightOffset] =
                    this->peripheralColors[topSurface][RotationPosition::DownLeft - topOffset];
                rotatedCube.peripheralColors[rightSurface][RotationPosition::MiddleLeft - rightOffset] =
                    this->peripheralColors[topSurface][RotationPosition::DownMiddle - topOffset];
                rotatedCube.peripheralColors[rightSurface][RotationPosition::DownLeft - rightOffset] =
                    this->peripheralColors[topSurface][RotationPosition::DownRight - topOffset];
            }
            else {  // 逆时针
                // 当前面逆时针旋转
                for (int i = 0; i < RoundCount; i++) {
                    rotatedCube.peripheralColors[targetSurface][(RotationPosition)i + (-1)] = 
                        this->peripheralColors[targetSurface][i];
                }
                
                // 相邻面的边缘块移动
                // 顶部面
                rotatedCube.peripheralColors[topSurface][RotationPosition::DownLeft - topOffset] =
                    this->peripheralColors[rightSurface][RotationPosition::UpLeft - rightOffset];
                rotatedCube.peripheralColors[topSurface][RotationPosition::DownMiddle - topOffset] =
                    this->peripheralColors[rightSurface][RotationPosition::MiddleLeft - rightOffset];
                rotatedCube.peripheralColors[topSurface][RotationPosition::DownRight - topOffset] =
                    this->peripheralColors[rightSurface][RotationPosition::DownLeft - rightOffset];
                
                // 底部面
                rotatedCube.peripheralColors[bottomSurface][RotationPosition::UpLeft - bottomOffset] =
                    this->peripheralColors[leftSurface][RotationPosition::UpRight - leftOffset];
                rotatedCube.peripheralColors[bottomSurface][RotationPosition::UpMiddle - bottomOffset] =
                    this->peripheralColors[leftSurface][RotationPosition::MiddleRight - leftOffset];
                rotatedCube.peripheralColors[bottomSurface][RotationPosition::UpRight - bottomOffset] =
                    this->peripheralColors[leftSurface][RotationPosition::DownRight - leftOffset];
                
                // 左侧面
                rotatedCube.peripheralColors[leftSurface][RotationPosition::UpRight - leftOffset] =
                    this->peripheralColors[topSurface][RotationPosition::DownRight - topOffset];
                rotatedCube.peripheralColors[leftSurface][RotationPosition::MiddleRight - leftOffset] =
                    this->peripheralColors[topSurface][RotationPosition::DownMiddle - topOffset];
                rotatedCube.peripheralColors[leftSurface][RotationPosition::DownRight - leftOffset] =
                    this->peripheralColors[topSurface][RotationPosition::DownLeft - topOffset];
                
                // 右侧面
                rotatedCube.peripheralColors[rightSurface][RotationPosition::UpLeft - rightOffset] =
                    this->peripheralColors[bottomSurface][RotationPosition::UpRight - bottomOffset];
                rotatedCube.peripheralColors[rightSurface][RotationPosition::MiddleLeft - rightOffset] =
                    this->peripheralColors[bottomSurface][RotationPosition::UpMiddle - bottomOffset];
                rotatedCube.peripheralColors[rightSurface][RotationPosition::DownLeft - rightOffset] =
                    this->peripheralColors[bottomSurface][RotationPosition::UpLeft - bottomOffset];
            }
        }
        else {  // 中层旋转
            if (action.direction == Direction::Clockwise) {
                // 顶部
                rotatedCube.centralColors[topSurface] = this->centralColors[leftSurface];
                rotatedCube.peripheralColors[topSurface][RotationPosition::MiddleLeft - topOffset] =
                    this->peripheralColors[leftSurface][RotationPosition::DownMiddle - leftOffset];
                rotatedCube.peripheralColors[topSurface][RotationPosition::MiddleRight - topOffset] =
                    this->peripheralColors[leftSurface][RotationPosition::UpMiddle - leftOffset];
                
                // 底部
                rotatedCube.centralColors[bottomSurface] = this->centralColors[rightSurface];
                rotatedCube.peripheralColors[bottomSurface][RotationPosition::MiddleRight - bottomOffset] =
                    this->peripheralColors[rightSurface][RotationPosition::UpMiddle - rightOffset];
                rotatedCube.peripheralColors[bottomSurface][RotationPosition::MiddleLeft - bottomOffset] =
                    this->peripheralColors[rightSurface][RotationPosition::DownMiddle - rightOffset];
                
                // 左侧
                rotatedCube.centralColors[leftSurface] = this->centralColors[bottomSurface];
                rotatedCube.peripheralColors[leftSurface][RotationPosition::DownMiddle - leftOffset] =
                    this->peripheralColors[bottomSurface][RotationPosition::MiddleRight - bottomOffset];
                rotatedCube.peripheralColors[leftSurface][RotationPosition::UpMiddle - leftOffset] =
                    this->peripheralColors[bottomSurface][RotationPosition::MiddleLeft - bottomOffset];
                
                // 右侧
                rotatedCube.centralColors[rightSurface] = this->centralColors[topSurface];
                rotatedCube.peripheralColors[rightSurface][RotationPosition::DownMiddle - rightOffset] =
                    this->peripheralColors[topSurface][RotationPosition::MiddleRight - topOffset];
                rotatedCube.peripheralColors[rightSurface][RotationPosition::UpMiddle - rightOffset] =
                    this->peripheralColors[topSurface][RotationPosition::MiddleLeft - topOffset];
            }
            else {  // 逆时针
                // 顶部
                rotatedCube.centralColors[topSurface] = this->centralColors[rightSurface];
                rotatedCube.peripheralColors[topSurface][RotationPosition::MiddleLeft - topOffset] =
                    this->peripheralColors[rightSurface][RotationPosition::UpMiddle - rightOffset];
                rotatedCube.peripheralColors[topSurface][RotationPosition::MiddleRight - topOffset] =
                    this->peripheralColors[rightSurface][RotationPosition::DownMiddle - rightOffset];
                
                // 底部
                rotatedCube.centralColors[bottomSurface] = this->centralColors[leftSurface];
                rotatedCube.peripheralColors[bottomSurface][RotationPosition::MiddleLeft - bottomOffset] =
                    this->peripheralColors[leftSurface][RotationPosition::UpMiddle - leftOffset];
                rotatedCube.peripheralColors[bottomSurface][RotationPosition::MiddleRight - bottomOffset] =
                    this->peripheralColors[leftSurface][RotationPosition::DownMiddle - leftOffset];
                
                // 左侧
                rotatedCube.centralColors[leftSurface] = this->centralColors[topSurface];
                rotatedCube.peripheralColors[leftSurface][RotationPosition::UpMiddle - leftOffset] =
                    this->peripheralColors[topSurface][RotationPosition::MiddleRight - topOffset];
                rotatedCube.peripheralColors[leftSurface][RotationPosition::DownMiddle - leftOffset] =
                    this->peripheralColors[topSurface][RotationPosition::MiddleLeft - topOffset];
                
                // 右侧
                rotatedCube.centralColors[rightSurface] = this->centralColors[bottomSurface];
                rotatedCube.peripheralColors[rightSurface][RotationPosition::UpMiddle - rightOffset] =
                    this->peripheralColors[bottomSurface][RotationPosition::MiddleRight - bottomOffset];
                rotatedCube.peripheralColors[rightSurface][RotationPosition::DownMiddle - rightOffset] =
                    this->peripheralColors[bottomSurface][RotationPosition::MiddleLeft - bottomOffset];
            }
        }
        
        return rotatedCube;
    }
};