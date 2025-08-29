@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: 设置控制台字体和编码以更好地显示Unicode字符
for /f "tokens=2 delims=:" %%a in ('chcp') do set "original_codepage=%%a"
chcp 65001 >nul

echo ====================================
echo 魔方求解器自动测试脚本
echo =====================================

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

echo 搜索深度: %SEARCH_DEPTH%
echo 测试目录: %TEST_DIR%
echo 结果将保存到: %RESULT_DIR%
echo 开始批量测试...
echo.

:: 初始化统计变量
set "total_tests=0"
set "successful_tests=0"
set "failed_tests=0"

::遍历所有测试文件
for %%f in ("%TEST_DIR%\*.txt") do (
    set /a total_tests+=1
    
    echo ==========================================
    echo 正在测试: %%~nxf
    echo ==========================================
    
    :: 生成临时文件和结果文件名
    set "temp_file=%RESULT_DIR%\%%~nf.temp.txt"
    set "result_file=%RESULT_DIR%\%%~nf.result.txt"
    
    :: 运行测试并捕获输出到临时文件
    "%EXE_PATH%" %SEARCH_DEPTH% < "%%f" > "!temp_file!" 2>&1
    set "exit_code=!ERRORLEVEL!"
    
    :: 显示完整输出到控制台
    type "!temp_file!"
    
    :: 提取求解步骤到结果文件
    echo Extracting solution steps...
    > "!result_file!" (
        for /f "usebackq delims=" %%i in ("!temp_file!") do (
            set "line=%%i"
            if "!line:解决步骤=!" neq "!line!" (
                :: 提取冒号后面的部分（求解步骤）
                for /f "tokens=2 delims=:" %%j in ("!line!") do (
                    set "steps=%%j"
                    :: 去掉前面的空格
                    set "steps=!steps: =!"
                    echo !steps!
                )
            )
        )
    )
    
    :: 删除临时文件
    del "!temp_file!"
    
    :: 检查是否提取到求解步骤
    set "solution_found=0"
    for /f %%s in ("!result_file!") do set "solution_found=1"
    
    :: 记录测试结果
    if !exit_code! equ 0 (
        if !solution_found! equ 1 (
            set /a successful_tests+=1
            echo [Success] %%~nxf - Solution saved to: %%~nf.result.txt
        ) else (
            echo 未找到解决方案 > "!result_file!"
            set /a successful_tests+=1
            echo [Success] %%~nxf - No solution found, recorded to: %%~nf.result.txt
        )
    ) else (
        echo 程序执行失败 > "!result_file!"
        set /a failed_tests+=1
        echo [Failed] %%~nxf ^(Exit code: !exit_code!^) - Error recorded to: %%~nf.result.txt
    )
    
    :: 显示提取的结果
    echo Extracted result:
    type "!result_file!"
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

echo ==========================================
echo 所有测试完成！
echo 结果文件保存在: %RESULT_DIR%
echo ==========================================
pause
