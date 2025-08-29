#include <chrono>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>
//* 为了提高效率，新增了一些头文件：
#include <queue>
#include <algorithm>
#include <string>
#include <sstream>
#include <cstring>
#include <cmath>
#include <set>

using namespace std;

// 常量
constexpr int MAXN = 40;
constexpr int MAXM = 30;
constexpr int MAX_TICKS = 256;
constexpr int MYID = 2023202295; 

constexpr int MOVE_SPEED = 2; // 蛇每次移动的速度

struct Point {
  int y, x;
  
  // 添加运算符重载以支持相等比较
  bool operator==(const Point& other) const {
    return y == other.y && x == other.x;
  }
  
  // 用于哈希表的哈希函数
  struct Hash {
    size_t operator()(const Point& p) const {
      return hash<int>()(p.y) ^ hash<int>()(p.x);
    }
  };
};

//? 基础工具函数
// 计算曼哈顿距离
int manhattan_distance(const Point& p1, const Point& p2) {
  return abs(p1.y - p2.y) + abs(p1.x - p2.x);
}

// 根据方向获取下一个位置
Point get_next_position(const Point& p, int direction) {
  switch (direction) {
    case 0: return {p.y, p.x - MOVE_SPEED};  // 左，移动2个单位
    case 1: return {p.y - MOVE_SPEED, p.x};  // 上，移动2个单位
    case 2: return {p.y, p.x + MOVE_SPEED};  // 右，移动2个单位
    case 3: return {p.y + MOVE_SPEED, p.x};  // 下，移动2个单位
    default: return p;  // 不移动
  }
}

// 获取从当前点到目标点的方向
int get_next_direction(const Point& current, const Point& next) {
  if (next.x < current.x) return 0;  // 左
  if (next.y < current.y) return 1;  // 上
  if (next.x > current.x) return 2;  // 右
  if (next.y > current.y) return 3;  // 下
  return -1;  // 错误
}

// 判断点是否在整格点上（坐标为20的倍数+3）
bool is_grid_point(const Point& p) {
  return ((p.x - 3) % 20 == 0) && ((p.y - 3) % 20 == 0);
}

//? MapAnalyzer类：负责地图分析和危险度评估
class MapAnalyzer {
public:
  MapAnalyzer() {
    memset(danger_map, 0, sizeof(danger_map));
  }

  // 分析地图并更新危险度矩阵
  void analyze(const GameState& state) {
    // 初始化危险度矩阵
    memset(danger_map, 0, sizeof(danger_map));
    
    // 1. 标记安全区外为极度危险
    for (int x = 0; x < MAXN; x++) {
      for (int y = 0; y < MAXM; y++) {
        if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max ||
            y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max) {
          danger_map[x][y] = 255;  //! 可调参数
          // 最高危险度
        }
      }
    }
    
    // 2. 标记宝箱为极度危险区域（对未持有钥匙的蛇）
    for (const auto& chest : state.chests) {
      if (!state.get_self().has_key) {
        danger_map[chest.pos.x][chest.pos.y] = 255; //! 可调参数
        // 最高危险度（无钥匙宝箱触碰即死，护盾无效）
      }
    }
    
    // 3. 标记其他蛇的位置为危险区域
    for (const auto& snake : state.snakes) {
      if (snake.id == MYID) continue;  // 跳过自己
      
      // 蛇身体
      for (const auto& body_part : snake.body) {
        danger_map[body_part.x][body_part.y] = 100; //! 可调参数
        
        // 蛇头周围的区域也标记为危险
        if (body_part == snake.get_head()) {
          for (int dir = 0; dir < 4; dir++) {
            Point next = get_next_position(body_part, dir);
            if (next.x >= 0 && next.x < MAXN && next.y >= 0 && next.y < MAXM) {
              danger_map[next.x][next.y] += 80;  //! 可调参数
              // 可能移动的位置危险度较高
            }
          }
        }
      }
    }
    
    // 4. 标记陷阱为危险区域
    for (const auto& item : state.items) {
      if (item.value == -2) {  // 陷阱
        danger_map[item.pos.x][item.pos.y] = 80;
      }
    }
    
    // 5. 处理安全区收缩
    analyze_safe_zone_shrinking(state);
  }
  
  // 分析安全区收缩情况，更新危险度矩阵
  void analyze_safe_zone_shrinking(const GameState& state) {
    int current_tick = MAX_TICKS - state.remaining_ticks;
    
    // 检查是否有即将到来的收缩
    if (state.next_shrink_tick > 0) {
      int ticks_until_shrink = state.next_shrink_tick - current_tick;
      
      // 根据收缩紧急程度设置危险度
      int danger_level = 0;
      
      if (ticks_until_shrink <= 3) {
        danger_level = 220;  // 非常危险，马上收缩
      } else if (ticks_until_shrink <= 10) {
        danger_level = 150;  // 很危险，快收缩了
      } else if (ticks_until_shrink <= 20) {
        danger_level = 100;  // 中等危险，需要注意
      } else {
        danger_level = 30;   // 低危险，收缩还远
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
      
      // 特别处理收缩区域边缘，使蛇尽量不靠近收缩边界
      int buffer = 2;  // 边缘缓冲区大小
      for (int x = state.current_safe_zone.x_min; x <= state.current_safe_zone.x_max; x++) {
        for (int y = state.current_safe_zone.y_min; y <= state.current_safe_zone.y_max; y++) {
          // 检查是否在下一个安全区内
          if (x >= state.next_safe_zone.x_min && x <= state.next_safe_zone.x_max &&
              y >= state.next_safe_zone.y_min && y <= state.next_safe_zone.y_max) {
            // 标记靠近边缘的位置为危险
            if (x <= state.next_safe_zone.x_min + buffer || 
                x >= state.next_safe_zone.x_max - buffer ||
                y <= state.next_safe_zone.y_min + buffer || 
                y >= state.next_safe_zone.y_max - buffer) {
              danger_map[x][y] += danger_level / 2;  // 边缘也有一定危险
            }
          }
        }
      }
    }
    
    // 处理游戏后期的安全区策略
    if (state.final_shrink_tick > 0 && current_tick > 200) {
      // 游戏后期，给最终安全区外的区域增加危险度
      int final_danger = 50;  // 最终收缩区外的额外危险度
      
      for (int x = state.next_safe_zone.x_min; x <= state.next_safe_zone.x_max; x++) {
        for (int y = state.next_safe_zone.y_min; y <= state.next_safe_zone.y_max; y++) {
          if (x < state.final_safe_zone.x_min || x > state.final_safe_zone.x_max ||
              y < state.final_safe_zone.y_min || y > state.final_safe_zone.y_max) {
            danger_map[x][y] += final_danger;
          }
        }
      }
    }
  }
  
  // 判断某个点是否安全
  bool is_safe(const Point& p) {
    // 检查点是否在地图范围内
    if (p.x < 0 || p.x >= MAXN || p.y < 0 || p.y >= MAXM) {
      return false;
    }
    
    // 根据危险度判断安全性
    return danger_map[p.x][p.y] < 80;  // 小于80的危险度视为安全
  }
  
  // 获取点的危险度
  int get_danger(const Point& p) {
    if (p.x < 0 || p.x >= MAXN || p.y < 0 || p.y >= MAXM) {
      return 255;  // 超出地图边界，极度危险
    }
    return danger_map[p.x][p.y];
  }
  
  // 判断点是否在安全区内
  bool is_in_safe_zone(const Point& p, const GameState& state) {
    return p.x >= state.current_safe_zone.x_min && p.x <= state.current_safe_zone.x_max &&
           p.y >= state.current_safe_zone.y_min && p.y <= state.current_safe_zone.y_max;
  }
  
  // 判断点是否在下一个安全区内
  bool is_in_next_safe_zone(const Point& p, const GameState& state) {
    return p.x >= state.next_safe_zone.x_min && p.x <= state.next_safe_zone.x_max &&
           p.y >= state.next_safe_zone.y_min && p.y <= state.next_safe_zone.y_max;
  }
  
  // 获取最安全的移动方向（避开危险区域）
  int get_safest_direction(const Point& p, const GameState& state) {
    int safest_dir = -1;
    int min_danger = 255;
    
    for (int dir = 0; dir < 4; dir++) {
      Point next = get_next_position(p, dir);
      
      // 检查点是否在地图和当前安全区内
      if (next.x < 0 || next.x >= MAXN || next.y < 0 || next.y >= MAXM ||
          !is_in_safe_zone(next, state)) {
        continue;
      }
      
      int danger = get_danger(next);
      if (danger < min_danger) {
        min_danger = danger;
        safest_dir = dir;
      }
    }
    
    return safest_dir;
  }

private:
  int danger_map[MAXN][MAXM];  // 地图危险度矩阵
};

// 路径规划类
class PathFinder {
public:
  PathFinder(MapAnalyzer& analyzer) : map_analyzer(analyzer) {}
  
  // 考虑整格点转向限制的路径搜索
  vector<Point> find_path(const Point& start, const Point& goal, const GameState& state) {
    // 若起点终点相同，返回空路径
    if (start.x == goal.x && start.y == goal.y) {
      return {};
    }
    
    struct Node {
      Point pos;
      int g_cost;    // 从起点到当前点的代价
      int h_cost;    // 启发式估计到目标的代价
      int f_cost;    // f = g + h
      int direction; // 当前方向 (0-3)
      Node* parent;
      
      Node(Point p, int g, int h, int dir, Node* par) : 
        pos(p), g_cost(g), h_cost(h), f_cost(g + h), direction(dir), parent(par) {}
      
      bool operator<(const Node& other) const {
        return f_cost > other.f_cost;  // 优先队列需要用大于号
      }
    };
    
    // A*算法实现
    priority_queue<Node> open_list;
    
    // 为了处理整格点转向限制，状态需要包含位置和方向
    unordered_map<string, bool> closed_set;
    
    // 创建起点节点，方向初始为-1（未确定）
    int h_start = manhattan_distance(start, goal);
    Node* start_node = new Node(start, 0, h_start, -1, nullptr);
    open_list.push(*start_node);
    
    // 记录节点指针，便于后续内存释放
    vector<Node*> created_nodes = {start_node};
    
    Node* goal_node = nullptr;
    bool found_path = false;
    
    while (!open_list.empty()) {
      // 取出f值最小的节点
      Node current = open_list.top();
      open_list.pop();
      
      // 生成当前节点的唯一标识（包括位置和方向）
      string key = to_string(current.pos.y) + "," + to_string(current.pos.x) + "," + to_string(current.direction);
      
      // 如果已经访问过此节点，则跳过
      if (closed_set.count(key) > 0) {
        continue;
      }
      
      // 将节点加入关闭列表
      closed_set[key] = true;
      
      // 检查是否到达目标
      if (manhattan_distance(current.pos, goal) <= MOVE_SPEED) {
        found_path = true;
        
        // 创建目标节点并连接到当前路径
        goal_node = new Node(goal, current.g_cost + MOVE_SPEED, 0, current.direction, nullptr);
        // 找到当前节点的指针版本
        for (auto* node : created_nodes) {
          if (node->pos.x == current.pos.x && node->pos.y == current.pos.y && node->direction == current.direction) {
            goal_node->parent = node;
            break;
          }
        }
        created_nodes.push_back(goal_node);
        break;
      }
      
      // 检查是否在整格点上，只有在整格点才能改变方向
      bool can_change_direction = is_grid_point(current.pos);
      
      // 获取可能的移动方向
      vector<int> possible_directions;
      
      if (can_change_direction) {
        // 在整格点上可以选择任意方向
        possible_directions = {0, 1, 2, 3};  // 左、上、右、下
      } else if (current.direction != -1) {
        // 非整格点只能沿当前方向继续移动
        possible_directions = {current.direction};
      } else {
        // 起点可能不是整格点，需要特殊处理
        // 尝试所有方向，但后续会检查是否安全
        possible_directions = {0, 1, 2, 3};
      }
      
      // 尝试各个可能的方向
      for (int dir : possible_directions) {
        Point next_pos = get_next_position(current.pos, dir);
        
        // 检查位置有效性和安全性
        if (next_pos.x < 0 || next_pos.x >= MAXN || next_pos.y < 0 || next_pos.y >= MAXM || 
            !map_analyzer.is_safe(next_pos)) {
          continue;
        }
        
        // 生成下一个节点的唯一标识（包括位置和方向）
        string next_key = to_string(next_pos.y) + "," + to_string(next_pos.x) + "," + to_string(dir);
        
        // 如果已经在关闭列表中，则跳过
        if (closed_set.count(next_key) > 0) {
          continue;
        }
        
        // 计算代价
        int g_cost = current.g_cost + MOVE_SPEED;
        int h_cost = manhattan_distance(next_pos, goal);
        
        // 创建新节点
        Node* next_node = new Node(next_pos, g_cost, h_cost, dir, nullptr);
        // 找到当前节点的指针版本
        for (auto* node : created_nodes) {
          if (node->pos.x == current.pos.x && node->pos.y == current.pos.y && node->direction == current.direction) {
            next_node->parent = node;
            break;
          }
        }
        created_nodes.push_back(next_node);
        
        // 加入开放列表
        open_list.push(*next_node);
      }
    }
    
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
    
    // 清理内存
    for (auto* node : created_nodes) {
      delete node;
    }
    
    return path;
  }
  
  // 保留简单版本的路径搜索（不考虑整格点转向限制）
  vector<Point> find_simple_path(const Point& start, const Point& goal, const GameState& state) {
    // 若起点终点相同，返回空路径
    if (start.x == goal.x && start.y == goal.y) {
      return {};
    }
    
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
    
    // A*算法实现
    priority_queue<Node> open_list;
    // 使用哈希集合记录已经访问过的节点
    unordered_set<string> closed_set;
    
    // 创建起点节点
    int h_start = manhattan_distance(start, goal);
    Node* start_node = new Node(start, 0, h_start, nullptr);
    open_list.push(*start_node);
    
    // 记录节点指针，便于后续内存释放
    vector<Node*> created_nodes = {start_node};
    
    Node* goal_node = nullptr;
    bool found_path = false;
    
    while (!open_list.empty()) {
      // 取出f值最小的节点
      Node current = open_list.top();
      open_list.pop();
      
      // 生成当前节点的唯一标识
      string key = to_string(current.pos.y) + "," + to_string(current.pos.x);
      
      // 如果已经访问过此节点，则跳过
      if (closed_set.count(key) > 0) {
        continue;
      }
      
      // 将节点加入关闭列表
      closed_set.insert(key);
      
      // 检查是否到达目标
      if (manhattan_distance(current.pos, goal) <= MOVE_SPEED) {
        found_path = true;
        
        // 创建目标节点并连接到当前路径
        goal_node = new Node(goal, current.g_cost + MOVE_SPEED, 0, nullptr);
        // 找到当前节点的指针版本
        for (auto* node : created_nodes) {
          if (node->pos.x == current.pos.x && node->pos.y == current.pos.y) {
            goal_node->parent = node;
            break;
          }
        }
        created_nodes.push_back(goal_node);
        break;
      }
      
      // 尝试四个方向
      for (int dir = 0; dir < 4; dir++) {
        Point next_pos = get_next_position(current.pos, dir);
        
        // 检查位置有效性和安全性
        if (next_pos.x < 0 || next_pos.x >= MAXN || next_pos.y < 0 || next_pos.y >= MAXM || 
            !map_analyzer.is_safe(next_pos)) {
          continue;
        }
        
        // 生成下一个节点的唯一标识
        string next_key = to_string(next_pos.y) + "," + to_string(next_pos.x);
        
        // 如果已经在关闭列表中，则跳过
        if (closed_set.count(next_key) > 0) {
          continue;
        }
        
        // 计算代价
        int g_cost = current.g_cost + MOVE_SPEED;
        int h_cost = manhattan_distance(next_pos, goal);
        
        // 创建新节点
        Node* next_node = new Node(next_pos, g_cost, h_cost, nullptr);
        // 找到当前节点的指针版本
        for (auto* node : created_nodes) {
          if (node->pos.x == current.pos.x && node->pos.y == current.pos.y) {
            next_node->parent = node;
            break;
          }
        }
        created_nodes.push_back(next_node);
        
        // 加入开放列表
        open_list.push(*next_node);
      }
    }
    
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
    
    // 清理内存
    for (auto* node : created_nodes) {
      delete node;
    }
    
    return path;
  }
  
  // 检查两点之间是否可以直接移动
  bool can_move_directly(const Point& start, const Point& goal, const GameState& state) {
    // 计算方向
    int dir = get_next_direction(start, goal);
    if (dir == -1) return false;  // 无效方向
    
    // 检查是否在整格点上，如果不是，只能沿当前方向移动
    if (!is_grid_point(start)) {
      // 获取当前的移动方向
      for (const auto& snake : state.snakes) {
        if (snake.id == MYID) {
          // 如果请求的方向与当前方向不同，则无法直接移动
          if (dir != snake.direction) {
            return false;
          }
          break;
        }
      }
    }
    
    // 获取下一个位置
    Point next = get_next_position(start, dir);
    
    // 检查是否安全
    return map_analyzer.is_safe(next);
  }
  
private:
  MapAnalyzer& map_analyzer;
};

// 目标结构体
struct Target {
  Point pos;
  int value;  // 目标价值
  int distance;  // 到目标的距离
  double priority;  // 综合优先级
};

// SnakeAI类：主要决策逻辑
class SnakeAI {
public:
  SnakeAI() : map_analyzer(), path_finder(map_analyzer) {}
  
  // 做出决策
  int make_decision(const GameState& state) {
    // 读取记忆数据（从第二个tick开始）
    if (state.remaining_ticks < MAX_TICKS) {
      read_memory();
    }
    
    // 分析地图
    map_analyzer.analyze(state);
    
    // 初始化状态
    const auto& self = state.get_self();
    const auto& head = self.get_head();
    
    // 检查是否应该使用护盾
    if (should_use_shield(state)) {
      save_memory(state, 4);
      return 4;  // 激活护盾
    }
    
    // 检查当前路径是否仍然有效
    bool need_new_path = current_path.empty() || 
                         !is_target_still_valid(current_target, state) ||
                         !is_path_still_safe(current_path, state);
    
    // 如果需要，选择新目标并规划路径
    if (need_new_path) {
      // 选择最优目标
      Target target = select_target(state);
      current_target = target;
      
      // 如果找不到目标，随机移动
      if (target.pos.x == -1 && target.pos.y == -1) {
        int safe_dir = find_safe_direction(state);
        save_memory(state, safe_dir);
        return safe_dir;
      }
      
      // 考虑整格点转向限制的路径规划
      vector<Point> path = path_finder.find_path(head, target.pos, state);
      current_path = path;
      
      // 如果无法规划路径，退回到简单路径
      if (path.empty()) {
        path = path_finder.find_simple_path(head, target.pos, state);
        current_path = path;
        
        // 如果仍然无法规划路径，找一个安全的方向
        if (path.empty()) {
          int safe_dir = find_safe_direction(state);
          save_memory(state, safe_dir);
          return safe_dir;
        }
      }
    }
    
    // 获取下一步方向
    int next_dir = -1;
    if (!current_path.empty() && current_path.size() > 1) {
      next_dir = get_next_direction(head, current_path[1]);
      
      // 如果下一步不安全，找一个安全的方向
      if (!is_direction_safe(next_dir, state)) {
        next_dir = find_safe_direction(state);
      }
    } else {
      next_dir = find_safe_direction(state);
    }
    
    // 保存记忆数据并返回决策
    save_memory(state, next_dir);
    return next_dir;
  }
  
  // 判断是否应该使用护盾
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
    
    // 安全区收缩检查
    int current_tick = MAX_TICKS - state.remaining_ticks;
    if (state.next_shrink_tick > 0) {
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
    
    // 检查下一步是否会碰撞
    // 根据当前方向预测下一步位置
    Point next_pos = get_next_position(self.get_head(), self.direction);
    
    // 检查是否会撞到其他蛇
    for (const auto& snake : state.snakes) {
      if (snake.id == MYID) continue;
      
      // 检查是否会与其他蛇的头部碰撞
      if (snake.shield_time == 0) {  // 只有对方没有护盾才激活
        Point other_next_pos = get_next_position(snake.get_head(), snake.direction);
        if (next_pos.x == other_next_pos.x && next_pos.y == other_next_pos.y) {
          return true;
        }
      }
      
      // 检查是否会撞到其他蛇的身体
      for (size_t j = 0; j < snake.body.size(); j++) {
        const auto& body_part = snake.body[j];
        if (next_pos.x == body_part.x && next_pos.y == body_part.y) {
          return true;  // 激活护盾避免撞到其他蛇
        }
      }
    }
    
    // 当持有钥匙且靠近宝箱时，如果有其他带钥匙的蛇也在附近，激活护盾
    if (self.has_key && !state.chests.empty()) {
      const auto& chest = state.chests[0];
      int distance_to_chest = manhattan_distance(self.get_head(), chest.pos);
      
      if (distance_to_chest <= 10) {  // 接近宝箱
        // 检查是否有其他蛇可能争夺宝箱
        for (const auto& snake : state.snakes) {
          if (snake.id == MYID) continue;
          if (snake.has_key && manhattan_distance(snake.get_head(), chest.pos) <= distance_to_chest + 5) {
            return true;  // 激活护盾争夺宝箱
          }
        }
      }
    }
    
    return false;
  }
  
  // 选择最优目标
  Target select_target(const GameState& state) {
    vector<Target> potential_targets;
    
    const auto& self = state.get_self();
    const auto& head = self.get_head();
    
    // 添加食物作为潜在目标
    for (const auto& item : state.items) {
      // 普通食物和增长豆
      if (item.value > 0 || item.value == -1) {
        Target t;
        t.pos = item.pos;
        t.value = item.value > 0 ? item.value : 3;  // 增长豆价值设为3
        t.distance = manhattan_distance(head, item.pos);
        
        // 计算优先级：价值/距离的比率，再考虑安全因素
        double safety_factor = map_analyzer.is_safe(item.pos) ? 1.0 : 0.2;
        t.priority = (double)t.value / t.distance * safety_factor;
        
        potential_targets.push_back(t);
      }
    }
    
    // 添加钥匙作为潜在目标
    handle_keys_and_chests(state, head, potential_targets);
    
    // 根据安全区收缩情况，考虑添加安全区中心作为目标
    int current_tick = MAX_TICKS - state.remaining_ticks;
    if (state.next_shrink_tick > 0 && state.next_shrink_tick - current_tick <= 20) {
      // 安全区即将收缩，添加下一个安全区中心作为目标
      Point next_center = {
        (state.next_safe_zone.y_min + state.next_safe_zone.y_max) / 2,
        (state.next_safe_zone.x_min + state.next_safe_zone.x_max) / 2
      };
      
      Target t;
      t.pos = next_center;
      t.value = 10;  // 安全区价值
      t.distance = manhattan_distance(head, next_center);
      
      // 安全区收缩越近，优先级越高
      double urgency_factor = 1.0;
      int ticks_until_shrink = state.next_shrink_tick - current_tick;
      if (ticks_until_shrink <= 5) {
        urgency_factor = 3.0;  // 非常紧急
      } else if (ticks_until_shrink <= 10) {
        urgency_factor = 2.0;  // 较为紧急
      }
      
      // 检查自己是否在即将收缩的区域外
      bool is_outside_next_zone = 
        head.x < state.next_safe_zone.x_min || head.x > state.next_safe_zone.x_max ||
        head.y < state.next_safe_zone.y_min || head.y > state.next_safe_zone.y_max;
      
      if (is_outside_next_zone) {
        urgency_factor *= 2.0;  // 如果在收缩区外，加倍紧急
      }
      
      t.priority = (double)t.value / t.distance * urgency_factor;
      
      potential_targets.push_back(t);
    }
    
    // 选择优先级最高的目标
    Target best_target;
    best_target.pos = {-1, -1};
    best_target.value = 0;
    best_target.distance = INT_MAX;
    best_target.priority = 0;
    
    for (const auto& t : potential_targets) {
      if (t.priority > best_target.priority) {
        best_target = t;
      }
    }
    
    return best_target;
  }
  
  // 处理钥匙和宝箱的目标选择
  void handle_keys_and_chests(const GameState& state, const Point& head, vector<Target>& targets) {
    const auto& self = state.get_self();
    int current_tick = MAX_TICKS - state.remaining_ticks;
    
    // 游戏阶段因子（后期更重视宝箱和钥匙）
    double game_stage_factor = 1.0;
    if (current_tick > 200) {
      game_stage_factor = 2.0;  // 后期
    } else if (current_tick > 160) {
      game_stage_factor = 1.5;  // 中后期
    } else if (current_tick > 80) {
      game_stage_factor = 1.2;  // 中期
    }
    
    // 如果我们已经持有钥匙，处理宝箱
    if (self.has_key) {
      // 寻找钥匙的剩余时间
      int key_remaining_time = 0;
      for (const auto& key : state.keys) {
        if (key.holder_id == MYID) {
          key_remaining_time = key.remaining_time;
          break;
        }
      }
      
      // 处理宝箱
      for (const auto& chest : state.chests) {
        Target t;
        t.pos = chest.pos;
        t.value = chest.score;
        t.distance = manhattan_distance(head, chest.pos);
        
        // 根据钥匙剩余时间调整优先级
        double time_urgency = 1.0;
        if (key_remaining_time <= 5) {
          time_urgency = 3.0;  // 非常紧急，钥匙即将掉落
        } else if (key_remaining_time <= 10) {
          time_urgency = 2.0;  // 较为紧急
        }
        
        // 检查其他持有钥匙的蛇与宝箱的距离
        bool competitors = false;
        int competitor_distance = INT_MAX;
        for (const auto& snake : state.snakes) {
          if (snake.id != MYID && snake.has_key) {
            int dist = manhattan_distance(snake.get_head(), chest.pos);
            competitor_distance = min(competitor_distance, dist);
            competitors = true;
          }
        }
        
        // 如果有竞争者且他们离宝箱更近，提高优先级
        double competition_factor = 1.0;
        if (competitors) {
          if (competitor_distance < t.distance) {
            // 对方更近，视情况调整策略
            if (competitor_distance < t.distance - 10) {
              competition_factor = 0.5;  // 对方远远领先，降低优先级
            } else {
              competition_factor = 1.5;  // 竞争激烈，提高优先级
            }
          } else {
            competition_factor = 2.0;  // 我方领先，提高优先级
          }
        }
        
        double safety_factor = map_analyzer.is_safe(t.pos) ? 1.0 : 0.3;
        
        // 宝箱分数越高优先级越高
        double score_factor = (double)t.value / 50.0;  // 标准化分数
        
        // 综合计算优先级
        t.priority = score_factor * game_stage_factor * time_urgency * competition_factor * safety_factor / sqrt(t.distance);
        
        targets.push_back(t);
      }
    }
    // 如果没有持有钥匙，考虑获取钥匙
    else {
      // 处理地图上的钥匙
      for (const auto& key : state.keys) {
        if (key.holder_id == -1) {  // 钥匙在地图上
          Target t;
          t.pos = key.pos;
          t.value = 20;  // 基础钥匙价值
          t.distance = manhattan_distance(head, key.pos);
          
          double key_priority = 1.0;
          
          // 如果没有宝箱，钥匙优先级降低
          if (state.chests.empty()) {
            key_priority = 0.5;
          } else {
            // 计算钥匙到最近宝箱的距离
            int min_chest_distance = INT_MAX;
            int max_chest_score = 0;
            for (const auto& chest : state.chests) {
              int chest_distance = manhattan_distance(key.pos, chest.pos);
              min_chest_distance = min(min_chest_distance, chest_distance);
              max_chest_score = max(max_chest_score, chest.score);
            }
            
            // 钥匙离宝箱越近，优先级越高
            if (min_chest_distance < 20) {
              key_priority = 1.5 * (1.0 - (double)min_chest_distance / 20.0);
            }
            
            // 宝箱分数越高，钥匙优先级越高
            key_priority *= (double)max_chest_score / 50.0;
          }
          
          // 检查其他蛇到钥匙的距离
          bool close_competitors = false;
          for (const auto& snake : state.snakes) {
            if (snake.id != MYID && !snake.has_key) {
              int dist = manhattan_distance(snake.get_head(), key.pos);
              if (dist <= t.distance + 5) {
                close_competitors = true;
                break;
              }
            }
          }
          
          // 有竞争者时提高优先级
          double competition_factor = close_competitors ? 1.5 : 1.0;
          
          double safety_factor = map_analyzer.is_safe(t.pos) ? 1.0 : 0.3;
          
          // 综合计算优先级
          t.priority = key_priority * game_stage_factor * competition_factor * safety_factor / sqrt(t.distance);
          
          targets.push_back(t);
        }
      }
    }
  }
  
  // 从所有食物中找最近的一个（已被select_target替代）
  Target find_nearest_food(const GameState& state) {
    Target nearest;
    nearest.pos = {-1, -1};
    nearest.value = 0;
    nearest.distance = INT_MAX;
    nearest.priority = 0;
    
    const auto& head = state.get_self().get_head();
    
    // 遍历所有食物
    for (const auto& item : state.items) {
      // 只考虑食物（正分数或增长豆）
      if (item.value > 0 || item.value == -1) {
        int dist = manhattan_distance(head, item.pos);
        int value = item.value > 0 ? item.value : 3;  // 增长豆价值设为3
        
        // 如果路径不安全，跳过
        if (!map_analyzer.is_safe(item.pos)) {
          continue;
        }
        
        // 更新最近的食物
        if (dist < nearest.distance) {
          nearest.pos = item.pos;
          nearest.value = value;
          nearest.distance = dist;
          nearest.priority = (double)value / dist;
        }
      }
    }
    
    return nearest;
  }
  
  // 判断方向是否安全
  bool is_direction_safe(int direction, const GameState& state) {
    const auto& self = state.get_self();
    const auto& head = self.get_head();
    Point next = get_next_position(head, direction);
    
    // 检查是否在地图范围内
    if (next.x < 0 || next.x >= MAXN || next.y < 0 || next.y >= MAXM) {
      return false;
    }
    
    // 检查是否在安全区内
    if (next.x < state.current_safe_zone.x_min || next.x > state.current_safe_zone.x_max ||
        next.y < state.current_safe_zone.y_min || next.y > state.current_safe_zone.y_max) {
      return false;
    }
    
    // 检查是否会撞到其他蛇
    for (const auto& snake : state.snakes) {
      if (snake.id == MYID) continue;  // 跳过自己
      
      for (const auto& body_part : snake.body) {
        if (next.x == body_part.x && next.y == body_part.y) {
          return false;
        }
      }
    }
    
    // 检查是否会撞到宝箱（如果没有钥匙）
    if (!self.has_key) {
      for (const auto& chest : state.chests) {
        if (next.x == chest.pos.x && next.y == chest.pos.y) {
          return false;
        }
      }
    }
    
    // 检查是否会撞到陷阱
    for (const auto& item : state.items) {
      if (item.value == -2 && next.x == item.pos.x && next.y == item.pos.y) {
        return false;
      }
    }
    
    return true;
  }
  
  // 找一个安全的方向
  int find_safe_direction(const GameState& state) {
    const auto& head = state.get_self().get_head();
    
    // 检查所有方向，找一个安全的方向
    for (int dir = 0; dir < 4; dir++) {
      if (is_direction_safe(dir, state)) {
        return dir;
      }
    }
    
    // 如果没有安全方向，尝试向安全区中心移动
    Point center = {
      (state.current_safe_zone.y_max + state.current_safe_zone.y_min) / 2,
      (state.current_safe_zone.x_max + state.current_safe_zone.x_min) / 2
    };
    
    int dir = get_next_direction(head, center);
    if (dir != -1) {
      return dir;
    }
    
    // 实在没有安全方向，随机选择
    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> dist(0, 3);
    return dist(rng);
  }
  
  // 判断当前目标是否仍然有效
  bool is_target_still_valid(const Target& target, const GameState& state) {
    // 如果没有目标，则需要新目标
    if (target.pos.x == -1 && target.pos.y == -1) {
      return false;
    }
    
    // 检查目标是否在安全区内
    if (!map_analyzer.is_in_safe_zone(target.pos, state)) {
      return false;
    }
    
    // 检查目标是否安全
    if (!map_analyzer.is_safe(target.pos)) {
      return false;
    }
    
    // 如果是食物目标，检查食物是否还存在
    bool target_exists = false;
    
    // 检查是否是宝箱
    for (const auto& chest : state.chests) {
      if (chest.pos.x == target.pos.x && chest.pos.y == target.pos.y) {
        // 如果是宝箱，检查是否有钥匙
        if (!state.get_self().has_key) {
          return false;  // 没有钥匙了，目标无效
        }
        target_exists = true;
        break;
      }
    }
    
    // 检查是否是钥匙
    if (!target_exists) {
      for (const auto& key : state.keys) {
        if (key.pos.x == target.pos.x && key.pos.y == target.pos.y && key.holder_id == -1) {
          target_exists = true;
          break;
        }
      }
    }
    
    // 检查是否是食物
    if (!target_exists) {
      for (const auto& item : state.items) {
        if (item.pos.x == target.pos.x && item.pos.y == target.pos.y && 
            (item.value > 0 || item.value == -1)) {  // 食物或增长豆
          target_exists = true;
          break;
        }
      }
    }
    
    return target_exists;
  }
  
  // 判断当前路径是否仍然安全
  bool is_path_still_safe(const vector<Point>& path, const GameState& state) {
    if (path.empty()) {
      return false;
    }
    
    // 检查路径中的每个点是否安全
    for (size_t i = 1; i < min(path.size(), size_t(5)); i++) {  // 只检查前5步
      if (!map_analyzer.is_safe(path[i])) {
        return false;
      }
    }
    
    return true;
  }
  
  // 保存记忆数据
  void save_memory(const GameState& state, int decision) {
    stringstream ss;
    
    // 保存当前目标
    ss << current_target.pos.y << " " << current_target.pos.x << " ";
    
    // 保存路径点数量和部分路径点（最多5个）
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
    int key_remaining_time = 0;
    for (const auto& key : state.keys) {
      if (key.holder_id == MYID) {
        key_remaining_time = key.remaining_time;
        break;
      }
    }
    ss << (state.get_self().has_key ? 1 : 0) << " " << key_remaining_time << " ";
    
    memory_data = ss.str();
  }
  
  // 读取记忆数据
  void read_memory() {
    string memory_line;
    if (getline(cin, memory_line)) {
      stringstream ss(memory_line);
      
      // 读取上次保存的目标
      Point target_pos;
      if (ss >> target_pos.y >> target_pos.x) {
        current_target.pos = target_pos;
      }
      
      // 读取路径信息
      size_t path_size;
      if (ss >> path_size) {
        current_path.clear();
        for (size_t i = 0; i < path_size; i++) {
          Point p;
          if (ss >> p.y >> p.x) {
            current_path.push_back(p);
          }
        }
      }
      
      // 读取上一个决策
      int last_decision;
      if (ss >> last_decision) {
        // 可以用于决策连续性判断
      }
      
      // 读取游戏阶段信息
      int current_tick;
      if (ss >> current_tick) {
        // 可以用于判断游戏进度
      }
      
      // 读取钥匙信息
      int has_key, key_time;
      if (ss >> has_key >> key_time) {
        // 可以用于跟踪钥匙状态变化
      }
    }
  }
  
  // 成员变量
  string memory_data;  // 记忆数据
  MapAnalyzer map_analyzer;
  PathFinder path_finder;
  Target current_target;
  vector<Point> current_path;
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
  //     todo: 处理 Memory 读取
  // }
}

int main() {
  // 读取当前 tick 的所有游戏状态
  GameState current_state;
  read_game_state(current_state);

  // 使用SnakeAI类来做决策
  static SnakeAI ai;  // 使用静态变量保持状态
  int decision = ai.make_decision(current_state);

  // 输出决策
  cout << decision << endl;
  
  // 输出记忆数据
  if (!ai.memory_data.empty()) {
    cout << ai.memory_data << endl;
  }

  return 0;
}
