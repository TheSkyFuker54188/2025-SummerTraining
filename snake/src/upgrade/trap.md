# 陷阱策略分析与改进方案

## 当前陷阱处理的不足

分析了代码中对陷阱的处理，发现以下问题：

1. **陷阱处理不一致**：

   - BFS函数(602行)给陷阱分配 -1000 高惩罚分数
   - 但cornerEval函数(1010行)却给陷阱拐角正面评价(+1)
   - 不同评估函数对陷阱态度矛盾
2. **陷阱不被视为障碍**：

   - illegalMove函数(339-452行)没有特殊处理陷阱
   - 不像处理墙壁和蛇身那样禁止通过
3. **缺乏紧急情况处理**：

   - 没有判断"陷阱vs死亡"的取舍逻辑
   - 没有基于蛇当前得分的权衡决策

## 改进方案

### 1. 将陷阱视为障碍物

```cpp
// 修改illegalMove函数，将陷阱视为障碍物
if (mp[y][x] == -2) // 陷阱
{
    // 将陷阱添加到非法移动中，除非是紧急情况
    if (!emergency_mode) {
        illegals.insert(dir);
    }
}
```

### 2. 添加紧急情况判断

```cpp
// 在illegalMove函数中添加紧急模式检测
bool emergency_mode = false;

// 先计算不将陷阱视为障碍的合法移动
unordered_set<Direction> normal_illegals = calculateIllegals(state, false);

// 如果没有合法移动，进入紧急模式
if (normal_illegals.size() == 4) {
    emergency_mode = true;
}
```

### 3. 陷阱评估函数-NAIVE

```cpp
// 新增NAIVE陷阱评估函数
double evaluateTrap(const GameState &state, int trap_y, int trap_x) {
    const auto &self = state.get_self();
  
    // 基础分：默认陷阱非常危险
    double score = -1000;
  
    // 如果蛇当前分数很高，减轻陷阱惩罚
    if (self.score > 50) {
        score = -800; // 减轻惩罚
    }
  
    // 如果蛇即将死亡(四周都是障碍)，接受陷阱
    if (/* 检测四周是否都是障碍 */) {
        score = -100; // 显著减轻惩罚
    }
  
    // 如果有护盾或可以开启护盾，更倾向接受陷阱
    if (self.shield_time > 0 || (self.shield_cd == 0 && self.score >= 20)) {
        score *= 0.5; // 减半惩罚
    }
  
    return score;
}
```

### 4. 整合到BFS评估

```cpp
// 修改BFS评估中的陷阱处理
if (mp[y][x] == -2) {
    // 不是拐角的陷阱，使用专用评估函数
    if (!flag) {
        num = evaluateTrap(state, y, x);
    } 
    // 拐角的陷阱，考虑紧急情况
    else {
        // 检测紧急情况
        bool is_emergency = /* 检测是否处于紧急情况 */;
        if (is_emergency) {
            num = -100; // 紧急情况下接受陷阱
        } else {
            num = -500; // 非紧急情况下尽量避免
        }
    }
}
```

### 5. 修复cornerEval中的陷阱处理

```cpp
// 修改cornerEval中的陷阱处理
if (mp[qy][qx] == -2) {
    return -50; // 改为负值，表示有陷阱的拐角不好
}
```

这套改进方案能够让蛇更好地处理陷阱：将陷阱基本视为障碍物避开，但在紧急情况下会进行评估判断是否值得吃陷阱。
Ran tool
