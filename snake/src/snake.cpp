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
#include <iomanip>

using namespace std;

// ========== 游戏规则固定常量 ==========
constexpr int MAXN = 40, MAXM = 30, MAX_TICKS = 256;
constexpr int MYID = 2023202295; // 学号
constexpr int MOVE_SPEED = 2, GRID_POINT_OFFSET = 3, GRID_POINT_INTERVAL = 20;

// 游戏机制常量（规则规定）
[[maybe_unused]] constexpr int INITIAL_SNAKE_LENGTH = 5;
[[maybe_unused]] constexpr int SCORE_GROWTH_THRESHOLD = 20;  // 每20分增长一格
[[maybe_unused]] constexpr int GROWTH_BEAN_LENGTH = 2;       // 增长豆增加长度
[[maybe_unused]] constexpr int TRAP_PENALTY = 10;            // 陷阱扣分
constexpr int SHIELD_ACTIVATION_COST = 20;                   // 护盾消耗分数
[[maybe_unused]] constexpr int SHIELD_DURATION = 5;          // 护盾持续时间
[[maybe_unused]] constexpr int SHIELD_COOLDOWN = 30;         // 护盾冷却时间
[[maybe_unused]] constexpr int INITIAL_SHIELD_TIME = 10;     // 初始护盾时间
[[maybe_unused]] constexpr int KEY_MAX_HOLDING_TIME = 30;    // 钥匙最大持有时间
[[maybe_unused]] constexpr int FOOD_GENERATION_INTERVAL = 3; // 食物生成间隔
[[maybe_unused]] constexpr int NORMAL_FOOD_LIFETIME = 60;    // 普通食物生命周期
[[maybe_unused]] constexpr int SPECIAL_ITEM_LIFETIME = 80;   // 特殊物品生命周期

// 物品类型值（规则规定）
constexpr int GROWTH_BEAN_VALUE = -1;
constexpr int TRAP_VALUE = -2;
constexpr int SHIELD_COMMAND = 4;

// ========== AI策略可调参数 ==========
// 距离类参数
constexpr int IMMEDIATE_RESPONSE_DISTANCE = 3;           // 即时反应距离
[[maybe_unused]] constexpr int NEAR_FOOD_THRESHOLD = 5;  // 近距离食物阈值
constexpr int COLLISION_DETECT_DISTANCE = 4;             // 碰撞检测距离
[[maybe_unused]] constexpr int WALL_BUFFER_DISTANCE = 2; // 墙体缓冲距离

// 危险等级参数
constexpr int DANGER_THRESHOLD = 80;    // 安全阈值
constexpr int HIGH_DANGER = 200;        // 高危险度
constexpr int EXTREME_DANGER = 255;     // 极度危险度
constexpr int SNAKE_BODY_DANGER = 255;  // 蛇身危险度
constexpr int SNAKE_AROUND_DANGER = 50; // 蛇周围危险度
constexpr int TRAP_DANGER = 150;        // 陷阱危险度

// 收缩区域危险参数
[[maybe_unused]] constexpr int SHRINK_TIMES[] = {5, 10, 15, 20};        // 收缩时间阈值
[[maybe_unused]] constexpr int SHRINK_DANGERS[] = {250, 200, 150, 100}; // 对应危险等级
[[maybe_unused]] constexpr int CENTER_PREDICTION_TIME = 15;             // 中心预测提前时间

// 食物价值参数
constexpr int NORMAL_FOOD_VALUE_MULTIPLIER = 30; // 普通食物价值倍数
// 增长豆评分价值 - 按游戏阶段递减
constexpr int GROWTH_BEAN_VALUE_EARLY = 30;      // 早期增长豆价值(等于1分普通食物)
constexpr int GROWTH_BEAN_VALUE_MID = 20;        // 中期增长豆价值
constexpr int GROWTH_BEAN_VALUE_LATE = 10;       // 后期增长豆价值
constexpr int KEY_VALUE_SCORE = 150;             // 钥匙评分价值
constexpr int CHEST_VALUE_SCORE = 200;           // 宝箱评分价值

// 距离因子参数
[[maybe_unused]] constexpr float NEAR_FOOD_FACTOR = 10.0f; // 近距离食物因子
[[maybe_unused]] constexpr float FAR_FOOD_FACTOR = 5.0f;   // 远距离食物因子

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
  int danger_map[MAXM][MAXN], value_map[MAXM][MAXN], center_map[MAXM][MAXN], combined_map[MAXM][MAXN];
  
  // 获取当前游戏阶段的增长豆价值
  int get_growth_bean_value(const GameState &state) {
    // 根据游戏阶段动态降低增长豆价值
    if (state.remaining_ticks > 180)
      return GROWTH_BEAN_VALUE_EARLY; // 早期
    else if (state.remaining_ticks > 100)
      return GROWTH_BEAN_VALUE_MID;   // 中期
    else
      return GROWTH_BEAN_VALUE_LATE;  // 后期
  }
  
  void analyze_danger(const GameState &state)
  {
    memset(danger_map, 0, sizeof(danger_map));

    int buffer = 2;          // 墙体缓冲区大小
    int boundary_buffer = 3; // 安全区内侧缓冲区大小

    for (int y = 0; y < MAXM; y++)
    {
      for (int x = 0; x < MAXN; x++)
      {
        // 标记绝对禁区 - 安全区外
        if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max ||
            y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max)
          danger_map[y][x] = EXTREME_DANGER;
        // 标记安全区内的边界缓冲区
        else if (x < state.current_safe_zone.x_min + boundary_buffer ||
                 x > state.current_safe_zone.x_max - boundary_buffer ||
                 y < state.current_safe_zone.y_min + boundary_buffer ||
                 y > state.current_safe_zone.y_max - boundary_buffer)
          danger_map[y][x] = HIGH_DANGER;

        // 墙体附近区域也标记为高危
        if (x <= buffer || x >= MAXN - buffer - 1 ||
            y <= buffer || y >= MAXM - buffer - 1)
          danger_map[y][x] = HIGH_DANGER;
      }
    }

    for (const auto &snake : state.snakes)
    { // 蛇的危险度分析
      if (snake.id == MYID)
        continue;

      // 蛇身体
      for (const auto &part : snake.body)
      {
        danger_map[part.y][part.x] = SNAKE_BODY_DANGER;

        // 周围区域也增加危险度
        for (int dy = -1; dy <= 1; dy++)
          for (int dx = -1; dx <= 1; dx++)
          {
            int nx = part.x + dx, ny = part.y + dy;
            if (nx >= 0 && nx < MAXN && ny >= 0 && ny < MAXM)
              danger_map[ny][nx] += SNAKE_AROUND_DANGER;
          }
      }

      // 预测蛇头可能移动的位置
      const auto &head = snake.get_head();

      // 分析可能的方向
      for (int dir = 0; dir < 4; dir++)
      {
        if (snake.direction == (dir + 2) % 4)
          continue; // 不能反向移动

        Point next = Utils::get_next_position(head, dir);
        if (next.y >= 0 && next.y < MAXM && next.x >= 0 && next.x < MAXN)
        {
          // 有护盾的蛇危险性更高
          int danger_value = snake.shield_time > 0 ? SNAKE_AROUND_DANGER * 2 : SNAKE_AROUND_DANGER;
          danger_map[next.y][next.x] += danger_value;

          // 二级移动预测 - 看两步远
          for (int dir2 = 0; dir2 < 4; dir2++)
          {
            if (dir2 == (dir + 2) % 4)
              continue; // 不能反向

            Point next2 = Utils::get_next_position(next, dir2);
            if (next2.y >= 0 && next2.y < MAXM && next2.x >= 0 && next2.x < MAXN)
              danger_map[next2.y][next2.x] += danger_value / 2;
          }
        }
      }
    }

    // 标记陷阱和宝箱
    for (const auto &item : state.items)
      if (item.value == TRAP_VALUE)
        danger_map[item.pos.y][item.pos.x] = TRAP_DANGER;

    for (const auto &chest : state.chests)
      if (!state.get_self().has_key)
        danger_map[chest.pos.y][chest.pos.x] = EXTREME_DANGER;

    // 标记收缩区域
    if (state.next_shrink_tick > 0)
      mark_shrinking_areas(state);
  }

  void mark_shrinking_areas(const GameState &state)
  {
    int current_tick = MAX_TICKS - state.remaining_ticks;
    int ticks_until_shrink = state.next_shrink_tick - current_tick;

    // 根据收缩紧迫程度设置危险度
    int danger_level;
    if (ticks_until_shrink <= 5)
      danger_level = 250; // 即将收缩，极度危险
    else if (ticks_until_shrink <= 10)
      danger_level = 200; // 很快收缩，高度危险
    else if (ticks_until_shrink <= 20)
      danger_level = 150; // 中等危险
    else
      danger_level = 100; // 需要关注

    // 标记即将被收缩的区域
    for (int y = 0; y < MAXM; y++)
    {
      for (int x = 0; x < MAXN; x++)
      {
        if ((x >= state.current_safe_zone.x_min && x <= state.current_safe_zone.x_max &&
             y >= state.current_safe_zone.y_min && y <= state.current_safe_zone.y_max) &&
            (x < state.next_safe_zone.x_min || x > state.next_safe_zone.x_max ||
             y < state.next_safe_zone.y_min || y > state.next_safe_zone.y_max))
          danger_map[y][x] = danger_level; // 直接设置为高危险度
      }
    }
  }

  void analyze_value(const GameState &state)
  {
    // 价值度分析 - 标记所有有价值的物品并考虑竞争因素
    memset(value_map, 0, sizeof(value_map));

    // 食物价值分析
    calculate_food_value(state);

    // 钥匙价值
    for (const auto &key : state.keys)
      if (key.holder_id == -1)
      {
        int base_value = KEY_VALUE_SCORE;

        // 如果附近有宝箱，钥匙价值更高
        int min_distance = 999;

        for (const auto &chest : state.chests)
        {
          int dist = Utils::manhattan_distance(key.pos, chest.pos);
          if (dist < min_distance)
          {
            min_distance = dist;
          }
        }

        // 计算竞争系数 - 检查其他蛇与钥匙的距离
        float competition_factor = calculate_competition_factor(state, key.pos);

        // 钥匙价值计算
        int final_value = base_value;

        // 如果宝箱很近，增加价值
        if (min_distance < 10)
        {
          final_value = (int)(final_value * (1.5f - min_distance * 0.05f));
        }

        // 应用竞争系数
        final_value = (int)(final_value * competition_factor);

        value_map[key.pos.y][key.pos.x] = final_value;
      }

    // 宝箱价值(如果有钥匙)
    if (state.get_self().has_key)
    {
      for (const auto &chest : state.chests)
      {
        int base_value = CHEST_VALUE_SCORE;

        // 计算竞争系数 - 检查其他有钥匙的蛇与宝箱的距离
        float competition_factor = 1.0f;
        const auto &self_head = state.get_self().get_head();
        int self_distance = Utils::manhattan_distance(self_head, chest.pos);

        for (const auto &snake : state.snakes)
        {
          if (snake.id != MYID && snake.has_key)
          {
            int dist = Utils::manhattan_distance(snake.get_head(), chest.pos);
            if (dist < self_distance)
            {
              // 如果有蛇比我更接近宝箱，大幅降低价值
              competition_factor *= 0.5f;
            }
          }
        }

        // 根据自身与宝箱的距离调整价值
        float distance_factor = 1.0f;
        if (self_distance > 15)
        {
          distance_factor = 0.8f;
        }

        // 找到自己持有的钥匙
        int key_remaining_time = 30; // 默认值
        for (const auto &key : state.keys)
        {
          if (key.holder_id == MYID)
          {
            key_remaining_time = key.remaining_time;
            break;
          }
        }

        // 根据钥匙剩余时间调整价值
        float time_factor = 1.0f;
        if (key_remaining_time < self_distance / 2 + 5)
        {
          // 如果钥匙可能在到达宝箱前过期，降低价值
          time_factor = 0.1f;
        }
        else if (key_remaining_time < 10)
        {
          // 如果钥匙即将过期但可能来得及
          time_factor = 1.5f;
        }

        int final_value = (int)(base_value * competition_factor * distance_factor * time_factor);
        value_map[chest.pos.y][chest.pos.x] = final_value;
      }
    }
  }

  // 食物密集度与价值计算
  void calculate_food_value(const GameState &state)
  {
    // 根据游戏阶段动态调整密集度半径
    int density_radius;
    if (state.remaining_ticks < 100)
      density_radius = 3; // 游戏后期
    else if (state.remaining_ticks < 150)
      density_radius = 4; // 中后期过渡阶段
    else
      density_radius = 5; // 早期阶段

    float game_progress = 1.0f - (float)state.remaining_ticks / MAX_TICKS;

    // 根据安全区大小衡量地图限制程度
    int safe_zone_width = state.current_safe_zone.x_max - state.current_safe_zone.x_min;
    int safe_zone_height = state.current_safe_zone.y_max - state.current_safe_zone.y_min;
    float map_size_factor = (float)(safe_zone_width * safe_zone_height) / (MAXN * MAXM);

    // 动态调整密集度因子 早期(0.4) -> 后期(0.1)
    const float density_factor = 0.4f - 0.3f * game_progress * (1.0f - map_size_factor);

    // 获取蛇头位置和搜索范围
    const auto &head = state.get_self().get_head();
    const int max_search_distance = 12;

    // 创建食物位置列表，避免重复计算
    std::vector<std::pair<Point, int>> food_points;
    food_points.reserve(state.items.size());

    // 预处理食物信息
    for (const auto &item : state.items)
    {
      if (item.value > 0 || item.value == GROWTH_BEAN_VALUE)
      {
        int dist_to_head = Utils::manhattan_distance(head, item.pos);

        // 只考虑蛇头附近的食物和已有食物的区域
        if (dist_to_head <= max_search_distance || value_map[item.pos.y][item.pos.x] > 0)
        {
          // 检查食物是否能在生命周期内到达
          int estimated_time_to_reach = static_cast<int>(dist_to_head * 1.5f / MOVE_SPEED);
          if (estimated_time_to_reach >= item.lifetime)
            continue; // 忽略无法到达的食物

          int base_value = item.value > 0 ? item.value * NORMAL_FOOD_VALUE_MULTIPLIER : get_growth_bean_value(state);
          food_points.emplace_back(item.pos, base_value);
        }
      }
    }

    if (food_points.empty())
      return;

    // 设置食物自身位置的价值
    for (const auto &[pos, base_value] : food_points)
    {
      value_map[pos.y][pos.x] = base_value;

      // 即将过期的食物价值提高
      for (const auto &item : state.items)
        if (item.pos.y == pos.y && item.pos.x == pos.x && item.lifetime < 15)
          value_map[pos.y][pos.x] = (int)(base_value * 1.5f);
    }

    // 计算食物密集度贡献
    for (int y = std::max(0, head.y - max_search_distance); y < std::min(MAXM, head.y + max_search_distance); y++)
    {
      for (int x = std::max(0, head.x - max_search_distance); x < std::min(MAXN, head.x + max_search_distance); x++)
      {
        // 跳过安全区外和高危区域
        if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max ||
            y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max ||
            (value_map[y][x] == 0 && danger_map[y][x] >= DANGER_THRESHOLD))
          continue;

        // 计算该点的密集度
        int density_value = 0;
        Point current_pos = {y, x};

        for (const auto &[food_pos, base_value] : food_points)
        {
          int dist = Utils::manhattan_distance(current_pos, food_pos);
          if (dist <= density_radius)
          {
            // 线性衰减：距离越远，贡献越小
            float contribution = base_value * (1.0f - (float)dist / (density_radius + 1));
            density_value += (int)contribution;
          }
        }

        // 应用密集度到价值图
        if (value_map[y][x] > 0)
          value_map[y][x] += (int)(density_value * density_factor);
        else if (density_value > 0)
        {
          // 空地密集度价值随游戏进程降低
          float empty_space_factor = 0.03f * (1.0f - game_progress * 0.8f);
          value_map[y][x] += (int)(density_value * density_factor * empty_space_factor);
        }
      }
    }

    // 应用竞争因素
    for (const auto &[pos, _] : food_points)
    {
      float competition_factor = calculate_competition_factor(state, pos);
      value_map[pos.y][pos.x] = (int)(value_map[pos.y][pos.x] * competition_factor);
    }
  }

  // 计算竞争系数 - 如果有其他蛇距离某个点比自己近，则降低价值
  float calculate_competition_factor(const GameState &state, const Point &pos)
  {
    float competition_factor = 1.0f;
    const auto &self_head = state.get_self().get_head();
    int self_distance = Utils::manhattan_distance(self_head, pos);

    for (const auto &snake : state.snakes)
    {
      if (snake.id != MYID && snake.id != -1)
      {
        int dist = Utils::manhattan_distance(snake.get_head(), pos);

        // 如果敌方蛇更近，竞争系数降低
        if (dist < self_distance)
        {
          competition_factor *= 0.7f; // 减少30%价值
        }
        // 如果敌方蛇距离相近，轻微降低价值
        else if (dist <= self_distance + 2)
        {
          competition_factor *= 0.9f; // 减少10%价值
        }
      }
    }

    return competition_factor;
  }

  void analyze_central(const GameState &state)
  {
    memset(center_map, 0, sizeof(center_map));

    // 确定中心点 - 如果即将收缩则使用下一个安全区中心
    int center_x, center_y;
    int current_tick = MAX_TICKS - state.remaining_ticks;

    if (state.next_shrink_tick > 0 && state.next_shrink_tick - current_tick <= 15)
    {
      center_x = (state.next_safe_zone.x_min + state.next_safe_zone.x_max) / 2;
      center_y = (state.next_safe_zone.y_min + state.next_safe_zone.y_max) / 2;
    }
    else
    {
      center_x = (state.current_safe_zone.x_min + state.current_safe_zone.x_max) / 2;
      center_y = (state.current_safe_zone.y_min + state.current_safe_zone.y_max) / 2;
    }

    // 动态调整中心价值
    float game_progress = (float)(MAX_TICKS - state.remaining_ticks) / MAX_TICKS;
    int center_value_base = 10 + (int)(90 * game_progress); // 从10增加到100

    // 计算安全区大小和最大距离
    int safe_width = state.current_safe_zone.x_max - state.current_safe_zone.x_min;
    int safe_height = state.current_safe_zone.y_max - state.current_safe_zone.y_min;
    int max_distance = (safe_width + safe_height) / 2;

    // 填充中心度地图 - 越靠近中心价值越高
    for (int y = 0; y < MAXM; y++)
    {
      for (int x = 0; x < MAXN; x++)
      {
        int distance = abs(x - center_x) + abs(y - center_y);
        float distance_factor = 1.0f - (float)distance / max_distance;
        if (distance_factor < 0)
          distance_factor = 0;
        center_map[y][x] = (int)(center_value_base * distance_factor);
      }
    }
  }

  void calculate_combined_map()
  {
    // 综合评分地图 - 结合三个维度
    for (int y = 0; y < MAXM; y++)
    {
      for (int x = 0; x < MAXN; x++)
      {
        // 检查危险度 - 如果危险度超过阈值，设置为负值
        if (danger_map[y][x] >= HIGH_DANGER)
        {
          combined_map[y][x] = -danger_map[y][x]; // 高危区域为负分
        }
        else
        {
          // 计算综合分数: 价值度 + 中心度 - 危险度的加权和
          combined_map[y][x] =
              value_map[y][x] * VALUE_WEIGHT +
              center_map[y][x] * CENTRAL_WEIGHT -
              danger_map[y][x] * DANGER_WEIGHT / 50;
        }
      }
    }
  }

public:
  void analyze(const GameState &state)
  {
    analyze_danger(state);
    analyze_value(state);
    analyze_central(state);
    calculate_combined_map();
  }

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

  int center(const Point &p) const
  {
    if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN)
      return 0;
    return center_map[p.y][p.x];
  }

  int combined_score(const Point &p) const
  {
    if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN)
      return -9999;
    return combined_map[p.y][p.x];
  }

  bool is_safe(const Point &p) const
  {
    if (p.y < 0 || p.y >= MAXM || p.x < 0 || p.x >= MAXN)
      return false;
    return danger_map[p.y][p.x] < DANGER_THRESHOLD;
  }

  // 调试打印方法 - 查看某点周围的评分情况
  void print_area_around(const Point &p, int radius) const
  {
    std::cerr << "===== 地图分析 =====" << std::endl;
    std::cerr << "位置: (" << p.y << ", " << p.x << ")" << std::endl;

    // 打印危险度
    std::cerr << "危险度矩阵:" << std::endl;
    for (int y = p.y - radius; y <= p.y + radius; y++)
    {
      for (int x = p.x - radius; x <= p.x + radius; x++)
      {
        if (y >= 0 && y < MAXM && x >= 0 && x < MAXN)
        {
          if (y == p.y && x == p.x)
            std::cerr << "[" << std::setw(3) << danger_map[y][x] << "]";
          else
            std::cerr << " " << std::setw(3) << danger_map[y][x] << " ";
        }
        else
        {
          std::cerr << " ### ";
        }
      }
      std::cerr << std::endl;
    }

    // 打印价值度
    std::cerr << "价值度矩阵:" << std::endl;
    for (int y = p.y - radius; y <= p.y + radius; y++)
    {
      for (int x = p.x - radius; x <= p.x + radius; x++)
      {
        if (y >= 0 && y < MAXM && x >= 0 && x < MAXN)
        {
          if (y == p.y && x == p.x)
            std::cerr << "[" << std::setw(3) << value_map[y][x] << "]";
          else
            std::cerr << " " << std::setw(3) << value_map[y][x] << " ";
        }
        else
        {
          std::cerr << " ### ";
        }
      }
      std::cerr << std::endl;
    }

    // 打印中心度
    std::cerr << "中心度矩阵:" << std::endl;
    for (int y = p.y - radius; y <= p.y + radius; y++)
    {
      for (int x = p.x - radius; x <= p.x + radius; x++)
      {
        if (y >= 0 && y < MAXM && x >= 0 && x < MAXN)
        {
          if (y == p.y && x == p.x)
            std::cerr << "[" << std::setw(3) << center_map[y][x] << "]";
          else
            std::cerr << " " << std::setw(3) << center_map[y][x] << " ";
        }
        else
        {
          std::cerr << " ### ";
        }
      }
      std::cerr << std::endl;
    }

    // 打印综合评分
    std::cerr << "综合评分矩阵:" << std::endl;
    for (int y = p.y - radius; y <= p.y + radius; y++)
    {
      for (int x = p.x - radius; x <= p.x + radius; x++)
      {
        if (y >= 0 && y < MAXM && x >= 0 && x < MAXN)
        {
          if (y == p.y && x == p.x)
            std::cerr << "[" << std::setw(4) << combined_map[y][x] << "]";
          else
            std::cerr << " " << std::setw(4) << combined_map[y][x] << " ";
        }
        else
        {
          std::cerr << " #### ";
        }
      }
      std::cerr << std::endl;
    }
  }
};

class PathFinder
{ // 路径规划类 - 简化版A*算法
private:
  Map *map;

  vector<Point> generate_basic_path(const Point &start, const Point &goal, const GameState & /* state */)
  { // 生成基础路径 - 先水平后垂直移动
    vector<Point> path;
    Point current = start;
    path.push_back(current);

    while (abs(current.x - goal.x) > MOVE_SPEED)
    { // 水平移动
      int dir = (current.x < goal.x) ? 2 : 0;
      Point next = Utils::get_next_position(current, dir);
      if (!map->is_safe(next))
        break;
      path.push_back(next);
      current = next;
    }

    while (abs(current.y - goal.y) > MOVE_SPEED)
    { // 垂直移动
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
  { // 寻找从起点到终点的路径
    if (start.x == goal.x && start.y == goal.y)
      return {};
    return generate_basic_path(start, goal, state);
  }
};

class SnakeAI
{ // 主决策类 - 蛇AI大脑
private:
  Map *map;
  PathFinder *path_finder;
  vector<Point> current_path;

  Target select_target(const GameState &state)
  {
    const auto &head = state.get_self().get_head();
    const auto &self = state.get_self();

    // 即时反应 - 处理近距离高价值目标
    for (const auto &item : state.items)
    {
      if ((item.value > 0 || item.value == GROWTH_BEAN_VALUE) && map->is_safe(item.pos))
      {
        int dist = Utils::manhattan_distance(head, item.pos);

        // 检查食物是否能及时到达
        int estimated_time_to_reach = static_cast<int>(dist * 1.5f / MOVE_SPEED);
        if (estimated_time_to_reach >= item.lifetime)
          continue;

        if (dist <= IMMEDIATE_RESPONSE_DISTANCE)
          return {item.pos, item.value > 0 ? item.value : 3, dist, 1000.0};
      }
    }

    // 特殊场景处理 - 宝箱和钥匙
    if (self.has_key && !state.chests.empty())
    {
      // 持有钥匙且有宝箱
      for (const auto &chest : state.chests)
      {
        if (!map->is_safe(chest.pos))
          continue;

        int dist = Utils::manhattan_distance(head, chest.pos);

        // 查找钥匙剩余时间
        int key_remaining_time = 30; // 默认值
        for (const auto &key : state.keys)
          if (key.holder_id == MYID)
          {
            key_remaining_time = key.remaining_time;
            break;
          }

        // 如果钥匙即将过期且能及时到达
        if (key_remaining_time < dist / 2 + 5)
        {
          if (dist < key_remaining_time * 2 - 5) // 预计能赶到
            return {chest.pos, chest.score, dist, 2000.0};
        }
        else if (map->value(chest.pos) > 150) // 高价值宝箱
          return {chest.pos, chest.score, dist, 800.0};
      }
    }
    else if (!self.has_key)
    {
      // 没有钥匙，检查钥匙优先级
      for (const auto &key : state.keys)
      {
        if (key.holder_id == -1 && map->is_safe(key.pos))
        {
          int dist = Utils::manhattan_distance(head, key.pos);
          if (map->value(key.pos) > 100)
            return {key.pos, 10, dist, 500.0};
        }
      }
    }

    // 综合评估 - 使用多维度评分
    vector<Target> targets;

    // 限制搜索范围提高效率
    int search_radius = state.remaining_ticks < 100 ? 12 : 18;
    int min_y = max(0, head.y - search_radius);
    int max_y = min(MAXM - 1, head.y + search_radius);
    int min_x = max(0, head.x - search_radius);
    int max_x = min(MAXN - 1, head.x + search_radius);

    for (int y = min_y; y <= max_y; y++)
    {
      for (int x = min_x; x <= max_x; x++)
      {
        Point p = {y, x};

        // 使用综合评分
        int combined_score = map->combined_score(p);
        if (combined_score <= 0)
          continue;

        int distance = Utils::manhattan_distance(head, p);

        // 检查食物可达性
        for (const auto &item : state.items)
        {
          if (item.pos.y == y && item.pos.x == x)
          {
            int estimated_time = static_cast<int>(distance * 1.5f / MOVE_SPEED);
            if (estimated_time >= item.lifetime)
            {
              combined_score = -1;
              break;
            }
          }
        }

        if (combined_score <= 0)
          continue;

        // 计算优先级 - 综合评分与距离平衡
        float priority = (float)combined_score / (distance + 1);

        // 对整格点位置加成（便于转向）
        if (Utils::is_grid_point(p))
          priority *= 1.1f;

        targets.push_back({p, combined_score, distance, priority});
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
    Point center;
    int current_tick = MAX_TICKS - state.remaining_ticks;

    // 如果安全区即将收缩，使用下一个安全区中心
    if (state.next_shrink_tick > 0 && state.next_shrink_tick - current_tick <= 15)
      center = {
          (state.next_safe_zone.y_min + state.next_safe_zone.y_max) / 2,
          (state.next_safe_zone.x_min + state.next_safe_zone.x_max) / 2};
    else
      center = {
          (state.current_safe_zone.y_min + state.current_safe_zone.y_max) / 2,
          (state.current_safe_zone.x_min + state.current_safe_zone.x_max) / 2};

    return {center, 0, Utils::manhattan_distance(head, center), 10.0};
  }

  float evaluate_direction(int direction, const GameState &state, const Target &target)
  {
    const auto &head = state.get_self().get_head();
    Point next_pos = Utils::get_next_position(head, direction);

    // 安全检查
    if (!is_direction_safe(direction, state))
      return -1000.0f;

    // 使用综合评分
    float pos_score = map->combined_score(next_pos);

    // 距离评分 - 正值表示更接近目标
    int curr_dist = Utils::manhattan_distance(head, target.pos);
    int next_dist = Utils::manhattan_distance(next_pos, target.pos);
    float distance_score = (curr_dist - next_dist) * DISTANCE_WEIGHT;

    // 转向奖励
    float turn_score = 0;
    if (Utils::is_grid_point(head))
      turn_score = TURN_BONUS;
    else if (Utils::is_grid_point(next_pos))
      turn_score = TURN_BONUS / 2; // 即将到达整格点的奖励

    // 安全退路检查
    float safety_score = 0;
    int safe_exit_count = 0;

    // 计算安全出口数量
    for (int dir = 0; dir < 4; dir++)
    {
      Point exit_pos = Utils::get_next_position(next_pos, dir);
      if (map->is_safe(exit_pos))
        safe_exit_count++;
    }

    // 评估出口安全性
    if (safe_exit_count == 0)
      safety_score = -50.0f; // 没有安全出口
    else if (safe_exit_count == 1)
      safety_score = -20.0f; // 只有一个出口
    else
      safety_score = safe_exit_count * 5.0f; // 多个出口更安全

    return pos_score + distance_score + turn_score + safety_score;
  }

  bool should_use_shield(const GameState &state)
  {
    const auto &self = state.get_self();

    // 基本条件检查
    if (self.shield_cd > 0 || self.score < SHIELD_ACTIVATION_COST || self.shield_time > 0)
      return false;

    // 检查头对头碰撞风险
    for (const auto &snake : state.snakes)
    {
      if (snake.id == MYID || !snake.body.size())
        continue;

      const auto &other_head = snake.get_head();
      const auto &my_head = self.get_head();
      int distance = Utils::manhattan_distance(my_head, other_head);

      if (distance <= COLLISION_DETECT_DISTANCE)
      {
        // 方向相对检查
        int my_dir = self.direction;
        int other_dir = snake.direction;

        // 直接相对碰撞风险
        bool head_on_collision =
            (my_dir == 0 && other_dir == 2) ||
            (my_dir == 2 && other_dir == 0) ||
            (my_dir == 1 && other_dir == 3) ||
            (my_dir == 3 && other_dir == 1);

        if (head_on_collision && distance <= 6)
          return true; // 即将发生头对头碰撞

        // 交叉路口相遇风险
        if (distance <= 4)
        {
          Point my_next = Utils::get_next_position(my_head, my_dir);
          Point other_next = Utils::get_next_position(other_head, other_dir);

          if (my_next.x == other_next.x && my_next.y == other_next.y)
            return true; // 下一步会在同一位置
        }
      }
    }

    // 安全区收缩风险
    int current_tick = MAX_TICKS - state.remaining_ticks;
    if (state.next_shrink_tick > 0)
    {
      int ticks_until_shrink = state.next_shrink_tick - current_tick;

      // 即将收缩且处于危险区域
      if (ticks_until_shrink <= 3)
      {
        const auto &head = self.get_head();
        if (head.x < state.next_safe_zone.x_min || head.x > state.next_safe_zone.x_max ||
            head.y < state.next_safe_zone.y_min || head.y > state.next_safe_zone.y_max)
        {
          // 计算到最近安全区边界的距离
          int dist_to_safe = 0;
          if (head.x < state.next_safe_zone.x_min)
            dist_to_safe = state.next_safe_zone.x_min - head.x;
          else if (head.x > state.next_safe_zone.x_max)
            dist_to_safe = head.x - state.next_safe_zone.x_max;

          if (head.y < state.next_safe_zone.y_min)
            dist_to_safe = max(dist_to_safe, state.next_safe_zone.y_min - head.y);
          else if (head.y > state.next_safe_zone.y_max)
            dist_to_safe = max(dist_to_safe, head.y - state.next_safe_zone.y_max);

          // 无法及时到达安全区
          if (dist_to_safe > ticks_until_shrink * MOVE_SPEED - 1)
            return true;
        }
      }
    }

    // 宝箱竞争情况
    if (self.has_key && !state.chests.empty())
    {
      for (const auto &chest : state.chests)
      {
        int self_dist = Utils::manhattan_distance(self.get_head(), chest.pos);

        // 检查其他持有钥匙的蛇
        for (const auto &snake : state.snakes)
        {
          if (snake.id != MYID && snake.has_key)
          {
            int other_dist = Utils::manhattan_distance(snake.get_head(), chest.pos);

            // 对方更近且都接近宝箱
            if (other_dist < self_dist && self_dist <= 6 && other_dist <= 4)
            {
              // 钥匙即将过期检查
              for (const auto &key : state.keys)
                if (key.holder_id == MYID && key.remaining_time <= 10)
                  return true; // 激活护盾抢先开箱
            }
          }
        }
      }
    }

    return false;
  }

  bool is_direction_safe(int direction, const GameState &state)
  {
    const auto &self = state.get_self();
    Point next = Utils::get_next_position(self.get_head(), direction);

    // 安全区边界检查
    if (next.x < state.current_safe_zone.x_min || next.x > state.current_safe_zone.x_max ||
        next.y < state.current_safe_zone.y_min || next.y > state.current_safe_zone.y_max)
      return false;

    // 护盾状态下的安全检查
    if (self.shield_time > 0)
    {
      // 即使有护盾，无钥匙碰宝箱也会死
      for (const auto &chest : state.chests)
        if (!self.has_key && next.x == chest.pos.x && next.y == chest.pos.y)
          return false;
      return true; // 其他碰撞情况护盾有效
    }

    // 检查与其他蛇的碰撞
    for (const auto &snake : state.snakes)
    {
      if (snake.id == MYID)
        continue;
      for (const auto &part : snake.body)
        if (next.x == part.x && next.y == part.y)
          return false;
    }

    // 检查陷阱
    for (const auto &item : state.items)
      if (item.value == TRAP_VALUE && next.x == item.pos.x && next.y == item.pos.y)
        return false;

    return true;
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
  {
    // 分析地图
    map->analyze(state);
    const auto &head = state.get_self().get_head();

    // 检测头部附近的直接可得食物
    for (const auto &item : state.items) {
      if ((item.value > 0 || item.value == GROWTH_BEAN_VALUE) && map->is_safe(item.pos)) {
        int dist = Utils::manhattan_distance(head, item.pos);
        // 使用更小的即时反应距离阈值
        if (dist <= 2) {
          int dir = Utils::get_direction(head, item.pos);
          if (dir != -1 && is_direction_safe(dir, state)) {
            return dir; // 直接吃附近的食物
          }
        }
      }
    }

    // 选择目标并规划路径
    Target target = select_target(state);
    current_path = path_finder->find_path(head, target.pos, state);

    // 检查路径上是否有食物
    if (!current_path.empty()) {
      Point next_pos = current_path[0];
      int next_dir = Utils::get_direction(head, next_pos);
      
      // 检查下一步位置是否有食物
      for (const auto &item : state.items) {
        if ((item.value > 0 || item.value == GROWTH_BEAN_VALUE) && 
            item.pos.x == next_pos.x && item.pos.y == next_pos.y) {
          // 路径上有食物，确认安全后直接走
          if (next_dir != -1 && is_direction_safe(next_dir, state))
            return next_dir;
        }
      }
      
      // 常规路径行动
      if (next_dir != -1 && is_direction_safe(next_dir, state))
        return next_dir;
    }

    // 评估所有方向
    vector<pair<int, float>> direction_scores;
    for (int dir = 0; dir < 4; dir++)
      direction_scores.push_back({dir, evaluate_direction(dir, state, target)});

    // 按得分从高到低排序
    sort(direction_scores.begin(), direction_scores.end(),
         [](const auto &a, const auto &b)
         { return a.second > b.second; });

    // 检查护盾使用
    if (should_use_shield(state))
      return SHIELD_COMMAND;

    // 选择最佳方向
    for (const auto &[dir, score] : direction_scores)
      if (score > 0)
        return dir;

    // 后备方案 - 随机方向
    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
    return uniform_int_distribution<int>(0, 3)(rng);
  }
};

void read_game_state(GameState &s)
{
  cin >> s.remaining_ticks;

  // 读取物品
  int item_count;
  cin >> item_count;
  s.items.resize(item_count);
  for (int i = 0; i < item_count; ++i)
    cin >> s.items[i].pos.y >> s.items[i].pos.x >> s.items[i].value >> s.items[i].lifetime;

  // 读取蛇
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
      cin >> sn.body[j].y >> sn.body[j].x;
    if (sn.id == MYID)
      s.self_idx = i;
    id2idx[sn.id] = i;
  }

  // 读取宝箱
  int chest_count;
  cin >> chest_count;
  s.chests.resize(chest_count);
  for (int i = 0; i < chest_count; ++i)
    cin >> s.chests[i].pos.y >> s.chests[i].pos.x >> s.chests[i].score;

  // 读取钥匙
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
        s.snakes[it->second].has_key = true;
    }
  }

  // 读取安全区信息
  cin >> s.current_safe_zone.x_min >> s.current_safe_zone.y_min >> s.current_safe_zone.x_max >> s.current_safe_zone.y_max;
  cin >> s.next_shrink_tick >> s.next_safe_zone.x_min >> s.next_safe_zone.y_min >> s.next_safe_zone.x_max >> s.next_safe_zone.y_max;
  cin >> s.final_shrink_tick >> s.final_safe_zone.x_min >> s.final_safe_zone.y_min >> s.final_safe_zone.x_max >> s.final_safe_zone.y_max;
}

int main()
{
  GameState current_state;
  read_game_state(current_state);

  static SnakeAI ai;
  int decision = ai.make_decision(current_state);

  cout << decision << endl;
  return 0;
}
