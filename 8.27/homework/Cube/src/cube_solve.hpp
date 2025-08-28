/*
 *  Description:    实现魔方求解器类
 */

#include <iostream>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <string>
#include <queue>
#include "cube.hpp"
#include "task_system.hpp"

/// @brief 魔方任务，会被加入任务队列
typedef struct CubeTask {
    Cube cube;
    std::vector<CubeAction> actions;

    CubeTask(Cube&& cube, std::vector<CubeAction>&& actions)
        : cube(cube), actions(actions) {}
}* PCubeTask;

/// @brief 魔方求解器，用于处理魔方任务
class CubeHandler : public IHandler<PCubeTask> {
   private:
    // 常量成员
    const int max_depth;
    int actions_count = sizeof(StandardActions) / sizeof(StandardActions[0]);
    CubeAction actions[sizeof(StandardActions) / sizeof(StandardActions[0])];
    bool enable_debug_log;
    bool stop_on_find_one;

    // 变量成员
    std::mutex success_mutex; // 防止多个成功解同时写入
    bool is_solved = false;
    std::string action_description;

    // 状态缓存，避免重复状态
    std::unordered_map<std::string, bool> visited_states;

   public:
    CubeHandler(int max_depth, bool enable_debug_log, bool stop_on_find_one)
        : max_depth(max_depth), enable_debug_log(enable_debug_log), stop_on_find_one(stop_on_find_one) {
        for (int i = 0; i < actions_count; i++) {
            actions[i] = ConvertCubeActionFormat(StandardActions[i]);
        }
    }

    void Handle(PCubeTask& task, IEnqueuer<PCubeTask>& enqueuer) override {
        // 首先检查任务的初始状态是否已经是解决状态
        if (task->actions.size() == 0) {
            if (task->cube.HasBeenSolved()) {
                std::lock_guard<std::mutex> lock(success_mutex);
                action_description = "已经是解决状态，无需操作";
                is_solved = true;
                delete task;
                return;
            }
        }
        
        // 检查是否已经解决
        if (task->cube.HasBeenSolved()) {
            std::lock_guard<std::mutex> lock(success_mutex);
            // 只记录最短的解决方案
            if (!is_solved || task->actions.size() < action_description.size()) {
                is_solved = true;
                action_description = "";
                for (auto action : task->actions) {
                    CubeActionStandard standard_action = ConvertCubeActionFormat(action);
                    action_description += std::to_string(standard_action.standard_surface_index);
                    action_description += standard_action.is_positive_direction ? "+" : "-";
                    action_description += " ";
                }
                if (action_description.size() > 0) {
                    action_description.pop_back();
                }
            }
        } else {
            // 未解决且未超过最大深度，继续搜索
            if ((!is_solved || !stop_on_find_one) && task->actions.size() < (unsigned int)max_depth) {
                std::string state_key = task->cube.ToString();
                
                // 检查状态是否已访问过
                if (visited_states.find(state_key) == visited_states.end()) {
                    visited_states[state_key] = true;
                    
                    // 尝试所有可能的动作
                    for (int i = 0; i < actions_count; i++) {
                        CubeAction action = actions[i];
                        Cube rotated_cube = task->cube.Rotate(action);
                        std::vector<CubeAction> task_actions(task->actions);
                        task_actions.push_back(action);
                        
                        if (enable_debug_log) {
                            printf("----------------------------\n");
                            printf("深度: %zu\n", task_actions.size());
                            printf("动作序列: ");
                            for (auto task_action : task_actions) {
                                CubeActionStandard standard_action = ConvertCubeActionFormat(task_action);
                                printf("%d%s ", standard_action.standard_surface_index, standard_action.is_positive_direction ? "+" : "-");
                            }
                            printf("\n");
                            std::cout << rotated_cube.ToString() << std::endl;
                        }
                        
                        enqueuer.Enqueue(new CubeTask(std::move(rotated_cube), std::move(task_actions)));
                    }
                }
            }
        }
        delete task;
    }

    bool IsSolved() {
        return is_solved;
    }

    std::string GetActionDescription() {
        return action_description;
    }
};
