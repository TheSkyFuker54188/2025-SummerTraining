# 安全区机制分析与实现方案

## 现有代码安全区处理分析

检查了`snake_algorithm_template.cpp`后，发现代码已经读取了安全区信息（第260-269行），但实际决策逻辑中**未考虑安全区边界和收缩**的影响：

1. **未检查边界**：`illegalMove`函数没有将安全区外判定为非法移动
2. **未考虑收缩**：策略评估函数未降低即将收缩区域内食物的价值
3. **未趋向新安全区**：缺乏当安全区即将收缩时主动向新安全区移动的逻辑

## 安全区实现方案

### 1. 修改`illegalMove`函数添加安全区边界检查

```cpp
unordered_set<Direction> illegalMove(const GameState &state)
{
    unordered_set<Direction> illegals;
    const Snake &self = state.get_self();
    
    // 现有代码...
    
    // 添加安全区检查：任何导致蛇头离开安全区的移动都是非法的
    for (auto dir : validDirections)
    {
        const Point &head = self.get_head();
        const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
        
        // 判断是否会离开安全区
        if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max || 
            y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max)
        {
            // 只有当护盾时间足够时才允许离开安全区
            if (self.shield_time <= 1) {
                illegals.insert(dir);
            }
        }
    }
    
    return illegals;
}
```

### 2. 在`Strategy::bfs`中考虑安全区收缩风险

```cpp
double bfs(int sy, int sx, int fy, int fx, const GameState &state)
{
    double score = 0;
    
    // 现有代码...
    
    // 安全区收缩风险评估
    const int remaining_ticks = state.remaining_ticks;
    const int ticks_to_shrink = state.next_shrink_tick - remaining_ticks;
    
    // 如果即将收缩（小于20个tick）
    if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20)
    {
        // 检查位置是否在下一个安全区内
        if (sy < state.next_safe_zone.x_min || sy > state.next_safe_zone.x_max ||
            sx < state.next_safe_zone.y_min || sx > state.next_safe_zone.y_max)
        {
            // 安全区外位置价值大幅降低，危险度随收缩时间临近增加
            if (ticks_to_shrink <= 5) {
                // 极度危险，几乎立即放弃
                score -= 5000;
            } else if (ticks_to_shrink <= 10) {
                // 很危险，强烈避免
                score -= 3000; 
            } else {
                // 有风险，尽量避免
                score -= 1000;
            }
        }
    }
    
    // 继续现有代码...
    
    return score;
}
```

### 3. 添加安全区中心倾向逻辑

```cpp
// 在Strategy命名空间中添加函数
double safeZoneCenterScore(const GameState &state, int y, int x)
{
    // 计算相对于当前安全区中心的位置
    int center_x = (state.current_safe_zone.x_min + state.current_safe_zone.x_max) / 2;
    int center_y = (state.current_safe_zone.y_min + state.current_safe_zone.y_max) / 2;
    
    // 当安全区即将收缩时，使用下一个安全区的中心
    if (state.next_shrink_tick - state.remaining_ticks <= 25) {
        center_x = (state.next_safe_zone.x_min + state.next_safe_zone.x_max) / 2;
        center_y = (state.next_safe_zone.y_min + state.next_safe_zone.y_max) / 2;
    }
    
    // 计算到中心的距离，并根据游戏阶段调整中心倾向性
    double distance = abs(y - center_y) + abs(x - center_x);
    double center_factor = 0;
    
    // 游戏后期更重视中心位置
    if (state.remaining_ticks < 100) {
        center_factor = 5.0;
    } else if (state.remaining_ticks < 180) {
        center_factor = 3.0;
    } else {
        center_factor = 1.0;
    }
    
    return -distance * center_factor;
}
```

### 4. 修改主评估函数，整合安全区考虑

```cpp
double eval(const GameState &state, int y, int x, int fy, int fx)
{
    // 现有的拐角评估...
    
    // 安全区评估
    double safe_zone_score = safeZoneCenterScore(state, y, x);
    
    // BFS评估
    double bfs_score = bfs(y, x, fy, fx, state);
    
    // 整合评分
    return bfs_score + safe_zone_score;
}
```

这些改进将使蛇能够:
1. 避免离开安全区导致死亡
2. 提前感知安全区收缩带来的危险
3. 随着游戏进行更倾向于安全区中心位置