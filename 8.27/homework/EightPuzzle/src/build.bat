@echo off
chcp 65001 > nul
echo 编译八数码程序...
D:\msys2\clang64\bin\c++.exe -std=c++11 -o EightPuzzle.exe main.cpp EightPuzzleState.cpp EightPuzzleSolver.cpp -I.

if %ERRORLEVEL% neq 0 (
    echo 编译失败！
    pause
    exit /b 1
)

echo 编译成功！
echo 运行八数码程序...
EightPuzzle.exe
pause 