# 优化食物策略改进方案

基于对规则的深度分析，我设计了更加精细化的食物策略：

## 1. 动态食物价值评估系统

```cpp
// 在BFS函数中增加动态食物价值计算
double calculateFoodValue(const GameState &state, int y, int x, int food_value, int lifetime) {
    double value = 0;
    int current_tick = MAX_TICKS - state.remaining_ticks;
    bool is_early = state.remaining_ticks > 176;
    bool is_mid = state.remaining_ticks <= 176 && state.remaining_ticks > 56;
    bool is_late = state.remaining_ticks <= 56;
  
    // 基于游戏阶段的食物价值调整
    if (food_value > 0) { // 普通食物
        // 高分尸体(来自蛇死亡)
        if (food_value >= 10) {
            value = food_value * 60 + 50;
        } else if (food_value >= 5) {
            value = food_value * 50 + 25;
        } else {
            value = food_value * 30;
        }
      
        // 考虑食物生命周期 - 避免追逐即将消失的食物
        if (lifetime < 15) {
            value *= (0.5 + lifetime / 30.0);
        }
    }
    else if (food_value == -1) { // 增长豆
        // 早期重视增长豆，后期降低价值
        if (is_early) {
            value = 40; // 早期高价值(相当于10分×2的价值)
        } else if (is_mid) {
            value = 20;
        } else {
            value = 10; // 后期低价值
        }
      
        // 根据当前分数距离下一个增长点的远近调整增长豆价值
        int self_score = state.get_self().score;
        int next_growth = ((self_score / 20) + 1) * 20;
        int points_needed = next_growth - self_score;
      
        if (points_needed <= 5) {
            // 接近增长点时，增长豆相对价值下降(因为只需几分就能增长)
            value *= 0.8;
        }
    }
    else if (food_value == -2) { // 陷阱
        // 基本上总是避免陷阱
        value = -20;
      
        // 但在特殊情况下可以接受低风险
        if (state.get_self().shield_time > 0) {
            value = -5; // 有护盾时风险较低
        }
    }
  
    // 安全区收缩风险评估
    int ticks_to_shrink = getTicksToNextShrink(state);
    if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20) {
        // 检查位置是否在下一个安全区内
        if (!isInNextSafeZone(state, y, x)) {
            // 食物将在安全区收缩后消失，降低价值
            double shrink_factor = ticks_to_shrink / 20.0;
            value *= shrink_factor;
        }
    }
  
    return value;
}
```

## 2. 宝箱与钥匙高级策略

```cpp
// 宝箱与钥匙处理逻辑
double evaluateChestKeyStrategy(const GameState &state, int y, int x) {
    double bonus = 0;
    const auto &self = state.get_self();
    bool has_key = self.has_key;
    int current_tick = MAX_TICKS - state.remaining_ticks;
  
    // 1. 持有钥匙时的宝箱评估
    if (has_key) {
        // 计算钥匙剩余持有时间
        int key_hold_time = 0;
        for (const auto &key : state.keys) {
            if (key.holder_id == MYID) {
                key_hold_time = key.remaining_time;
                break;
            }
        }
      
        for (const auto &chest : state.chests) {
            int chest_dist = abs(y - chest.pos.y) + abs(x - chest.pos.x);
          
            // 计算是否有足够时间到达宝箱
            bool can_reach_in_time = chest_dist < key_hold_time - 5;
          
            // 如果钥匙即将掉落，大幅提升宝箱优先级
            if (key_hold_time <= 10) {
                // 紧急情况 - 钥匙即将掉落
                double urgency_factor = can_reach_in_time ? 3.0 : 0.2;
                bonus += (chest.score * 150) * urgency_factor / (chest_dist + 1);
            } else {
                // 正常情况
                double base_factor = can_reach_in_time ? 1.0 : 0.5;
                bonus += (chest.score * 100) * base_factor / (chest_dist + 1);
            }
        }
    }
    // 2. 未持有钥匙时的钥匙评估
    else if (!state.chests.empty()) {
        // 评估当前排名情况，决定是否积极争夺宝箱
        bool is_leading = isLeadingInScore(state);
      
        for (const auto &key : state.keys) {
            if (key.holder_id == -1) { // 钥匙在地图上
                int key_dist = abs(y - key.pos.y) + abs(x - key.pos.x);
              
                // 如果没有宝箱，钥匙价值大幅降低
                if (state.chests.empty()) continue;
              
                // 计算拾取钥匙后能否及时到达宝箱
                int min_chest_dist = INT_MAX;
                int max_chest_score = 0;
              
                for (const auto &chest : state.chests) {
                    int dist = abs(key.pos.y - chest.pos.y) + abs(key.pos.x - chest.pos.x);
                    min_chest_dist = min(min_chest_dist, dist);
                    max_chest_score = max(max_chest_score, chest.score);
                }
              
                // 检查是否有足够时间拾取钥匙并到达宝箱
                bool viable_strategy = (key_dist + min_chest_dist) < 25;
              
                // 根据排名情况调整钥匙价值
                double rank_factor = is_leading ? 0.9 : 1.2; // 领先时更保守
              
                // 计算价值
                if (viable_strategy) {
                    bonus += (max_chest_score * 70 * rank_factor) / (key_dist + 1);
                } else {
                    // 如果时间不够，大幅降低价值
                    bonus += (max_chest_score * 20 * rank_factor) / (key_dist + 1);
                }
            }
        }
    }
  
    return bonus;
}
```

## 3. 即时反应与食物竞争评估

```cpp
// 即时反应函数
int immediateReactionCheck(const GameState &state) {
    const auto &head = state.get_self().get_head();
    const int immediate_range = 3; // 扩大即时反应范围
  
    // 蛇死亡尸体检测 - 最高优先级
    for (const auto &item : state.items) {
        if (item.value >= 8) { // 高价值尸体
            int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
            if (dist <= immediate_range) {
                Direction move_dir = getDirectionTowards(head, item.pos);
                if (isSafeMove(state, move_dir)) {
                    return Utils::dir2num(move_dir);
                }
            }
        }
    }
  
    // 钥匙-宝箱系统优先级
    if (state.get_self().has_key) {
        // 持有钥匙 - 检查宝箱
        int key_remaining = 0;
        for (const auto &key : state.keys) {
            if (key.holder_id == MYID) {
                key_remaining = key.remaining_time;
                break;
            }
        }
      
        // 钥匙快掉了，紧急寻找宝箱
        if (key_remaining <= 10 && !state.chests.empty()) {
            int closest_dist = INT_MAX;
            Direction best_dir = Direction::UP;
            bool found = false;
          
            for (const auto &chest : state.chests) {
                int dist = abs(head.y - chest.pos.y) + abs(head.x - chest.pos.x);
                if (dist < closest_dist) {
                    closest_dist = dist;
                    best_dir = getDirectionTowards(head, chest.pos);
                    found = true;
                }
            }
          
            if (found && closest_dist <= 8 && isSafeMove(state, best_dir)) {
                return Utils::dir2num(best_dir);
            }
        }
    } 
    else if (!state.chests.empty() && !state.keys.empty()) {
        // 没有钥匙但有附近的钥匙
        int closest_dist = INT_MAX;
        Direction best_dir = Direction::UP;
        bool found = false;
      
        for (const auto &key : state.keys) {
            if (key.holder_id == -1) { // 钥匙在地图上
                int dist = abs(head.y - key.pos.y) + abs(head.x - key.pos.x);
                if (dist <= immediate_range && dist < closest_dist) {
                    closest_dist = dist;
                    best_dir = getDirectionTowards(head, key.pos);
                    found = true;
                }
            }
        }
      
        if (found && isSafeMove(state, best_dir)) {
            return Utils::dir2num(best_dir);
        }
    }
  
    // 紧急情况 - 安全区收缩
    int ticks_to_shrink = getTicksToNextShrink(state);
    if (ticks_to_shrink >= 0 && ticks_to_shrink <= 5) {
        // 如果即将收缩且不在新安全区内
        if (!isInNextSafeZone(state, head.y, head.x)) {
            // 找到最快到达新安全区的方向
            Direction safe_dir = getDirectionToNextSafeZone(state);
            if (isSafeMove(state, safe_dir)) {
                return Utils::dir2num(safe_dir);
            }
        }
    }
  
    // 普通近距离食物检测
    // 普通近距离食物检测 - 优先考虑高分、低竞争的食物
    for (const auto &item : state.items) {
        if (item.value == -2) continue; // 跳过陷阱
      
        int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
        if (dist <= immediate_range) {
            // 计算竞争因素
            int competition = countCompetitors(state, item.pos.y, item.pos.x);
          
            // 只有竞争较少时才即时反应
            if (competition <= 1 || (item.value >= 5 && competition <= 2)) {
                Direction move_dir = getDirectionTowards(head, item.pos);
                if (isSafeMove(state, move_dir)) {
                    return Utils::dir2num(move_dir);
                }
            }
        }
    }
  
    // 没有触发即时反应
    return -1;
}
```

## 4. 食物竞争与成长周期优化

```cpp
// 食物竞争评估函数
double evaluateCompetition(const GameState &state, int y, int x, int food_value) {
    const auto &self_head = state.get_self().get_head();
    int self_distance = abs(self_head.y - y) + abs(self_head.x - x);
    double competition_factor = 1.0;
  
    // 计算竞争者数量和距离
    int competitor_count = 0;
    int min_competitor_dist = INT_MAX;
  
    for (const auto &snake : state.snakes) {
        if (snake.id != MYID && snake.id != -1) {
            int dist = abs(snake.get_head().y - y) + abs(snake.get_head().x - x);
          
            if (dist < self_distance) {
                // 对方更近，几乎不可能抢到
                competitor_count++;
                min_competitor_dist = min(min_competitor_dist, dist);
              
                // 不同类型食物的竞争系数调整
                if (food_value >= 8) {
                    // 高分尸体，即使竞争也值得一争
                    competition_factor *= 0.7;
                } else if (food_value >= 5) {
                    competition_factor *= 0.5;
                } else if (food_value > 0) {
                    competition_factor *= 0.3; // 普通食物，竞争很不利时大幅降低价值
                } else if (food_value == -1) {
                    // 增长豆价值降低更多
                    competition_factor *= 0.2;
                }
            } 
            else if (dist <= self_distance + 3) {
                // 对方距离相近，有竞争
                competitor_count++;
              
                if (food_value >= 8) {
                    competition_factor *= 0.9;
                } else if (food_value >= 5) {
                    competition_factor *= 0.8;
                } else {
                    competition_factor *= 0.7;
                }
            }
        }
    }
  
    return competition_factor;
}
```

## 5. 路径重评估与决策整合

```cpp
// 主要决策函数重构
int judge(const GameState &state) {
    // 首先检查是否有即时反应情况
    int immediate_reaction = immediateReactionCheck(state);
    if (immediate_reaction != -1) {
        return immediate_reaction;
    }
  
    // 确定合法移动
    unordered_set<Direction> illegals = illegalMove(state);
    vector<Direction> legalMoves;
  
    for (auto dir : validDirections) {
        if (illegals.count(dir) == 0) {
            legalMoves.push_back(dir);
        }
    }
  
    // 没有合法移动，使用护盾
    if (legalMoves.empty()) {
        return SHIELD_COMMAND;
    }
  
    // 评估每个合法移动的分数
    double maxF = -4000;
    Direction choice = legalMoves[0];
  
    // 动态调整评估权重
    double food_weight = 1.0;
    double safety_weight = 1.0;
    double center_weight = 1.0;
  
    // 根据游戏阶段调整权重
    if (state.remaining_ticks > 180) {
        // 早期：成长为主
        food_weight = 1.2;
        safety_weight = 0.8;
        center_weight = 0.7;
    } else if (state.remaining_ticks > 80) {
        // 中期：均衡发展
        food_weight = 1.0;
        safety_weight = 1.0;
        center_weight = 0.9;
    } else {
        // 后期：安全和中心位置更重要
        food_weight = 0.9;
        safety_weight = 1.2;
        center_weight = 1.3;
    }
  
    for (auto dir : legalMoves) {
        const auto &head = state.get_self().get_head();
        const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
      
        // 综合评估
        double food_score = evaluateFoodPathValue(state, y, x);
        double chest_key_score = evaluateChestKeyStrategy(state, y, x);
        double safety_score = evaluateSafety(state, y, x);
        double center_score = evaluateCenterValue(state, y, x);
      
        double total_score = (food_score + chest_key_score) * food_weight + 
                            safety_score * safety_weight + 
                            center_score * center_weight;
      
        // 更新最高分数的方向
        if (total_score > maxF) {
            maxF = total_score;
            choice = dir;
        }
    }
  
    // 极低分数时考虑使用护盾
    if (maxF == -4000) {
        return SHIELD_COMMAND;
    }
  
    return Utils::dir2num(choice);
}
```

这个优化后的策略将考虑游戏规则中的各种细节，包括阶段性食物分布变化、食物生命周期、成长机制、宝箱与钥匙系统、安全区收缩等，为蛇的行为提供更智能、更具适应性的决策。
