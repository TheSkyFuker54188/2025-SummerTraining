# 贪吃蛇AI实现方案

## 1. 整体架构

### 1.1 核心类设计

```cpp
// 目标结构体
struct Target {
  Point pos;
  int value;  // 目标价值
  int distance;  // 到目标的距离
  double priority;  // 综合优先级
};

// 地图分析类
class MapAnalyzer {
public:
  void analyze(const GameState& state);
  bool is_safe(const Point& p);
  // 其他辅助方法...
private:
  int danger_map[MAXN][MAXM];  // 地图危险度
};

// 路径规划类
class PathFinder {
public:
  vector<Point> find_path(const Point& start, const Point& goal, const GameState& state);
  bool can_move_directly(const Point& start, const Point& goal);
  // 其他辅助方法...
};

// 主决策类
class SnakeAI {
public:
  int make_decision(const GameState& state);
  string memory_data;  // 记忆数据
private:
  Target select_target(const GameState& state);
  bool should_use_shield(const GameState& state);
  int evaluate_direction(int direction, const GameState& state);
  bool is_direction_safe(int direction, const GameState& state);
  void save_memory(const GameState& state, int decision);
  void read_memory(const GameState& state);
  // 成员变量
  MapAnalyzer map_analyzer;
  PathFinder path_finder;
  Target current_target;
  vector<Point> current_path;
};
```

## 2. 核心算法实现

### 2.1 地图分析

```cpp
void MapAnalyzer::analyze(const GameState& state) {
  // 初始化危险度矩阵
  memset(danger_map, 0, sizeof(danger_map));
  
  // 标记安全区外为极度危险
  for (int x = 0; x < MAXN; x++) {
    for (int y = 0; y < MAXM; y++) {
      if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max ||
          y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max) {
        danger_map[x][y] = 255;  // 最高危险度
      }
    }
  }
  
  // 标记其他蛇的位置为危险区域
  for (const auto& snake : state.snakes) {
    if (snake.id == MYID) continue;
    
    // 蛇身体
    for (const auto& body_part : snake.body) {
      danger_map[body_part.x][body_part.y] = 100;
    }
    
    // 预测蛇头可能移动的位置
    const auto& head = snake.get_head();
    for (int dir = 0; dir < 4; dir++) {
      Point next = get_next_position(head, dir);
      danger_map[next.x][next.y] += 80;  // 可能移动的位置危险度较高
    }
  }
  
  // 标记陷阱为危险区域
  for (const auto& item : state.items) {
    if (item.value == -2) {  // 陷阱
      danger_map[item.pos.x][item.pos.y] = 80;
    }
  }
  
  // 分析安全区即将收缩的情况
  if (state.next_shrink_tick > 0) {
    int current_tick = MAX_TICKS - state.remaining_ticks;
    if (state.next_shrink_tick - current_tick < 20) {
      // 安全区即将收缩，标记边界区域为高危
      mark_shrinking_areas(state);
    }
  }
}
```

### 2.2 路径规划

```cpp
vector<Point> PathFinder::find_path(const Point& start, const Point& goal, const GameState& state) {
  // A*算法实现
  
  struct Node {
    Point pos;
    int g_cost;  // 从起点到当前点的代价
    int h_cost;  // 启发式估计到目标的代价
    int f_cost;  // f = g + h
    Node* parent;
    
    Node(Point p, int g, int h, Node* par) : 
      pos(p), g_cost(g), h_cost(h), f_cost(g + h), parent(par) {}
      
    bool operator<(const Node& other) const {
      return f_cost > other.f_cost;  // 优先队列需要用大于号
    }
  };
  
  // 关键：考虑蛇只能在整格点(坐标为20倍数+3)改变方向
  bool is_grid_point(const Point& p) {
    return ((p.x - 3) % 20 == 0) && ((p.y - 3) % 20 == 0);
  }
  
  // A*算法主体
  priority_queue<Node> open_list;
  unordered_set<string> closed_set;
  // ...A*算法实现...
  
  // 构建路径
  vector<Point> path;
  if (found_path) {
    Node* current = goal_node;
    while (current != nullptr) {
      path.push_back(current->pos);
      current = current->parent;
    }
    reverse(path.begin(), path.end());
  }
  
  return path;
}
```

### 2.3 目标选择

```cpp
Target SnakeAI::select_target(const GameState& state) {
  vector<Target> potential_targets;
  
  // 添加食物作为潜在目标
  for (const auto& item : state.items) {
    // 普通食物和增长豆
    if (item.value > 0 || item.value == -1) {
      Target t;
      t.pos = item.pos;
      t.value = item.value > 0 ? item.value : 3;  // 增长豆价值设为3
      t.distance = manhattan_distance(state.get_self().get_head(), item.pos);
      
      // 计算优先级：价值/距离的比率，再考虑安全因素
      double safety_factor = map_analyzer.is_safe(item.pos) ? 1.0 : 0.2;
      t.priority = (double)t.value / t.distance * safety_factor;
      
      potential_targets.push_back(t);
    }
  }
  
  // 钥匙处理
  for (const auto& key : state.keys) {
    if (key.holder_id == -1) {  // 钥匙在地上
      Target t;
      t.pos = key.pos;
      t.value = 15;  // 钥匙价值设定高一些
      t.distance = manhattan_distance(state.get_self().get_head(), key.pos);
      t.priority = (double)t.value / t.distance;
      
      if (map_analyzer.is_safe(key.pos)) {
        potential_targets.push_back(t);
      }
    }
  }
  
  // 宝箱处理
  if (state.get_self().has_key && !state.chests.empty()) {
    Target t;
    t.pos = state.chests[0].pos;
    t.value = state.chests[0].score;
    t.distance = manhattan_distance(state.get_self().get_head(), t.pos);
    t.priority = t.value / (t.distance * 0.5);  // 宝箱优先级更高
    
    if (map_analyzer.is_safe(t.pos)) {
      potential_targets.push_back(t);
    }
  }
  
  // 选择优先级最高的目标
  if (!potential_targets.empty()) {
    sort(potential_targets.begin(), potential_targets.end(), 
        [](const Target& a, const Target& b) { return a.priority > b.priority; });
    return potential_targets[0];
  }
  
  // 如果没有合适目标，返回安全区中心
  Target default_target;
  default_target.pos = {
    (state.current_safe_zone.y_max + state.current_safe_zone.y_min) / 2,
    (state.current_safe_zone.x_max + state.current_safe_zone.x_min) / 2
  };
  default_target.priority = 0;
  return default_target;
}
```

### 2.4 决策逻辑

```cpp
int SnakeAI::make_decision(const GameState& state) {
  // 读取上一回合存储的记忆
  read_memory(state);
  
  // 分析地图
  map_analyzer.analyze(state);
  
  // 检查当前路径是否仍然有效
  bool need_new_path = current_path.empty() || 
                      !is_target_still_valid(current_target, state) ||
                      !is_path_still_safe(current_path, state);
  
  // 如果需要，选择新目标并规划路径
  if (need_new_path) {
    current_target = select_target(state);
    current_path = path_finder.find_path(state.get_self().get_head(), current_target.pos, state);
  }
  
  // 确定下一步方向
  int direction = -1;
  
  // 检查是否应该激活护盾
  if (should_use_shield(state)) {
    direction = 4;  // 激活护盾
  }
  else if (!current_path.empty()) {
    // 获取路径中的下一个点
    Point next_point = current_path[0];
    current_path.erase(current_path.begin());
    
    // 计算方向
    direction = get_next_direction(state.get_self().get_head(), next_point);
    
    // 安全检查
    if (!is_direction_safe(direction, state)) {
      direction = find_safe_direction(state);
    }
  }
  else {
    // 找安全方向
    direction = find_safe_direction(state);
  }
  
  // 保存记忆数据
  save_memory(state, direction);
  
  return direction;
}
```

### 2.5 护盾策略

```cpp
bool SnakeAI::should_use_shield(const GameState& state) {
  const auto& self = state.get_self();
  
  // 护盾条件检查
  if (self.shield_cd > 0 || self.score < 20 || self.shield_time > 0) {
    return false;
  }
  
  // 危险情况检查：
  
  // 1. 周围直接威胁
  if (check_immediate_danger(state)) return true;
  
  // 2. 即将离开安全区
  if (is_about_to_leave_safe_zone(state)) return true;
  
  // 3. 需要穿过危险区域到达高价值目标
  if (need_to_cross_danger(state, current_path)) return true;
  
  // 4. 其他蛇即将相撞
  if (is_collision_imminent(state)) return true;
  
  return false;
}
```

### 2.6 记忆系统

```cpp
void SnakeAI::save_memory(const GameState& state, int decision) {
  stringstream ss;
  
  // 保存当前目标
  ss << current_target.pos.y << " " << current_target.pos.x << " ";
  
  // 保存路径点数量和部分路径点
  ss << min(current_path.size(), size_t(5)) << " ";
  for (size_t i = 0; i < min(current_path.size(), size_t(5)); i++) {
    ss << current_path[i].y << " " << current_path[i].x << " ";
  }
  
  // 保存上一个决策
  ss << decision << " ";
  
  // 保存游戏阶段信息
  int current_tick = MAX_TICKS - state.remaining_ticks;
  ss << current_tick << " ";
  
  memory_data = ss.str();
}

void SnakeAI::read_memory(const GameState& state) {
  // 第一个tick没有记忆数据
  if (state.remaining_ticks == MAX_TICKS) return;
  
  string memory_line;
  if (getline(cin, memory_line)) {
    stringstream ss(memory_line);
    
    // 读取上次保存的目标
    Point target_pos;
    if (ss >> target_pos.y >> target_pos.x) {
      current_target.pos = target_pos;
    }
    
    // 读取路径信息
    int path_size;
    if (ss >> path_size) {
      current_path.clear();
      for (int i = 0; i < path_size; i++) {
        Point p;
        if (ss >> p.y >> p.x) {
          current_path.push_back(p);
        }
      }
    }
    
    // 读取其他信息...
  }
}
```

## 3. 主函数

```cpp
int main() {
  // 读取当前tick的游戏状态
  GameState current_state;
  read_game_state(current_state);
  
  // 创建AI实例并获取决策
  static SnakeAI ai;  // 使用静态变量保持状态
  int decision = ai.make_decision(current_state);
  
  // 输出决策
  cout << decision << endl;
  
  // 输出记忆数据
  if (!ai.memory_data.empty()) {
    cout << ai.memory_data;
  }
  
  return 0;
}
```

## 4. 辅助功能

### 4.1 方向计算

```cpp
int get_next_direction(const Point& current, const Point& next) {
  if (next.x < current.x) return 0;  // 左
  if (next.y < current.y) return 1;  // 上
  if (next.x > current.x) return 2;  // 右
  if (next.y > current.y) return 3;  // 下
  return -1;  // 错误
}
```

### 4.2 安全检查

```cpp
bool is_direction_safe(int direction, const GameState& state) {
  const auto& head = state.get_self().get_head();
  Point next = get_next_position(head, direction);
  
  // 检查是否在安全区内
  if (next.x < state.current_safe_zone.x_min || next.x > state.current_safe_zone.x_max ||
      next.y < state.current_safe_zone.y_min || next.y > state.current_safe_zone.y_max) {
    return false;
  }
  
  // 检查是否与其他蛇碰撞
  for (const auto& snake : state.snakes) {
    if (snake.id == MYID) continue;
    for (const auto& body_part : snake.body) {
      if (next.x == body_part.x && next.y == body_part.y) {
        return false;
      }
    }
  }
  
  return true;
}
```

### 4.3 位置计算

```cpp
Point get_next_position(const Point& p, int direction) {
  switch (direction) {
    case 0: return {p.y, p.x - 1};  // 左
    case 1: return {p.y - 1, p.x};  // 上
    case 2: return {p.y, p.x + 1};  // 右
    case 3: return {p.y + 1, p.x};  // 下
    default: return p;
  }
}
```

## 5. 实现优先级和计划

### 5.1 阶段一：基础功能

- [ ] 安全移动：避免撞墙和撞蛇
- [ ] 简单目标选择：就近吃食物
- [ ] 安全区边界检测
- [ ] 方向安全性评估

### 5.2 阶段二：核心功能

- [ ] 完整的地图分析系统
- [ ] A*路径规划算法
- [ ] 目标价值评估系统
- [ ] 护盾使用基本策略
- [ ] 记忆系统基础实现

### 5.3 阶段三：高级功能

- [ ] 宝箱和钥匙策略
- [ ] 智能护盾使用
- [ ] 对手行为预测
- [ ] 安全区收缩应对策略
- [ ] 记忆系统的高级应用

### 5.4 阶段四：优化和测试

- [ ] 算法性能优化
- [ ] 特殊情况处理
- [ ] 策略参数调优
- [ ] 全面测试和BUG修复

## 6. 注意事项

1. **整格点转向限制**：蛇只能在坐标为20倍数+3的点改变方向，这对路径规划至关重要
2. **安全区收缩**：需要提前规划，避免被收缩区域困住
3. **护盾使用时机**：护盾是宝贵资源，需在关键时刻使用
4. **记忆系统容量**：最大4KB，需合理规划存储内容

## 7. 调试技巧

1. 在记忆数据中存储关键调试信息
2. 使用状态跟踪记录决策逻辑
3. 测试不同参数对策略的影响
4. 针对特定场景进行单元测试
