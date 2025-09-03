# 边缘/角落困境分析与优化方案

分析了代码后，我发现有几个关键问题导致蛇经常沦落到边缘或角落:

## 主要问题

1. **中心区域价值评估不足**

   - `safeZoneCenterScore`函数(1334行)对边界位置的惩罚太小(仅 `-5.0 * (3 - dist_to_border)`)
   - 缺乏对安全区中心位置的明确奖励
2. **食物选择没考虑战略位置**

   - `processFoodWithDynamicPriority`函数(3394行)在选择目标时不考虑食物位置的战略价值
   - 过于关注食物价值和短距离，忽视了可能导致蛇被引导到角落
3. **路径规划缺乏对开阔区域的偏好**

   - 现有路径安全评估专注于避开危险，但没有积极寻找开阔区域
   - 缺少对中长期移动空间的规划

## 优化方案

### 1. 增强中心区域偏好

```cpp
// 在safeZoneCenterScore函数(1334行)中增加中心奖励
// 计算到区域中心的距离
int center_x = (zone.x_min + zone.x_max) / 2;
int center_y = (zone.y_min + zone.y_max) / 2;
int dist_to_center = abs(x - center_x) + abs(y - center_y);

// 区域直径
int zone_diameter = max(zone.x_max - zone.x_min, zone.y_max - zone.y_min);
// 根据到中心的距离比例给予奖励，越靠近中心奖励越高
double center_bonus = 50.0 * (1.0 - min(1.0, (double)dist_to_center / (zone_diameter / 2)));
score += center_bonus;
```

### 2. 增加战略移动空间评估

```cpp
// 在evaluatePathSafety函数中添加移动空间评估(第2590行附近)
// 计算点位的开阔度
double openness_score = 0;
int open_cells = 0;
int max_range = 3; // 检查周围3格范围内的开阔度

// 使用BFS计算周围可达点数量
unordered_set<string> visited;
queue<pair<Point, int>> q;
q.push({point, 0});
visited.insert(Utils::idx2str({point.y, point.x}));

while (!q.empty()) {
    auto [curr, depth] = q.front(); q.pop();
    if (depth > max_range) continue;
  
    open_cells++;
  
    for (auto dir : validDirections) {
        auto [ny, nx] = Utils::nextPos({curr.y, curr.x}, dir);
        string key = Utils::idx2str({ny, nx});
      
        if (Utils::boundCheck(ny, nx) && map_item[ny][nx] != -4 && map_snake[ny][nx] != -5 && 
            visited.find(key) == visited.end()) {
            q.push({{ny, nx}, depth + 1});
            visited.insert(key);
        }
    }
}

// 根据开阔程度评分
int max_possible_cells = (2*max_range+1) * (2*max_range+1);
openness_score = 30.0 * ((double)open_cells / max_possible_cells);
point_safety += openness_score;
```

### 3. 食物战略价值评估

```cpp
// 在processFoodWithDynamicPriority函数中添加战略位置评估(第3660行附近)
// 计算食物位置的战略价值
int zone_width = state.current_safe_zone.x_max - state.current_safe_zone.x_min;
int zone_height = state.current_safe_zone.y_max - state.current_safe_zone.y_min;
int center_x = (state.current_safe_zone.x_min + state.current_safe_zone.x_max) / 2;
int center_y = (state.current_safe_zone.y_min + state.current_safe_zone.y_max) / 2;

// 计算食物到中心的距离占区域半径的比例
double dist_to_center = abs(item.pos.x - center_x) + abs(item.pos.y - center_y);
double center_factor = max(0.0, 1.0 - dist_to_center / (max(zone_width, zone_height) / 2.0));

// 计算食物到边缘的距离
int min_border_dist = min({
    item.pos.x - state.current_safe_zone.x_min,
    state.current_safe_zone.x_max - item.pos.x,
    item.pos.y - state.current_safe_zone.y_min,
    state.current_safe_zone.y_max - item.pos.y
});
double edge_penalty = min_border_dist <= 2 ? 0.8 : 1.0; // 靠近边缘降低20%价值

// 计算最终战略系数
double strategic_factor = center_factor * edge_penalty;

// 应用到食物价值评估
final_value *= (1.0 + center_factor * 0.3); // 增加最多30%的中心奖励
```

### 4. 死角检测和避免

```cpp
// 在analyzeTerrainRisk函数中增强角落检测(2098行附近)
// 检测角落 - 增强型
bool is_corner = false;
int wall_directions = 0;
Direction last_wall_dir;
  
// 计算周围墙的方向
for (auto dir : validDirections) {
    const auto [ny, nx] = Utils::nextPos({y, x}, dir);
    if (!Utils::boundCheck(ny, nx) || map_item[ny][nx] == -4) {
        wall_directions++;
        last_wall_dir = dir;
    }
}

// 如果有两个及以上方向是墙，检查是否形成角落
if (wall_directions >= 2) {
    is_corner = true;
    result.risk_score -= 600; // 角落惩罚
  
    // 如果是死角(三面墙)，给予更严厉的惩罚
    if (wall_directions >= 3) {
        result.risk_score -= 1200;
    }
}
```

实现这些修改后，蛇将更倾向于:

1. 主动保持在安全区中央区域
2. 选择前往战略价值更高(更开阔)的区域
3. 避开危险的角落和窄通道
4. 在选择食物时同时考虑位置的战略价值

这些改进是微妙且高效的，不需要添加大量代码，而是巧妙地增强现有评估函数的战略眼光。
