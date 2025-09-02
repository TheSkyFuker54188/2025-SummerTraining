# 蛇的安全性分析与分批优化方案

通过审视 `snake.cpp`的安全相关代码，发现有几处可能导致蛇进入危险区域的薄弱环节。下面是分批次优化方案，每次只针对一块功能进行改动。

## 批次1：改进特殊区域检测系统

**问题**：当前的特殊区域（关口位置）检测系统使用硬编码坐标(707-744行)，过于死板且只覆盖有限区域。在线700-745左右的代码：

```cpp
// 特殊检查：如果在关口位置，小心进入
bool flag = false;
if (sy == 9 && sx == 20)
{
    const int t = mp2[18 - fy][20];
    flag = true;
    if (t == -5 || t == -6 || t == -7)
    {
        return -1000;
    }
}
// ... 其他几个关口位置检查
```

**优化方案**：改进特殊区域检测，使用更通用的方法识别地图上的潜在危险关口

```cpp
// Strategy::bfs函数中替换现有关口检测代码
// 特殊检查：识别并评估潜在的危险关口位置
bool is_potential_bottleneck = false;
int wall_count = 0;
int open_directions = 0;

// 检查周围有多少墙或障碍物
for (auto dir : validDirections) {
    const auto [ny, nx] = Utils::nextPos({sy, sx}, dir);
    if (!Utils::boundCheck(ny, nx) || mp[ny][nx] == -4) {
        wall_count++; // 墙或边界
    } else if (mp2[ny][nx] == -5 || mp2[ny][nx] == -6 || mp2[ny][nx] == -7) {
        wall_count += 0.5; // 其他蛇体视为半个障碍(可能移动)
    } else {
        open_directions++;
    }
}

// 如果某个方向只有一个出口，而且附近有敌方蛇，视为危险关口
if (open_directions <= 1 && wall_count >= 3) {
    is_potential_bottleneck = true;
  
    // 检查唯一出口方向是否有危险
    for (auto dir : validDirections) {
        const auto [ny, nx] = Utils::nextPos({sy, sx}, dir);
        if (Utils::boundCheck(ny, nx) && mp[ny][nx] != -4 && 
            mp2[ny][nx] != -5 && mp2[ny][nx] != -6 && mp2[ny][nx] != -7) {
          
            // 检查出口方向前方是否有敌方蛇
            for (const auto &snake : state.snakes) {
                if (snake.id != MYID && snake.id != -1) {
                    const Point &enemy_head = snake.get_head();
                    int dist_to_exit = abs(enemy_head.y - ny) + abs(enemy_head.x - nx);
                  
                    if (dist_to_exit <= 3) { // 敌方蛇在出口附近
                        return -2000; // 极度危险，比原来的-1000更严格
                    }
                }
            }
        }
    }
  
    // 即使没有直接威胁，关口位置也有风险
    if (is_potential_bottleneck) {
        score -= 800; // 轻微惩罚关口位置
    }
}
```

## 批次2：改进紧急模式触发条件

**问题**：当前紧急模式(illegalMove函数中545-590行)仅在四面被困时激活，可能太迟。

**优化方案**：添加预警紧急模式，根据周围危险程度提前激活

```cpp
// illegalMove函数中修改紧急模式部分
// 添加预警紧急模式检测
bool warning_mode = false;
int legal_moves_count = 4 - illegals.size();

// 如果只剩下1-2个合法移动方向且处于危险区域，进入预警模式
if (legal_moves_count <= 2) {
    // 检查剩余合法方向是否通向潜在危险
    for (auto dir : validDirections) {
        if (illegals.count(dir) == 0) { // 如果这个方向是合法的
            const Point &head = self.get_head();
            const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
          
            if (Utils::boundCheck(y, x)) {
                // 检查这个方向前方是否是潜在死胡同
                int wall_count = 0;
                for (auto next_dir : validDirections) {
                    const auto [next_y, next_x] = Utils::nextPos({y, x}, next_dir);
                    if (!Utils::boundCheck(next_y, next_x) || 
                        mp[next_y][next_x] == -4 || 
                        mp2[next_y][next_x] == -5) {
                        wall_count++;
                    }
                }
              
                if (wall_count >= 3) { // 这个方向通向潜在死胡同
                    warning_mode = true;
                    break;
                }
            }
        }
    }
}

// 如果四种方向都不行或处于预警模式，进入紧急模式重新评估
if (illegals.size() == 4 || (warning_mode && self.shield_cd == 0)) {
    emergency_mode = true;
    // ... 原有紧急模式代码 ...
}
```

## 批次3：增强路径时间估计

**问题**：当前的 `canReachFoodInTime`函数(76-87行)对转弯限制考虑不足，可能高估能到达的食物。

**优化方案**：改进路径时间估计，更准确考虑转弯限制

```cpp
// 替换现有的canReachFoodInTime函数
bool canReachFoodInTime(int head_y, int head_x, int food_y, int food_x, int lifetime) 
{
    // 基本曼哈顿距离
    int dist = abs(head_y - food_y) + abs(head_x - food_x);
  
    // 计算需要转弯的次数 - 如果需要水平和垂直移动则至少要转一次
    int turns = 0;
    if (head_y != food_y && head_x != food_x) {
        turns = 1; // 至少需要一次转弯
    }
  
    // 考虑蛇只能在特定位置转弯的限制
    // 根据游戏规则，蛇只能在坐标为20*N+3的位置转弯
  
    // 检查是否需要垂直移动
    if (head_y != food_y) {
        int vertical_steps = abs(head_y - food_y);
      
        // 检查当前位置是否可以转弯
        bool can_turn_at_start = (head_x % 20 == 3);
      
        // 如果不能在当前位置转弯，需要先移动到能转弯的位置
        if (!can_turn_at_start && head_x != food_x) {
            // 计算到最近转弯点的距离
            int dist_to_turn = min(
                (20 - (head_x % 20)) + 3, 
                (head_x % 20 > 3) ? (head_x % 20) - 3 : 20 + 3 - (head_x % 20)
            );
            dist += dist_to_turn * 2; // 来回距离
            turns += 2; // 额外增加两次转弯
        }
    }
  
    // 估计实际所需时间
    int estimated_ticks = dist + turns * 2; // 每次转弯可能需要额外2个tick
  
    // 如果预计到达时间超过食物剩余生命周期，则忽略该食物
    return estimated_ticks < lifetime;
}
```

## 批次4：改进危险评分系统

**问题**：当前的危险评分系统过于二元化（要么是-1000，要么不惩罚），缺乏细粒度。

**优化方案**：在Strategy::bfs函数中加入更细粒度的危险评分

```cpp
// 在Strategy::bfs函数中加入以下代码段(在评估BFS搜索前)
// 增加危险区域评分

// 检查该位置是否靠近敌方蛇
for (const auto &snake : state.snakes) {
    if (snake.id != MYID && snake.id != -1) {
        const Point &enemy_head = snake.get_head();
        int dist_to_enemy = abs(sy - enemy_head.y) + abs(sx - enemy_head.x);
      
        // 根据距离给予不同程度的惩罚
        if (dist_to_enemy <= 1) {
            // 极度危险 - 直接返回极低分数
            return -3000;
        } else if (dist_to_enemy <= 2) {
            // 高度危险
            score -= 1500;
        } else if (dist_to_enemy <= 3) {
            // 中度危险
            score -= 800;
        } else if (dist_to_enemy <= 5) {
            // 轻度危险
            score -= 300;
        }
      
        // 考虑敌方蛇的移动方向
        if (dist_to_enemy <= 5) {
            // 如果敌方蛇正朝这个方向移动，增加危险评分
            int enemy_direction = snake.direction;
            bool is_moving_towards = false;
          
            switch (enemy_direction) {
                case 0: // LEFT
                    if (sx < enemy_head.x) is_moving_towards = true;
                    break;
                case 1: // UP
                    if (sy < enemy_head.y) is_moving_towards = true;
                    break;
                case 2: // RIGHT
                    if (sx > enemy_head.x) is_moving_towards = true;
                    break;
                case 3: // DOWN
                    if (sy > enemy_head.y) is_moving_towards = true;
                    break;
            }
          
            if (is_moving_towards) {
                score -= 300; // 额外惩罚敌方蛇正在靠近的区域
            }
        }
    }
}

// 检查该位置是否靠近墙，增加对潜在死胡同的识别
int wall_count = 0;
for (auto dir : validDirections) {
    const auto [next_y, next_x] = Utils::nextPos({sy, sx}, dir);
    if (!Utils::boundCheck(next_y, next_x) || mp[next_y][next_x] == -4) {
        wall_count++;
    }
}

// 根据周围墙的数量给予惩罚
if (wall_count == 3) {
    // 这是一个死胡同
    score -= 1200;
} else if (wall_count == 2) {
    // 两面墙，可能是拐角或走廊
    score -= 300;
}
```

## 批次5：优化护盾使用策略

**问题**：当前护盾使用策略(1506-1510行和1544-1548行)比较简单，没有充分利用护盾资源。

**优化方案**：改进护盾使用策略，增加预判和价值评估

```cpp
// 在judge函数中替换现有护盾使用代码
// 没有合法移动，使用护盾
if (legalMoves.empty()) {
    // 检查是否可以使用护盾
    if (state.get_self().shield_cd == 0) {
        return SHIELD_COMMAND;
    } else {
        // 护盾冷却中，选择伤害最小的方向
        Direction least_dangerous = Direction::UP; // 默认方向
        int min_danger = INT_MAX;
      
        for (auto dir : validDirections) {
            const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
          
            // 如果是越界或墙，直接跳过
            if (!Utils::boundCheck(y, x) || mp[y][x] == -4) {
                continue;
            }
          
            // 计算这个方向的危险度
            int danger_score = 0;
          
            // 检查是否会撞到蛇
            if (mp2[y][x] == -5 || mp2[y][x] == -6 || mp2[y][x] == -7) {
                danger_score += 1000;
            }
          
            // 检查是否会离开安全区
            if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max ||
                y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max) {
                danger_score += 2000;
            }
          
            // 检查是否会走入陷阱
            if (mp[y][x] == -2) {
                danger_score += 500;
            }
          
            // 更新最小危险方向
            if (danger_score < min_danger) {
                min_danger = danger_score;
                least_dangerous = dir;
            }
        }
      
        // 返回伤害最小的方向
        return Utils::dir2num(least_dangerous);
    }
}

// 修改评分极低时的护盾使用策略
if (maxF <= -3500) { // 比原来的阈值更严格
    // 检查是否有高价值目标或者蛇当前健康状况
    bool has_high_value_target = false;
  
    // 如果有钥匙或正在前往高价值食物，可能值得使用护盾冒险
    if (state.get_self().has_key || is_key_target || is_chest_target) {
        has_high_value_target = true;
    }
  
    // 检查附近是否有高价值食物
    for (const auto &item : state.items) {
        if (item.value >= 8) { // 高价值食物
            int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
            if (dist <= 5) { // 近距离高价值食物
                has_high_value_target = true;
                break;
            }
        }
    }
  
    // 根据目标价值和护盾状态决定是否使用护盾
    if (has_high_value_target && state.get_self().shield_cd == 0) {
        return SHIELD_COMMAND;
    }
}
```

这些优化方案分批次实施，每次只针对一个功能进行改动，可逐步提高蛇的安全性能。建议先实施批次1，测试效果后再考虑后续优化。
