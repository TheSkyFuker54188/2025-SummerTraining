#!/bin/bash
echo "编译魔方程序..."
g++ -std=c++11 -o Cube main.cpp CubeState.cpp CubeSolver.cpp -I.

if [ $? -ne 0 ]; then
    echo "编译失败！"
    exit 1
fi

echo "编译成功！"
echo "运行魔方程序..."
./Cube 