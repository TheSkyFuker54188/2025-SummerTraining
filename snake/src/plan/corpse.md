# 尸体处理策略优化

针对您的实战观察，我确实看到现有代码中对高分食物的处理存在优化空间，特别是尸体系统。

## 尸体处理优化

现有代码只是简单判断 `item.value >= 5` 来识别高分食物，没有根据不同分值进行精细化处理。让我们改进这部分：

```cpp
// 在 select_target 函数中修改尸体处理逻辑
// 现有代码(834-838行):
if (item.value >= 5 && dist <= IMMEDIATE_RESPONSE_DISTANCE * 2) {
  // 高分食物有更大的即时响应范围和更高优先级
  return {item.pos, item.value, dist, 1200.0};
}

// 优化为:
if (item.value >= 5) {
  // 动态计算高分食物的优先级，分值越高优先级越高
  double corpse_priority;
  
  // 根据游戏阶段和食物分值动态调整优先级
  float game_progress = 1.0f - (float)state.remaining_ticks / MAX_TICKS;
  float stage_factor = 1.0f + game_progress * 0.5f; // 后期更重视高分食物
  
  // 高分食物基础优先级(5分=1200，20分=2500)
  corpse_priority = 1000.0 + 100.0 * item.value * stage_factor;
  
  // 考虑距离因素，近距离更有优势
  if (dist <= IMMEDIATE_RESPONSE_DISTANCE * 2) {
    // 使用非线性距离衰减，近的食物价值衰减很小
    corpse_priority *= (10.0 / (dist + 5.0));
    
    // 计算竞争因素 - 检查其他蛇是否更近
    bool is_contested = false;
    for (const auto &snake : state.snakes) {
      if (snake.id != MYID && snake.id != -1) {
        int other_dist = Utils::manhattan_distance(snake.get_head(), item.pos);
        if (other_dist < dist - 4) { // 对方明显更近
          is_contested = true;
          corpse_priority *= 0.5; // 降低一半优先级
          break;
        }
      }
    }
    
    // 没有竞争的高分食物，大幅提高优先级
    if (!is_contested && item.value >= 10) {
      corpse_priority *= 1.5;
    }
    
    return {item.pos, item.value, dist, corpse_priority};
  } else if (item.value >= 15) {
    // 即使距离较远，超高分食物(15+)也值得考虑
    corpse_priority *= (8.0 / (dist + 4.0));
    return {item.pos, item.value, dist, corpse_priority};
  }
}
```

对尸体食物进行更细致的分值评估和竞争分析
