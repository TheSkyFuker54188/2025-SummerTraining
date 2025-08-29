@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ====================================
echo 魔方求解器自动测试脚本
echo ====================================

set "SCRIPT_DIR=%~dp0"
set "TEST_DIR=%SCRIPT_DIR%test"
set "RESULT_DIR=%SCRIPT_DIR%result"
set "EXE_PATH=%SCRIPT_DIR%src\Cube.exe"
set "SEARCH_DEPTH=6"

:: 检查必要文件和目录
if not exist "%TEST_DIR%" (
    echo 错误: 测试目录不存在 - %TEST_DIR%
    pause
    exit /b 1
)

if not exist "%EXE_PATH%" (
    echo 错误: 可执行文件不存在 - %EXE_PATH%
    echo 请先运行 build.bat 编译程序
    pause
    exit /b 1
)

:: 创建结果目录（如果不存在）
if not exist "%RESULT_DIR%" (
    echo 创建结果目录: %RESULT_DIR%
    mkdir "%RESULT_DIR%"
)

:: 生成时间戳用于结果文件名
for /f "tokens=1-6 delims=:/. " %%a in ("%date% %time%") do (
    set "timestamp=%%c%%a%%b_%%d%%e%%f"
)
set "timestamp=%timestamp: =0%"

set "SUMMARY_FILE=%RESULT_DIR%\test_summary_%timestamp%.txt"
set "DETAIL_FILE=%RESULT_DIR%\test_details_%timestamp%.txt"

echo 搜索深度: %SEARCH_DEPTH%
echo 测试目录: %TEST_DIR%
echo 结果将保存到: %RESULT_DIR%
echo 开始批量测试...
echo.

:: 初始化统计变量
set "total_tests=0"
set "successful_tests=0"
set "failed_tests=0"

:: 创建摘要文件头部
echo 魔方求解器测试报告 > "%SUMMARY_FILE%"
echo 测试时间: %date% %time% >> "%SUMMARY_FILE%"
echo 搜索深度: %SEARCH_DEPTH% >> "%SUMMARY_FILE%"
echo ================================== >> "%SUMMARY_FILE%"
echo. >> "%SUMMARY_FILE%"

:: 创建详细结果文件头部
echo 魔方求解器详细测试结果 > "%DETAIL_FILE%"
echo 测试时间: %date% %time% >> "%DETAIL_FILE%"
echo 搜索深度: %SEARCH_DEPTH% >> "%DETAIL_FILE%"
echo ================================== >> "%DETAIL_FILE%"
echo. >> "%DETAIL_FILE%"

:: 遍历所有测试文件
for %%f in ("%TEST_DIR%\*.txt") do (
    set /a total_tests+=1
    
    echo ==========================================
    echo 正在测试: %%~nxf
    echo ==========================================
    
    :: 记录到详细结果文件
    echo ========================================== >> "%DETAIL_FILE%"
    echo 测试文件: %%~nxf >> "%DETAIL_FILE%"
    echo 开始时间: %time% >> "%DETAIL_FILE%"
    echo ========================================== >> "%DETAIL_FILE%"
    
    :: 运行测试并捕获输出
    set "test_output_file=%RESULT_DIR%\%%~nf_output_%timestamp%.txt"
    
    "%EXE_PATH%" %SEARCH_DEPTH% < "%%f" > "!test_output_file!" 2>&1
    set "exit_code=!ERRORLEVEL!"
    
    :: 显示输出到控制台
    type "!test_output_file!"
    
    :: 将输出追加到详细结果文件
    type "!test_output_file!" >> "%DETAIL_FILE%"
    
    :: 记录测试结果
    if !exit_code! equ 0 (
        set /a successful_tests+=1
        echo 结果: 成功 >> "%SUMMARY_FILE%"
        echo 结果: 成功 >> "%DETAIL_FILE%"
        echo [成功] %%~nxf
    ) else (
        set /a failed_tests+=1
        echo 结果: 失败 ^(退出代码: !exit_code!^) >> "%SUMMARY_FILE%"
        echo 结果: 失败 ^(退出代码: !exit_code!^) >> "%DETAIL_FILE%"
        echo [失败] %%~nxf ^(退出代码: !exit_code!^)
    )
    
    echo 测试文件: %%~nxf >> "%SUMMARY_FILE%"
    echo 输出文件: %%~nf_output_%timestamp%.txt >> "%SUMMARY_FILE%"
    echo 结束时间: %time% >> "%DETAIL_FILE%"
    echo. >> "%SUMMARY_FILE%"
    echo. >> "%DETAIL_FILE%"
    echo.
)

:: 生成测试统计
echo ==========================================
echo 测试统计
echo ==========================================
echo 总测试数: %total_tests%
echo 成功: %successful_tests%
echo 失败: %failed_tests%
echo.

:: 将统计信息写入摘要文件
echo ================================== >> "%SUMMARY_FILE%"
echo 测试统计: >> "%SUMMARY_FILE%"
echo 总测试数: %total_tests% >> "%SUMMARY_FILE%"
echo 成功: %successful_tests% >> "%SUMMARY_FILE%"
echo 失败: %failed_tests% >> "%SUMMARY_FILE%"
echo 测试完成时间: %date% %time% >> "%SUMMARY_FILE%"

echo ==========================================
echo 所有测试完成！
echo 测试摘要已保存到: %SUMMARY_FILE%
echo 详细结果已保存到: %DETAIL_FILE%
echo 各测试的输出文件保存在: %RESULT_DIR%
echo ==========================================
pause
