#include <chrono>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <cstring>
#include <queue>
#include <algorithm>
#include <unordered_set>

using namespace std;

// 常量
constexpr int MAXN = 40;
constexpr int MAXM = 30;
constexpr int MAX_TICKS = 256;
constexpr int MYID = 2023202295; // 此处替换为你的学号！

constexpr int EmptyIdx = -1;     // 空位置标识
constexpr int SHIELD_COMMAND = 4; // 护盾指令值

enum class Direction
{
    LEFT,
    UP,
    RIGHT,
    DOWN
};

const vector<Direction> validDirections{Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT};

namespace Utils
{
    // 检查是否越界
    bool boundCheck(int y, int x)
    {
        return (y >= 0) && (y < MAXM) && (x >= 0) && (x < MAXN);
    }

    // 获取下一个位置，在 dir 方向上
    pair<int, int> nextPos(pair<int, int> pos, Direction dir)
    {
        switch (dir)
        {
        case Direction::LEFT:
            return make_pair(pos.first, pos.second - 1);
        case Direction::RIGHT:
            return make_pair(pos.first, pos.second + 1);
        case Direction::UP:
            return make_pair(pos.first - 1, pos.second);
        default:
            return make_pair(pos.first + 1, pos.second);
        }
    }

    // 将方向转换为数字
    int dir2num(const Direction dir)
    {
        switch (dir)
        {
        case Direction::LEFT:
            return 0;
        case Direction::UP:
            return 1;
        case Direction::RIGHT:
            return 2;
        default:
            return 3;
        }
    }

    // 判断食物是否可以及时到达 - 重载版本1：直接使用坐标和生命周期
    bool canReachFoodInTime(int head_y, int head_x, int food_y, int food_x, int lifetime) 
    {
        // 计算曼哈顿距离
        int dist = abs(head_y - food_y) + abs(head_x - food_x);
        
        // 考虑转弯限制，估计实际所需时间
        // 由于转弯限制，实际路径通常会比曼哈顿距离长
        int estimated_ticks = dist + dist/5; // 粗略估计：每5步可能需要额外1步用于转弯
        
        // 简单判断：如果预计到达时间超过食物剩余生命周期，则忽略该食物
        return estimated_ticks < lifetime;
    }

    // 随机地从一个 vector 中抽取一个元素
    template <typename T>
    T randomChoice(const vector<T> &vec)
    {
        static mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
        uniform_int_distribution<> distrib(0, vec.size() - 1);
        const int randomIndex = distrib(rng);
        return vec[randomIndex];
    }

    // 将坐标转化为string (用于拐角检测)
    string idx2str(const pair<int, int> idx)
    {
        string a = to_string(idx.first);
        if (a.length() == 1)
            a = "0" + a;
        string b = to_string(idx.second);
        if (b.length() == 1)
            b = "0" + b;
        return a + b;
    }

    // 将string转化为坐标
    pair<int, int> str2idx(const string &str)
    {
        string substr1{str[0], str[1]};
        string substr2{str[2], str[3]};
        return make_pair(stoi(substr1), stoi(substr2));
    }
}

struct Point {
  int y, x;
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

// ===== 移植的主要辅助数据结构 =====
// 游戏地图状态: mp用于物品, mp2用于蛇的位置
int mp[MAXM][MAXN], mp2[MAXM][MAXN];

// 简化的目标锁定机制（暂未启用）
static Point current_target = {-1, -1};
static int target_value = 0;
static int target_lock_time = 0;

// 目标锁定函数（保留接口，但暂不实际启用）
void lock_on_target(const GameState &state) {
    // 删除未使用的变量
    // const auto &head = state.get_self().get_head();
    
    // 更新锁定状态
    if (target_lock_time > 0) {
        target_lock_time--;
    }
    
    // 检查当前目标是否仍然存在
    bool target_exists = false;
    if (current_target.y != -1 && current_target.x != -1) {
        for (const auto &item : state.items) {
            // 使用近似匹配检测目标是否仍然存在
            if (abs(item.pos.y - current_target.y) <= 1 && 
                abs(item.pos.x - current_target.x) <= 1 &&
                item.value >= target_value * 0.8) { // 允许价值有小幅下降
                
                current_target = item.pos; // 更新为精确位置
                target_exists = true;
                break;
            }
        }
    }
    
    // 如果目标不再存在或锁定时间结束，重置锁定状态
    if (!target_exists && target_lock_time <= 0) {
        current_target = {-1, -1};
        target_value = 0;
    }
    
    // 如果没有锁定目标，尝试锁定新目标
    // 注意：暂不启用目标锁定功能，因此此处不执行实际锁定
    /*
    if (current_target.y == -1 || current_target.x == -1) {
        for (const auto &item : state.items) {
            if (item.value >= 10) { // 极高价值尸体
                int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
                if (dist <= 5) { // 更严格的距离要求
                    current_target = item.pos;
                    target_value = item.value;
                    target_lock_time = min(dist + 2, 7); // 更短的锁定时间
                    break;
                }
            }
        }
    }
    */
}

void read_game_state(GameState &s) {
  cin >> s.remaining_ticks;

  // 初始化地图状态
  memset(mp, 0, sizeof(mp));
  memset(mp2, 0, sizeof(mp2));

  // 读取物品信息
  int item_count;
  cin >> item_count;
  s.items.resize(item_count);
  for (int i = 0; i < item_count; ++i) {
    cin >> s.items[i].pos.y >> s.items[i].pos.x >>
        s.items[i].value >> s.items[i].lifetime;
    
    // 更新mp地图
    if (Utils::boundCheck(s.items[i].pos.y, s.items[i].pos.x)) {
      mp[s.items[i].pos.y][s.items[i].pos.x] = s.items[i].value;
    }
  }

  // 读取蛇信息
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
      
      // 更新mp2地图 - 蛇的身体部分
      if (Utils::boundCheck(sn.body[j].y, sn.body[j].x)) {
        mp2[sn.body[j].y][sn.body[j].x] = sn.id == MYID ? 10 : -5;
      }
    }
    
    // 标记蛇头周围的危险区域
    if (sn.id != MYID) {
      auto &head = sn.body[0];
      for (auto dir : validDirections) {
        const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
        if (Utils::boundCheck(y, x)) {
          if (mp2[y][x] != -5) {
            mp2[y][x] = -6; // 蛇头可能移动区域
          }
        }
      }
      
      // 蛇尾部可能移动区域
      auto &tail = sn.body[sn.length - 1];
      for (auto dir : validDirections) {
        const auto [y, x] = Utils::nextPos({tail.y, tail.x}, dir);
        if (Utils::boundCheck(y, x)) {
          if (mp2[y][x] != -5 && mp2[y][x] != -6) {
            mp2[y][x] = -7; // 尾部可能移动区域
          }
        }
      }
    }
    
    if (sn.id == MYID)
      s.self_idx = i;
    id2idx[sn.id] = i;
  }

  // 读取宝箱信息
  int chest_count;
  cin >> chest_count;
  s.chests.resize(chest_count);
  for (int i = 0; i < chest_count; ++i) {
    cin >> s.chests[i].pos.y >> s.chests[i].pos.x >>
        s.chests[i].score;
  }

  // 读取钥匙信息
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

  // 读取安全区信息
  cin >> s.current_safe_zone.x_min >> s.current_safe_zone.y_min >>
      s.current_safe_zone.x_max >> s.current_safe_zone.y_max;
  cin >> s.next_shrink_tick >> s.next_safe_zone.x_min >>
      s.next_safe_zone.y_min >> s.next_safe_zone.x_max >>
      s.next_safe_zone.y_max;
  cin >> s.final_shrink_tick >> s.final_safe_zone.x_min >>
      s.final_safe_zone.y_min >> s.final_safe_zone.x_max >>
      s.final_safe_zone.y_max;
}

unordered_set<Direction> illegalMove(const GameState &state)
{
    unordered_set<Direction> illegals;
    const Snake &self = state.get_self();
    
    // 确定反方向（不能往回走）
    auto addReverse = [&]()
    {
        switch (self.direction)
        {
        case 3: // DOWN
            illegals.insert(Direction::UP);
            break;
        case 1: // UP
            illegals.insert(Direction::DOWN);
            break;
        case 0: // LEFT
            illegals.insert(Direction::RIGHT);
            break;
        default: // RIGHT
            illegals.insert(Direction::LEFT);
            break;
        }
    };

    // 自己方向的反方向不能走
    addReverse();

    // 碰到其他蛇的身子不能走，但是开了护盾除外
    // 墙绝对不能走
    for (auto dir : validDirections)
    {
        const Point &head = self.get_head();
        const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
        
        if (!Utils::boundCheck(y, x))
        {
            illegals.insert(dir);
        }
        else
        {
            // 添加安全区检查
            if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max || 
                y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max)
            {
                // 只有当护盾时间足够时才允许离开安全区
                if (self.shield_time <= 1) {
                    illegals.insert(dir);
                }
            }
            else if (mp[y][x] == -4) // 墙
            {
                illegals.insert(dir);
            }
            else if (mp2[y][x] == -5 || mp2[y][x] == -6 || mp2[y][x] == -7)
            {
                // 如果护盾时间快要结束，并且开不了护盾了
                // 一般护盾都是不值得的
                if (self.shield_time < 2)
                {
                    illegals.insert(dir);
                }
            }
        }
    }

    // 如果四种方向都不行，试图放宽限制，可以开护盾
    if (illegals.size() == 4)
    {
        illegals.clear();

        // 自己方向的反方向不能走
        addReverse();

        // 放宽条件：墙绝对不能走，但在有护盾或可以开护盾的情况下可以穿过蛇
        for (auto dir : validDirections)
        {
            const Point &head = self.get_head();
            const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
            
            if (!Utils::boundCheck(y, x))
            {
                illegals.insert(dir);
            }
            else
            {
                // 安全区检查 - 即使在紧急情况下也不允许离开安全区
                if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max || 
                    y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max)
                {
                    // 只有当护盾时间足够时才允许离开安全区
                    if (self.shield_time <= 1) {
                        illegals.insert(dir);
                    }
                }
                else if (mp[y][x] == -4) // 墙
                {
                    illegals.insert(dir);
                }
                else if (mp2[y][x] == -5) // 蛇身
                {
                    // 如果护盾时间够或者能开护盾，则可以穿过
                    if (!(self.shield_time >= 2 || (self.shield_cd == 0 && self.score > 20) ||
                          state.remaining_ticks >= 255 - 9 + 2))
                    {
                        illegals.insert(dir);
                    }
                }
            }
        }
    }

    return illegals;
}

namespace Strategy
{
    // 计算其他蛇离某个目标的距离，如果太近就放弃这个地方
    pair<int, int> count(const GameState &state, int y, int x)
    {
        const auto &self = state.get_self();
        const int target = abs(self.get_head().y - y) + abs(self.get_head().x - x);
        int tot = 0;
        int minimized = target;
        
        for (const auto &snake : state.snakes)
        {
            if (snake.id == MYID)
                continue;
                
            const int current = abs(snake.get_head().y - y) + abs(snake.get_head().x - x);
            if (current < target)
            {
                minimized = min(minimized, current);
                tot += 1;
            }
        }
        return make_pair(tot, minimized);
    }

    // 食物密度评估函数 - 计算指定区域内食物密集程度
    double calculateFoodDensity(const GameState &state, int center_y, int center_x, int radius = 5) 
    {
        double total_value = 0;
        int count = 0;
      
        // 计算指定区域内所有食物的总价值
        for (const auto &item : state.items) {
            // 检查食物是否在指定半径内
            if (abs(item.pos.y - center_y) <= radius && abs(item.pos.x - center_x) <= radius) {
                // 增加食物价值计数
                if (item.value > 0) { // 普通食物
                    total_value += item.value;
                    count++;
                } else if (item.value == -1) { // 增长豆
                    // 根据游戏阶段确定增长豆价值
                    int growth_value = (state.remaining_ticks > 176) ? 3 : 2;
                    total_value += growth_value;
                    count++;
                }
            }
        }
      
        // 返回密度分数，避免除以零
        return (count > 0) ? (total_value / count) * count : 0;
    }

    // 安全区边界评分函数（不再有中心倾向）
    double safeZoneCenterScore(const GameState &state, int y, int x)
    {
        // 不再计算到中心的距离和惩罚
        // 而是检查位置是否在安全区内，并根据到边界的距离给予适当评分
        
        // 确定要考虑的安全区
        SafeZoneBounds zone = state.current_safe_zone;
        
        // 当安全区即将收缩时，使用下一个安全区
        int ticks_to_shrink = state.next_shrink_tick - (MAX_TICKS - state.remaining_ticks);
        if (ticks_to_shrink >= 0 && ticks_to_shrink <= 25) {
            zone = state.next_safe_zone;
        }
        
        // 计算到安全区边界的最小距离
        int dist_to_border = min({
            x - zone.x_min,
            zone.x_max - x,
            y - zone.y_min,
            zone.y_max - y
        });
        
        // 如果距离边界太近(<=3格)，给予轻微惩罚，否则返回0（不偏好任何区域）
        return (dist_to_border <= 3) ? -5.0 * (3 - dist_to_border) : 0;
    }

    // BFS搜索评估函数
    double bfs(int sy, int sx, int fy, int fx, const GameState &state)
    {
        double score = 0;
        
        // 安全区收缩风险评估
        int current_tick = MAX_TICKS - state.remaining_ticks;
        int ticks_to_shrink = state.next_shrink_tick - current_tick;
        
        // 如果即将收缩（小于20个tick）
        if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20)
        {
            // 检查位置是否在下一个安全区内
            if (sx < state.next_safe_zone.x_min || sx > state.next_safe_zone.x_max ||
                sy < state.next_safe_zone.y_min || sy > state.next_safe_zone.y_max)
            {
                // 安全区外位置价值大幅降低，危险度随收缩时间临近增加
                if (ticks_to_shrink <= 5) {
                    // 极度危险，几乎立即放弃
                    score -= 5000;
                } else if (ticks_to_shrink <= 10) {
                    // 很危险，强烈避免
                    score -= 3000; 
                } else {
                    // 有风险，尽量避免
                    score -= 1000;
                }
            }
        }
        
        // 特殊检查：如果在关口位置，小心进入
        bool flag = false;
        if (sy == 9 && sx == 20)
        {
            const int t = mp2[18 - fy][20];
            flag = true;
            if (t == -5 || t == -6 || t == -7)
            {
                return -1000;
            }
        }
        if (sy == 21 && sx == 20)
        {
            const int t = mp2[42 - fy][20];
            flag = true;
            if (t == -5 || t == -6 || t == -7)
            {
                return -1000;
            }
        }
        if (sy == 15 && sx == 14)
        {
            const int t = mp2[15][28 - fx];
            flag = true;
            if (t == -5 || t == -6 || t == -7)
            {
                return -1000;
            }
        }
        if (sy == 15 && sx == 26)
        {
            const int t = mp2[15][52 - fx];
            flag = true;
            if (t == -5 || t == -6 || t == -7)
            {
                return -1000;
            }
        }
        
        // 处理陷阱的情况
        if (mp[sy][sx] == -2 && !flag)
        {
            score = -1000;
        }
        
        // 游戏后期向中心靠拢的策略
        const double start = 150, end = 25, maxNum2 = 30;
        const int timeRest = state.remaining_ticks;
        
        if (mp[sy][sx] == -2 && flag && mp2[sy][sx] != -5 && mp2[sy][sx] != -6 && mp2[sy][sx] != -7)
        {
            score = -26;
            if (start > timeRest && timeRest >= end)
            {
                const double r = maxNum2 * (timeRest - start) * (timeRest - start);
                score += r / ((start - end) * (start - end));
            }
            return score;
        }
        else
        {
            // 在第二次收缩前考虑食物密度
            if (state.remaining_ticks > 76) { // 1-160刻
                // 计算当前位置区域的食物密度
                double density_score = calculateFoodDensity(state, sy, sx, 5);
                
                // 根据游戏进程调整密度权重
                double density_weight = 0.0;
                if (state.remaining_ticks > 176) { // 早期阶段
                    density_weight = 0.8; // 早期更看重食物密集区
                } else { // 中期第一部分
                    density_weight = 0.5; // 中期适当关注
                }
                
                // 将密度分数加入总评分
                score += density_score * density_weight;
            }
        }

        // BFS搜索价值区域
        unordered_set<string> visited;
        queue<pair<string, int>> q;
        string initial = Utils::idx2str(make_pair(sy, sx));
        q.emplace(initial, 1);
        visited.insert(initial);
        
        while (!q.empty())
        {
            auto [currentState, layer] = q.front();
            q.pop();
            
            // 设定视野搜索范围
            double maxLayer = 12;
            if (timeRest < 50)
            {
                maxLayer = 10; // 后期减小搜索范围
            }
            
            if (layer >= maxLayer)
            {
                break;
            }
            
            // 解析当前位置
            const auto [y, x] = Utils::str2idx(currentState);
            double num = mp[y][x] * 1.0;
            
            // 找到对应的物品以检查生命周期
            bool can_reach = true;
            for (const auto &item : state.items) {
                if (item.pos.y == y && item.pos.x == x) {
                    // 1. 检查食物是否会因安全区收缩而消失
                    int food_current_tick = MAX_TICKS - state.remaining_ticks;
                    int food_ticks_to_shrink = state.next_shrink_tick - food_current_tick;
                    
                    // 如果即将收缩(<=20个tick)且食物在下一个安全区外，预计会消失
                    if (food_ticks_to_shrink >= 0 && food_ticks_to_shrink <= 20) {
                        if (x < state.next_safe_zone.x_min || x > state.next_safe_zone.x_max ||
                            y < state.next_safe_zone.y_min || y > state.next_safe_zone.y_max) {
                            // 食物将消失，将其价值设为0
                            can_reach = false;
                            break;
                        }
                    }
                    
                    // 2. 然后再检查是否能及时到达
                                         const auto &snake_head = state.get_self().get_head();
                    if (!Utils::canReachFoodInTime(snake_head.y, snake_head.x, item.pos.y, item.pos.x, item.lifetime)) {
                        // 如果不能及时到达，将该食物价值设为0
                        can_reach = false;
                        break;
                    }
                }
            }
            
            // 如果食物无法及时到达，则价值为0
            if (mp[y][x] != 0 && mp[y][x] != -2 && !can_reach) {
                num = 0;
            }
            // 评估物品价值
            else if (mp[y][x] > 0) // 普通食物
            {
                // 动态评估尸体价值 - 基于距离
                const auto &snake_head = state.get_self().get_head();
                int head_dist = abs(snake_head.y - y) + abs(snake_head.x - x);
                
                if (mp[y][x] >= 10) {
                    // 极高价值的尸体
                    if (head_dist <= 6) {
                        // 非常近的高价值尸体，价值翻倍
                        num = mp[y][x] * 200 + 200;
                    } else if (head_dist <= 10) {
                        // 较近的高价值尸体
                        num = mp[y][x] * 120 + 120;
                    } else {
                        // 远距离高价值尸体，标准价值
                        num = mp[y][x] * 80 + 80;
                    }
                } else if (mp[y][x] >= 5) {
                    // 中等价值的尸体
                    if (head_dist <= 4) {
                        // 非常近的中等尸体，价值提高
                        num = mp[y][x] * 100 + 80;
                    } else {
                        // 标准价值
                        num = mp[y][x] * 70 + 40;
                    }
                } else if (mp[y][x] > 0) {
                    // 普通食物也增加距离差异化处理
                    if (head_dist <= 3) {
                        // 非常近的普通食物，提高价值
                        num = mp[y][x] * 45; // 提高50%
                    } else if (head_dist <= 5) {
                        // 较近的普通食物，略微提高价值
                        num = mp[y][x] * 35; // 提高约17%
                    } else {
                        // 远距离普通食物，标准价值
                        num = mp[y][x] * 30;
                    }
                }
            }
            else if (mp[y][x] == -1) // 增长豆
            {
                // 根据游戏阶段动态调整增长豆价值
                if (state.remaining_ticks > 180)
                    num = 30; // 早期价值高
                else if (state.remaining_ticks > 100)
                    num = 20; // 中期价值中等
                else
                    num = 10; // 后期价值低
            }
            
            // 根据游戏阶段动态调整普通食物价值
            if (mp[y][x] > 0 && mp[y][x] < 5) { // 只处理普通食物
                // 早期更看重普通食物(避免过分追求增长豆)
                if (state.remaining_ticks > 180) {
                    num *= 1.2; // 提高20%
                }
                // 后期提高所有食物价值(加速得分)
                else if (state.remaining_ticks < 60) {
                    num *= 1.3; // 提高30%
                }
            }
            
            else if (mp[y][x] == -2) // 陷阱
            {
                num = -0.5;
            }
            
            // 基于安全区阶段的食物价值动态调整
            double safeZoneFactor = 1.0;
            
            // 安全区因子计算
            int zone_current_tick = MAX_TICKS - state.remaining_ticks;
            int zone_ticks_to_shrink = state.next_shrink_tick - zone_current_tick;
            
            // 分析当前所处安全区阶段
            if (zone_current_tick < 80) { 
                // 早期阶段 - 标准评估
                safeZoneFactor = 1.0;
            } else if (zone_current_tick < 160) {
                // 第一次收缩阶段
                if (zone_ticks_to_shrink >= 0 && zone_ticks_to_shrink <= 25) {
                    // 即将收缩时，提高下个安全区内食物价值
                    if (x >= state.next_safe_zone.x_min && x <= state.next_safe_zone.x_max &&
                        y >= state.next_safe_zone.y_min && y <= state.next_safe_zone.y_max) {
                        safeZoneFactor = 1.3; // 提升30%价值
                    }
                }
            } else if (zone_current_tick < 220) {
                // 第二次收缩阶段
                if (zone_ticks_to_shrink >= 0 && zone_ticks_to_shrink <= 25) {
                    // 更强烈提升下个安全区内食物价值
                    if (x >= state.next_safe_zone.x_min && x <= state.next_safe_zone.x_max &&
                        y >= state.next_safe_zone.y_min && y <= state.next_safe_zone.y_max) {
                        safeZoneFactor = 1.5; // 提升50%价值
                    }
                }
            } else {
                // 最终收缩阶段
                if (zone_ticks_to_shrink >= 0 && zone_ticks_to_shrink <= 25) {
                    // 最终安全区内食物价值极高
                    if (x >= state.next_safe_zone.x_min && x <= state.next_safe_zone.x_max &&
                        y >= state.next_safe_zone.y_min && y <= state.next_safe_zone.y_max) {
                        safeZoneFactor = 2.0; // 提升100%价值
                    }
                }
            }
            
            // 应用安全区因子
            num *= safeZoneFactor;
            
            // 计算位置权重
            double weight;
            //! 目前只存储竞争系数
            auto [tot, _] = count(state, y, x);  // 使用_表示未使用的变量
            
            // 根据层级分配权重
            switch (layer)
            {
            case 1:
                weight = 100;
                break;
            case 2:
                weight = 50;
                break;
            case 3:
                weight = 30;
                break;
            case 4:
                weight = 10;
                break;
            case 5:
                weight = 5;
                break;
            case 6:
                weight = 4.5;
                break;
            case 7:
                weight = 4;
                break;
            case 8:
                weight = 3.5;
                break;
            case 9:
                weight = 3;
                break;
            default:
                weight = 3 - 0.5 * min(6, layer - 9);
            }
            
            // 考虑竞争因素调整权重
            if (mp[y][x] != -2) // 不是陷阱
            {
                // 强化竞争因素评估
                float competition_factor = 1.0f;
                const auto &self_head = state.get_self().get_head();
                int self_distance = abs(self_head.y - y) + abs(self_head.x - x);
                
                // 检查其他蛇与目标的距离关系
                for (const auto &snake : state.snakes) {
                    if (snake.id != MYID && snake.id != -1) {
                        int dist = abs(snake.get_head().y - y) + abs(snake.get_head().x - x);
                        
                        // 如果敌方蛇更近，竞争系数降低
                        if (dist < self_distance) {
                            // 对高价值尸体的竞争调整
                            if (mp[y][x] >= 10) {
                                // 极高价值尸体
                                if (self_distance <= 6) {
                                    // 近距离内的极高价值尸体，更强烈争夺
                                    competition_factor *= 0.95f; // 仅轻微降低价值
                                } else {
                                    competition_factor *= 0.85f; // 适度降低价值
                                }
                            } else if (mp[y][x] >= 5) {
                                // 高价值尸体
                                if (self_distance <= 4) {
                                    // 非常近的高价值尸体，值得争夺
                                    competition_factor *= 0.90f; // 轻微降低价值
                                } else {
                                    competition_factor *= 0.80f; // 适度降低价值
                                }
                            } else {
                                competition_factor *= 0.8f; // 从0.7提高到0.8，减轻普通食物竞争惩罚
                            }
                        }
                        // 如果敌方蛇距离相近，轻微降低价值
                        else if (dist <= self_distance + 2) {
                            if (mp[y][x] >= 8) {
                                // 对高分尸体，竞争性调整
                                if (self_distance <= 6) {
                                    competition_factor *= 0.98f; // 几乎不降低价值
                                } else {
                                    competition_factor *= 0.95f; // 轻微降低价值
                                }
                            } else {
                                competition_factor *= 0.9f; // 普通食物降低价值
                            }
                        }
                    }
                }
                
                // 应用竞争系数
                weight *= competition_factor;
            }
            
            // 计算总得分
            score += num * weight;
            
            // 扩展搜索
            for (auto dir : validDirections)
            {
                const auto [next_y, next_x] = Utils::nextPos(make_pair(y, x), dir);
                if (!Utils::boundCheck(next_y, next_x))
                {
                    // 越界
                    continue;
                }
                
                // 检查障碍物
                if (mp[next_y][next_x] == -4 || mp2[next_y][next_x] == -5)
                {
                    // 墙或蛇身
                    if (layer == 1 && mp[next_y][next_x] == -4)
                    {
                        score -= 10; // 靠近墙的惩罚
                    }
                }
                else
                {
                    // 加入待访问队列
                    string neighbour = Utils::idx2str(make_pair(next_y, next_x));
                    if (visited.find(neighbour) == visited.end())
                    {
                        q.emplace(neighbour, layer + 1);
                        visited.insert(neighbour);
                    }
                }
            }
        }
        
        return score;
    }

    // 拐角评估函数 - 检查是否进入了特殊拐角位置
    int cornerEval(int y, int x, int fy, int fx)
    {
        // 从 fx fy 走到 x, y
        // 首先判断是否是corner
        string target = Utils::idx2str({y, x});
        unordered_set<string> corners = {"0917", "1018", "1015", "1116", "1214", "1315",
                                         "0923", "1022", "1025", "1124", "1226", "1325",
                                         "1814", "1715", "2015", "1916", "2117", "2018",
                                         "2123", "2022", "2025", "1924", "1826", "1725"};
        if (corners.count(target) == 0)
        {
            return -10000; // 不是拐角
        }
        
        // 检查墙体情况，确定拐角类型
        int checkWall = 0, qx, qy;
        if (mp[y - 1][x] == -4 || y - 1 == fy)
        {
            // 上面是墙
            checkWall += 100;
        }
        if (mp[y + 1][x] == -4 || y + 1 == fy)
        {
            checkWall -= 100;
        }
        if (mp[y][x - 1] == -4 || x - 1 == fx)
        {
            checkWall += 1;
        }
        if (mp[y][x + 1] == -4 || x + 1 == fx)
        {
            checkWall -= 1;
        }
        
        // 根据墙的位置确定下一个必须移动的方向
        switch (checkWall)
        {
        case 1:
            qx = x + 1;
            qy = y;
            break;
        case -1:
            qx = x - 1;
            qy = y;
            break;
        case 100:
            qx = x;
            qy = y + 1;
            break;
        default:
            qx = x;
            qy = y - 1;
            break;
        }
        
        // 检查下一个位置是否安全
        if (mp2[qy][qx] == -5 || mp2[qy][qx] == -6)
        {
            return -5000; // 危险拐角
        }
        if (mp[qy][qx] == -2)
        {
            return 1; // 有陷阱的拐角
        }
        return 0; // 安全拐角
    }

    // 综合评估函数 - 结合BFS、拐角评估和安全区边界考虑
    double eval(const GameState &state, int y, int x, int fy, int fx)
    {
        int test = cornerEval(y, x, fy, fx);
        if (test != -10000)
        {
            // 是拐角位置
            if (test == -5000)
            {
                return -50000; // 危险拐角，强烈避免
            }
            if (test == 1)
            {
                mp[y][x] = -9; // 标记陷阱拐角
            }
        }
        
        // 安全区评分 - 现在只对靠近边界的位置有轻微惩罚
        double safe_zone_score = safeZoneCenterScore(state, y, x);
        
        // BFS评估
        double bfs_score = bfs(y, x, fy, fx, state);
        
        // 整合评分，安全区评分不再包含中心偏好
        return bfs_score + safe_zone_score;
    }
}

int judge(const GameState &state)
{
    // 更新目标锁定状态（保留接口，暂不启用实际锁定功能）
    lock_on_target(state);
    
    // 即时反应 - 处理近距离高价值目标
    const auto &head = state.get_self().get_head();
    
    // 距离分层常量定义
    const int VERY_CLOSE_RANGE = 4;  // 增加普通食物即时反应范围
    const int CLOSE_RANGE = 6;       // 近距离
    const int EXTENDED_RANGE = 10;   // 扩展检测范围
    
    // 第一优先级：极近距离高价值尸体 (<=3格)
    for (const auto &item : state.items)
    {
        if (item.value >= 5)  // 高价值食物（蛇尸体）
        {
            int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
            if (dist <= VERY_CLOSE_RANGE)  // 极近距离最高优先级
            {
                // 首先检查是否会因安全区收缩而消失
                int current_tick = MAX_TICKS - state.remaining_ticks;
                int ticks_to_shrink = state.next_shrink_tick - current_tick;
                
                if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20) {
                    if (item.pos.x < state.next_safe_zone.x_min || item.pos.x > state.next_safe_zone.x_max ||
                        item.pos.y < state.next_safe_zone.y_min || item.pos.y > state.next_safe_zone.y_max) {
                        continue; // 食物将消失，跳过
                    }
                }
                
                // 检查是否能够及时到达
                if (!Utils::canReachFoodInTime(head.y, head.x, item.pos.y, item.pos.x, item.lifetime)) {
                    continue; // 跳过无法及时到达的食物
                }
                
                // 确定移动方向
                Direction move_dir;
                if (head.x > item.pos.x) move_dir = Direction::LEFT;
                else if (head.x < item.pos.x) move_dir = Direction::RIGHT;
                else if (head.y > item.pos.y) move_dir = Direction::UP;
                else move_dir = Direction::DOWN;
                
                // 检查移动安全性
                unordered_set<Direction> illegals = illegalMove(state);
                if (illegals.count(move_dir) == 0) {
                    return Utils::dir2num(move_dir);
                }
            }
        }
    }
    
    // 特殊情况：如果有极高价值尸体但距离远，而附近有普通食物，权衡选择
    for (const auto &item : state.items) {
        // 只考虑普通食物
        if (item.value > 0 && item.value < 5) {
            int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
            if (dist <= 2) { // 非常近的食物
                // 检查是否会因安全区收缩而消失
                int current_tick = MAX_TICKS - state.remaining_ticks;
                int ticks_to_shrink = state.next_shrink_tick - current_tick;
                
                if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20) {
                    if (item.pos.x < state.next_safe_zone.x_min || item.pos.x > state.next_safe_zone.x_max ||
                        item.pos.y < state.next_safe_zone.y_min || item.pos.y > state.next_safe_zone.y_max) {
                        continue; // 食物将消失，跳过
                    }
                }
                
                // 检查是否能够及时到达
                if (!Utils::canReachFoodInTime(head.y, head.x, item.pos.y, item.pos.x, item.lifetime)) {
                    continue; // 跳过无法及时到达的食物
                }
                
                // 如果可达且安全，直接获取这个近距离食物
                Direction move_dir;
                if (head.x > item.pos.x) move_dir = Direction::LEFT;
                else if (head.x < item.pos.x) move_dir = Direction::RIGHT;
                else if (head.y > item.pos.y) move_dir = Direction::UP;
                else move_dir = Direction::DOWN;
                
                // 检查移动安全性
                unordered_set<Direction> illegals = illegalMove(state);
                if (illegals.count(move_dir) == 0) {
                    return Utils::dir2num(move_dir);
                }
            }
        }
    }
    
    // 第二优先级：近距离更高价值尸体 (<=6格，价值>=8)
    for (const auto &item : state.items)
    {
        if (item.value >= 8)  // 更高价值食物
        {
            int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
            if (dist <= CLOSE_RANGE)  // 近距离
            {
                // 首先检查是否会因安全区收缩而消失
                int current_tick = MAX_TICKS - state.remaining_ticks;
                int ticks_to_shrink = state.next_shrink_tick - current_tick;
                
                if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20) {
                    if (item.pos.x < state.next_safe_zone.x_min || item.pos.x > state.next_safe_zone.x_max ||
                        item.pos.y < state.next_safe_zone.y_min || item.pos.y > state.next_safe_zone.y_max) {
                        continue; // 食物将消失，跳过
                    }
                }
                
                // 检查是否能够及时到达
                if (!Utils::canReachFoodInTime(head.y, head.x, item.pos.y, item.pos.x, item.lifetime)) {
                    continue; // 跳过无法及时到达的食物
                }
                
                // 确定移动方向
                Direction move_dir;
                if (head.x > item.pos.x) move_dir = Direction::LEFT;
                else if (head.x < item.pos.x) move_dir = Direction::RIGHT;
                else if (head.y > item.pos.y) move_dir = Direction::UP;
                else move_dir = Direction::DOWN;
                
                // 检查移动安全性
                unordered_set<Direction> illegals = illegalMove(state);
                if (illegals.count(move_dir) == 0) {
                    return Utils::dir2num(move_dir);
                }
            }
        }
    }
    
    // 第三优先级：扩展距离高价值尸体 (<=10格，价值>=10)
    for (const auto &item : state.items)
    {
        if (item.value >= 10)  // 极高价值食物
        {
            int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
            if (dist <= EXTENDED_RANGE)  // 扩展距离
            {
                // 首先检查是否会因安全区收缩而消失
                int current_tick = MAX_TICKS - state.remaining_ticks;
                int ticks_to_shrink = state.next_shrink_tick - current_tick;
                
                if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20) {
                    if (item.pos.x < state.next_safe_zone.x_min || item.pos.x > state.next_safe_zone.x_max ||
                        item.pos.y < state.next_safe_zone.y_min || item.pos.y > state.next_safe_zone.y_max) {
                        continue; // 食物将消失，跳过
                    }
                }
                
                // 检查是否能够及时到达
                if (!Utils::canReachFoodInTime(head.y, head.x, item.pos.y, item.pos.x, item.lifetime)) {
                    continue; // 跳过无法及时到达的食物
                }
                
                // 确定移动方向
                Direction move_dir;
                if (head.x > item.pos.x) move_dir = Direction::LEFT;
                else if (head.x < item.pos.x) move_dir = Direction::RIGHT;
                else if (head.y > item.pos.y) move_dir = Direction::UP;
                else move_dir = Direction::DOWN;
                
                // 检查移动安全性
                unordered_set<Direction> illegals = illegalMove(state);
                if (illegals.count(move_dir) == 0) {
                    return Utils::dir2num(move_dir);
                }
            }
        }
    }
    
    // 普通近距离食物检测
    for (const auto &item : state.items)
    {
        if (item.value == -2) continue; // 跳过陷阱
        
        int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
        if (dist <= VERY_CLOSE_RANGE)  // 修复：使用VERY_CLOSE_RANGE替代immediate_range
        {
            // 首先检查是否会因安全区收缩而消失
            int current_tick = MAX_TICKS - state.remaining_ticks;
            int ticks_to_shrink = state.next_shrink_tick - current_tick;
            
            if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20) {
                if (item.pos.x < state.next_safe_zone.x_min || item.pos.x > state.next_safe_zone.x_max ||
                    item.pos.y < state.next_safe_zone.y_min || item.pos.y > state.next_safe_zone.y_max) {
                    continue; // 食物将消失，跳过
                }
            }
            
            // 然后检查是否能够及时到达
            if (!Utils::canReachFoodInTime(head.y, head.x, item.pos.y, item.pos.x, item.lifetime)) {
                continue; // 跳过无法及时到达的食物
            }
            
            // 确定移动方向
            Direction move_dir;
            if (head.x > item.pos.x) move_dir = Direction::LEFT;
            else if (head.x < item.pos.x) move_dir = Direction::RIGHT;
            else if (head.y > item.pos.y) move_dir = Direction::UP;
            else move_dir = Direction::DOWN;
            
            // 检查移动安全性
            unordered_set<Direction> illegals = illegalMove(state);
            if (illegals.count(move_dir) == 0) {
                return Utils::dir2num(move_dir);
            }
        }
    }
    
    // 确定合法移动
    unordered_set<Direction> illegals = illegalMove(state);
    vector<Direction> legalMoves;
    
    for (auto dir : validDirections)
    {
        if (illegals.count(dir) == 0)
        {
            legalMoves.push_back(dir);
        }
    }
    
    // 没有合法移动，使用护盾
    if (legalMoves.empty())
    {
        return SHIELD_COMMAND;
    }
    
    // 评估每个合法移动的分数
    double maxF = -4000;
    Direction choice = legalMoves[0];
    
    for (auto dir : legalMoves)
    {
        // 重用前面已经声明的head变量
        const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
        
        double eval = Strategy::eval(state, y, x, head.y, head.x);
        
        // 更新最高分数的方向
        if (eval > maxF)
        {
            maxF = eval;
            choice = dir;
        }
        
        // 如果分数相同，有一定概率选择新方向（增加随机性）
        if (eval == maxF)
        {
            const int p = 2;
            static mt19937 gen(chrono::steady_clock::now().time_since_epoch().count());
            uniform_int_distribution<> distrib(0, p - 1);
            const int random = distrib(gen);
            if (random == 0)
            {
                choice = dir;
            }
        }
    }
    
    // 如果评分极低，使用护盾
    if (maxF == -4000)
    {
        return SHIELD_COMMAND;
    }
    
    return Utils::dir2num(choice);
}

int main() {
  // 读取当前 tick 的所有游戏状态
  GameState current_state;
  read_game_state(current_state);

  int decision = judge(current_state);

  // 输出决策
  cout << decision << endl;
  // C++ 23 也可使用 std::print
  // 如果需要写入 Memory，在此处写入

  return 0;
}
