# 平衡的尸体策略优化方案

以下是一个更加平衡、风险可控的尸体捡拾策略优化方案，通过调整现有代码而非添加独立组件实现：

## 1. 适度提升尸体价值

```cpp
// 在processFoodWithDynamicPriority函数中(3442行附近)
// 计算食物的基础价值 - 适度提升尸体价值
double base_value;
if (item.value > 0) { // 尸体
    // 根据游戏阶段动态调整尸体价值系数
    double corpse_multiplier;
    if (game_phase < 100) 
        corpse_multiplier = 8.0;      // 早期
    else if (game_phase < 180) 
        corpse_multiplier = 12.0;     // 中期更看重尸体
    else 
        corpse_multiplier = 10.0;     // 后期略微降低
      
    // 根据蛇长度微调 - 短蛇更需要快速成长
    if (self.length < 8) corpse_multiplier *= 1.2;
  
    // 根据尸体价值进一步调整 - 高价值尸体更值得争取
    if (item.value >= 10)
        corpse_multiplier *= 1.25;
      
    base_value = item.value * corpse_multiplier;
} else if (item.value == -1) {
    base_value = 3; // 增长豆价值保持不变
} else {
    base_value = 1; // 普通食物
}
```

## 2. 适度调整风险容忍度

```cpp
// 在风险评估部分(3452行附近)
if (!path_safety.is_safe) {
    // 为尸体适度提高风险容忍度
    double risk_threshold_factor = 1.0;
    double risk_penalty_factor = 1.0;
  
    if (item.value > 0) { // 尸体
        // 根据尸体价值调整风险容忍度
        if (item.value >= 10) {
            risk_threshold_factor = 1.2; // 高价值尸体阈值提高20%
            risk_penalty_factor = 0.85;  // 风险惩罚降低15%
        } else {
            risk_threshold_factor = 1.1; // 普通尸体阈值提高10%
            risk_penalty_factor = 0.9;   // 风险惩罚降低10%
        }
      
        // 考虑蛇长度 - 短蛇更保守
        if (self.length < 8) {
            risk_threshold_factor = max(1.0, risk_threshold_factor - 0.05);
            risk_penalty_factor = min(1.0, risk_penalty_factor + 0.05);
        }
      
        // 考虑游戏阶段 - 后期更保守
        if (game_phase > 180) {
            risk_threshold_factor = max(1.0, risk_threshold_factor - 0.1);
            risk_penalty_factor = min(1.0, risk_penalty_factor + 0.1);
        }
    }
  
    // 应用调整后的风险评估
    double adjusted_risk_threshold = dynamic_risk_threshold * risk_threshold_factor;
    double risk_factor = 1.0 - min(0.8 * risk_penalty_factor, -path_safety.safety_score / 2000.0);
    adjusted_value *= risk_factor;
  
    // 梯度惩罚
    if (path_safety.safety_score < adjusted_risk_threshold) {
        double severity = min(1.0, (adjusted_risk_threshold - path_safety.safety_score) / 500.0);
        adjusted_value *= (0.8 - 0.6 * severity * risk_penalty_factor);
    }
}
```

## 3. 合理扩大尸体搜索范围

```cpp
// 在processFoodWithDynamicPriority函数距离检查部分(3431行附近)
// 计算距离
int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
int effective_max_range = maxRange;

// 适度扩大尸体搜索范围
if (item.value > 0) { // 尸体
    if (item.value >= 10)
        effective_max_range = min(12, maxRange + 4); // 高价值尸体搜索范围+4
    else
        effective_max_range = min(10, maxRange + 2); // 普通尸体搜索范围+2
      
    // 游戏阶段调整 - 中期最积极寻找尸体
    if (game_phase > 100 && game_phase < 180)
        effective_max_range += 1;
}

if (dist > effective_max_range) {
    continue;  // 跳过超出范围的食物
}
```

## 4. 优化尸体竞争策略

```cpp
// 在竞争评估部分(3498行附近)
// 竞争分析 - 对尸体优化竞争评估
double competition_penalty = 0;
for (const auto &snake : state.snakes) {
    if (snake.id != MYID && snake.id != -1) {
        int enemy_dist = abs(snake.get_head().y - item.pos.y) + abs(snake.get_head().x - item.pos.x);
      
        // 针对尸体的特殊竞争评估
        if (item.value > 0) { // 尸体
            // 计算距离差异 - 正值表示我们更近
            int distance_advantage = enemy_dist - dist;
          
            // 评估优势因素
            bool has_length_advantage = self.length >= snake.length;
            bool has_shield_advantage = self.shield_time > 0 && self.shield_time >= 3;
            bool is_high_value = item.value >= 8;
          
            // 如果我方更近或距离相近
            if (distance_advantage >= 0) {
                // 我方占优，几乎不惩罚
                competition_penalty += 0.05;
            }
            // 敌方略微更近
            else if (distance_advantage >= -2) {
                // 如果有优势或尸体价值高，适度惩罚
                if (has_length_advantage || has_shield_advantage || is_high_value) {
                    competition_penalty += 0.15;
                } else {
                    competition_penalty += 0.3;
                }
            }
            // 敌方明显更近
            else if (distance_advantage >= -4) {
                // 如果同时具有长度和护盾优势，或是极高价值尸体，仍然考虑争夺
                if ((has_length_advantage && has_shield_advantage) || item.value >= 10) {
                    competition_penalty += 0.4;
                } else {
                    competition_penalty += 0.6;
                }
            }
            // 敌方远远更近，几乎放弃
            else {
                competition_penalty += 0.8;
            }
        } else { // 非尸体食物保持原有逻辑
            if (enemy_dist < dist * 1.2) { // 敌蛇更近或距离相近
                // 评估我方蛇在竞争中的优势
                bool has_length_advantage = self.length > snake.length + 2;
                bool has_position_advantage = Strategy::countSafeExits(state, item.pos) >= 2;
              
                // 根据优势调整惩罚
                if (!has_length_advantage && !has_position_advantage)
                    competition_penalty += 0.4; // 重大劣势
                else if (has_length_advantage || has_position_advantage)
                    competition_penalty += 0.2; // 部分劣势
            }
        }
    }
}

// 应用竞争惩罚 - 确保至少保留30%价值
double competition_factor = max(0.3, 1.0 - competition_penalty);
```

## 5. 调整优先级配置

```cpp
// 在动态优先级配置部分(3384-3420行)
// 调整优先级配置，提高尸体优先级
if (self.length < 8) { // 短蛇
    if (late_game) {
        priorities = {
            {8, 6, 1.4},    // 高价值尸体(>=8)搜索范围6格，权重1.4
            {5, 4, 1.2},    // 中价值尸体(>=5)搜索范围4格，权重1.2
            {2, 2, 1.1},    // 低价值尸体(>=2)搜索范围2格，权重1.1
            {0, 5, 0.6}     // 普通食物搜索范围5格，权重0.6
        };
    } else {
        // 前中期短蛇对尸体更积极
        priorities = {
            {8, 7, 1.5},    // 高价值尸体权重更高
            {4, 5, 1.3},    // 扩大中价值尸体搜索范围
            {1, 3, 1.1},    // 极近距离任意食物
            {0, 6, 0.7}     // 普通食物
        };
    }
} else { // 长蛇
    if (late_game) {
        priorities = {
            {10, 5, 1.3},   // 高价值尸体
            {6, 3, 1.1},    // 中价值尸体
            {3, 2, 0.9},    // 低价值尸体
            {0, 4, 0.5}     // 普通食物
        };
    } else {
        priorities = {
            {8, 6, 1.3},    // 高价值尸体
            {5, 4, 1.1},    // 中价值尸体
            {2, 3, 0.9},    // 低价值尸体
            {0, 5, 0.6}     // 普通食物
        };
    }
}
```

## 优点总结

1. **平衡风险与收益**：适度提高尸体价值和风险容忍度，不会过分激进
2. **动态调整系数**：根据尸体价值、蛇长度和游戏阶段动态调整各项参数
3. **更精细的竞争评估**：考虑距离差异、蛇长度和护盾状态，制定更明智的竞争决策
4. **优化现有系统**：通过调整参数增强尸体捡拾能力，不需要添加新组件
5. **保持安全机制**：保留基本安全检查，不会为了尸体而冒过度风险

这个方案提高了蛇对尸体的重视程度和搜索能力，同时保持了决策系统的稳定性和安全性，是一个平衡且风险可控的优化方案。
