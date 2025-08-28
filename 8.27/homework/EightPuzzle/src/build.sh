#!/bin/bash
echo "编译八数码程序..."
g++ -std=c++11 -o EightPuzzle main.cpp EightPuzzleState.cpp EightPuzzleSolver.cpp -I.

if [ $? -ne 0 ]; then
    echo "编译失败！"
    exit 1
fi

echo "编译成功！"
echo "运行八数码程序..."
./EightPuzzle 