/*
 *  Description:    The main function of the program.
 *
 *  Author(s):      Nictheboy Li <nictheboy@outlook.com>
 *
 *  Last Updated:   2024-06-25
 *
 */

#include "cube_solve.hpp"

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
    if (argc < 3 ||
        (std::string(argv[1]) != "single-thread" && std::string(argv[1]) != "multi-thread") ||
        std::stoi(argv[2]) < 1) {
        std::cout << "Usage: " << argv[0] << " single-thread <max_depth>" << std::endl;
        std::cout << "       " << argv[0] << " multi-thread <max_depth>" << std::endl;
        std::cout << "Arguments:" << std::endl;
        std::cout << "  single-thread:  Use single thread to solve the cube." << std::endl;
        std::cout << "  multi-thread:   Use multi thread to solve the cube." << std::endl;
        std::cout << "  max_depth:      The maximum depth of the search tree." << std::endl;
        std::cout << "Input:" << std::endl;
        std::cout << "  Initial state should be put into stdin." << std::endl;
        std::cout << "  You need to use EOF to end your input." << std::endl;
        std::cout << "  The format can be seen from the test folder." << std::endl;
        std::cout << "Output:" << std::endl;
        std::cout << "  All solutions within max_depth steps." << std::endl;
        std::cout << "Author:" << std::endl;
        std::cout << "  Nictheboy Li <nictheboy@outlook.com> from Renmin University of China, Jun. 25 2024" << std::endl;
        return 1;
    }
    const int max_depth = std::stoi(argv[2]);
    bool multi_thread = std::string(argv[1]) == "multi-thread";

    // A task system that will be used as a BFS algorithm.
    IHandleQueueSystem<PCubeTask>* pSystem =
        multi_thread
            ? (IHandleQueueSystem<PCubeTask>*)new HandleQueueSystemMultiThread<PCubeTask>()
            : (IHandleQueueSystem<PCubeTask>*)new HandleQueueSystemSingleThread<PCubeTask>();
    
    // A handler that will be used to handle the tasks.
    CubeHandler* pHandler = new CubeHandler(max_depth, false, false);

    // The initial state of the cube.
    PCubeTask pTask = new CubeTask(Cube(GetAllStdin()), std::vector<CubeAction>());

    std::cout << "Start solving..." << std::endl;
    std::chrono::time_point start = std::chrono::system_clock::now();
    pSystem->Solve(pTask, *pHandler);
    std::chrono::time_point end = std::chrono::system_clock::now();
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

    if (pHandler->IsSolved()) {
        std::cout << "All solutions within " << max_depth << " steps:" << std::endl;
        std::cout << pHandler->GetActionDescription() << std::endl;
    } else
        std::cout << "No solution within " << max_depth << " steps" << std::endl;
    delete pHandler;
    delete pSystem;
    return 0;
}
