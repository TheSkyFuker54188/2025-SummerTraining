# 食物密度评估方案

根据规则，第二次收缩发生在161-180游戏刻之间。我设计一个在此前阶段（1-160刻）优先考虑食物密度高区域的策略：

```cpp
// 食物密度评估函数 - 添加到snake_algorithm_template.cpp中
double calculateFoodDensity(const GameState &state, int center_y, int center_x, int radius = 5) {
    double total_value = 0;
    int count = 0;
  
    // 计算指定区域内所有食物的总价值
    for (const auto &item : state.items) {
        // 检查食物是否在指定半径内
        if (abs(item.pos.y - center_y) <= radius && abs(item.pos.x - center_x) <= radius) {
            // 增加食物价值计数
            if (item.value > 0) { // 普通食物
                total_value += item.value;
                count++;
            } else if (item.value == GROWTH_BEAN_VALUE) { // 增长豆
                // 根据游戏阶段确定增长豆价值
                int growth_value = (state.remaining_ticks > 176) ? 3 : 2;
                total_value += growth_value;
                count++;
            }
        }
    }
  
    // 返回密度分数，避免除以零
    return (count > 0) ? (total_value / count) * count : 0;
}

// 在BFS函数中添加密度评估逻辑
double bfs(int sy, int sx, int fy, int fx, const GameState &state) {
    // ...现有代码...
  
    // 在第二次收缩前考虑食物密度
    if (state.remaining_ticks > 76) { // 1-160刻
        // 计算当前位置区域的食物密度
        double density_score = calculateFoodDensity(state, y, x, 5);
      
        // 根据游戏进程调整密度权重
        double density_weight = 0.0;
        if (state.remaining_ticks > 176) { // 早期阶段
            density_weight = 0.8; // 早期更看重食物密集区
        } else { // 中期第一部分
            density_weight = 0.5; // 中期适当关注
        }
      
        // 将密度分数加入总评分
        score += density_score * density_weight;
    }
  
    return score;
}
```

这个设计有几个关键优势：

1. **区域评估**：不只考虑单个食物，而是评估一个半径为5的区域内所有食物的价值总和
2. **密度计算**：同时考虑食物总价值和数量，同等价值下优先选择食物聚集的区域
3. **阶段适应**：在早期阶段(1-80)给予更高权重，随游戏进程逐渐降低
4. **整合现有系统**：无需大幅改变代码结构，只需添加这个评分因素

这样设计可以引导蛇优先前往食物密集区域，提高进食效率，从而在第二次收缩前获得更多分数和成长。
