/*
 *  Description:    Implements Cube class, which contains the cube state and
 *                  has the ability to import, export and rotate the cube.
 *
 *  Author(s):      Nictheboy Li <nictheboy@outlook.com>
 *
 *  Last Updated:   2024-06-25
 *
 */

#pragma once
#include <sstream>
#include "cube_common.hpp"

class Cube {
   private:
    //  0    1    2
    //  7   mid   3
    //  6    5    4
    Color color_middle[SurfaceCount];
    Color color_arround[SurfaceCount][RoundCount];

   public:
    Cube(std::string cube_description) {
        std::stringstream ss(cube_description);
        for (int i = 0; i < SurfaceCount; i++) {
            std::string surface_name;
            ss >> surface_name;
            if (surface_name.length() == 0)
                throw std::invalid_argument("Invalid cube description format");
            Surface surface = ToSurface(surface_name);
            Color colors[Rank * Rank];
            for (int j = 0; j < Rank * Rank; j++) {
                std::string color_name;
                ss >> color_name;
                if (color_name.length() == 0)
                    throw std::invalid_argument("Invalid cube description format");
                colors[j] = ToColor(color_name);
            }
            color_middle[surface] = colors[4];
            color_arround[surface][0] = colors[0];
            color_arround[surface][1] = colors[1];
            color_arround[surface][2] = colors[2];
            color_arround[surface][3] = colors[5];
            color_arround[surface][4] = colors[8];
            color_arround[surface][5] = colors[7];
            color_arround[surface][6] = colors[6];
            color_arround[surface][7] = colors[3];
        }
    }

    Cube(const Cube& another) {
        for (int i = 0; i < SurfaceCount; i++) {
            color_middle[i] = another.color_middle[i];
            for (int j = 0; j < RoundCount; j++) {
                color_arround[i][j] = another.color_arround[i][j];
            }
        }
    }

    std::string ToString() const {
        std::string cube_description;
        for (int i = 0; i < SurfaceCount; i++) {
            cube_description += ToSurfaceName(static_cast<Surface>(i));
            cube_description += ":\n";
            //  0    1    2
            //  7   mid   3
            //  6    5    4
            Color colors[Rank][Rank];
            colors[0][0] = color_arround[i][0];
            colors[0][1] = color_arround[i][1];
            colors[0][2] = color_arround[i][2];
            colors[1][0] = color_arround[i][7];
            colors[1][1] = color_middle[i];
            colors[1][2] = color_arround[i][3];
            colors[2][0] = color_arround[i][6];
            colors[2][1] = color_arround[i][5];
            colors[2][2] = color_arround[i][4];
            for (int row = 0; row < Rank; row++) {
                for (int column = 0; column < Rank; column++) {
                    cube_description += ToColorName(colors[row][column]);
                    cube_description += " ";
                }
                cube_description.pop_back();
                cube_description += "\n";
            }
            cube_description += "\n";
        }
        cube_description.pop_back();
        return cube_description;
    }

    bool HasBeenSolved() const {
        for (int i = 0; i < SurfaceCount; i++) {
            Color middle_color = color_middle[i];
            for (int j = 0; j < RoundCount; j++) {
                if (color_arround[i][j] != middle_color)
                    return false;
            }
        }
        return true;
    }

    Cube Rotate(CubeAction action) const {
        Surface surface = action.surface;
        SurfaceWithRotationOffset surface_upside_with_rotation_offset = surface + RelativePosition::Upside;
        SurfaceWithRotationOffset surface_downside_with_rotation_offset = surface + RelativePosition::Downside;
        SurfaceWithRotationOffset surface_leftside_with_rotation_offset = surface + RelativePosition::Leftside;
        SurfaceWithRotationOffset surface_rightside_with_rotation_offset = surface + RelativePosition::Rightside;
        Surface surface_upside = surface_upside_with_rotation_offset.surface;
        Surface surface_downside = surface_downside_with_rotation_offset.surface;
        Surface surface_leftside = surface_leftside_with_rotation_offset.surface;
        Surface surface_rightside = surface_rightside_with_rotation_offset.surface;
        int rotation_offset_upside = surface_upside_with_rotation_offset.rotation_offset;
        int rotation_offset_downside = surface_downside_with_rotation_offset.rotation_offset;
        int rotation_offset_leftside = surface_leftside_with_rotation_offset.rotation_offset;
        int rotation_offset_rightside = surface_rightside_with_rotation_offset.rotation_offset;
        Cube rotated_cube(*this);
        if (action.is_middle_layer == false) {
            switch (action.direction) {
                case Direction::Clockwise:
                    for (int i = 0; i < RoundCount; i++) {
                        rotated_cube.color_arround[surface][(RotationPosition)i + 1] = this->color_arround[surface][i];
                    }
                    // Upside:
                    rotated_cube.color_arround[surface_upside][RotationPosition::DownLeft - rotation_offset_upside] =
                        this->color_arround[surface_leftside][RotationPosition::DownRight - rotation_offset_leftside];
                    rotated_cube.color_arround[surface_upside][RotationPosition::DownMiddle - rotation_offset_upside] =
                        this->color_arround[surface_leftside][RotationPosition::MiddleRight - rotation_offset_leftside];
                    rotated_cube.color_arround[surface_upside][RotationPosition::DownRight - rotation_offset_upside] =
                        this->color_arround[surface_leftside][RotationPosition::UpRight - rotation_offset_leftside];
                    // Downside:
                    rotated_cube.color_arround[surface_downside][RotationPosition::UpLeft - rotation_offset_downside] =
                        this->color_arround[surface_rightside][RotationPosition::DownLeft - rotation_offset_rightside];
                    rotated_cube.color_arround[surface_downside][RotationPosition::UpMiddle - rotation_offset_downside] =
                        this->color_arround[surface_rightside][RotationPosition::MiddleLeft - rotation_offset_rightside];
                    rotated_cube.color_arround[surface_downside][RotationPosition::UpRight - rotation_offset_downside] =
                        this->color_arround[surface_rightside][RotationPosition::UpLeft - rotation_offset_rightside];
                    // Leftside:
                    rotated_cube.color_arround[surface_leftside][RotationPosition::UpRight - rotation_offset_leftside] =
                        this->color_arround[surface_downside][RotationPosition::UpLeft - rotation_offset_downside];
                    rotated_cube.color_arround[surface_leftside][RotationPosition::MiddleRight - rotation_offset_leftside] =
                        this->color_arround[surface_downside][RotationPosition::UpMiddle - rotation_offset_downside];
                    rotated_cube.color_arround[surface_leftside][RotationPosition::DownRight - rotation_offset_leftside] =
                        this->color_arround[surface_downside][RotationPosition::UpRight - rotation_offset_downside];
                    // Rightside:
                    rotated_cube.color_arround[surface_rightside][RotationPosition::UpLeft - rotation_offset_rightside] =
                        this->color_arround[surface_upside][RotationPosition::DownLeft - rotation_offset_upside];
                    rotated_cube.color_arround[surface_rightside][RotationPosition::MiddleLeft - rotation_offset_rightside] =
                        this->color_arround[surface_upside][RotationPosition::DownMiddle - rotation_offset_upside];
                    rotated_cube.color_arround[surface_rightside][RotationPosition::DownLeft - rotation_offset_rightside] =
                        this->color_arround[surface_upside][RotationPosition::DownRight - rotation_offset_upside];
                    break;

                case Direction::CounterClockwise:
                    for (int i = 0; i < RoundCount; i++) {
                        rotated_cube.color_arround[surface][(RotationPosition)i + (-1)] = this->color_arround[surface][i];
                    }
                    // Upside:
                    rotated_cube.color_arround[surface_upside][RotationPosition::DownLeft - rotation_offset_upside] =
                        this->color_arround[surface_rightside][RotationPosition::UpLeft - rotation_offset_rightside];
                    rotated_cube.color_arround[surface_upside][RotationPosition::DownMiddle - rotation_offset_upside] =
                        this->color_arround[surface_rightside][RotationPosition::MiddleLeft - rotation_offset_rightside];
                    rotated_cube.color_arround[surface_upside][RotationPosition::DownRight - rotation_offset_upside] =
                        this->color_arround[surface_rightside][RotationPosition::DownLeft - rotation_offset_rightside];
                    // Downside:
                    rotated_cube.color_arround[surface_downside][RotationPosition::UpLeft - rotation_offset_downside] =
                        this->color_arround[surface_leftside][RotationPosition::UpRight - rotation_offset_leftside];
                    rotated_cube.color_arround[surface_downside][RotationPosition::UpMiddle - rotation_offset_downside] =
                        this->color_arround[surface_leftside][RotationPosition::MiddleRight - rotation_offset_leftside];
                    rotated_cube.color_arround[surface_downside][RotationPosition::UpRight - rotation_offset_downside] =
                        this->color_arround[surface_leftside][RotationPosition::DownRight - rotation_offset_leftside];
                    // Leftside:
                    rotated_cube.color_arround[surface_leftside][RotationPosition::UpRight - rotation_offset_leftside] =
                        this->color_arround[surface_upside][RotationPosition::DownRight - rotation_offset_upside];
                    rotated_cube.color_arround[surface_leftside][RotationPosition::MiddleRight - rotation_offset_leftside] =
                        this->color_arround[surface_upside][RotationPosition::DownMiddle - rotation_offset_upside];
                    rotated_cube.color_arround[surface_leftside][RotationPosition::DownRight - rotation_offset_leftside] =
                        this->color_arround[surface_upside][RotationPosition::DownLeft - rotation_offset_upside];
                    // Rightside:
                    rotated_cube.color_arround[surface_rightside][RotationPosition::UpLeft - rotation_offset_rightside] =
                        this->color_arround[surface_downside][RotationPosition::UpRight - rotation_offset_downside];
                    rotated_cube.color_arround[surface_rightside][RotationPosition::MiddleLeft - rotation_offset_rightside] =
                        this->color_arround[surface_downside][RotationPosition::UpMiddle - rotation_offset_downside];
                    rotated_cube.color_arround[surface_rightside][RotationPosition::DownLeft - rotation_offset_rightside] =
                        this->color_arround[surface_downside][RotationPosition::UpLeft - rotation_offset_downside];
                    break;
            }
        } else {
            switch (action.direction) {
                case Direction::Clockwise:
                    // Upside
                    rotated_cube.color_middle[surface_upside] = this->color_middle[surface_leftside];
                    rotated_cube.color_arround[surface_upside][RotationPosition::MiddleLeft - rotation_offset_upside] =
                        this->color_arround[surface_leftside][RotationPosition::DownMiddle - rotation_offset_leftside];
                    rotated_cube.color_arround[surface_upside][RotationPosition::MiddleRight - rotation_offset_upside] =
                        this->color_arround[surface_leftside][RotationPosition::UpMiddle - rotation_offset_leftside];
                    // Downside
                    rotated_cube.color_middle[surface_downside] = this->color_middle[surface_rightside];
                    rotated_cube.color_arround[surface_downside][RotationPosition::MiddleRight - rotation_offset_downside] =
                        this->color_arround[surface_rightside][RotationPosition::UpMiddle - rotation_offset_rightside];
                    rotated_cube.color_arround[surface_downside][RotationPosition::MiddleLeft - rotation_offset_downside] =
                        this->color_arround[surface_rightside][RotationPosition::DownMiddle - rotation_offset_rightside];
                    // Leftside
                    rotated_cube.color_middle[surface_leftside] = this->color_middle[surface_downside];
                    rotated_cube.color_arround[surface_leftside][RotationPosition::DownMiddle - rotation_offset_leftside] =
                        this->color_arround[surface_downside][RotationPosition::MiddleRight - rotation_offset_downside];
                    rotated_cube.color_arround[surface_leftside][RotationPosition::UpMiddle - rotation_offset_leftside] =
                        this->color_arround[surface_downside][RotationPosition::MiddleLeft - rotation_offset_downside];
                    // Rightside
                    rotated_cube.color_middle[surface_rightside] = this->color_middle[surface_upside];
                    rotated_cube.color_arround[surface_rightside][RotationPosition::DownMiddle - rotation_offset_rightside] =
                        this->color_arround[surface_upside][RotationPosition::MiddleRight - rotation_offset_upside];
                    rotated_cube.color_arround[surface_rightside][RotationPosition::UpMiddle - rotation_offset_rightside] =
                        this->color_arround[surface_upside][RotationPosition::MiddleLeft - rotation_offset_upside];
                    break;
                case Direction::CounterClockwise:
                    // Upside
                    rotated_cube.color_middle[surface_upside] = this->color_middle[surface_rightside];
                    rotated_cube.color_arround[surface_upside][RotationPosition::MiddleLeft - rotation_offset_upside] =
                        this->color_arround[surface_rightside][RotationPosition::UpMiddle - rotation_offset_rightside];
                    rotated_cube.color_arround[surface_upside][RotationPosition::MiddleRight - rotation_offset_upside] =
                        this->color_arround[surface_rightside][RotationPosition::DownMiddle - rotation_offset_rightside];
                    // Downside
                    rotated_cube.color_middle[surface_downside] = this->color_middle[surface_leftside];
                    rotated_cube.color_arround[surface_downside][RotationPosition::MiddleLeft - rotation_offset_downside] =
                        this->color_arround[surface_leftside][RotationPosition::UpMiddle - rotation_offset_leftside];
                    rotated_cube.color_arround[surface_downside][RotationPosition::MiddleRight - rotation_offset_downside] =
                        this->color_arround[surface_leftside][RotationPosition::DownMiddle - rotation_offset_leftside];
                    // Leftside
                    rotated_cube.color_middle[surface_leftside] = this->color_middle[surface_upside];
                    rotated_cube.color_arround[surface_leftside][RotationPosition::UpMiddle - rotation_offset_leftside] =
                        this->color_arround[surface_upside][RotationPosition::MiddleRight - rotation_offset_upside];
                    rotated_cube.color_arround[surface_leftside][RotationPosition::DownMiddle - rotation_offset_leftside] =
                        this->color_arround[surface_upside][RotationPosition::MiddleLeft - rotation_offset_upside];
                    // Rightside
                    rotated_cube.color_middle[surface_rightside] = this->color_middle[surface_downside];
                    rotated_cube.color_arround[surface_rightside][RotationPosition::UpMiddle - rotation_offset_rightside] =
                        this->color_arround[surface_downside][RotationPosition::MiddleRight - rotation_offset_downside];
                    rotated_cube.color_arround[surface_rightside][RotationPosition::DownMiddle - rotation_offset_rightside] =
                        this->color_arround[surface_downside][RotationPosition::MiddleLeft - rotation_offset_downside];
                    break;
            }
        }
        return rotated_cube;
    }
};