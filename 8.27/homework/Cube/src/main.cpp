/**
 * 魔方求解主程序
 * 读取魔方初始状态，使用BFS算法求解
 */

#include <windows.h>
#include "solver.hpp"

/**
 * 从标准输入读取所有文本
 */
std::string ReadAllInput() {
    std::string content;
    std::string line;
    
    while (std::getline(std::cin, line)) {
        content += line + "\n";
    }
    
    return content;
}

int main(int argc, char* argv[]) {
    // 设置控制台输出编码为UTF-8
    SetConsoleOutputCP(65001);
    
    // 检查命令行参数
    if (argc < 2 || std::stoi(argv[1]) < 1) {
        std::cout << "用法: " << argv[0] << " <搜索深度>" << std::endl;
        std::cout << "参数说明:" << std::endl;
        std::cout << "  搜索深度: 指定BFS搜索的最大深度" << std::endl;
        std::cout << "输入格式:" << std::endl;
        std::cout << "  请将魔方的初始状态从标准输入传入" << std::endl;
        std::cout << "  使用EOF结束输入" << std::endl;
        return 1;
    }
    
    // 解析最大搜索深度
    const int maxDepth = std::stoi(argv[1]);
    
    // 创建任务系统(单线程)
    TaskSystem<PMagicCubeTask>* taskSystem = new SingleThreadTaskSystem<PMagicCubeTask>();
    
    // 创建魔方求解器
    MagicCubeSolver* solver = new MagicCubeSolver(maxDepth, false, true);
    
    // 从标准输入读取魔方初始状态
    PMagicCubeTask initialTask = new MagicCubeTask(
        Cube(ReadAllInput()), 
        std::vector<CubeAction>()
    );
    
    // 开始计时并求解
    std::cout << "开始求解..." << std::endl;
    auto startTime = std::chrono::system_clock::now();
    
    taskSystem->Execute(initialTask, *solver);
    
    auto endTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "耗时: " << duration.count() << "毫秒" << std::endl;
    
    // 输出结果
    if (solver->HasSolution()) {
        std::cout << "在 " << maxDepth << " 步内的解决方案:" << std::endl;
        std::cout << solver->GetSolutionSteps() << std::endl;
    } else {
        std::cout << "在 " << maxDepth << " 步内没有找到解决方案" << std::endl;
    }
    
    // 释放资源
    delete solver;
    delete taskSystem;
    
    return 0;
}
