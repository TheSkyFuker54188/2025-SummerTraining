
# 最终宝箱与钥匙处理方案

以下是完整的钥匙-宝箱处理方案，基于简化处理策略，不区分钥匙与宝箱的具体对应关系：

```cpp
// 在全局变量区域
static bool is_key_target = false;  // 是否锁定钥匙
static bool is_chest_target = false; // 是否锁定宝箱

// 目标锁定函数
void lock_on_target(const GameState &state) {
    const auto &self = state.get_self();
    const auto &head = self.get_head();
  
    // 更新锁定状态
    if (target_lock_time > 0) {
        target_lock_time--;
    }
  
    // 如果宝箱消失了（可能被开启）
    if (is_chest_target && state.chests.empty()) {
        current_target = {-1, -1};
        target_value = 0;
        is_chest_target = false;
    }
  
    // 如果所有钥匙都消失了
    if (is_key_target && state.keys.empty()) {
        current_target = {-1, -1};
        target_value = 0;
        is_key_target = false;
    }
  
    // 如果持有钥匙，优先锁定宝箱
    if (self.has_key && !state.chests.empty()) {
        // 重置之前的钥匙目标
        is_key_target = false;
      
        // 如果还没有锁定宝箱或宝箱位置已变化，重新锁定宝箱
        if (!is_chest_target || current_target.x == -1 || current_target.y == -1) {
            // 找到最近的宝箱
            int min_dist = INT_MAX;
            const Chest* nearest_chest = nullptr;
          
            for (const auto &chest : state.chests) {
                int dist = abs(head.y - chest.pos.y) + abs(head.x - chest.pos.x);
              
                if (dist < min_dist) {
                    min_dist = dist;
                    nearest_chest = &chest;
                }
            }
          
            // 锁定最近的宝箱
            if (nearest_chest != nullptr) {
                current_target = nearest_chest->pos;
                target_value = nearest_chest->score;
                target_lock_time = min(min_dist + 10, 30); // 给足够时间去宝箱
                is_chest_target = true;
              
                // 如果钥匙即将掉落，缩短锁定时间
                for (const auto &key : state.keys) {
                    if (key.holder_id == MYID) {
                        if (key.remaining_time < min_dist) {
                            // 钥匙可能会在到达宝箱前掉落，调整锁定时间
                            target_lock_time = key.remaining_time - 1;
                        }
                        break;
                    }
                }
            }
        }
      
        // 检查宝箱是否仍然存在
        bool chest_exists = false;
        for (const auto &chest : state.chests) {
            if (chest.pos.y == current_target.y && chest.pos.x == current_target.x) {
                chest_exists = true;
                break;
            }
        }
      
        if (!chest_exists) {
            // 宝箱已被开启或消失
            current_target = {-1, -1};
            target_value = 0;
            is_chest_target = false;
        }
    }
    // 如果没有钥匙，考虑锁定钥匙
    else if (!self.has_key && !state.keys.empty()) {
        // 重置之前的宝箱目标
        is_chest_target = false;
      
        // 如果没有锁定钥匙或锁定时间结束，寻找新的钥匙
        if (!is_key_target || target_lock_time <= 0 || current_target.x == -1 || current_target.y == -1) {
            const Key* best_key = nullptr;
            int min_dist = INT_MAX;
          
            // 查找最近的安全钥匙
            for (const auto &key : state.keys) {
                // 只考虑地图上的钥匙
                if (key.holder_id == -1) {
                    // 检查钥匙是否在安全区内
                    if (key.pos.x < state.current_safe_zone.x_min || key.pos.x > state.current_safe_zone.x_max ||
                        key.pos.y < state.current_safe_zone.y_min || key.pos.y > state.current_safe_zone.y_max) {
                        continue;  // 忽略安全区外的钥匙
                    }
                  
                    // 计算到钥匙的距离
                    int dist = abs(head.y - key.pos.y) + abs(head.x - key.pos.x);
                  
                    // 检查钥匙是否会因安全区收缩而消失
                    int current_tick = MAX_TICKS - state.remaining_ticks;
                    int ticks_to_shrink = state.next_shrink_tick - current_tick;
                  
                    if (ticks_to_shrink >= 0 && ticks_to_shrink <= 15) {
                        if (key.pos.x < state.next_safe_zone.x_min || key.pos.x > state.next_safe_zone.x_max ||
                            key.pos.y < state.next_safe_zone.y_min || key.pos.y > state.next_safe_zone.y_max) {
                            continue;  // 即将在安全区收缩中消失的钥匙
                        }
                    }
                  
                    if (dist < min_dist) {
                        min_dist = dist;
                        best_key = &key;
                    }
                }
            }
          
            // 锁定最近的安全钥匙
            if (best_key != nullptr) {
                current_target = best_key->pos;
                target_value = 40;  // 钥匙的虚拟价值（考虑到宝箱的高价值）
                target_lock_time = min(min_dist + 5, 20);  // 给予足够的时间去拿钥匙
                is_key_target = true;
            }
        }
      
        // 检查钥匙是否仍然存在
        if (is_key_target) {
            bool key_exists = false;
            for (const auto &key : state.keys) {
                if (key.holder_id == -1 && 
                    abs(key.pos.y - current_target.y) <= 1 && 
                    abs(key.pos.x - current_target.x) <= 1) {
                    key_exists = true;
                    // 更新为精确位置
                    current_target = key.pos;
                    break;
                }
            }
          
            if (!key_exists) {
                // 钥匙已被拿走或消失
                current_target = {-1, -1};
                target_value = 0;
                is_key_target = false;
            }
        }
    }
    // 如果没有钥匙和宝箱相关目标，重置目标锁定
    else if (is_key_target || is_chest_target) {
        current_target = {-1, -1};
        target_value = 0;
        is_key_target = false;
        is_chest_target = false;
    }
  
    // 检查当前目标是否仍然存在 (普通食物目标)
    if (!is_key_target && !is_chest_target && current_target.y != -1 && current_target.x != -1) {
        bool target_exists = false;
        for (const auto &item : state.items) {
            // 使用近似匹配检测目标是否仍然存在
            if (abs(item.pos.y - current_target.y) <= 1 && 
                abs(item.pos.x - current_target.x) <= 1 &&
                item.value >= target_value * 0.8) { // 允许价值有小幅下降
              
                current_target = item.pos; // 更新为精确位置
                target_exists = true;
                break;
            }
        }
      
        // 如果目标不再存在或锁定时间结束，重置锁定状态
        if (!target_exists && target_lock_time <= 0) {
            current_target = {-1, -1};
            target_value = 0;
        }
    }
}

// 在judge函数中添加目标优先级处理
int judge(const GameState &state)
{
    // 更新目标锁定状态
    lock_on_target(state);
  
    // 即时反应 - 处理近距离高价值目标
    const auto &head = state.get_self().get_head();
  
    // 如果持有钥匙且有宝箱目标，优先前往宝箱
    if (state.get_self().has_key && is_chest_target && current_target.x != -1 && current_target.y != -1) {
        // 确定移动方向
        Direction move_dir;
        if (head.x > current_target.x) move_dir = Direction::LEFT;
        else if (head.x < current_target.x) move_dir = Direction::RIGHT;
        else if (head.y > current_target.y) move_dir = Direction::UP;
        else move_dir = Direction::DOWN;
      
        // 检查移动安全性
        unordered_set<Direction> illegals = illegalMove(state);
        if (illegals.count(move_dir) == 0) {
            return Utils::dir2num(move_dir);
        }
    }
  
    // 如果有钥匙目标，优先前往钥匙
    if (!state.get_self().has_key && is_key_target && current_target.x != -1 && current_target.y != -1) {
        // 确定移动方向
        Direction move_dir;
        if (head.x > current_target.x) move_dir = Direction::LEFT;
        else if (head.x < current_target.x) move_dir = Direction::RIGHT;
        else if (head.y > current_target.y) move_dir = Direction::UP;
        else move_dir = Direction::DOWN;
      
        // 检查移动安全性
        unordered_set<Direction> illegals = illegalMove(state);
        if (illegals.count(move_dir) == 0) {
            return Utils::dir2num(move_dir);
        }
    }
  
    // ...现有的其他决策逻辑...
}
```

这个方案的主要特点：

1. **动态目标选择**：根据当前持有物品状态决定目标

   - 没钥匙时优先寻找安全钥匙
   - 有钥匙时优先前往最近宝箱
2. **安全性检查**：

   - 只考虑安全区内的钥匙
   - 避免即将在安全区收缩中消失的钥匙
   - 确保没钥匙时不会撞击宝箱
3. **资源有效利用**：

   - 考虑钥匙30个游戏刻的有效时间
   - 如果钥匙时间不足以到达宝箱，调整策略
4. **边缘情况处理**：

   - 宝箱消失时重置目标状态
   - 钥匙消失时重置目标状态
   - 自动适应新出现的宝箱和钥匙

这个方案不需要显式区分哪个钥匙对应哪个宝箱，而是完全依靠游戏机制自动处理这种关系，确保代码简洁可靠。
