# 移除地图中心倾向的设计方案

根据您的实战经验，我设计了以下方案来去除代码中向地图几何中心移动的倾向：

## 1. 移除 `safeZoneCenterScore` 函数的中心偏好

```cpp
// 修改 safeZoneCenterScore 函数 (429-456行)
double safeZoneCenterScore(const GameState &state, int y, int x) {
    // 不再计算到中心的距离和惩罚
    // 而是检查位置是否在安全区内，并根据到边界的距离给予适当评分
  
    // 计算到当前安全区边界的最小距离
    int dist_to_border = min({
        x - state.current_safe_zone.x_min,
        state.current_safe_zone.x_max - x,
        y - state.current_safe_zone.y_min,
        state.current_safe_zone.y_max - y
    });
  
    // 如果距离边界太近(<=3格)，给予轻微惩罚，否则返回0（不偏好任何区域）
    return (dist_to_border <= 3) ? -5.0 * (3 - dist_to_border) : 0;
}
```

## 2. 移除 BFS 评分中的中心倾向代码

```cpp
// 完全删除550-553行的中心倾向代码
// 删除以下内容:
if (start > timeRest && timeRest >= end) {
    const double r = maxNum * (timeRest - start) * (timeRest - start);
    score -= (abs(sy - 14.5) + abs(sx - 19.5)) * r / ((start - end) * (start - end));
}
```

## 3. 修改评估函数整合方式

```cpp
// 修改 eval 函数 (870-894行)
double eval(const GameState &state, int y, int x, int fy, int fx) {
    int test = cornerEval(y, x, fy, fx);
    if (test != -10000) {
        // 是拐角位置处理保持不变
        if (test == -5000) {
            return -50000;
        }
        if (test == 1) {
            mp[y][x] = -9;
        }
    }
  
    // 安全区评分 - 现在只对靠近边界的位置有轻微惩罚
    double safe_zone_score = safeZoneCenterScore(state, y, x);
  
    // BFS评估
    double bfs_score = bfs(y, x, fy, fx, state);
  
    // 整合评分，但安全区评分不再包含中心偏好
    return bfs_score + safe_zone_score;
}
```

这些修改将：

1. 取消对蛇向地图中心移动的奖励
2. 仍然保留了对远离安全区边界的轻微偏好，确保蛇不会太靠近边界
3. 保持了关键的安全区收缩处理逻辑，确保蛇会避开即将收缩区域

这样蛇将更自由地在地图上移动，主要根据食物位置和安全因素做出决策，而不是被强制向中心移动。
