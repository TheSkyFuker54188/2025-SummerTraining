@echo off
:: 设置UTF-8编码，确保中文显示正常
chcp 65001 > nul
echo 设置UTF-8编码完成
echo 编译八数码程序...

:: 编译程序
D:\msys2\clang64\bin\c++.exe -std=c++11 -o EightPuzzle.exe main.cpp EightPuzzleState.cpp EightPuzzleSolver.cpp -I.

if %ERRORLEVEL% neq 0 (
    echo 编译失败！
    pause
    exit /b 1
)

echo 编译成功！
echo 运行八数码程序...
echo ====================================

:: 再次确保UTF-8编码，然后运行程序
chcp 65001 > nul
EightPuzzle.exe
pause 