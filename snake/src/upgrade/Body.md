# 尸体策略优化方案

分析了代码中与尸体(高价值食物)相关的部分，找到以下可以调优的关键点：

## 1. 即时反应范围扩大 (934-971行)

```cpp
// 高分食物检测（优先级最高）
for (const auto &item : state.items) {
    if (item.value >= 5)  // 高价值食物（蛇尸体）
    {
        int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
        if (dist <= immediate_range * 2)  // 当前为6格，建议扩大
```

**优化方案**：

```cpp
const int immediate_range = 3;
const int corpse_detection_range = 8; // 尸体检测范围扩大到8格

// 高分食物检测
for (const auto &item : state.items) {
    if (item.value >= 5)  // 高价值食物（蛇尸体）
    {
        int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
        if (dist <= corpse_detection_range) // 使用更大的检测范围
```

## 2. 尸体价值系数提升 (650-658行)

```cpp
// 识别高分值食物（蛇尸体通常是高分值食物>=5）
if (mp[y][x] >= 10) {
    // 极高价值的尸体，极大提高优先级
    num = mp[y][x] * 60 + 50; // 更加强调高分尸体价值
} else if (mp[y][x] >= 5) {
    // 高价值的尸体，显著提高优先级
    num = mp[y][x] * 50 + 25; // 显著提高高分食物优先级
}
```

**优化方案**：

```cpp
// 大幅提高尸体价值
if (mp[y][x] >= 10) {
    // 极高价值的尸体
    num = mp[y][x] * 100 + 100; // 提高到原来的近2倍
} else if (mp[y][x] >= 5) {
    // 高价值的尸体
    num = mp[y][x] * 80 + 50; // 提高到原来的1.6倍
}
```

## 3. 竞争因素优化 (772-776行)

```cpp
// 如果敌方蛇更近，竞争系数降低
if (dist < self_distance) {
    // 对于高价值尸体，即使竞争也要争取
    if (mp[y][x] >= 8) {
        competition_factor *= 0.85f; // 对高分尸体，只减少15%价值
    } else {
        competition_factor *= 0.7f; // 普通食物减少30%价值
    }
}
```

**优化方案**：

```cpp
// 降低竞争因素对尸体的影响
if (dist < self_distance) {
    // 对于高价值尸体，几乎忽略竞争
    if (mp[y][x] >= 8) {
        competition_factor *= 0.95f; // 只减少5%价值
    } else if (mp[y][x] >= 5) {
        competition_factor *= 0.90f; // 减少10%价值
    } else {
        competition_factor *= 0.7f; // 普通食物不变
    }
}
```

## 4. 增加尸体专用搜索策略

在judge函数增加尸体专用搜索:

```cpp
// 增加尸体扫描函数 - 在judge函数的开始部分添加
bool hasCorpseNearby = false;
Point best_corpse;
int best_corpse_value = 0;
int best_corpse_dist = 999;

// 扫描地图寻找最佳尸体
for (const auto &item : state.items) {
    if (item.value >= 5) { // 尸体
        int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
      
        // 计算综合价值（考虑分值和距离）
        int adjusted_value = item.value * 10 - dist;
      
        // 更新最佳尸体
        if (adjusted_value > best_corpse_value) {
            best_corpse = item.pos;
            best_corpse_value = adjusted_value;
            best_corpse_dist = dist;
            hasCorpseNearby = true;
        }
    }
}

// 如果有高价值尸体且距离适中，规划路径前往
if (hasCorpseNearby && best_corpse_dist <= 12) {
    // 确定移动方向
    Direction corpse_dir;
    if (head.x > best_corpse.x) corpse_dir = Direction::LEFT;
    else if (head.x < best_corpse.x) corpse_dir = Direction::RIGHT;
    else if (head.y > best_corpse.y) corpse_dir = Direction::UP;
    else corpse_dir = Direction::DOWN;
  
    // 检查移动安全性
    unordered_set<Direction> illegals = illegalMove(state);
    if (illegals.count(corpse_dir) == 0) {
        return Utils::dir2num(corpse_dir);
    }
}
```

这些优化调整会使蛇更积极地寻找和获取尸体，特别是那些高价值的尸体，从而提高总分。
