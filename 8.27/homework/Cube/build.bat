@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ====================================
echo 魔方求解器编译脚本
echo ====================================

set "SCRIPT_DIR=%~dp0"
set "SRC_DIR=%SCRIPT_DIR%src"
set "EXE_PATH=%SRC_DIR%\Cube.exe"

echo 源代码目录: %SRC_DIR%
echo 输出可执行文件: %EXE_PATH%
echo.

:: 检查源代码目录
if not exist "%SRC_DIR%" (
    echo 错误: 源代码目录不存在 - %SRC_DIR%
    pause
    exit /b 1
)

:: 检查必要的源文件
if not exist "%SRC_DIR%\main.cpp" (
    echo 错误: 主程序文件不存在 - %SRC_DIR%\main.cpp
    pause
    exit /b 1
)

echo 开始编译...

:: 进入源代码目录
pushd "%SRC_DIR%"

:: 尝试多个编译器，按优先级顺序
set "COMPILER_FOUND=0"
set "COMPILE_SUCCESS=0"

:: 检查是否有 clang++
"D:\msys2\clang64\bin\c++.exe" --version >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo 使用编译器: D:\msys2\clang64\bin\c++.exe
    echo 正在编译...
    "D:\msys2\clang64\bin\c++.exe" -std=c++17 -O2 -o Cube.exe main.cpp
    set "COMPILE_RESULT=!ERRORLEVEL!"
    set "COMPILER_FOUND=1"
    goto :check_result
) else (
    :: 检查是否有 g++
    g++ --version >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        echo 使用编译器: g++
        echo 正在编译...
        g++ -std=c++17 -O2 -o Cube.exe main.cpp
        set "COMPILE_RESULT=!ERRORLEVEL!"
        set "COMPILER_FOUND=1"
        goto :check_result
    ) else (
        :: 检查是否有 cl (MSVC)
        cl >nul 2>&1
        if %ERRORLEVEL% neq 9009 (
            echo 使用编译器: cl (MSVC)
            echo 正在编译...
            cl /std:c++17 /O2 /Fe:Cube.exe main.cpp
            set "COMPILE_RESULT=!ERRORLEVEL!"
            set "COMPILER_FOUND=1"
            goto :check_result
        )
    )
)

:: 如果没有找到任何编译器
if !COMPILER_FOUND! equ 0 (
    echo 错误: 未找到可用的编译器！
    echo 请确保以下编译器之一已安装并添加到 PATH：
    echo - D:\msys2\clang64\bin\c++.exe
    echo - g++
    echo - cl (MSVC)
    popd
    pause
    exit /b 1
)

:check_result
:: 检查编译结果
echo.
if !COMPILE_RESULT! equ 0 (
    if exist "Cube.exe" (
        echo ==========================================
        echo 编译成功！
        echo ==========================================
        set "COMPILE_SUCCESS=1"
    ) else (
        echo ==========================================
        echo 编译失败！
        echo ==========================================
    )
) else (
    echo ==========================================
    echo 编译失败！错误代码: !COMPILE_RESULT!
    echo ==========================================
)

if !COMPILE_SUCCESS! equ 0 (
    popd
    pause
    exit /b 1
)

popd

echo.
echo 编译完成，按任意键退出...
pause
