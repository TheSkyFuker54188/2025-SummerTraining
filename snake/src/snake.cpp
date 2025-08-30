#include <chrono>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include <cmath>
#include <cstring>
#include <queue>
#include <string>
#include <sstream>

using namespace std;

// 常量
constexpr int MAXN = 40;
constexpr int MAXM = 30;
constexpr int MAX_TICKS = 256;
constexpr int MYID = 2023202295; // 此处替换为你的学号！

struct Point {
  int y, x;
  
  bool operator==(const Point& other) const {
    return y == other.y && x == other.x;
  }
  
  bool operator!=(const Point& other) const {
    return !(*this == other);
  }
};

struct Item {
  Point pos;
  int value;
  int lifetime;
};

struct Snake {
  int id;
  int length;
  int score;
  int direction;
  int shield_cd;
  int shield_time;
  bool has_key;
  vector<Point> body;

  const Point &get_head() const { return body.front(); }
};

struct Chest {
  Point pos;
  int score;
};

struct Key {
  Point pos;
  int holder_id;
  int remaining_time;
};

struct SafeZoneBounds {
  int x_min, y_min, x_max, y_max;
};

struct GameState {
  int remaining_ticks;
  vector<Item> items;
  vector<Snake> snakes;
  vector<Chest> chests;
  vector<Key> keys;
  SafeZoneBounds current_safe_zone;
  int next_shrink_tick;
  SafeZoneBounds next_safe_zone;
  int final_shrink_tick;
  SafeZoneBounds final_safe_zone;

  int self_idx;

  const Snake &get_self() const { return snakes[self_idx]; }
};

// 目标结构体
struct Target {
  Point pos;
  int value;  // 目标价值
  int distance;  // 到目标的距离
  double priority;  // 综合优先级
};

// 辅助函数
int manhattan_distance(const Point& a, const Point& b) {
  return abs(a.x - b.x) + abs(a.y - b.y);
}

bool is_grid_point(const Point& p) {
  return ((p.x - 3) % 20 == 0) && ((p.y - 3) % 20 == 0);
}

Point get_next_position(const Point& p, int direction) {
  switch (direction) {
    case 0: return {p.y, p.x - 2};  // 左，移动2个单位
    case 1: return {p.y - 2, p.x};  // 上，移动2个单位
    case 2: return {p.y, p.x + 2};  // 右，移动2个单位
    case 3: return {p.y + 2, p.x};  // 下，移动2个单位
    default: return p;
  }
}

int get_direction(const Point& from, const Point& to) {
  if (to.x < from.x) return 0;  // 左
  if (to.y < from.y) return 1;  // 上
  if (to.x > from.x) return 2;  // 右
  if (to.y > from.y) return 3;  // 下
  return -1;  // 相同位置
}

// 地图分析类
class MapAnalyzer {
public:
  void analyze(const GameState& state) {
    // 初始化危险度矩阵
    memset(danger_map, 0, sizeof(danger_map));
    
    // 标记地图边界为极度危险（增加边界缓冲区）
    int buffer = 1;  // 墙体缓冲区大小
    int boundary_buffer = 1;  // 安全区内侧缓冲区大小
    
    for (int y = 0; y < MAXM; y++) {
      for (int x = 0; x < MAXN; x++) {
        // 标记绝对禁区 - 安全区外
        if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max ||
            y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max) {
          danger_map[y][x] = 255;  // 极度危险
        }
        
        // 标记安全区内的边界缓冲区
        else if (x < state.current_safe_zone.x_min + boundary_buffer || 
                 x > state.current_safe_zone.x_max - boundary_buffer ||
                 y < state.current_safe_zone.y_min + boundary_buffer ||
                 y > state.current_safe_zone.y_max - boundary_buffer) {
          // 边界内侧缓冲区域也设为高危险度
          danger_map[y][x] = 200;
        }
        
        // 墙体附近区域也标记为高危
        if (x <= buffer || x >= MAXN - buffer - 1 || 
            y <= buffer || y >= MAXM - buffer - 1) {
          danger_map[y][x] = 200;  // 非常高的危险度
        }
      }
    }
    
    // 标记宝箱为极度危险区域（对未持有钥匙的蛇）
    for (const auto& chest : state.chests) {
      if (!state.get_self().has_key) {
        danger_map[chest.pos.y][chest.pos.x] = 255;  // 最高危险度，触碰即死，护盾无效
      }
    }
    
    // 标记其他蛇的位置为危险区域
    for (const auto& snake : state.snakes) {
      if (snake.id == MYID) continue;
    
      // 蛇身体
      for (const auto& body_part : snake.body) {
        danger_map[body_part.y][body_part.x] = 100;
      }
    
      // 预测蛇头可能移动的位置
      const auto& head = snake.get_head();
      for (int dir = 0; dir < 4; dir++) {
        Point next = get_next_position(head, dir);
        if (next.y >= 0 && next.y < MAXM && next.x >= 0 && next.x < MAXN) {
          danger_map[next.y][next.x] += 80;  // 可能移动的位置危险度较高
        }
      }
    }
    
    // 标记陷阱为危险区域
    for (const auto& item : state.items) {
      if (item.value == -2) {  // 陷阱
        danger_map[item.pos.y][item.pos.x] = 80;
      }
    }
    
    // 检查并标记安全区即将收缩的区域
    if (state.next_shrink_tick > 0) {
      int current_tick = MAX_TICKS - state.remaining_ticks;
      if (state.next_shrink_tick - current_tick <= 20) {  // 如果20个tick内将收缩
        mark_shrinking_areas(state);
      }
    }
  }

  bool is_safe(const Point& p) const {
    if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN) {
      return false;
    }
    return danger_map[p.y][p.x] < 80;  // 危险度低于80认为安全
  }
  
  int get_danger_level(const Point& p) const {
    if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN) {
      return 255;  // 越界极度危险
    }
    return danger_map[p.y][p.x];
  }

private:
  int danger_map[MAXM][MAXN];  // 地图危险度
  
  //? 缩圈处理函数
  void mark_shrinking_areas(const GameState& state) {
    int current_tick = MAX_TICKS - state.remaining_ticks;
    int ticks_until_shrink = state.next_shrink_tick - current_tick;
    
    // 根据收缩紧迫程度设置更高危险度
    int danger_level = 0;
    
    // 大幅提高危险度级别
    if (ticks_until_shrink <= 5) {
      danger_level = 250;  // 即将收缩，极度危险
    } else if (ticks_until_shrink <= 10) {
      danger_level = 200;  // 很快收缩，高度危险
    } else if (ticks_until_shrink <= 20) {
      danger_level = 150;  // 中等危险
    } else {
      danger_level = 100;  // 需要关注
    }
    
    // 标记即将被收缩的区域
    for (int y = 0; y < MAXM; y++) {
      for (int x = 0; x < MAXN; x++) {
        if ((x >= state.current_safe_zone.x_min && x <= state.current_safe_zone.x_max &&
             y >= state.current_safe_zone.y_min && y <= state.current_safe_zone.y_max) &&
            (x < state.next_safe_zone.x_min || x > state.next_safe_zone.x_max ||
             y < state.next_safe_zone.y_min || y > state.next_safe_zone.y_max)) {
          // 这个区域将被收缩
          danger_map[y][x] = danger_level;  // 直接设置为高危险度，而不是累加
        }
      }
    }
  }
};

//? 路径规划类
class PathFinder {
public:
  PathFinder(const MapAnalyzer& analyzer) : map_analyzer(analyzer) {}
  
  // 简单路径生成
  vector<Point> find_path(const Point& start, const Point& goal, const GameState& state) {
    // 如果起点和终点相同，返回空路径
    if (start.x == goal.x && start.y == goal.y) {
      return {};
    }
    
    // 创建基础路径
    vector<Point> basic_path = generate_basic_path(start, goal, state);
    
    // 优化转弯点
    vector<Point> optimized_path = optimize_turning_points(basic_path, state);
    
    return optimized_path;
  }

private:
  const MapAnalyzer& map_analyzer;
  
  // 生成从起点到终点的基础路径
  vector<Point> generate_basic_path(const Point& start, const Point& goal, const GameState& state) {
    vector<Point> path;
    Point current = start;
    path.push_back(current);
    
    // 简单实现：先水平移动到目标的x坐标，然后垂直移动到目标的y坐标
    // 在实际移动中考虑整格点转向限制
    
    // 找到下一个整格点
    Point next_grid_point = current;
    if (!is_grid_point(current)) {
      // 根据当前方向找到下一个整格点
      int current_dir = state.get_self().direction;
      while (!is_grid_point(next_grid_point)) {
        next_grid_point = get_next_position(next_grid_point, current_dir);
        if (next_grid_point.x < 0 || next_grid_point.x >= MAXN || 
            next_grid_point.y < 0 || next_grid_point.y >= MAXM) {
          // 越界，回退到上一个点
          break;
        }
        path.push_back(next_grid_point);
      }
      current = next_grid_point;
    }
    
    // 水平移动到目标的x坐标
    while (abs(current.x - goal.x) > 2) {
      int dir = (current.x < goal.x) ? 2 : 0; // 右或左
      Point next = get_next_position(current, dir);
      
      // 检查点是否安全
      if (!map_analyzer.is_safe(next)) {
        break; // 不安全，停止路径规划
      }
      
      path.push_back(next);
      current = next;
    }
    
    // 垂直移动到目标的y坐标
    while (abs(current.y - goal.y) > 2) {
      int dir = (current.y < goal.y) ? 3 : 1; // 下或上
      Point next = get_next_position(current, dir);
      
      // 检查点是否安全
      if (!map_analyzer.is_safe(next)) {
        break; // 不安全，停止路径规划
      }
      
      path.push_back(next);
      current = next;
    }
    
    // 最后添加目标点
    if (current.x != goal.x || current.y != goal.y) {
      path.push_back(goal);
    }
    
    return path;
  }
  
  // 转弯点优化
  vector<Point> optimize_turning_points(const vector<Point>& original_path, const GameState& state) {
    if (original_path.size() < 3) return original_path;
    
    vector<Point> optimized_path;
    optimized_path.push_back(original_path[0]);
    
    for (size_t i = 1; i < original_path.size() - 1; i++) {
      Point prev = original_path[i-1];
      Point current = original_path[i];
      Point next = original_path[i+1];
      
      // 检测是否为转弯点
      int dir_in = get_direction(prev, current);
      int dir_out = get_direction(current, next);
      
      if (dir_in != dir_out && is_grid_point(current)) {
        // 这是个转弯点，尝试优化
        Point early_turn = find_early_turning_point(prev, current, next, state);
        if (early_turn.x != -1 && early_turn.y != -1) {  // 有效点
          optimized_path.push_back(early_turn);
        } else {
          optimized_path.push_back(current);  // 无法优化，保持原转弯点
        }
      } else {
        optimized_path.push_back(current);
      }
    }
    
    // 添加终点
    if (!original_path.empty()) {
      optimized_path.push_back(original_path.back());
    }
    
    return optimized_path;
  }
  
  // 查找可能的提前转向点
  Point find_early_turning_point(const Point& prev, const Point& turn, const Point& next, const GameState& state) {
    // 获取进入和输出方向
    int dir_in = get_direction(prev, turn);
    int dir_out = get_direction(turn, next);
    
    // 检查是否是有效的转向（直角转向）
    if (abs(dir_in - dir_out) % 2 != 1) {
      return {-1, -1};  // 不是直角转向，返回无效点
    }
    
    // 尝试找到一个更早的转向点
    // 从原转向点向前退1-2步
    for (int step = 2; step <= 6; step += 2) {
      Point candidate;
      
      // 根据进入方向，从转弯点向后退step步
      switch (dir_in) {
        case 0: candidate = {turn.y, turn.x + step}; break;  // 从左来，向右退
        case 1: candidate = {turn.y + step, turn.x}; break;  // 从上来，向下退
        case 2: candidate = {turn.y, turn.x - step}; break;  // 从右来，向左退
        case 3: candidate = {turn.y - step, turn.x}; break;  // 从下来，向上退
        default: return {-1, -1};
      }
      
      // 检查候选点是否安全
      if (map_analyzer.is_safe(candidate)) {
        // 检查从候选点到next是否有安全路径
        Point after_turn = get_next_position(candidate, dir_out);
        if (map_analyzer.is_safe(after_turn)) {
          return candidate;
        }
      }
    }
    
    return {-1, -1};  // 没有找到合适的提前转向点
  }
};

// 主决策类
class SnakeAI {
public:
  SnakeAI() : path_finder(map_analyzer) {}
  
  int make_decision(const GameState& state) {
    // 分析地图
    map_analyzer.analyze(state);
    
    // 选择目标
    Target target = select_target(state);
    
    // 规划路径
    const auto& head = state.get_self().get_head();
    current_path = path_finder.find_path(head, target.pos, state);
    
    // 如果有路径，按路径行动
    if (!current_path.empty()) {
      // 获取下一个点
      Point next_point = current_path[0];
      int next_dir = get_direction(head, next_point);
      
      // 检查方向是否安全
      if (next_dir != -1 && is_direction_safe(next_dir, state)) {
        return next_dir;
      }
    }
    
    // 评估各个方向的安全性并选择最佳方向
    vector<pair<int, double>> direction_scores;
    for (int dir = 0; dir < 4; dir++) {
      double score = evaluate_direction(dir, state, target);
      direction_scores.push_back({dir, score});
    }
    
    // 按分数降序排序
    sort(direction_scores.begin(), direction_scores.end(), 
         [](const pair<int, double>& a, const pair<int, double>& b) {
           return a.second > b.second;
         });
    
    // 检查是否应该激活护盾
    if (should_use_shield(state)) {
      return 4;  // 激活护盾
    }
    
    // 选择最佳方向
    for (const auto& [dir, score] : direction_scores) {
      if (score > 0) {  // 正分数表示安全方向
        return dir;
      }
    }
    
    // 如果没有安全方向，随机选择一个（作为后备）
    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> dist(0, 3);
    return dist(rng);
  }

private:
  Target select_target(const GameState& state) {
    vector<Target> potential_targets;
    const auto& self = state.get_self();
    const auto& head = self.get_head();
    
    // 即时反应机制：检测头部极近的食物并立即响应
    const int immediate_response_distance = 3; // 头部极近的定义（3格内）
    for (const auto& item : state.items) {
      if ((item.value > 0 || item.value == -1) && map_analyzer.is_safe(item.pos)) {
        int dist = manhattan_distance(head, item.pos);
        if (dist <= immediate_response_distance) {
          // 发现头部附近的食物，直接返回为最高优先级目标
          Target immediate_target;
          immediate_target.pos = item.pos;
          immediate_target.value = item.value > 0 ? item.value : 3;
          immediate_target.distance = dist;
          immediate_target.priority = 1000.0; // 极高优先级
          return immediate_target; // 立即返回，不再考虑其他目标
        }
      }
    }
    
    // 添加食物作为潜在目标（头部极近食物处理后的常规逻辑）
    for (const auto& item : state.items) {
      // 普通食物和增长豆
      if ((item.value > 0 || item.value == -1) && map_analyzer.is_safe(item.pos)) {
        Target t;
        t.pos = item.pos;
        t.value = item.value > 0 ? item.value : 3;  // 增长豆价值设为3
        t.distance = manhattan_distance(head, item.pos);
        
        // 计算优先级：优化近距离食物的权重
        double safety_factor = map_analyzer.is_safe(item.pos) ? 1.0 : 0.2;
        
        // 非线性提升近距离食物的优先级
        double distance_factor;
        if (t.distance <= 5) {
          distance_factor = 10.0 / (t.distance + 1);  // +1避免除以零
        } else {
          distance_factor = 5.0 / t.distance;
        }
        
        t.priority = t.value * distance_factor * safety_factor;
        
        potential_targets.push_back(t);
      }
    }
    
    // 如果有钥匙，考虑去拿钥匙
    if (!self.has_key) {
      for (const auto& key : state.keys) {
        if (key.holder_id == -1 && map_analyzer.is_safe(key.pos)) {  // 钥匙在地上
          Target t;
          t.pos = key.pos;
          t.value = 10;  // 钥匙价值设定高一些
          t.distance = manhattan_distance(head, key.pos);
          t.priority = (double)t.value / (t.distance + 1);
          
          potential_targets.push_back(t);
        }
      }
    }
    
    // 如果持有钥匙且有宝箱，考虑去开宝箱
    if (self.has_key && !state.chests.empty()) {
      Target t;
      t.pos = state.chests[0].pos;
      t.value = state.chests[0].score;
      t.distance = manhattan_distance(head, t.pos);
      t.priority = (double)t.value / (t.distance + 1) * 1.5;  // 宝箱优先级高一些
      
      potential_targets.push_back(t);
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
      (state.current_safe_zone.y_min + state.current_safe_zone.y_max) / 2,
      (state.current_safe_zone.x_min + state.current_safe_zone.x_max) / 2
    };
    default_target.priority = 0;
    return default_target;
  }
  
  double evaluate_direction(int direction, const GameState& state, const Target& target) {
    const auto& self = state.get_self();
    const auto& head = self.get_head();
    Point next_pos = get_next_position(head, direction);
    
    // 检查是否安全
    if (!is_direction_safe(direction, state)) {
      return -1000;  // 非常危险的方向
    }
    
    // 计算到目标的距离变化
    int curr_dist = manhattan_distance(head, target.pos);
    int next_dist = manhattan_distance(next_pos, target.pos);
    double distance_score = curr_dist - next_dist;  // 正值表示靠近目标
    
    // 考虑危险程度
    double danger_score = -map_analyzer.get_danger_level(next_pos) / 50.0;
    
    // 计算转向限制影响
    double turn_score = 0;
    if (is_grid_point(head)) {
      turn_score = 2.0;  // 在格点上可以自由转向，加分
    } else if (!is_grid_point(next_pos)) {
      // 如果不在格点上且下一步也不在格点，考虑是否朝向目标方向
      int target_dir = get_direction(head, target.pos);
      if (direction == target_dir) {
        turn_score = 1.0;
      }
    }
    
    // 综合评分
    return distance_score * 3.0 + danger_score + turn_score;
  }
  
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
        // 通常应该避免这种情况
        return false;
      }
      
      // 检查与蛇身体碰撞
      for (size_t j = 0; j < snake.body.size(); j++) {
        const auto& body_part = snake.body[j];
        if (next.x == body_part.x && next.y == body_part.y) {
          return false;  // 碰撞到其他蛇的身体
        }
      }
    }
    
    // 检查是否与自己的身体碰撞
    for (size_t i = 1; i < self.body.size(); i++) {  // 从1开始跳过蛇头
      if (next.x == self.body[i].x && next.y == self.body[i].y) {
        return false;  // 碰撞到自己的身体
      }
    }
    
    // 检查是否为陷阱
    for (const auto& item : state.items) {
      if (item.value == -2 && next.x == item.pos.x && next.y == item.pos.y) {
        return false;  // 陷阱
      }
    }
    
    return true;
  }
  
  bool should_use_shield(const GameState& state) {
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
    
    return false;
  }
  
  MapAnalyzer map_analyzer;
  PathFinder path_finder;
  vector<Point> current_path;
};

void read_game_state(GameState &s) {
  cin >> s.remaining_ticks;

  int item_count;
  cin >> item_count;
  s.items.resize(item_count);
  for (int i = 0; i < item_count; ++i) {
    cin >> s.items[i].pos.y >> s.items[i].pos.x >>
        s.items[i].value >> s.items[i].lifetime;
  }

  int snake_count;
  cin >> snake_count;
  s.snakes.resize(snake_count);
  unordered_map<int, int> id2idx;
  id2idx.reserve(snake_count * 2);

  for (int i = 0; i < snake_count; ++i) {
    auto &sn = s.snakes[i];
    cin >> sn.id >> sn.length >> sn.score >> sn.direction >> sn.shield_cd >>
        sn.shield_time;
    sn.body.resize(sn.length);
    for (int j = 0; j < sn.length; ++j) {
      cin >> sn.body[j].y >> sn.body[j].x;
    }
    if (sn.id == MYID)
      s.self_idx = i;
    id2idx[sn.id] = i;
  }

  int chest_count;
  cin >> chest_count;
  s.chests.resize(chest_count);
  for (int i = 0; i < chest_count; ++i) {
    cin >> s.chests[i].pos.y >> s.chests[i].pos.x >>
        s.chests[i].score;
  }

  int key_count;
  cin >> key_count;
  s.keys.resize(key_count);
  for (int i = 0; i < key_count; ++i) {
    auto& key = s.keys[i];
    cin >> key.pos.y >> key.pos.x >> key.holder_id >> key.remaining_time;
    if (key.holder_id != -1) {
      auto it = id2idx.find(key.holder_id);
      if (it != id2idx.end()) {
        s.snakes[it->second].has_key = true;
      }
    }
  }

  cin >> s.current_safe_zone.x_min >> s.current_safe_zone.y_min >>
      s.current_safe_zone.x_max >> s.current_safe_zone.y_max;
  cin >> s.next_shrink_tick >> s.next_safe_zone.x_min >>
      s.next_safe_zone.y_min >> s.next_safe_zone.x_max >>
      s.next_safe_zone.y_max;
  cin >> s.final_shrink_tick >> s.final_safe_zone.x_min >>
      s.final_safe_zone.y_min >> s.final_safe_zone.x_max >>
      s.final_safe_zone.y_max;

  // 如果上一个 tick 往 Memory 里写入了内容，在这里读取，注意处理第一个 tick
  // 的情况 if (s.remaining_ticks < MAX_TICKS) {
  //     // 处理 Memory 读取
  // }
}

int main() {
  // 读取当前 tick 的所有游戏状态
  GameState current_state;
  read_game_state(current_state);

  // 决策
  static SnakeAI ai;  // 使用静态变量保持状态
  int decision = ai.make_decision(current_state);

  cout << decision << endl;
  // 如果需要写入 Memory，在此处写入

  return 0;
}
