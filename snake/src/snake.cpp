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

// ========== 游戏规则固定常量 ==========
constexpr int MAXN = 40, MAXM = 30, MAX_TICKS = 256;
constexpr int MYID = 2023202295; // 学号
constexpr int MOVE_SPEED = 2, GRID_POINT_OFFSET = 3, GRID_POINT_INTERVAL = 20;

// 游戏机制常量（规则规定）
constexpr int INITIAL_SNAKE_LENGTH = 5;
constexpr int SCORE_GROWTH_THRESHOLD = 20;  // 每20分增长一格
constexpr int GROWTH_BEAN_LENGTH = 2;       // 增长豆增加长度
constexpr int TRAP_PENALTY = 10;            // 陷阱扣分
constexpr int SHIELD_ACTIVATION_COST = 20;  // 护盾消耗分数
constexpr int SHIELD_DURATION = 5;          // 护盾持续时间
constexpr int SHIELD_COOLDOWN = 30;         // 护盾冷却时间
constexpr int INITIAL_SHIELD_TIME = 10;     // 初始护盾时间
constexpr int KEY_MAX_HOLDING_TIME = 30;    // 钥匙最大持有时间
constexpr int FOOD_GENERATION_INTERVAL = 3; // 食物生成间隔
constexpr int NORMAL_FOOD_LIFETIME = 60;    // 普通食物生命周期
constexpr int SPECIAL_ITEM_LIFETIME = 80;   // 特殊物品生命周期

// 物品类型值（规则规定）
constexpr int GROWTH_BEAN_VALUE = -1;
constexpr int TRAP_VALUE = -2;
constexpr int SHIELD_COMMAND = 4;

// ========== AI策略可调参数 ==========
// 距离类参数
constexpr int IMMEDIATE_RESPONSE_DISTANCE = 3; // 即时反应距离
constexpr int NEAR_FOOD_THRESHOLD = 5;         // 近距离食物阈值
constexpr int COLLISION_DETECT_DISTANCE = 4;   // 碰撞检测距离
constexpr int WALL_BUFFER_DISTANCE = 2;        // 墙体缓冲距离

// 危险等级参数
constexpr int DANGER_THRESHOLD = 80;    // 安全阈值
constexpr int HIGH_DANGER = 200;        // 高危险度
constexpr int EXTREME_DANGER = 255;     // 极度危险度
constexpr int SNAKE_BODY_DANGER = 255;  // 蛇身危险度
constexpr int SNAKE_AROUND_DANGER = 50; // 蛇周围危险度
constexpr int TRAP_DANGER = 150;        // 陷阱危险度

// 收缩区域危险参数
constexpr int SHRINK_TIMES[] = {5, 10, 15, 20};        // 收缩时间阈值
constexpr int SHRINK_DANGERS[] = {250, 200, 150, 100}; // 对应危险等级
constexpr int CENTER_PREDICTION_TIME = 15;             // 中心预测提前时间

// 食物价值参数
constexpr int NORMAL_FOOD_VALUE_MULTIPLIER = 30; // 普通食物价值倍数
constexpr int GROWTH_BEAN_VALUE_SCORE = 90;      // 增长豆评分价值
constexpr int KEY_VALUE_SCORE = 150;             // 钥匙评分价值
constexpr int CHEST_VALUE_SCORE = 200;           // 宝箱评分价值

// 距离因子参数
constexpr float NEAR_FOOD_FACTOR = 10.0f; // 近距离食物因子
constexpr float FAR_FOOD_FACTOR = 5.0f;   // 远距离食物因子

// 评分权重参数
constexpr float DANGER_WEIGHT = 5.0f;   // 危险度权重
constexpr float VALUE_WEIGHT = 3.0f;    // 价值度权重
constexpr float CENTRAL_WEIGHT = 1.0f;  // 中心度权重
constexpr float DISTANCE_WEIGHT = 3.0f; // 距离权重
constexpr float TURN_BONUS = 2.0f;      // 转向奖励

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

struct GameState
{ // 游戏状态结构
  int remaining_ticks, next_shrink_tick, final_shrink_tick, self_idx;
  vector<Item> items;
  vector<Snake> snakes;
  vector<Chest> chests;
  vector<Key> keys;
  SafeZoneBounds current_safe_zone, next_safe_zone, final_safe_zone;
  const Snake &get_self() const { return snakes[self_idx]; }
};

class Utils
{ // 工具函数类 - 提供基础计算功能
public:
  static int manhattan_distance(const Point &a, const Point &b)
  { // 曼哈顿距离计算
    return abs(a.x - b.x) + abs(a.y - b.y);
  }

  static bool is_grid_point(const Point &p)
  { // 检查可转向位置(是否为整格点)
    return ((p.x - GRID_POINT_OFFSET) % GRID_POINT_INTERVAL == 0) &&
           ((p.y - GRID_POINT_OFFSET) % GRID_POINT_INTERVAL == 0);
  }

  static Point get_next_position(const Point &p, int direction)
  { // 根据方向获取下一个位置
    static const int dx[] = {-MOVE_SPEED, 0, MOVE_SPEED, 0};
    static const int dy[] = {0, -MOVE_SPEED, 0, MOVE_SPEED};
    return {p.y + dy[direction], p.x + dx[direction]};
  }

  static int get_direction(const Point &from, const Point &to)
  { // 根据两点计算方向
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

class Map
{ // 多维度地图分析类 - 管理危险度、价值度、中心度三个维度
private:
  int danger_map[MAXM][MAXN], value_map[MAXM][MAXN], central_map[MAXM][MAXN];

  void analyze_danger(const GameState &state)
  { // 危险度分析 - 标记所有危险区域
    memset(danger_map, 0, sizeof(danger_map));

    for (int y = 0; y < MAXM; y++)
    { // 标记安全区外和边界
      for (int x = 0; x < MAXN; x++)
      {
        if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max ||
            y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max)
          danger_map[y][x] = EXTREME_DANGER;
        else if (x <= WALL_BUFFER_DISTANCE || x >= MAXN - WALL_BUFFER_DISTANCE - 1 ||
                 y <= WALL_BUFFER_DISTANCE || y >= MAXM - WALL_BUFFER_DISTANCE - 1)
          danger_map[y][x] += 100; // 墙边危险
      }
    }

    for (const auto &snake : state.snakes) //! 容易被蛇杀？优化这里
    {                                      // 标记其他蛇和其他危险物
      if (snake.id == MYID)
        continue;
      for (const auto &part : snake.body)
      {
        danger_map[part.y][part.x] = SNAKE_BODY_DANGER;

        for (int dy = -1; dy <= 1; dy++) //! 这里其实可以优化，因为蛇身危险度已经很高了，不必增加周围危险度（后期更是如此），或许可以考虑更有利的防撞措施          
          for (int dx = -1; dx <= 1; dx++)
          { // 周围区域增加危险度
            int nx = part.x + dx, ny = part.y + dy;
            if (nx >= 0 && nx < MAXN && ny >= 0 && ny < MAXM)
              danger_map[ny][nx] += SNAKE_AROUND_DANGER;
          }
      }
    }

    for (const auto &item : state.items)// 标记陷阱和宝箱
      if (item.value == TRAP_VALUE)
        danger_map[item.pos.y][item.pos.x] = TRAP_DANGER;
    for (const auto &chest : state.chests)
      if (!state.get_self().has_key)
        danger_map[chest.pos.y][chest.pos.x] = EXTREME_DANGER;

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
      if (ticks_until_shrink <= SHRINK_TIMES[i])
      {
        danger_level = SHRINK_DANGERS[i];
        break;
      }

    for (int y = 0; y < MAXM; y++)
      for (int x = 0; x < MAXN; x++)
        if ((x >= state.current_safe_zone.x_min && x <= state.current_safe_zone.x_max &&
             y >= state.current_safe_zone.y_min && y <= state.current_safe_zone.y_max) &&
            (x < state.next_safe_zone.x_min || x > state.next_safe_zone.x_max ||
             y < state.next_safe_zone.y_min || y > state.next_safe_zone.y_max))
          danger_map[y][x] = danger_level;
    }

  void analyze_value(const GameState &state)
  {// 价值度分析 - 标记所有有价值的物品
    memset(value_map, 0, sizeof(value_map));

    for (const auto &item : state.items)// 食物和增长豆
    {
      if (item.value > 0)
        value_map[item.pos.y][item.pos.x] = item.value * NORMAL_FOOD_VALUE_MULTIPLIER;
      else if (item.value == GROWTH_BEAN_VALUE)
        value_map[item.pos.y][item.pos.x] = GROWTH_BEAN_VALUE_SCORE;
    }

    for (const auto &key : state.keys)// 钥匙价值
      if (key.holder_id == -1)
        value_map[key.pos.y][key.pos.x] = KEY_VALUE_SCORE;

    if (state.get_self().has_key)// 宝箱价值(如果有钥匙)
      for (const auto &chest : state.chests)
        value_map[chest.pos.y][chest.pos.x] = CHEST_VALUE_SCORE;
  }

  
  void analyze_central(const GameState &state)
  {// 中心度分析 - 计算到安全区中心的距离
    memset(central_map, 0, sizeof(central_map));

    int center_x = (state.current_safe_zone.x_min + state.current_safe_zone.x_max) / 2;
    int center_y = (state.current_safe_zone.y_min + state.current_safe_zone.y_max) / 2;

    int current_tick = MAX_TICKS - state.remaining_ticks;// 如果即将收缩，使用下一个安全区中心
    if (state.next_shrink_tick > 0 && state.next_shrink_tick - current_tick <= CENTER_PREDICTION_TIME)
    {
      center_x = (state.next_safe_zone.x_min + state.next_safe_zone.x_max) / 2;
      center_y = (state.next_safe_zone.y_min + state.next_safe_zone.y_max) / 2;
    }

    int max_dist = MAXN + MAXM;
    for (int y = 0; y < MAXM; y++)
      for (int x = 0; x < MAXN; x++)
      {
        int dist = abs(x - center_x) + abs(y - center_y);
        central_map[y][x] = EXTREME_DANGER * (1.0 - (float)dist / max_dist);
      }
  }

public:
  void analyze(const GameState &state)
  {// 分析地图所有维度
    analyze_danger(state);
    analyze_value(state);
    analyze_central(state);
  }

  int danger(const Point &p) const
  {// 各维度危险评估接口
    if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN)
      return EXTREME_DANGER;
    return danger_map[p.y][p.x];
  }

  int value(const Point &p) const
  {// 各维度安全评估接口
    if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN)
      return 0;
    return value_map[p.y][p.x];
  }

  bool is_safe(const Point &p) const
  {
    if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN)
      return false;
    return danger_map[p.y][p.x] < DANGER_THRESHOLD;
  }

  float score(const Point &p) const
  {// 综合评分 - 结合三个维度
    if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN)
      return -1000.0f;
    if (danger_map[p.y][p.x] >= HIGH_DANGER)
      return -500.0f;

    float danger_score = 1.0f - danger_map[p.y][p.x] / 255.0f;
    float value_score = value_map[p.y][p.x] / 255.0f;
    float central_score = central_map[p.y][p.x] / 255.0f;

    return danger_score * DANGER_WEIGHT +
           value_score * VALUE_WEIGHT +
           central_score * CENTRAL_WEIGHT;
  }
};

class PathFinder
{// 路径规划类 - 简化版A*算法
private:
  Map *map;

  vector<Point> generate_basic_path(const Point &start, const Point &goal, const GameState & /* state */)
  {// 生成基础路径 - 先水平后垂直移动
    vector<Point> path;
    Point current = start;
    path.push_back(current);

    while (abs(current.x - goal.x) > MOVE_SPEED)
    {// 水平移动
      int dir = (current.x < goal.x) ? 2 : 0;
      Point next = Utils::get_next_position(current, dir);
      if (!map->is_safe(next))
        break;
      path.push_back(next);
      current = next;
    }

    while (abs(current.y - goal.y) > MOVE_SPEED)
    {// 垂直移动
      int dir = (current.y < goal.y) ? 3 : 1;
      Point next = Utils::get_next_position(current, dir);
      if (!map->is_safe(next))
        break;
      path.push_back(next);
      current = next;
    }

    if (current.x != goal.x || current.y != goal.y)
      path.push_back(goal);
    return path;
  }

public:
  PathFinder(Map *map_ptr) : map(map_ptr) {}
  
  vector<Point> find_path(const Point &start, const Point &goal, const GameState &state)
  {// 寻找从起点到终点的路径
    if (start.x == goal.x && start.y == goal.y)
      return {};
    return generate_basic_path(start, goal, state);
  }
};

class SnakeAI
{// 主决策类 - 蛇AI大脑
private:
  Map *map;
  PathFinder *path_finder;
  vector<Point> current_path;
  
  Target select_target(const GameState &state)
  {// 目标选择 - 根据即时反应和综合评估选择最佳目标
    const auto &head = state.get_self().get_head();
    
    const double time_buffer_factor = 1.5; // 预估到达时间计算参数 - 考虑转弯和路径不直接带来的额外开销

    for (const auto &item : state.items)
    {// 即时反应 - 检测头部附近的食物
      if ((item.value > 0 || item.value == GROWTH_BEAN_VALUE) && map->is_safe(item.pos))
      {
        int dist = Utils::manhattan_distance(head, item.pos);
        
        int estimated_time_to_reach = static_cast<int>(dist * time_buffer_factor / MOVE_SPEED);
        if (estimated_time_to_reach >= item.lifetime)// 检查食物是否能在其生命周期内到达
          continue; // 来不及吃到，忽略这个食物
            
        if (dist <= IMMEDIATE_RESPONSE_DISTANCE)
          return {item.pos, item.value > 0 ? item.value : 3, dist, 1000.0};
      }
    }

    // 综合评估 - 扫描地图寻找高价值点
    vector<Target> targets;
    for (int y = 0; y < MAXM; y++)
      for (int x = 0; x < MAXN; x++)
      {
        Point p = {y, x};
        if (!map->is_safe(p))
          continue;

        int pos_value = map->value(p);
        if (pos_value <= 0)
          continue;

        int distance = Utils::manhattan_distance(head, p);
        
        bool is_reachable = true;// 对于食物类物品，检查是否能在其生命周期内到达
        for (const auto &item : state.items) {
          if (item.pos.y == y && item.pos.x == x) {
            int estimated_time_to_reach = static_cast<int>(distance * time_buffer_factor / MOVE_SPEED);
            if (estimated_time_to_reach >= item.lifetime) {
              is_reachable = false;
              break;
            }
          }
        }
        if (!is_reachable)
          continue; // 来不及吃到，忽略这个食物
        
        float dist_factor = (distance <= NEAR_FOOD_THRESHOLD) ? NEAR_FOOD_FACTOR / (distance + 1) : FAR_FOOD_FACTOR / distance; // 计算距离因子，近距离食物优先级更高

        targets.push_back({p, pos_value, distance, map->score(p) * dist_factor});
      }

    if (!targets.empty())
    {
      sort(targets.begin(), targets.end(),
           [](const Target &a, const Target &b)
           { return a.priority > b.priority; });
      return targets[0];
    }

    return {{(state.current_safe_zone.y_min + state.current_safe_zone.y_max) / 2,(state.current_safe_zone.x_min + state.current_safe_zone.x_max) / 2},0,0,0};
  }// 默认目标 - 安全区中心

  float evaluate_direction(int direction, const GameState &state, const Target &target)
  {// 方向评估 - 计算每个方向的综合得分
    const auto &head = state.get_self().get_head();
    Point next_pos = Utils::get_next_position(head, direction);

    if (!is_direction_safe(direction, state))
      return -1000.0f;

    float pos_score = map->score(next_pos);
    int curr_dist = Utils::manhattan_distance(head, target.pos);
    int next_dist = Utils::manhattan_distance(next_pos, target.pos);
    float distance_score = curr_dist - next_dist;
    float turn_score = Utils::is_grid_point(head) ? TURN_BONUS : 0.0f;

    return pos_score + distance_score * DISTANCE_WEIGHT + turn_score;
  }

  bool is_direction_safe(int direction, const GameState &state)
  {// 安全检查 - 检查指定方向是否安全
    const auto &self = state.get_self();
    Point next = Utils::get_next_position(self.get_head(), direction);

    if (next.x < state.current_safe_zone.x_min || next.x > state.current_safe_zone.x_max || next.y < state.current_safe_zone.y_min || next.y > state.current_safe_zone.y_max)
      return false; // 检查边界

    if (self.shield_time > 0)//! 有护盾时应该前往安全地方，防止盾消失就死
    {// 有护盾时大部分碰撞可忽略
      for (const auto &chest : state.chests)
        if (!self.has_key && next.x == chest.pos.x && next.y == chest.pos.y)
          return false; // 无钥匙碰撞宝箱，护盾无效
      return true;
    }

    for (const auto &snake : state.snakes)
    {//敌人身体
      if (snake.id == MYID)
        continue;
      for (const auto &part : snake.body)
        if (next.x == part.x && next.y == part.y)
          return false;
    }

    for (const auto &item : state.items)//陷阱 -10
      if (item.value == TRAP_VALUE && next.x == item.pos.x && next.y == item.pos.y)
        return false;

    return true;
  }

  bool should_use_shield(const GameState &state)
  {//! 护盾使用判断 - 检查是否应该激活护盾  很需要改进，护盾成本太高了
    const auto &self = state.get_self();
    if (self.shield_cd > 0 || self.score < SHIELD_ACTIVATION_COST || self.shield_time > 0)
      return false;

    for (const auto &snake : state.snakes)
    {//! 检查头对头碰撞风险  这是该用护盾的时候吗
      if (snake.id == MYID)
        continue;

      int distance = Utils::manhattan_distance(self.get_head(), snake.get_head());
      if (distance <= COLLISION_DETECT_DISTANCE)
      {
        int my_dir = self.direction, other_dir = snake.direction;
        if ((my_dir == 0 && other_dir == 2) || (my_dir == 2 && other_dir == 0) || (my_dir == 1 && other_dir == 3) || (my_dir == 3 && other_dir == 1))
          return true;
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

  int make_decision(const GameState &state)
  {// 主决策函数 - AI的核心逻辑
    map->analyze(state); // 分析当前地图状态

    Target target = select_target(state); // 选择目标
    const auto &head = state.get_self().get_head();
    current_path = path_finder->find_path(head, target.pos, state); // 规划路径

    if (!current_path.empty())
    {// 按路径行动
      int next_dir = Utils::get_direction(head, current_path[0]);
      if (next_dir != -1 && is_direction_safe(next_dir, state))
        return next_dir;
    }

    
    vector<pair<int, float>> direction_scores;
    for (int dir = 0; dir < 4; dir++)// 评估所有方向并选择最佳
      direction_scores.push_back({dir, evaluate_direction(dir, state, target)});

    sort(direction_scores.begin(), direction_scores.end(),[](const auto &a, const auto &b){ return a.second > b.second; });
//! 上面这个的意思是按照得分从高到低排序
    if (should_use_shield(state))// 检查护盾使用
      return SHIELD_COMMAND;

    for (const auto &[dir, score] : direction_scores)// 选择最佳方向
      if (score > 0)
        return dir;

    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());//! 后备方案 - 随机方向 很需要改进，随机方向很可能会撞墙
    return uniform_int_distribution<int>(0, 3)(rng);
  }
};

void read_game_state(GameState &s)
{// 游戏状态读取函数 - 解析输入数据
  cin >> s.remaining_ticks;

  int item_count;
  cin >> item_count;// 读取物品信息
  s.items.resize(item_count);
  for (int i = 0; i < item_count; ++i)
    cin >> s.items[i].pos.y >> s.items[i].pos.x >> s.items[i].value >> s.items[i].lifetime;

  int snake_count;
  cin >> snake_count;// 读取蛇信息
  s.snakes.resize(snake_count);
  unordered_map<int, int> id2idx;

  for (int i = 0; i < snake_count; ++i)
  {
    auto &sn = s.snakes[i];
    cin >> sn.id >> sn.length >> sn.score >> sn.direction >> sn.shield_cd >> sn.shield_time;
    sn.body.resize(sn.length);
    for (int j = 0; j < sn.length; ++j)
      cin >> sn.body[j].y >> sn.body[j].x;
    if (sn.id == MYID)
      s.self_idx = i;
    id2idx[sn.id] = i;
  }

  int chest_count;
  cin >> chest_count;// 读取宝箱信息
  s.chests.resize(chest_count);
  for (int i = 0; i < chest_count; ++i)
    cin >> s.chests[i].pos.y >> s.chests[i].pos.x >> s.chests[i].score;

  int key_count;
  cin >> key_count;// 读取钥匙信息
  s.keys.resize(key_count);
  for (int i = 0; i < key_count; ++i)
  {
    auto &key = s.keys[i];
    cin >> key.pos.y >> key.pos.x >> key.holder_id >> key.remaining_time;
    if (key.holder_id != -1)
    {
      auto it = id2idx.find(key.holder_id);
      if (it != id2idx.end())
        s.snakes[it->second].has_key = true;
    }
  }

  cin >> s.current_safe_zone.x_min >> s.current_safe_zone.y_min >> s.current_safe_zone.x_max >> s.current_safe_zone.y_max;
  cin >> s.next_shrink_tick >> s.next_safe_zone.x_min >> s.next_safe_zone.y_min >> s.next_safe_zone.x_max >> s.next_safe_zone.y_max;
  cin >> s.final_shrink_tick >> s.final_safe_zone.x_min >> s.final_safe_zone.y_min >> s.final_safe_zone.x_max >> s.final_safe_zone.y_max;
} //  ↑ 读取安全区信息

int main()
{// 主函数 - 程序入口点
  GameState current_state;
  read_game_state(current_state);

  static SnakeAI ai; // 静态变量保持AI状态
  int decision = ai.make_decision(current_state);

  cout << decision << endl;
  // todo 如果需要写入 Memory，在此处写入

  return 0;
}
