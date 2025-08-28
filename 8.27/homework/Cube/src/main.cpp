/*
 *  Description:    The main function of the program.
 *
 *  Author(s):      Nictheboy Li <nictheboy@outlook.com>
 *
 *  Last Updated:   2024-06-25
 *
 */

#include "cube_solve.hpp"
#include <windows.h> // 添加Windows头文件，用于设置控制台编码

std::string GetAllStdin() {
    std::string all_stdin;
    std::string line;
    while (std::getline(std::cin, line)) {
        all_stdin += line;
        all_stdin += "\n";
    }
    return all_stdin;
}

int main(int argc, char* argv[]) {
    // 设置控制台输出为UTF-8编码
    SetConsoleOutputCP(65001);
    
    if (argc < 2 || std::stoi(argv[1]) < 1) {
        std::cout << "Usage: " << argv[0] << " <max_depth>" << std::endl;
        std::cout << "Arguments:" << std::endl;
        std::cout << "  max_depth:      The maximum depth of the search tree." << std::endl;
        std::cout << "Input:" << std::endl;
        std::cout << "  Initial state should be put into stdin." << std::endl;
        std::cout << "  You need to use EOF to end your input." << std::endl;
        std::cout << "Output:" << std::endl;
        std::cout << "  All solutions within max_depth steps." << std::endl;
        return 1;
    }
    const int max_depth = std::stoi(argv[1]);

    // 使用单线程任务系统
    IHandleQueueSystem<PCubeTask>* pSystem = new HandleQueueSystemSingleThread<PCubeTask>();
    
    // 创建一个求解器，设置stop_on_find_one为true，只输出第一个解
    CubeHandler* pHandler = new CubeHandler(max_depth, false, true);

    // 创建初始状态
    PCubeTask pTask = new CubeTask(Cube(GetAllStdin()), std::vector<CubeAction>());

    std::cout << "开始求解..." << std::endl;
    std::chrono::time_point start = std::chrono::system_clock::now();
    pSystem->Solve(pTask, *pHandler);
    std::chrono::time_point end = std::chrono::system_clock::now();
    std::cout << "耗时: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "毫秒" << std::endl;

    if (pHandler->IsSolved()) {
        std::cout << "在 " << max_depth << " 步内的解决方案:" << std::endl;
        std::cout << pHandler->GetActionDescription() << std::endl;
    } else {
        std::cout << "在 " << max_depth << " 步内没有找到解决方案" << std::endl;
    }
    
    delete pHandler;
    delete pSystem;
    return 0;
}
