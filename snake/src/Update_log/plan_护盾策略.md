# 优化护盾使用策略

分析当前护盾使用代码后，发现确实存在过度使用护盾的情况。护盾成本高达20分，应该只在真正必要或高回报场景使用。

## 当前护盾使用情景

1. **紧急求生情况**

   - 无路可走时（第3193行）
   - 这是必要的，应该保留
2. **高价值食物获取**

   - 任何高价值食物路径不安全时（第2916-2928行，第3109-3111行）
   - 这里门槛太低，需要提高标准
3. **安全区收缩**

   - 安全区即将收缩时（第3166-3168行）
   - 只考虑了时间紧迫性，未充分评估必要性
4. **宝箱/钥匙获取**

   - 路径不安全但安全分数在阈值以上（第3024-3032行）
   - 没有评估宝箱/钥匙实际价值与成本的比较

## 优化建议

### 1. 严格的生死攸关情形

```cpp
// 仅当真正无路可走时使用护盾
if (legalMoves.empty() && self.shield_cd == 0 && self.score >= 20) {
    return SHIELD_COMMAND;
}
```

### 2. 宝箱/钥匙收益明确计算

```cpp
// 计算预期收益并与护盾成本(20分)比较
int expected_gain = is_chest ? chest.score : 40; // 钥匙价值约40分
double risk_factor = 1.0 - min(1.0, -path_safety.safety_score / 1500.0);
int adjusted_gain = expected_gain * risk_factor;

// 只有当预期收益显著高于护盾成本时才使用
if (adjusted_gain > 30 && self.shield_cd == 0 && self.score >= 20) {
    return SHIELD_COMMAND;
}
```

### 3. 安全区收缩风险精确评估

```cpp
// 评估是否能在不使用护盾的情况下安全到达
bool can_reach_safely = false;
int dist_to_safe_zone = /* 计算到安全区的最短距离 */;
if (dist_to_safe_zone < ticks_to_shrink - 2) {
    can_reach_safely = true;
}

// 仅在无法安全到达时使用护盾
if (!can_reach_safely && is_emergency_shrink && self.shield_cd == 0 && self.score >= 20) {
    return SHIELD_COMMAND;
}
```

### 4. 食物价值严格筛选

```cpp
// 只为极高价值食物(>=8)且有路径风险时开盾
if (!best_path_safety.is_safe && best_path_safety.safety_score > -1200 && 
    best_food.value >= 8 && self.shield_cd == 0 && self.score >= 20) {
    should_use_shield = true;
}
```

### 5. 游戏后期策略调整

```cpp
// 后期更愿意保存分数而非冒险
bool is_late_game = state.remaining_ticks < 80;
if (is_late_game && self.score >= 60) {
    // 提高开盾门槛，避免后期损失分数
    if (best_food.value < 12) { // 只为极高价值食物开盾
        should_use_shield = false;
    }
}
```

通过这些调整，可以显著减少"非必要护盾"的使用，保留护盾用于真正的生死存亡和高收益场景，从而提高整体得分。
