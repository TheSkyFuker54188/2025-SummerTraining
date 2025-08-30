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

// 游戏基础配置
constexpr int MAXN = 40, MAXM = 30, MAX_TICKS = 256;
constexpr int MYID = 2023202295; // 学号
constexpr int MOVE_SPEED = 2, GRID_POINT_OFFSET = 3, GRID_POINT_INTERVAL = 20;

// 游戏数值配置
constexpr int SHIELD_COMMAND = 4, GROWTH_BEAN_VALUE = -1, TRAP_VALUE = -2;
constexpr int DANGER_THRESHOLD = 80, HIGH_DANGER = 200, EXTREME_DANGER = 255;

// AI策略参数
constexpr int IMMEDIATE_RESPONSE_DISTANCE = 3, NEAR_FOOD_THRESHOLD = 5;
constexpr float NEAR_FOOD_FACTOR = 10.0f, FAR_FOOD_FACTOR = 5.0f;
constexpr int COLLISION_DETECT_DISTANCE = 4;

// 收缩区域危险等级
constexpr int SHRINK_TIMES[] = {5, 10, 15, 20}; // 即将/很快/中等/较低
constexpr int SHRINK_DANGERS[] = {250, 200, 150, 100};

// 配置参数类 - 单例模式管理AI参数
class Config
{
public:
  static Config &get()
  {
    static Config instance;
    return instance;
  }
  int wall_buffer = 2, safe_threshold = DANGER_THRESHOLD;
  float danger_weight = 5.0f, value_weight = 3.0f, central_weight = 1.0f;

private:
  Config() = default;
};

// 基础数据结构
struct Point
{
  int y, x;
  bool operator==(const Point &other) const { return y == other.y && x == other.x; }
  bool operator!=(const Point &other) const { return !(*this == other); }
};

struct Item
{
  Point pos;
  int value, lifetime;
};
struct Snake
{
  int id, length, score, direction, shield_cd, shield_time;
  bool has_key = false;
  vector<Point> body;
  const Point &get_head() const { return body.front(); }
};
struct Chest
{
  Point pos;
  int score;
};
struct Key
{
  Point pos;
  int holder_id, remaining_time;
};
struct SafeZoneBounds
{
  int x_min, y_min, x_max, y_max;
};
struct Target
{
  Point pos;
  int value, distance;
  double priority;
};

// 游戏状态结构
struct GameState
{
  int remaining_ticks, next_shrink_tick, final_shrink_tick, self_idx;
  vector<Item> items;
  vector<Snake> snakes;
  vector<Chest> chests;
  vector<Key> keys;
  SafeZoneBounds current_safe_zone, next_safe_zone, final_safe_zone;
  const Snake &get_self() const { return snakes[self_idx]; }
};

// 工具函数类 - 提供基础计算功能
class Utils
{
public:
  // 曼哈顿距离计算
  static int manhattan_distance(const Point &a, const Point &b)
  {
    return abs(a.x - b.x) + abs(a.y - b.y);
  }

  // 检查是否为整格点(可转向位置)
  static bool is_grid_point(const Point &p)
  {
    return ((p.x - GRID_POINT_OFFSET) % GRID_POINT_INTERVAL == 0) &&
           ((p.y - GRID_POINT_OFFSET) % GRID_POINT_INTERVAL == 0);
  }

  // 根据方向获取下一个位置
  static Point get_next_position(const Point &p, int direction)
  {
    static const int dx[] = {-MOVE_SPEED, 0, MOVE_SPEED, 0};
    static const int dy[] = {0, -MOVE_SPEED, 0, MOVE_SPEED};
    return {p.y + dy[direction], p.x + dx[direction]};
  }

  // 根据两点计算方向
  static int get_direction(const Point &from, const Point &to)
  {
    if (to.x < from.x)
      return 0; // 左
    if (to.y < from.y)
      return 1; // 上
    if (to.x > from.x)
      return 2; // 右
    if (to.y > from.y)
      return 3; // 下
    return -1;
  }
};

// 多维度地图分析类 - 管理危险度、价值度、中心度三个维度
class Map
{
private:
  int danger_map[MAXM][MAXN], value_map[MAXM][MAXN], central_map[MAXM][MAXN];

  // 危险度分析 - 标记所有危险区域
  void analyze_danger(const GameState &state)
  {
    memset(danger_map, 0, sizeof(danger_map));

    // 标记安全区外和边界
    for (int y = 0; y < MAXM; y++)
    {
      for (int x = 0; x < MAXN; x++)
      {
        if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max ||
            y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max)
        {
          danger_map[y][x] = EXTREME_DANGER;
        }
        else if (x <= 2 || x >= MAXN - 3 || y <= 2 || y >= MAXM - 3)
        {
          danger_map[y][x] += 100; // 墙边危险
        }
      }
    }

    // 标记蛇和其他危险物
    for (const auto &snake : state.snakes)
    {
      if (snake.id == MYID)
        continue;
      for (const auto &part : snake.body)
      {
        danger_map[part.y][part.x] = HIGH_DANGER;
        // 周围区域增加危险度
        for (int dy = -1; dy <= 1; dy++)
        {
          for (int dx = -1; dx <= 1; dx++)
          {
            int nx = part.x + dx, ny = part.y + dy;
            if (nx >= 0 && nx < MAXN && ny >= 0 && ny < MAXM)
            {
              danger_map[ny][nx] += 50;
            }
          }
        }
      }
    }

    // 标记陷阱和宝箱
    for (const auto &item : state.items)
    {
      if (item.value == TRAP_VALUE)
      {
        danger_map[item.pos.y][item.pos.x] = 150;
      }
    }
    for (const auto &chest : state.chests)
    {
      if (!state.get_self().has_key)
      {
        danger_map[chest.pos.y][chest.pos.x] = EXTREME_DANGER;
      }
    }

    // 收缩区域标记
    if (state.next_shrink_tick > 0)
      mark_shrinking_areas(state);
  }

  // 标记收缩区域危险度
  void mark_shrinking_areas(const GameState &state)
  {
    int current_tick = MAX_TICKS - state.remaining_ticks;
    int ticks_until_shrink = state.next_shrink_tick - current_tick;

    int danger_level = 100; // 默认危险度
    for (int i = 0; i < 4; i++)
    {
      if (ticks_until_shrink <= SHRINK_TIMES[i])
      {
        danger_level = SHRINK_DANGERS[i];
        break;
      }
    }

    // 标记将被收缩的区域
    for (int y = 0; y < MAXM; y++)
    {
      for (int x = 0; x < MAXN; x++)
      {
        if ((x >= state.current_safe_zone.x_min && x <= state.current_safe_zone.x_max &&
             y >= state.current_safe_zone.y_min && y <= state.current_safe_zone.y_max) &&
            (x < state.next_safe_zone.x_min || x > state.next_safe_zone.x_max ||
             y < state.next_safe_zone.y_min || y > state.next_safe_zone.y_max))
        {
          danger_map[y][x] = danger_level;
        }
      }
    }
  }

  // 价值度分析 - 标记所有有价值的物品
  void analyze_value(const GameState &state)
  {
    memset(value_map, 0, sizeof(value_map));

    // 食物和增长豆
    for (const auto &item : state.items)
    {
      if (item.value > 0)
      {
        value_map[item.pos.y][item.pos.x] = item.value * 30;
      }
      else if (item.value == GROWTH_BEAN_VALUE)
      {
        value_map[item.pos.y][item.pos.x] = 90;
      }
    }

    // 钥匙价值
    for (const auto &key : state.keys)
    {
      if (key.holder_id == -1)
      {
        value_map[key.pos.y][key.pos.x] = 150;
      }
    }

    // 宝箱价值(如果有钥匙)
    if (state.get_self().has_key)
    {
      for (const auto &chest : state.chests)
      {
        value_map[chest.pos.y][chest.pos.x] = 200;
      }
    }
  }

  // 中心度分析 - 计算到安全区中心的距离
  void analyze_central(const GameState &state)
  {
    memset(central_map, 0, sizeof(central_map));

    int center_x = (state.current_safe_zone.x_min + state.current_safe_zone.x_max) / 2;
    int center_y = (state.current_safe_zone.y_min + state.current_safe_zone.y_max) / 2;

    // 如果即将收缩，使用下一个安全区中心
    int current_tick = MAX_TICKS - state.remaining_ticks;
    if (state.next_shrink_tick > 0 && state.next_shrink_tick - current_tick <= 15)
    {
      center_x = (state.next_safe_zone.x_min + state.next_safe_zone.x_max) / 2;
      center_y = (state.next_safe_zone.y_min + state.next_safe_zone.y_max) / 2;
    }

    int max_dist = MAXN + MAXM;
    for (int y = 0; y < MAXM; y++)
    {
      for (int x = 0; x < MAXN; x++)
      {
        int dist = abs(x - center_x) + abs(y - center_y);
        central_map[y][x] = EXTREME_DANGER * (1.0 - (float)dist / max_dist);
      }
    }
  }

public:
  // 分析地图所有维度
  void analyze(const GameState &state)
  {
    analyze_danger(state);
    analyze_value(state);
    analyze_central(state);
  }

  // 各维度评估接口
  int danger(const Point &p) const
  {
    if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN)
      return EXTREME_DANGER;
    return danger_map[p.y][p.x];
  }

  int value(const Point &p) const
  {
    if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN)
      return 0;
    return value_map[p.y][p.x];
  }

  bool is_safe(const Point &p) const
  {
    if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN)
      return false;
    return danger_map[p.y][p.x] < Config::get().safe_threshold;
  }

  // 综合评分 - 结合三个维度
  float score(const Point &p) const
  {
    if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN)
      return -1000.0f;
    if (danger_map[p.y][p.x] >= HIGH_DANGER)
      return -500.0f;

    float danger_score = 1.0f - danger_map[p.y][p.x] / 255.0f;
    float value_score = value_map[p.y][p.x] / 255.0f;
    float central_score = central_map[p.y][p.x] / 255.0f;

    return danger_score * Config::get().danger_weight +
           value_score * Config::get().value_weight +
           central_score * Config::get().central_weight;
  }
};

// 路径规划类 - 简化版A*算法
class PathFinder
{
private:
  Map *map;

  // 生成基础路径 - 先水平后垂直移动
  vector<Point> generate_basic_path(const Point &start, const Point &goal, const GameState &state)
  {
    vector<Point> path;
    Point current = start;
    path.push_back(current);

    // 水平移动
    while (abs(current.x - goal.x) > MOVE_SPEED)
    {
      int dir = (current.x < goal.x) ? 2 : 0;
      Point next = Utils::get_next_position(current, dir);
      if (!map->is_safe(next))
        break;
      path.push_back(next);
      current = next;
    }

    // 垂直移动
    while (abs(current.y - goal.y) > MOVE_SPEED)
    {
      int dir = (current.y < goal.y) ? 3 : 1;
      Point next = Utils::get_next_position(current, dir);
      if (!map->is_safe(next))
        break;
      path.push_back(next);
      current = next;
    }

    if (current.x != goal.x || current.y != goal.y)
    {
      path.push_back(goal);
    }
    return path;
  }

public:
  PathFinder(Map *map_ptr) : map(map_ptr) {}

  // 寻找从起点到终点的路径
  vector<Point> find_path(const Point &start, const Point &goal, const GameState &state)
  {
    if (start.x == goal.x && start.y == goal.y)
      return {};
    return generate_basic_path(start, goal, state);
  }
};

// 主决策类 - 蛇AI大脑
class SnakeAI
{
private:
  Map *map;
  PathFinder *path_finder;
  vector<Point> current_path;

  // 目标选择 - 根据即时反应和综合评估选择最佳目标
  Target select_target(const GameState &state)
  {
    const auto &head = state.get_self().get_head();

    // 即时反应 - 检测头部附近的食物
    for (const auto &item : state.items)
    {
      if ((item.value > 0 || item.value == GROWTH_BEAN_VALUE) && map->is_safe(item.pos))
      {
        int dist = Utils::manhattan_distance(head, item.pos);
        if (dist <= IMMEDIATE_RESPONSE_DISTANCE)
        {
          return {item.pos, item.value > 0 ? item.value : 3, dist, 1000.0};
        }
      }
    }

    // 综合评估 - 扫描地图寻找高价值点
    vector<Target> targets;
    for (int y = 0; y < MAXM; y++)
    {
      for (int x = 0; x < MAXN; x++)
      {
        Point p = {y, x};
        if (!map->is_safe(p))
          continue;

        int pos_value = map->value(p);
        if (pos_value <= 0)
          continue;

        int distance = Utils::manhattan_distance(head, p);
        float dist_factor = (distance <= NEAR_FOOD_THRESHOLD) ? NEAR_FOOD_FACTOR / (distance + 1) : FAR_FOOD_FACTOR / distance;

        targets.push_back({p, pos_value, distance, map->score(p) * dist_factor});
      }
    }

    if (!targets.empty())
    {
      sort(targets.begin(), targets.end(),
           [](const Target &a, const Target &b)
           { return a.priority > b.priority; });
      return targets[0];
    }

    // 默认目标 - 安全区中心
    return {{(state.current_safe_zone.y_min + state.current_safe_zone.y_max) / 2,
             (state.current_safe_zone.x_min + state.current_safe_zone.x_max) / 2},
            0,
            0,
            0};
  }

  // 方向评估 - 计算每个方向的综合得分
  float evaluate_direction(int direction, const GameState &state, const Target &target)
  {
    const auto &head = state.get_self().get_head();
    Point next_pos = Utils::get_next_position(head, direction);

    if (!is_direction_safe(direction, state))
      return -1000.0f;

    float pos_score = map->score(next_pos);
    int curr_dist = Utils::manhattan_distance(head, target.pos);
    int next_dist = Utils::manhattan_distance(next_pos, target.pos);
    float distance_score = curr_dist - next_dist;
    float turn_score = Utils::is_grid_point(head) ? 2.0f : 0.0f;

    return pos_score + distance_score * 3.0f + turn_score;
  }

  // 安全检查 - 检查指定方向是否安全
  bool is_direction_safe(int direction, const GameState &state)
  {
    const auto &self = state.get_self();
    Point next = Utils::get_next_position(self.get_head(), direction);

    // 检查边界
    if (next.x < state.current_safe_zone.x_min || next.x > state.current_safe_zone.x_max ||
        next.y < state.current_safe_zone.y_min || next.y > state.current_safe_zone.y_max)
    {
      return false;
    }

    // 有护盾时大部分碰撞可忽略
    if (self.shield_time > 0)
    {
      for (const auto &chest : state.chests)
      {
        if (!self.has_key && next.x == chest.pos.x && next.y == chest.pos.y)
        {
          return false; // 无钥匙碰撞宝箱，护盾无效
        }
      }
      return true;
    }

    // 检查各种碰撞
    for (const auto &snake : state.snakes)
    {
      if (snake.id == MYID)
        continue;
      for (const auto &part : snake.body)
      {
        if (next.x == part.x && next.y == part.y)
          return false;
      }
    }

    for (size_t i = 1; i < self.body.size(); i++)
    {
      if (next.x == self.body[i].x && next.y == self.body[i].y)
        return false;
    }

    for (const auto &item : state.items)
    {
      if (item.value == TRAP_VALUE && next.x == item.pos.x && next.y == item.pos.y)
      {
        return false;
      }
    }

    return true;
  }

  // 护盾使用判断 - 检查是否应该激活护盾
  bool should_use_shield(const GameState &state)
  {
    const auto &self = state.get_self();
    if (self.shield_cd > 0 || self.score < 20 || self.shield_time > 0)
      return false;

    // 检查头对头碰撞风险
    for (const auto &snake : state.snakes)
    {
      if (snake.id == MYID)
        continue;

      int distance = Utils::manhattan_distance(self.get_head(), snake.get_head());
      if (distance <= COLLISION_DETECT_DISTANCE)
      {
        int my_dir = self.direction, other_dir = snake.direction;
        if ((my_dir == 0 && other_dir == 2) || (my_dir == 2 && other_dir == 0) ||
            (my_dir == 1 && other_dir == 3) || (my_dir == 3 && other_dir == 1))
        {
          return true;
        }
      }
    }
    return false;
  }

public:
  SnakeAI()
  {
    map = new Map();
    path_finder = new PathFinder(map);
  }

  ~SnakeAI()
  {
    delete path_finder;
    delete map;
  }

  // 主决策函数 - AI的核心逻辑
  int make_decision(const GameState &state)
  {
    map->analyze(state); // 分析当前地图状态

    Target target = select_target(state); // 选择目标
    const auto &head = state.get_self().get_head();
    current_path = path_finder->find_path(head, target.pos, state); // 规划路径

    // 按路径行动
    if (!current_path.empty())
    {
      int next_dir = Utils::get_direction(head, current_path[0]);
      if (next_dir != -1 && is_direction_safe(next_dir, state))
      {
        return next_dir;
      }
    }

    // 评估所有方向并选择最佳
    vector<pair<int, float>> direction_scores;
    for (int dir = 0; dir < 4; dir++)
    {
      direction_scores.push_back({dir, evaluate_direction(dir, state, target)});
    }

    sort(direction_scores.begin(), direction_scores.end(),
         [](const auto &a, const auto &b)
         { return a.second > b.second; });

    // 检查护盾使用
    if (should_use_shield(state))
      return SHIELD_COMMAND;

    // 选择最佳方向
    for (const auto &[dir, score] : direction_scores)
    {
      if (score > 0)
        return dir;
    }

    // 后备方案 - 随机方向
    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
    return uniform_int_distribution<int>(0, 3)(rng);
  }
};

// 游戏状态读取函数 - 解析输入数据
void read_game_state(GameState &s)
{
  cin >> s.remaining_ticks;

  // 读取物品信息
  int item_count;
  cin >> item_count;
  s.items.resize(item_count);
  for (int i = 0; i < item_count; ++i)
  {
    cin >> s.items[i].pos.y >> s.items[i].pos.x >> s.items[i].value >> s.items[i].lifetime;
  }

  // 读取蛇信息
  int snake_count;
  cin >> snake_count;
  s.snakes.resize(snake_count);
  unordered_map<int, int> id2idx;

  for (int i = 0; i < snake_count; ++i)
  {
    auto &sn = s.snakes[i];
    cin >> sn.id >> sn.length >> sn.score >> sn.direction >> sn.shield_cd >> sn.shield_time;
    sn.body.resize(sn.length);
    for (int j = 0; j < sn.length; ++j)
    {
      cin >> sn.body[j].y >> sn.body[j].x;
    }
    if (sn.id == MYID)
      s.self_idx = i;
    id2idx[sn.id] = i;
  }

  // 读取宝箱信息
  int chest_count;
  cin >> chest_count;
  s.chests.resize(chest_count);
  for (int i = 0; i < chest_count; ++i)
  {
    cin >> s.chests[i].pos.y >> s.chests[i].pos.x >> s.chests[i].score;
  }

  // 读取钥匙信息
  int key_count;
  cin >> key_count;
  s.keys.resize(key_count);
  for (int i = 0; i < key_count; ++i)
  {
    auto &key = s.keys[i];
    cin >> key.pos.y >> key.pos.x >> key.holder_id >> key.remaining_time;
    if (key.holder_id != -1)
    {
      auto it = id2idx.find(key.holder_id);
      if (it != id2idx.end())
      {
        s.snakes[it->second].has_key = true;
      }
    }
  }

  // 读取安全区信息
  cin >> s.current_safe_zone.x_min >> s.current_safe_zone.y_min >> s.current_safe_zone.x_max >> s.current_safe_zone.y_max;
  cin >> s.next_shrink_tick >> s.next_safe_zone.x_min >> s.next_safe_zone.y_min >> s.next_safe_zone.x_max >> s.next_safe_zone.y_max;
  cin >> s.final_shrink_tick >> s.final_safe_zone.x_min >> s.final_safe_zone.y_min >> s.final_safe_zone.x_max >> s.final_safe_zone.y_max;
}

// 主函数 - 程序入口点
int main()
{
  GameState current_state;
  read_game_state(current_state);

  static SnakeAI ai; // 静态变量保持AI状态
  int decision = ai.make_decision(current_state);

  cout << decision << endl;
  //todo 如果需要写入 Memory，在此处写入

  return 0;
}
