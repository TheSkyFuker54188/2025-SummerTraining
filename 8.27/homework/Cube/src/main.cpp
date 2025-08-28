/**
 * 魔方求解程序
 * 从标准输入读取魔方状态，输出解法步骤
 */

#include <windows.h>
#include <iostream>
#include <chrono>
#include <string>
#include "solver.hpp"

/**
 * 从标准输入读取文本直到EOF
 */
std::string 读取输入() {
    std::string 文本内容;
    std::string 当前行;
    
    while (std::getline(std::cin, 当前行)) {
        文本内容 += 当前行 + "\n";
    }
    
    return 文本内容;
}

int main(int argc, char* argv[]) {
    // 设置控制台编码为UTF-8
    SetConsoleOutputCP(65001);
    
    // 检查参数
    if (argc < 2 || std::stoi(argv[1]) < 1) {
        std::cout << "用法: " << argv[0] << " <搜索深度>" << std::endl;
        std::cout << "参数说明:" << std::endl;
        std::cout << "  搜索深度: BFS算法的最大搜索深度" << std::endl;
        std::cout << "输入格式:" << std::endl;
        std::cout << "  从标准输入读取魔方状态描述" << std::endl;
        std::cout << "  使用EOF结束输入" << std::endl;
        return 1;
    }
    
    // 解析搜索深度参数
    const int 搜索深度 = std::stoi(argv[1]);
    
    // 创建任务系统
    TaskSystem<PCubeTask>* 任务系统 = new SingleThreadTaskSystem<PCubeTask>();
    
    // 创建求解器
    CubeSolver* 求解器 = new CubeSolver(搜索深度, false, true);
    
    // 读取魔方初始状态
    Cube 初始状态(读取输入());
    
    // 创建初始任务
    PCubeTask 初始任务 = new CubeTask(
        std::move(初始状态), 
        std::vector<MoveAction>()
    );
    
    // 开始求解，并计时
    std::cout << "开始求解..." << std::endl;
    auto 开始时间 = std::chrono::high_resolution_clock::now();
    
    任务系统->Execute(初始任务, *求解器);
    
    auto 结束时间 = std::chrono::high_resolution_clock::now();
    auto 耗时 = std::chrono::duration_cast<std::chrono::milliseconds>(结束时间 - 开始时间).count();
    std::cout << "计算耗时: " << 耗时 << " 毫秒" << std::endl;
    
    // 输出结果
    if (求解器->HasSolution()) {
        std::cout << "在 " << 搜索深度 << " 步内的解决方案:" << std::endl;
        std::cout << 求解器->GetSolution() << std::endl;
    } else {
        std::cout << "在 " << 搜索深度 << " 步内未找到解决方案" << std::endl;
    }
    
    // 释放资源
    delete 求解器;
    delete 任务系统;
    
    return 0;
}
