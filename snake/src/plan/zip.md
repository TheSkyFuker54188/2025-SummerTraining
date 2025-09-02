# 代码压缩方案分析

通过详细分析snake.cpp，我发现有几处可以在不改变功能的前提下压缩代码行数的区域。以下是分批次的压缩方案：

## 第一批：压缩judge函数中宝箱和钥匙的处理逻辑

在1382-1411行，处理宝箱和钥匙目标的代码几乎完全相同，只是条件和目标类型不同。这里可以合并为一个辅助函数：

```cpp
// 添加到judge函数之前（约1372行）
Direction moveToTarget(const GameState &state, const Point &target) {
    const auto &head = state.get_self().get_head();
  
    // 确定移动方向
    Direction move_dir;
    if (head.x > target.x) move_dir = Direction::LEFT;
    else if (head.x < target.x) move_dir = Direction::RIGHT;
    else if (head.y > target.y) move_dir = Direction::UP;
    else move_dir = Direction::DOWN;
  
    // 检查移动安全性
    unordered_set<Direction> illegals = illegalMove(state);
    return (illegals.count(move_dir) == 0) ? move_dir : Direction::UP; // 返回UP作为无效值
}

int judge(const GameState &state)
{
    // 更新目标锁定状态
    lock_on_target(state);
  
    // 处理宝箱和钥匙的目标锁定
    if ((state.get_self().has_key && is_chest_target && current_target.x != -1 && current_target.y != -1) ||
        (!state.get_self().has_key && is_key_target && current_target.x != -1 && current_target.y != -1)) {
      
        Direction move_dir = moveToTarget(state, current_target);
        if (move_dir != Direction::UP || state.get_self().direction == 1) { // 不是默认无效值，或者当前方向就是UP
            return Utils::dir2num(move_dir);
        }
    }
  
    // 继续原有代码...
```

这样可以减少约25行代码。

## 第二批：压缩illegalMove函数中的重复逻辑

在illegalMove函数（467-593行）中，紧急模式和非紧急模式的代码有大量重复，可以提取共同逻辑：

```cpp
unordered_set<Direction> illegalMove(const GameState &state)
{
    unordered_set<Direction> illegals;
    const Snake &self = state.get_self();
    bool emergency_mode = false;
  
    // 添加反方向限制
    auto addReverse = [&]() {
        switch (self.direction) {
        case 3: illegals.insert(Direction::UP); break;
        case 1: illegals.insert(Direction::DOWN); break;
        case 0: illegals.insert(Direction::RIGHT); break;
        default: illegals.insert(Direction::LEFT); break;
        }
    };
  
    // 自己方向的反方向不能走
    addReverse();
  
    // 检查每个方向的合法性
    auto checkDirectionLegality = [&](bool is_emergency) {
        for (auto dir : validDirections) {
            const Point &head = self.get_head();
            const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
          
            if (!Utils::boundCheck(y, x)) {
                illegals.insert(dir);
            } else {
                // 安全区检查
                if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max ||
                    y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max) {
                    if (self.shield_time <= 1) {
                        illegals.insert(dir);
                    }
                } else if (mp[y][x] == -4) { // 墙
                    illegals.insert(dir);
                } else if (mp[y][x] == -2 && !is_emergency) { // 陷阱
                    illegals.insert(dir);
                } else if (mp2[y][x] == -5 || mp2[y][x] == -6 || mp2[y][x] == -7) {
                    // 检查蛇身和危险区域
                    if (is_emergency) {
                        // 紧急模式下，只有蛇身且无法使用护盾时才非法
                        if (mp2[y][x] == -5 && 
                            !(self.shield_time >= 2 || (self.shield_cd == 0 && self.score > 20) ||
                              state.remaining_ticks >= 255 - 9 + 2)) {
                            illegals.insert(dir);
                        }
                    } else if (self.shield_time < 2) {
                        // 非紧急模式，护盾不足时非法
                        illegals.insert(dir);
                    }
                }
            }
        }
    };
  
    // 首次检查 - 非紧急模式
    checkDirectionLegality(false);
  
    // 如果四种方向都不行，进入紧急模式，重新评估
    if (illegals.size() == 4) {
        emergency_mode = true;
        illegals.clear();
        addReverse(); // 反方向仍不能走
        checkDirectionLegality(true); // 重新检查 - 紧急模式
    }
  
    return illegals;
}
```

这可以减少约50行代码。

## 第三批：压缩Strategy::bfs函数中的重复评估逻辑

在Strategy::bfs函数(677-1233行)中，有很多重复的评估逻辑，尤其是对安全区和食物价值的计算。可以提取出几个辅助函数：

```cpp
// 添加到Strategy命名空间中

// 安全区评估辅助函数
double evaluateSafeZoneStatus(const GameState &state, int x, int y, int ticks_to_shrink) {
    double score = 0;
  
    if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20) {
        // 检查位置是否在下一个安全区内
        if (x < state.next_safe_zone.x_min || x > state.next_safe_zone.x_max ||
            y < state.next_safe_zone.y_min || y > state.next_safe_zone.y_max) {
            // 安全区外位置价值大幅降低，危险度随收缩时间临近增加
            if (ticks_to_shrink <= 5) score -= 5000;      // 极度危险
            else if (ticks_to_shrink <= 10) score -= 3000; // 很危险
            else score -= 1000;                            // 有风险
        }
    }
    return score;
}

// 食物价值评估辅助函数
double evaluateFoodValue(double base_value, int food_type, const GameState &state, 
                         int y, int x, const Point &head, double safe_zone_factor) {
    if (food_type > 0) { // 普通食物或尸体
        int head_dist = abs(head.y - y) + abs(head.x - x);
      
        if (food_type >= 10) { // 极高价值尸体
            if (head_dist <= 6) return base_value * 200 + 200;
            else if (head_dist <= 10) return base_value * 120 + 120;
            else return base_value * 80 + 80;
        } 
        else if (food_type >= 5) { // 中等价值尸体
            if (head_dist <= 4) return base_value * 100 + 80;
            else return base_value * 70 + 40;
        }
        else { // 普通食物
            if (head_dist <= 3) return base_value * 45;
            else if (head_dist <= 5) return base_value * 35;
            else return base_value * 30;
        }
    }
    else if (food_type == -1) { // 增长豆
        // 根据游戏阶段动态调整增长豆价值
        if (state.remaining_ticks > 180) return 30;
        else if (state.remaining_ticks > 100) return 20;
        else return 10;
    }
    else if (food_type == -2) { // 陷阱
        return -0.5;
    }
  
    return base_value * safe_zone_factor; // 应用安全区因子
}

// 在bfs函数中使用这些辅助函数
```

通过提取这些辅助函数，可以减少约60-70行代码。

## 第四批：压缩lock_on_target函数中的重复逻辑

lock_on_target函数(179-359行)中的宝箱和钥匙处理逻辑有很多相似模式，可以简化：

```cpp
// 提取检查目标是否存在的辅助函数
bool targetExists(const Point &target, const vector<Item> &items, int min_value_percent = 80) {
    for (const auto &item : items) {
        if (abs(item.pos.y - target.y) <= 1 && 
            abs(item.pos.x - target.x) <= 1 &&
            item.value >= target_value * min_value_percent / 100) {
            return true;
        }
    }
    return false;
}

// 提取检查特定对象是否存在的辅助函数
template<typename T>
bool objectExists(const Point &target, const vector<T> &objects) {
    for (const auto &obj : objects) {
        if (obj.pos.y == target.y && obj.pos.x == target.x) {
            return true;
        }
    }
    return false;
}

// 然后在lock_on_target函数中使用这些辅助函数
```

通过提取这些辅助函数，可以减少约20-30行代码。

## 总结

这四批压缩方案可以总共减少约150-175行代码，同时完全保留原始功能。方案关注的是:

1. 将重复逻辑提取为辅助函数
2. 合并条件相似的代码块
3. 使用通用参数处理特殊情况
4. 使用lambda函数简化重复流程

所有这些优化都遵循"不改变原本代码功能"的原则，只是改变了代码的组织方式，使其更加简洁和可维护。
