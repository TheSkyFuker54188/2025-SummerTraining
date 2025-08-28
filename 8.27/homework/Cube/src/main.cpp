/****************************************************
 * 魔方求解程序
 * 
 * 从标准输入读取魔方状态描述
 * 使用BFS算法寻找最短解法步骤
 * 
 ****************************************************/

// 系统头文件
#include <windows.h>
#include <iostream>
#include <chrono>
#include <string>
#include <exception>
#include <iomanip>  // 新增
#include <memory>   // 新增

// 项目头文件
#include "solver.hpp"

// 使用cube命名空间
using namespace cube;

/**
 * 自定义异常类 - 输入错误
 */
class InputException : public std::runtime_error {
public:
    explicit InputException(const std::string& msg) : std::runtime_error(msg) {}
};

/**
 * 命令行参数结构体
 */
struct ProgramOptions {
    int searchDepth = 0;     // 搜索深度
    bool debugMode = false;  // 调试模式
    bool showHelp = false;   // 显示帮助
    
    // 解析命令行参数
    static ProgramOptions parseArgs(int argc, char* argv[]) {
        ProgramOptions opts;
        
        // 检查参数数量
        if (argc < 2) {
            opts.showHelp = true;
            return opts;
        }
        
        // 解析深度参数
        try {
            opts.searchDepth = std::stoi(argv[1]);
            
            if (opts.searchDepth < 1) {
                throw InputException("搜索深度必须为正整数");
            }
        }
        catch (const std::exception&) {
            throw InputException("无效的搜索深度参数");
        }
        
        // 解析额外选项
        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--debug" || arg == "-d") {
                opts.debugMode = true;
            }
            else if (arg == "--help" || arg == "-h") {
                opts.showHelp = true;
            }
        }
        
        return opts;
    }
};

/**
 * 显示帮助信息
 */
void 显示帮助(const char* programName) {
    std::cout << "魔方求解程序 - 寻找最短解法路径\n\n"
              << "用法: " << programName << " <搜索深度> [选项]\n\n"
              << "参数:\n"
              << "  搜索深度  指定BFS搜索的最大深度\n\n"
              << "选项:\n"
              << "  --debug, -d   启用调试模式\n"
              << "  --help, -h    显示此帮助信息\n\n"
              << "输入格式:\n"
              << "  从标准输入读取魔方状态描述\n"
              << "  使用EOF结束输入\n\n"
              << "示例:\n"
              << "  " << programName << " 5 < input.txt\n\n";
}

/**
 * 从标准输入读取魔方状态描述
 */
std::string 读取魔方状态() {
    std::string 完整输入;
    std::string 当前行;
    
    while (std::getline(std::cin, 当前行)) {
        完整输入 += 当前行 + "\n";
    }
    
    // 验证输入非空
    if (完整输入.empty()) {
        throw InputException("标准输入为空，请提供魔方状态描述");
    }
    
    return 完整输入;
}

/**
 * 主函数
 */
int main(int argc, char* argv[]) {
    try {
        // 设置控制台编码为UTF-8
        SetConsoleOutputCP(65001);
        
        // 解析命令行参数
        ProgramOptions 选项 = ProgramOptions::parseArgs(argc, argv);
        
        // 显示帮助信息并退出
        if (选项.showHelp) {
            显示帮助(argv[0]);
            return 0;
        }
        
        // 读取魔方初始状态
        std::string 魔方描述;
        try {
            魔方描述 = 读取魔方状态();
        }
        catch (const InputException& e) {
            std::cerr << "输入错误: " << e.what() << std::endl;
            return 1;
        }
        
        std::cout << "┌─────────────────────────────┐" << std::endl;
        std::cout << "│      魔方求解程序启动       │" << std::endl;
        std::cout << "└─────────────────────────────┘" << std::endl;
        
        // 创建求解器和任务系统
        std::unique_ptr<TaskSystem<PCubeTask>> 任务系统(new SingleThreadTaskSystem<PCubeTask>());
        std::unique_ptr<CubeSolver> 求解器(new CubeSolver(选项.searchDepth, 选项.debugMode, true));
        
        // 创建魔方初始状态
        Cube 初始魔方(魔方描述);
        
        // 创建初始任务
        PCubeTask 初始任务 = new CubeTaskData(
            std::move(初始魔方), 
            std::vector<MoveAction>()
        );
        
        // 开始求解并计时
        std::cout << "【开始求解】最大深度: " << 选项.searchDepth << std::endl;
        auto 开始时间 = std::chrono::high_resolution_clock::now();
        
        任务系统->Execute(初始任务, *求解器);
        
        auto 结束时间 = std::chrono::high_resolution_clock::now();
        auto 耗时 = std::chrono::duration_cast<std::chrono::milliseconds>(结束时间 - 开始时间).count();
        
        // 输出求解结果
        std::cout << "【求解完成】耗时: " << 耗时 << " 毫秒" << std::endl;
        std::cout << "┌─────────────────────────────┐" << std::endl;
        
        if (求解器->HasSolution()) {
            std::cout << "│      找到解决方案!          │" << std::endl;
            std::cout << "└─────────────────────────────┘" << std::endl;
            std::cout << "解决步骤: " << 求解器->GetSolution() << std::endl;
        } else {
            std::cout << "│      未找到解决方案         │" << std::endl;
            std::cout << "└─────────────────────────────┘" << std::endl;
            std::cout << "在指定深度 " << 选项.searchDepth << " 步内无解" << std::endl;
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "程序出错: " << e.what() << std::endl;
        return 1;
    }
}
