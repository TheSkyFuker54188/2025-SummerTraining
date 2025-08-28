@echo off
chcp 65001 > nul
echo 编译魔方程序...
D:\msys2\clang64\bin\c++.exe -std=c++11 -o Cube.exe main.cpp CubeState.cpp CubeSolver.cpp -I.

if %ERRORLEVEL% neq 0 (
    echo 编译失败！
    pause
    exit /b 1
)

echo 编译成功！

rem 直接运行程序，不输出额外文字
Cube.exe
pause 