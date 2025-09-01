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

// ===== 从EXAMPLE2.cpp移植的常量 =====
constexpr int EmptyIdx = -1;     // 空位置标识
constexpr int SHIELD_COMMAND = 4; // 护盾指令值

// ===== 从EXAMPLE2.cpp移植的辅助枚举和工具函数 =====
enum class Direction
{
    LEFT,
    UP,
    RIGHT,
    DOWN
};

const vector<Direction> validDirections{Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT};

// ===== 移植Utils命名空间的工具函数 =====
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

// 从EXAMPLE2.cpp移植的函数 - 确定非法移动
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
            if (mp[y][x] == -4) // 墙
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
                if (mp[y][x] == -4) // 墙
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

// 从EXAMPLE2.cpp移植的策略评估函数
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

    // BFS搜索评估函数
    double bfs(int sy, int sx, int fy, int fx, const GameState &state)
    {
        double score = 0;
        
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
        const double start = 150, end = 25, maxNum = 5, maxNum2 = 30;
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
            if (start > timeRest && timeRest >= end)
            {
                const double r = maxNum * (timeRest - start) * (timeRest - start);
                score -= (abs(sy - 14.5) + abs(sx - 19.5)) * r / ((start - end) * (start - end));
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
            
            // 评估物品价值
            if (mp[y][x] == -1) // 增长豆
            {
                num = 1.8;
            }
            if (mp[y][x] == -2) // 陷阱
            {
                num = -0.5;
            }
            
            // 计算位置权重
            double weight;
            auto [tot, mini] = count(state, y, x);
            
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
                if (tot >= 4)
                {
                    weight = 0.5; // 太多竞争者
                }
                if (tot >= 3 && mini <= 3)
                {
                    weight = 0.5; // 多竞争者且很近
                }
                if (tot >= 2 && mini <= 2)
                {
                    weight = 0.5; 
                }
                if (tot >= 1 && mini <= 1)
                {
                    weight = 0.5; // 有竞争者且很近
                }
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

    // 综合评估函数 - 结合BFS和拐角评估
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
        return bfs(y, x, fy, fx, state); // 传递state参数
    }
}

// 主决策函数 - 移植自EXAMPLE2.cpp的judge函数
int judge(const GameState &state)
{
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
        const auto &head = state.get_self().get_head();
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

  // 使用移植的judge函数进行决策
  int decision = judge(current_state);

  // 输出决策
  cout << decision << endl;
  // C++ 23 也可使用 std::print
  // 如果需要写入 Memory，在此处写入

  return 0;
}
