# 贪吃蛇实现方案

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
  
  // 标记宝箱为极度危险区域（对未持有钥匙的蛇）
for (const auto& chest : state.chests) {
  if (!state.get_self().has_key) {
    danger_map[chest.pos.x][chest.pos.y] = 255;  // 最高危险度，触碰即死，护盾无效
    
    /* // 宝箱周围也标记为高危区域，避免误入
    for (int dx = -1; dx <= 1; dx++) {
      for (int dy = -1; dy <= 1; dy++) {
        int nx = chest.pos.x + dx;
        int ny = chest.pos.y + dy;
        if (nx >= 0 && nx < MAXN && ny >= 0 && ny < MAXM) {
          danger_map[nx][ny] = 150;  // 宝箱周围区域高危
        }
      }
    } */
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

安全区收缩处理
```cpp
void MapAnalyzer::mark_shrinking_areas(const GameState& state) {
  int current_tick = MAX_TICKS - state.remaining_ticks;
  int ticks_until_shrink = state.next_shrink_tick - current_tick;
  
  // 根据当前游戏阶段确定收缩紧急程度
  int danger_level = 0;
  
  if (ticks_until_shrink <= 3) {
    danger_level = 220;  // 非常危险，马上收缩
  } else if (ticks_until_shrink <= 10) {
    danger_level = 150;  // 很危险，快收缩了
  } else if (ticks_until_shrink <= 20) {
    danger_level = 100;  // 中等危险，需要注意
  } else {
    danger_level = 50;   // 低危险，收缩还远
  }
  
  // 标记即将被收缩的区域
  for (int x = 0; x < MAXN; x++) {
    for (int y = 0; y < MAXM; y++) {
      if ((x >= state.current_safe_zone.x_min && x <= state.current_safe_zone.x_max &&
           y >= state.current_safe_zone.y_min && y <= state.current_safe_zone.y_max) &&
          (x < state.next_safe_zone.x_min || x > state.next_safe_zone.x_max ||
           y < state.next_safe_zone.y_min || y > state.next_safe_zone.y_max)) {
        // 这个区域将被收缩
        danger_map[x][y] += danger_level;
      }
    }
  }
  
/*   // 特别处理收缩区域边缘，使蛇尽量不靠近收缩边界
  int buffer = 1;  // 边缘缓冲区大小
  for (int x = state.next_safe_zone.x_min; x <= state.next_safe_zone.x_max; x++) {
    for (int y = state.next_safe_zone.y_min; y <= state.next_safe_zone.y_max; y++) {
      if (x <= state.next_safe_zone.x_min + buffer || 
          x >= state.next_safe_zone.x_max - buffer ||
          y <= state.next_safe_zone.y_min + buffer || 
          y >= state.next_safe_zone.y_max - buffer) {
        danger_map[x][y] += 30;  // 边缘也有一定危险
      }
    }
  }
} */
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
  
  // 整格点判断（基础判断函数）
  // 关键：考虑蛇只能在整格点(坐标为20倍数+3)改变方向
  bool is_grid_point(const Point& p) {
    return ((p.x - 3) % 20 == 0) && ((p.y - 3) % 20 == 0);
  }

  // 移动限制检查（整格点才能转向）
bool can_move_from_to(const Point& from, const Point& to) {
  // 如果不是整格点，只能沿当前方向移动
  if (!is_grid_point(from)) {
    // 计算当前方向
    int current_dir = -1;
    if (from.x % 20 != 3) {
      current_dir = (from.x % 20 > 3) ? 2 : 0;  // 右或左
    } else {
      current_dir = (from.y % 20 > 3) ? 3 : 1;  // 下或上
    }
    
    // 检查目标点是否在当前方向上
    return get_next_position(from, current_dir).x == to.x &&
           get_next_position(from, current_dir).y == to.y;
  }
  return true;  // 整格点可以任意转向
}

// 节点扩展函数（获取可达邻居节点）
vector<Node*> expand_node(Node* current, const GameState& state) {
  vector<Node*> neighbors;
  const auto& current_pos = current->pos;
  
  // 检查是否在整格点上，只有在整格点才能改变方向
  bool can_change_direction = is_grid_point(current_pos);
  
  // 确定当前方向
  int current_direction = -1;
  if (current->parent != nullptr) {
    current_direction = get_next_direction(current->parent->pos, current_pos);
  }
  
  // 尝试各个方向
  for (int dir = 0; dir < 4; dir++) {
    // 如果不是整格点且方向与当前方向不同，则跳过
    if (!can_change_direction && current_direction != -1 && dir != current_direction) {
      continue;
    }
    
    // 计算下一个位置（移动speed个单位）
    Point next_pos = get_next_position(current_pos, dir);
    
    // 检查位置有效性
    if (next_pos.x < 0 || next_pos.x >= MAXN || next_pos.y < 0 || next_pos.y >= MAXM) {
      continue;
    }
    
    // 创建新节点
    neighbors.push_back(new Node(next_pos, ...));
  }
  
  return neighbors;
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
    
    // 考虑安全区收缩和宝箱位置
    if (state.chests.empty()) {
      t.priority = 5.0;  // 没有宝箱时降低钥匙优先级
    } else {
      // 计算钥匙到最近宝箱的距离
      int min_chest_distance = INT_MAX;
      for (const auto& chest : state.chests) {
        int chest_distance = manhattan_distance(key.pos, chest.pos);
        min_chest_distance = min(min_chest_distance, chest_distance);
      }
      
      /* // 根据当前时间段调整钥匙优先级
      int current_tick = MAX_TICKS - state.remaining_ticks;
      double time_factor = current_tick > 200 ? 1.5 : 1.0;  // 后期提高钥匙优先级
       */

      t.priority = (double)t.value / t.distance * time_factor;
      // 如果钥匙离宝箱很近，提高优先级
      if (min_chest_distance < 10) {
        t.priority *= 1.5;
      }
    }
    
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
  
  // 查找当前持有的钥匙
  int remaining_time = 30;  // 默认值
  for (const auto& key : state.keys) {
    if (key.holder_id == MYID) {
      remaining_time = key.remaining_time;
      break;
    }
  }
  
   // 根据剩余持有时间调整优先级
  double time_factor = 1.0;
  if (remaining_time < 10) {
    time_factor = 2.0;  // 钥匙即将掉落，高优先级开宝箱
  } 
  
  t.priority = t.value / (t.distance * 0.5) * time_factor;
  
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
  
  // 检查是否即将有头对头碰撞
  for (const auto& snake : state.snakes) {
    if (snake.id == MYID) continue;
    
    // 计算两蛇头部距离
    const auto& other_head = snake.get_head();
    const auto& my_head = self.get_head();
    int distance = manhattan_distance(my_head, other_head);
    
    // 如果头部距离很近且方向相对，可能发生碰撞
    if (distance <= 4) {
      // 方向相对检查
      int my_dir = self.direction;
      int other_dir = snake.direction;
      
      if ((my_dir == 0 && other_dir == 2) || 
          (my_dir == 2 && other_dir == 0) ||
          (my_dir == 1 && other_dir == 3) ||
          (my_dir == 3 && other_dir == 1)) {
        return true;  // 可能发生头对头碰撞，激活护盾
      }
    }
  }
  
  // 安全区收缩检查
  if (state.next_shrink_tick > 0) {
    int current_tick = MAX_TICKS - state.remaining_ticks;
    int ticks_until_shrink = state.next_shrink_tick - current_tick;
    
    // 安全区即将收缩且自己在危险区域
    if (ticks_until_shrink <= 3) {
      const auto& head = self.get_head();
      if (head.x < state.next_safe_zone.x_min || head.x > state.next_safe_zone.x_max ||
          head.y < state.next_safe_zone.y_min || head.y > state.next_safe_zone.y_max) {
        return true;  // 激活护盾避免被收缩区域杀死
      }
    }
  }
  
 /*  // 当持有钥匙且接近宝箱但无法安全到达时
  if (self.has_key && !state.chests.empty()) {
    const auto& chest = state.chests[0];
    int distance = manhattan_distance(self.get_head(), chest.pos);
    
    if (distance <= 6) {  // 接近宝箱
      // 检查是否有其他蛇可能争夺宝箱
      bool competitors = false;
      for (const auto& snake : state.snakes) {
        if (snake.id == MYID) continue;
        if (snake.has_key && manhattan_distance(snake.get_head(), chest.pos) <= distance) {
          competitors = true;
          break;
        }
      }
      
      if (competitors) {
        return true;  // 激活护盾争夺宝箱
      }
    }
  } */
  
  return false;
}
```
 
### 2.6 记忆系统（暂时不加）
<!--
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
  
  // 记录是否持有钥匙以及持有时间
  ss << (state.get_self().has_key ? 1 : 0) << " ";
  
  // 记录当前最近的钥匙和宝箱信息
  if (!state.keys.empty()) {
    int nearest_key_idx = -1;
    int nearest_distance = INT_MAX;
    for (size_t i = 0; i < state.keys.size(); i++) {
      if (state.keys[i].holder_id == -1) {  // 只考虑地上的钥匙
        int dist = manhattan_distance(state.get_self().get_head(), state.keys[i].pos);
        if (dist < nearest_distance) {
          nearest_distance = dist;
          nearest_key_idx = i;
        }
      }
    }
    
    if (nearest_key_idx != -1) {
      ss << state.keys[nearest_key_idx].pos.y << " " 
         << state.keys[nearest_key_idx].pos.x << " ";
    } else {
      ss << "-1 -1 ";
    }
  } else {
    ss << "-1 -1 ";
  }
  
  //  下一次安全区收缩信息
  ss << state.next_shrink_tick << " ";
  
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
 -->
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
  const auto& self = state.get_self();
  const auto& head = self.get_head();
  Point next = get_next_position(head, direction);
  
  // 检查是否在安全区内
  if (next.x < state.current_safe_zone.x_min || next.x > state.current_safe_zone.x_max ||
      next.y < state.current_safe_zone.y_min || next.y > state.current_safe_zone.y_max) {
    return false;  // 墙壁/安全区外，护盾也无效
  }
  
  // 如果有护盾，大部分碰撞可以忽略
  if (self.shield_time > 0) {
    // 仍需检查宝箱（无钥匙时护盾无效）
    for (const auto& chest : state.chests) {
      if (!self.has_key && next.x == chest.pos.x && next.y == chest.pos.y) {
        return false;
      }
    }
    return true;  // 有护盾，其他碰撞安全
  }
  
  // 检查是否与其他蛇碰撞
  for (const auto& snake : state.snakes) {
    if (snake.id == MYID) continue;
    
    // 检查蛇头碰撞
    const auto& other_head = snake.get_head();
    if (next.x == other_head.x && next.y == other_head.y) {
      // 如果对方有护盾，我方死亡
      if (snake.shield_time > 0) {
        return false;
      }
      // 双方都无护盾，均死亡，但这是可行方向
      // 可以根据策略决定是否执行
    }
    
    // 检查与蛇身体碰撞
    for (size_t j = 1; j < snake.body.size(); j++) {  // 从1开始跳过蛇头
      const auto& body_part = snake.body[j];
      if (next.x == body_part.x && next.y == body_part.y) {
        return false;  // 碰撞到其他蛇的身体
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
    case 0: return {p.y, p.x - 2};  // 左，移动2个单位
    case 1: return {p.y - 2, p.x};  // 上，移动2个单位
    case 2: return {p.y, p.x + 2};  // 右，移动2个单位
    case 3: return {p.y + 2, p.x};  // 下，移动2个单位
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
