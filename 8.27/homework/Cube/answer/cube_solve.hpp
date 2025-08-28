/*
 *  Description:    Implements CubeHandler class, which is used to solve the cube.
 *                  The CubeHandler class implements IHandler<PCubeTask>, which
 *                  means it is a task handler.
 *
 *  Author(s):      Nictheboy Li <nictheboy@outlook.com>
 *
 *  Last Updated:   2024-06-25
 *
 */

#include <iostream>
#include <mutex>
#include <vector>
#include "cube.hpp"
#include "task_system.hpp"

/// @brief A task that will be queued in the task system.
typedef struct CubeTask {
    Cube cube;
    std::vector<CubeAction> actions;

    CubeTask(Cube&& cube, std::vector<CubeAction>&& actions)
        : cube(cube), actions(actions) {}
}* PCubeTask;

/// @brief A handler that will be used to solve the cube.
class CubeHandler : public IHandler<PCubeTask> {
   private:
    // Constant Members
    const int max_depth;
    int actions_count = sizeof(StandardActions) / sizeof(StandardActions[0]);
    CubeAction actions[sizeof(StandardActions) / sizeof(StandardActions[0])];
    bool enable_debug_log;
    bool stop_on_find_one;

    // Variable Members
    std::mutex success_mutex; // prevent two success happening at the same time
    bool is_solved = false;
    std::string action_description;

   public:
    CubeHandler(int max_depth, bool enable_debug_log, bool stop_on_find_one)
        : max_depth(max_depth), enable_debug_log(enable_debug_log), stop_on_find_one(stop_on_find_one) {
        for (int i = 0; i < actions_count; i++) {
            actions[i] = ConvertCubeActionFormat(StandardActions[i]);
        }
    }

    void Handle(PCubeTask& task, IEnqueuer<PCubeTask>& enqueuer) override {
        if (task->cube.HasBeenSolved()) {
            std::lock_guard<std::mutex> lock(success_mutex);
            if (!action_description.empty())
                action_description += "\n";
            for (auto action : task->actions) {
                CubeActionStandard standard_action = ConvertCubeActionFormat(action);
                action_description += "\'";
                action_description += std::to_string(standard_action.standard_surface_index);
                action_description += standard_action.is_positive_direction ? "+" : "-";
                action_description += "\', ";
            }
            if (action_description.size() > 0) {
                action_description.pop_back();
                action_description.pop_back();
            }
            is_solved = true;
        } else {
            if ((!is_solved || !stop_on_find_one) && task->actions.size() < (unsigned int)max_depth) {
                // Queue all possible actions
                for (int i = 0; i < actions_count; i++) {
                    CubeAction action = actions[i];
                    Cube rotated_cube = task->cube.Rotate(action);
                    std::vector<CubeAction> task_actions(task->actions);
                    task_actions.push_back(action);
                    if (enable_debug_log) {
                        printf("----------------------------\n");
                        printf("Depth: %zu\n", task_actions.size());
                        printf("Actions: ");
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
        delete task;
    }

    bool IsSolved() {
        return is_solved;
    }

    std::string GetActionDescription() {
        return action_description;
    }
};
