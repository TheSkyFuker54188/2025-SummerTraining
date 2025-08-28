@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ====================================
echo 魔方求解器批量测试
echo ====================================

set "SCRIPT_DIR=%~dp0"
set "TEST_DIR=%SCRIPT_DIR%test"
set "EXE_PATH=%SCRIPT_DIR%src\Cube.exe"
set "SEARCH_DEPTH=6"

:: 检查必要文件
if not exist "%TEST_DIR%" (
    echo 错误: 测试目录不存在 - %TEST_DIR%
    pause
    exit /b 1
)

if not exist "%EXE_PATH%" (
    echo 错误: 可执行文件不存在 - %EXE_PATH%
    pause
    exit /b 1
)

echo 搜索深度: %SEARCH_DEPTH%
echo 开始批量测试...
echo.

:: 遍历所有测试文件
for %%f in ("%TEST_DIR%\*.txt") do (
    echo ==========================================
    echo 测试文件: %%~nxf
    echo ==========================================
    
    :: 直接运行并显示输出
    "%EXE_PATH%" %SEARCH_DEPTH% < "%%f"
    
    echo.
)

echo ==========================================
echo 所有测试完成
echo ==========================================
pause
