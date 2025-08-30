# 改进后的地图多维度分析架构

## 1. Map类多维度分析结构

```cpp
// 地图分析核心类
class Map {
public:
  // 初始化地图分析
  void analyze(const GameState& state);
  
  // 三个维度的评估接口
  int danger(const Point& p) const;   // 危险度 (0-255)
  int value(const Point& p) const;    // 价值度 (0-255)
  int central(const Point& p) const;  // 居中度 (0-255)
  
  // 综合评估 - 安全性、价值和中心位置
  bool is_safe(const Point& p) const;
  float score(const Point& p) const;  // 综合得分
  
private:
  // 三个维度的地图
  int danger_map[MAXM][MAXN];   // 危险地图
  int value_map[MAXM][MAXN];    // 价值地图
  int central_map[MAXM][MAXN];  // 中心地图
  
  // 分析方法
  void analyze_danger(const GameState& state);
  void analyze_value(const GameState& state);
  void analyze_central(const GameState& state);
};
```

## 2. 多维度分析实现

```cpp
void Map::analyze(const GameState& state) {
  // 清空地图
  memset(danger_map, 0, sizeof(danger_map));
  memset(value_map, 0, sizeof(value_map));
  memset(central_map, 0, sizeof(central_map));
  
  // 分别分析三个维度
  analyze_danger(state);
  analyze_value(state);
  analyze_central(state);
}

void Map::analyze_danger(const GameState& state) {
  // 1. 标记安全区外区域为极度危险
  for (int y = 0; y < MAXM; y++) {
    for (int x = 0; x < MAXN; x++) {
      if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max ||
          y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max) {
        danger_map[y][x] = 255;  // 极度危险
      }
    }
  }
  
  // 2. 标记墙体边缘区域
  int wall_buffer = Config::get().wall_buffer;
  for (int y = 0; y < MAXM; y++) {
    for (int x = 0; x < MAXN; x++) {
      if (x <= wall_buffer || x >= MAXN - wall_buffer - 1 || 
          y <= wall_buffer || y >= MAXM - wall_buffer - 1) {
        danger_map[y][x] += 100;  // 靠近墙体危险
      }
    }
  }
  
  // 3. 标记其他蛇的位置和可能移动区域
  for (const auto& snake : state.snakes) {
    if (snake.id == MYID) continue;
    
    // 蛇身体
    for (const auto& body : snake.body) {
      danger_map[body.y][body.x] = 200;
      
      // 周围区域也有一定危险
      for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
          int nx = body.x + dx;
          int ny = body.y + dy;
          if (nx >= 0 && nx < MAXN && ny >= 0 && ny < MAXM) {
            danger_map[ny][nx] += 50;
          }
        }
      }
    }
    
    // 蛇头可能移动的区域
    const auto& head = snake.get_head();
    for (int dir = 0; dir < 4; dir++) {
      Point next = Utils::next_pos(head, dir);
      if (next.y >= 0 && next.y < MAXM && next.x >= 0 && next.x < MAXN) {
        danger_map[next.y][next.x] += 100;
      }
    }
  }
  
  // 4. 标记陷阱和危险物品
  for (const auto& item : state.items) {
    if (item.value == -2) {  // 陷阱
      danger_map[item.pos.y][item.pos.x] += 150;
    }
  }
  
  // 5. 处理安全区收缩
  if (state.next_shrink_tick > 0) {
    int current_tick = MAX_TICKS - state.remaining_ticks;
    int ticks_until_shrink = state.next_shrink_tick - current_tick;
    
    if (ticks_until_shrink <= 20) {
      // 标记即将被收缩的区域
      for (int y = 0; y < MAXM; y++) {
        for (int x = 0; x < MAXN; x++) {
          if ((x >= state.current_safe_zone.x_min && x <= state.current_safe_zone.x_max &&
               y >= state.current_safe_zone.y_min && y <= state.current_safe_zone.y_max) &&
              (x < state.next_safe_zone.x_min || x > state.next_safe_zone.x_max ||
               y < state.next_safe_zone.y_min || y > state.next_safe_zone.y_max)) {
            
            int danger_level = ticks_until_shrink <= 5 ? 200 : 100;
            danger_map[y][x] += danger_level;
          }
        }
      }
    }
  }
}

void Map::analyze_value(const GameState& state) {
  // 1. 标记食物价值
  for (const auto& item : state.items) {
    if (item.value > 0) {  // 普通食物
      value_map[item.pos.y][item.pos.x] = item.value * 30;  // 价值按食物分数比例
    }
    else if (item.value == -1) {  // 增长豆
      value_map[item.pos.y][item.pos.x] = 90;  // 增长豆价值
    }
  }
  
  // 2. 标记钥匙价值
  for (const auto& key : state.keys) {
    if (key.holder_id == -1) {  // 钥匙在地上
      value_map[key.pos.y][key.pos.x] = 150;  // 钥匙高价值
      
      // 如果有宝箱，附近区域也有价值
      if (!state.chests.empty()) {
        for (int dy = -3; dy <= 3; dy++) {
          for (int dx = -3; dx <= 3; dx++) {
            int nx = key.pos.x + dx;
            int ny = key.pos.y + dy;
            if (nx >= 0 && nx < MAXN && ny >= 0 && ny < MAXM) {
              value_map[ny][nx] += 20;  // 钥匙周围区域也有一定价值
            }
          }
        }
      }
    }
  }
  
  // 3. 标记宝箱价值 (如果有钥匙)
  if (state.get_self().has_key && !state.chests.empty()) {
    for (const auto& chest : state.chests) {
      value_map[chest.pos.y][chest.pos.x] = 200;  // 宝箱最高价值
      
      // 宝箱周围区域也有价值
      for (int dy = -5; dy <= 5; dy++) {
        for (int dx = -5; dx <= 5; dx++) {
          int nx = chest.pos.x + dx;
          int ny = chest.pos.y + dy;
          if (nx >= 0 && nx < MAXN && ny >= 0 && ny < MAXM) {
            int dist = abs(dx) + abs(dy);
            value_map[ny][nx] += 50 / (1 + dist);  // 距离越近价值越高
          }
        }
      }
    }
  }
}

void Map::analyze_central(const GameState& state) {
  // 计算当前安全区的中心
  int center_x = (state.current_safe_zone.x_min + state.current_safe_zone.x_max) / 2;
  int center_y = (state.current_safe_zone.y_min + state.current_safe_zone.y_max) / 2;
  
  // 如果即将收缩，使用下一个安全区的中心
  int current_tick = MAX_TICKS - state.remaining_ticks;
  if (state.next_shrink_tick > 0 && state.next_shrink_tick - current_tick <= 15) {
    center_x = (state.next_safe_zone.x_min + state.next_safe_zone.x_max) / 2;
    center_y = (state.next_safe_zone.y_min + state.next_safe_zone.y_max) / 2;
  }
  
  // 计算每个点到中心的距离，转换为中心性得分
  int max_dist = MAXN + MAXM;  // 最大可能距离
  for (int y = 0; y < MAXM; y++) {
    for (int x = 0; x < MAXN; x++) {
      int dist = abs(x - center_x) + abs(y - center_y);
      
      // 距离越近，中心性越高 (255是最高分)
      central_map[y][x] = 255 * (1.0 - (float)dist / max_dist);
    }
  }
}
```

## 3. 综合评估功能

```cpp
bool Map::is_safe(const Point& p) const {
  // 基本有效性检查
  if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN) {
    return false;
  }
  
  // 危险阈值判断
  return danger_map[p.y][p.x] < Config::get().safe_threshold;
}

float Map::score(const Point& p) const {
  // 基本有效性检查
  if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN) {
    return -1000.0f;  // 无效位置，极低分
  }
  
  // 极度危险的位置直接返回负分
  if (danger_map[p.y][p.x] >= 200) {
    return -500.0f;
  }
  
  // 三个维度加权计算得分
  float danger_score = 1.0f - danger_map[p.y][p.x] / 255.0f;  // 危险度越低越好
  float value_score = value_map[p.y][p.x] / 255.0f;  // 价值度越高越好
  float central_score = central_map[p.y][p.x] / 255.0f;  // 中心度越高越好
  
  // 加权组合
  float score = danger_score * 5.0f + value_score * 3.0f + central_score * 1.0f;
  return score;
}
```

## 4. 路径规划与Map类集成

```cpp
class PathFinder {
public:
  PathFinder(Map* map) : map(map) {}
  
  vector<Point> find_path(const Point& start, const Point& goal, const GameState& state) {
    // 使用A*算法，但启发式考虑多维度地图信息
    // ...
    
    // 计算节点评估函数
    float evaluate_node(const Node& node) {
      // 基础距离代价
      float base_cost = node.g_cost + node.h_cost;
      
      // 多维度地图评分
      float map_score = map->score(node.pos);
      
      return base_cost - map_score * 0.5f;  // 地图评分越高越好
    }
  }
  
private:
  Map* map;
};
```

## 5. Snake AI使用多维度地图

```cpp
class SnakeAI {
public:
  SnakeAI() {
    map = new Map();
    path_finder = new PathFinder(map);
  }
  
  ~SnakeAI() {
    delete path_finder;
    delete map;
  }
  
  int make_decision(const GameState& state) {
    // 分析地图
    map->analyze(state);
    
    // 选择目标
    Target target = select_target(state);
    
    // 路径规划
    vector<Point> path = path_finder->find_path(
        state.get_self().get_head(), target.pos, state);
    
    // 方向决策
    // ...
  }
  
private:
  Map* map;
  PathFinder* path_finder;
  
  Target select_target(const GameState& state) {
    const auto& head = state.get_self().get_head();
    vector<Target> candidates;
    
    // 遍历地图找出高价值点
    for (int y = 0; y < MAXM; y++) {
      for (int x = 0; x < MAXN; x++) {
        Point p = {y, x};
        
        // 只考虑安全的点
        if (!map->is_safe(p)) continue;
        
        // 计算位置综合得分
        float pos_score = map->score(p);
        if (pos_score <= 0) continue;
        
        // 考虑距离因素
        int distance = Utils::manhattan(head, p);
        float dist_factor = distance <= 5 ? 10.0f / (distance + 1) : 5.0f / distance;
        
        Target t;
        t.pos = p;
        t.value = map->value(p);
        t.distance = distance;
        t.priority = pos_score * dist_factor;
        
        candidates.push_back(t);
      }
    }
    
    // 选择最佳目标
    if (!candidates.empty()) {
      sort(candidates.begin(), candidates.end(), 
           [](const Target& a, const Target& b) { return a.priority > b.priority; });
      return candidates[0];
    }
    
    // 默认去安全区中心
    Target default_target;
    default_target.pos = {
      (state.current_safe_zone.y_min + state.current_safe_zone.y_max) / 2,
      (state.current_safe_zone.x_min + state.current_safe_zone.x_max) / 2
    };
    default_target.priority = 0;
    return default_target;
  }
};
```

## 6. 主要优势

1. **多维度评估**：
   - 危险度、价值度和中心度三个维度全面评估地图
   - 可以根据不同场景调整三个维度的权重

2. **简洁的设计**：
   - Map类统一管理地图分析
   - 清晰的接口设计，简单易用

3. **灵活的决策支持**：
   - 支持复杂的启发式路径规划
   - 灵活的目标选择机制

4. **强大的分析能力**：
   - 支持区域影响分析
   - 能够动态适应游戏状态变化

5. **可扩展性**：
   - 可以方便地添加新的地图分析维度
   - 调整各维度权重即可适应不同策略

这种设计既保持了简洁性，又提供了强大的分析能力，使AI能够做出更全面、更智能的决策。