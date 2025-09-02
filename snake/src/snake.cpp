#include <chrono>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>
//* 新增头文件
#include <cmath>
#include <cstring>
#include <queue>
#include <algorithm>
#include <unordered_set>
#include <climits>

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

// 将类型定义放到最前面
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

// 前向声明Strategy命名空间中的结构体和函数
namespace Strategy {
    struct TargetEvaluation {
        Point position;
        double value;
        int distance;
        bool is_safe;
    };
    TargetEvaluation evaluateTarget(const GameState &state, const Point &target_pos, double base_value, bool is_chest);
    struct DeadEndAnalysis;
    struct TerrainAnalysis;
}

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

    // 判断食物是否可以及时到达 - 直接使用坐标和生命周期
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

    // 判断位置是否会因安全区收缩而消失
    bool willDisappearInShrink(const GameState &state, int x, int y, int look_ahead_ticks = 20) {
        int current_tick = MAX_TICKS - state.remaining_ticks;
        int ticks_to_shrink = state.next_shrink_tick - current_tick;
      
        if (ticks_to_shrink >= 0 && ticks_to_shrink <= look_ahead_ticks) {
            if (x < state.next_safe_zone.x_min || x > state.next_safe_zone.x_max ||
                y < state.next_safe_zone.y_min || y > state.next_safe_zone.y_max) {
                return true; // 位置将在收缩中消失
            }
        }
      
        return false; // 位置安全或不会很快收缩
    }

    // 判断目标是否可达
    bool isTargetReachable(const GameState &state, const Point &target, int lifetime = INT_MAX) {
        const auto &head = state.get_self().get_head();
      
        // 检查安全区收缩
        if (willDisappearInShrink(state, target.x, target.y)) {
            return false;
        }
      
        // 检查能否及时到达
        if (lifetime < INT_MAX) {
            return canReachFoodInTime(head.y, head.x, target.y, target.x, lifetime);
        }
      
        return true;
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

// 游戏地图状态: mp用于物品, mp2用于蛇的位置
int mp[MAXM][MAXN], mp2[MAXM][MAXN];

// 简化的目标锁定机制
static Point current_target = {-1, -1};
static int target_value = 0;
static int target_lock_time = 0;

// 宝箱钥匙目标锁定标志
static bool is_key_target = false;  // 是否锁定钥匙
static bool is_chest_target = false; // 是否锁定宝箱

// 目标锁定函数
void lock_on_target(const GameState &state) {
    const auto &self = state.get_self();
    const auto &head = self.get_head();
  
    // 更新锁定状态
    if (target_lock_time > 0) {
        target_lock_time--;
    }
  
    // 如果宝箱消失了（可能被开启）
    if (is_chest_target && state.chests.empty()) {
        current_target = {-1, -1};
        target_value = 0;
        is_chest_target = false;
    }
  
    // 如果所有钥匙都消失了
    if (is_key_target && state.keys.empty()) {
        current_target = {-1, -1};
        target_value = 0;
        is_key_target = false;
    }
  
    // 如果持有钥匙，优先锁定宝箱
    if (self.has_key && !state.chests.empty()) {
        // 重置之前的钥匙目标
        is_key_target = false;
      
        // 如果还没有锁定宝箱或宝箱位置已变化，重新锁定宝箱
        if (!is_chest_target || current_target.x == -1 || current_target.y == -1) {
            // 评估所有宝箱
            Strategy::TargetEvaluation best_chest = {
                {-1, -1}, // position
                -1,       // value
                INT_MAX,  // distance
                false     // is_safe
            };
          
            for (const auto &chest : state.chests) {
                // 评估宝箱
                auto eval = Strategy::evaluateTarget(state, chest.pos, chest.score, true);
                
                // 选择价值最高且安全的宝箱，或者在没有安全选择时选择价值最高的
                if (eval.is_safe && (best_chest.position.x == -1 || eval.value > best_chest.value)) {
                    best_chest = eval;
                } else if (!best_chest.is_safe && eval.value > best_chest.value) {
                    best_chest = eval;
                }
            }
            
            // 如果找到了合适的宝箱
            if (best_chest.position.x != -1) {
                current_target = best_chest.position;
                target_value = best_chest.value;
                target_lock_time = min(best_chest.distance + 10, 30); // 给足够时间去宝箱
                is_chest_target = true;
              
                // 如果钥匙即将掉落，缩短锁定时间
                for (const auto &key : state.keys) {
                    if (key.holder_id == MYID) {
                        if (key.remaining_time < best_chest.distance) {
                            // 钥匙可能会在到达宝箱前掉落，调整锁定时间
                            target_lock_time = key.remaining_time - 1;
                        }
                        break;
                    }
                }
            }
        }
      
        // 检查宝箱是否仍然存在
        bool chest_exists = false;
        for (const auto &chest : state.chests) {
            if (chest.pos.y == current_target.y && chest.pos.x == current_target.x) {
                chest_exists = true;
                break;
            }
        }
      
        if (!chest_exists) {
            // 宝箱已被开启或消失
            current_target = {-1, -1};
            target_value = 0;
            is_chest_target = false;
        }
    }
    // 如果没有钥匙，考虑锁定钥匙
    else if (!self.has_key && !state.keys.empty()) {
        // 重置之前的宝箱目标
        is_chest_target = false;
      
        // 如果没有锁定钥匙或锁定时间结束，寻找新的钥匙
        if (!is_key_target || target_lock_time <= 0 || current_target.x == -1 || current_target.y == -1) {
            // 评估所有钥匙
            Strategy::TargetEvaluation best_key = {
                {-1, -1}, // position
                -1,       // value
                INT_MAX,  // distance
                false     // is_safe
            };
            
            for (const auto &key : state.keys) {
                // 只考虑地图上的钥匙
                if (key.holder_id == -1) {
                    // 评估钥匙
                    auto eval = Strategy::evaluateTarget(state, key.pos, 40.0, false);
                    
                    // 选择价值最高且安全的钥匙，或者在没有安全选择时选择价值最高的
                    if (eval.is_safe && (best_key.position.x == -1 || eval.value > best_key.value)) {
                        best_key = eval;
                    } else if (!best_key.is_safe && eval.value > best_key.value) {
                        best_key = eval;
                    }
                }
            }
            
            // 如果找到了合适的钥匙
            if (best_key.position.x != -1) {
                current_target = best_key.position;
                target_value = best_key.value;
                target_lock_time = min(best_key.distance + 5, 20);  // 给予足够的时间去拿钥匙
                is_key_target = true;
            }
        }
      
        // 检查钥匙是否仍然存在
        if (is_key_target) {
            bool key_exists = false;
            for (const auto &key : state.keys) {
                if (key.holder_id == -1 && 
                    abs(key.pos.y - current_target.y) <= 1 && 
                    abs(key.pos.x - current_target.x) <= 1) {
                    key_exists = true;
                    // 更新为精确位置
                    current_target = key.pos;
                    break;
                }
            }
          
            if (!key_exists) {
                // 钥匙已被拿走或消失
                current_target = {-1, -1};
                target_value = 0;
                is_key_target = false;
            }
        }
    }
    // 如果没有钥匙和宝箱相关目标，重置目标锁定
    else if (is_key_target || is_chest_target) {
        current_target = {-1, -1};
        target_value = 0;
        is_key_target = false;
        is_chest_target = false;
    }
  
    // 检查当前普通食物目标是否仍然存在
    if (!is_key_target && !is_chest_target && current_target.y != -1 && current_target.x != -1) {
        bool target_exists = false;
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
      
        // 如果目标不再存在或锁定时间结束，重置锁定状态
        if (!target_exists && target_lock_time <= 0) {
            current_target = {-1, -1};
            target_value = 0;
        }
    }
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
    
    // 本函数会更新紧急模式状态
    bool emergency_mode = false;
    
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

    // 检查方向是否合法的辅助函数
    auto checkDirectionLegality = [&](Direction dir, bool is_emergency) {
        const Point &head = self.get_head();
        const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
        
        // 检查是否越界
        if (!Utils::boundCheck(y, x)) {
            return false; // 不合法
        }
        
        // 安全区检查 - 即使在紧急情况下也不允许离开安全区
        if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max ||
            y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max) {
                // 只有当护盾时间足够时才允许离开安全区
                if (self.shield_time <= 1) {
                return false; // 不合法
            }
        }
        
        // 墙检查 - 任何情况下都不能穿墙
        if (mp[y][x] == -4) {
            return false; // 不合法
        }
        
        // 陷阱检查
        if (mp[y][x] == -2 && !is_emergency) {
            return false; // 非紧急情况下不能走陷阱
        }
        
        // 蛇身体检查
        if (mp2[y][x] == -5) {
            // 紧急模式下，如果护盾时间够或能开护盾，可以穿过蛇身
            if (is_emergency) {
                return (self.shield_time >= 2 || (self.shield_cd == 0 && self.score > 20) || 
                       state.remaining_ticks >= 255 - 9 + 2);
            } else {
                // 非紧急模式下，需要足够的护盾时间
                return (self.shield_time >= 2);
            }
        }
        
        // 蛇头或尾部可能移动区域
        if ((mp2[y][x] == -6 || mp2[y][x] == -7) && !is_emergency) {
            return (self.shield_time >= 2); // 非紧急模式下需要足够护盾时间
        }
        
        return true; // 默认合法
    };

    // 自己方向的反方向不能走
    addReverse();

    // 第一轮检查
    for (auto dir : validDirections) {
        if (!checkDirectionLegality(dir, false)) {
            illegals.insert(dir);
        }
    }

    // 如果四种方向都不行，进入紧急模式，重新评估
    if (illegals.size() == 4) {
        emergency_mode = true;
        illegals.clear();

        // 自己方向的反方向不能走
        addReverse();

        // 放宽条件下的第二轮检查
        for (auto dir : validDirections) {
            if (!checkDirectionLegality(dir, true)) {
                illegals.insert(dir);
            }
        }
    }

    return illegals;
}

namespace Strategy
{
    // 前向声明evaluateTrap函数，确保在bfs函数之前可见
    double evaluateTrap(const GameState &state, int trap_y, int trap_x);

    // 地形分析结构体，用于评估死胡同和瓶颈
    struct TerrainAnalysis {
        bool is_dead_end;      // 是否是死胡同
        bool is_bottleneck;    // 是否是瓶颈
        int exit_count;        // 出口数量
        double risk_score;     // 风险评分
        int depth;             // 空间深度

        // 默认构造函数
        TerrainAnalysis() : is_dead_end(false), is_bottleneck(false), 
                          exit_count(0), risk_score(0), depth(0) {}
    };

    // 检查敌方蛇是否在指定区域附近
    bool checkEnemyNearArea(const GameState &state, const unordered_set<string>& area, int max_distance = 3) {
        for (const auto &snake : state.snakes) {
            if (snake.id != MYID && snake.id != -1) {
                const Point &enemy_head = snake.get_head();
                
                // 检查敌方蛇头是否靠近区域中的任何点
                for (const auto &point_str : area) {
                    auto point = Utils::str2idx(point_str);
                    int dist = abs(enemy_head.y - point.first) + abs(enemy_head.x - point.second);
                    if (dist <= max_distance) {
                        return true; // 发现敌方蛇在附近
                    }
                }
            }
        }
        return false; // 区域附近没有敌方蛇
    }

    // 分析地形风险，识别死胡同和瓶颈
    TerrainAnalysis analyzeTerrainRisk(const GameState &state, int y, int x) {
        TerrainAnalysis result;
        
        // 使用广度优先搜索，分析可到达区域的拓扑结构
        unordered_set<string> visited;
        queue<pair<pair<int, int>, int>> q; // 位置和距离
        q.push({{y, x}, 0});
        visited.insert(Utils::idx2str({y, x}));
        
        // 记录深度方向拓展和宽度方向拓展
        int max_depth = 0;
        int max_width = 0;
        int bottleneck_width = INT_MAX;
        vector<vector<int>> reachable(MAXM, vector<int>(MAXN, -1));
        
        // 执行BFS
        while (!q.empty()) {
            auto [pos, depth] = q.front();
            q.pop();
            
            max_depth = max(max_depth, depth);
            reachable[pos.first][pos.second] = depth;
            
            // 检查四个方向
            for (auto dir : validDirections) {
                const auto [ny, nx] = Utils::nextPos(pos, dir);
                
                // 检查是否越界或者是障碍物
                if (!Utils::boundCheck(ny, nx) || mp[ny][nx] == -4 || mp2[ny][nx] == -5) {
                    continue;
                }
                
                string next = Utils::idx2str({ny, nx});
                if (visited.find(next) == visited.end()) {
                    visited.insert(next);
                    q.push({{ny, nx}, depth + 1});
                }
            }
        }
        
        // 统计每个深度的宽度
        vector<int> width_at_depth(max_depth + 1, 0);
        for (int i = 0; i < MAXM; i++) {
            for (int j = 0; j < MAXN; j++) {
                if (reachable[i][j] >= 0) {
                    width_at_depth[reachable[i][j]]++;
                }
            }
        }
        
        // 计算最大宽度和瓶颈宽度
        for (int d = 0; d <= max_depth; d++) {
            max_width = max(max_width, width_at_depth[d]);
            if (width_at_depth[d] > 0) {
                bottleneck_width = min(bottleneck_width, width_at_depth[d]);
            }
        }
        
        // 分析结果
        result.exit_count = visited.size();
        result.depth = max_depth;
        
        // 检测死胡同：深度大于宽度且空间有限
        if (max_depth > 3 && max_width < 3 && visited.size() < 12) {
            result.is_dead_end = true;
            result.risk_score -= 800;
            
            // 获取蛇长度，评估是否能在死胡同中调头
            int my_length = 0;
            for (const auto &snake : state.snakes) {
                if (snake.id == MYID) {
                    my_length = snake.length;
                    break;
                }
            }
            
            // 如果死胡同太窄不能调头，进一步增加风险
            if (my_length > max_width * 2) {
                result.risk_score -= 1000;
                
                // 极短死胡同且蛇较长时，给予极高惩罚
                if (max_depth <= 2 && my_length >= 8) {
                    result.risk_score = -1800; // 几乎必死
                }
            }
        }
        
        // 检测瓶颈：有收缩点且收缩点宽度小
        if (bottleneck_width <= 2 && max_width > 3) {
            result.is_bottleneck = true;
            result.risk_score -= 300 * (4 - bottleneck_width);
            
            // 检查瓶颈附近是否有其他蛇
            if (checkEnemyNearArea(state, visited, 3)) {
                result.risk_score -= 500;
            }
        }
        
        return result;
    }

    // 死胡同分析结构体
    struct DeadEndAnalysis {
        bool is_dead_end;
        int depth;
        double risk_score;
    };

    // 分析位置是否为死胡同
    DeadEndAnalysis analyzeDeadEnd(const GameState &state, int y, int x) {
        // 调用新的地形分析函数
        TerrainAnalysis terrain = analyzeTerrainRisk(state, y, x);
        
        // 兼容旧的DeadEndAnalysis结构体
        DeadEndAnalysis result;
        result.is_dead_end = terrain.is_dead_end;
        result.depth = terrain.depth;
        result.risk_score = terrain.risk_score;
        
        return result;
    }

    // 检查位置是否在安全区内，并评估收缩风险
    double checkSafeZoneRisk(const GameState &state, int x, int y) {
        double score = 0;
        int current_tick = MAX_TICKS - state.remaining_ticks;
        int ticks_to_shrink = state.next_shrink_tick - current_tick;
        
        // 如果即将收缩（小于20个tick）
        if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20) {
            // 检查位置是否在下一个安全区内
            if (x < state.next_safe_zone.x_min || x > state.next_safe_zone.x_max ||
                y < state.next_safe_zone.y_min || y > state.next_safe_zone.y_max) {
                // 安全区外位置价值大幅降低，危险度随收缩时间临近增加
                if (ticks_to_shrink <= 5) {
                    // 极度危险，几乎立即放弃
                    score = -5000;
                } else if (ticks_to_shrink <= 10) {
                    // 很危险，强烈避免
                    score = -3000; 
                } else {
                    // 有风险，尽量避免
                    score = -1000;
                }
            }
        }
        
        return score;
    }
    
    // 检查食物是否能及时到达并计算价值
    double evaluateFoodValue(const GameState &state, int y, int x, int base_value, int distance) {
        double num = 0;
        
        // 根据食物类型和距离评估价值
        if (base_value > 0) { // 普通食物
            // 动态评估尸体价值 - 基于距离
            if (base_value >= 10) {
                // 极高价值的尸体
                if (distance <= 6) {
                    // 非常近的高价值尸体，价值翻倍
                    num = base_value * 200 + 200;
                } else if (distance <= 10) {
                    // 较近的高价值尸体
                    num = base_value * 120 + 120;
                } else {
                    // 远距离高价值尸体，标准价值
                    num = base_value * 80 + 80;
                }
            } else if (base_value >= 5) {
                // 中等价值的尸体
                if (distance <= 4) {
                    // 非常近的中等尸体，价值提高
                    num = base_value * 100 + 80;
                } else {
                    // 标准价值
                    num = base_value * 70 + 40;
                }
            } else if (base_value > 0) {
                // 普通食物也增加距离差异化处理
                if (distance <= 3) {
                    // 非常近的普通食物，提高价值
                    num = base_value * 45; // 提高50%
                } else if (distance <= 5) {
                    // 较近的普通食物，略微提高价值
                    num = base_value * 35; // 提高约17%
                } else {
                    // 远距离普通食物，标准价值
                    num = base_value * 30;
                }
            }
        } else if (base_value == -1) { // 增长豆
            // 根据游戏阶段动态调整增长豆价值
            if (state.remaining_ticks > 180)
                num = 30; // 早期价值高
            else if (state.remaining_ticks > 100)
                num = 20; // 中期价值中等
            else
                num = 10; // 后期价值低
        } else if (base_value == -2) { // 陷阱
            num = -0.5;
        }
        
        // 根据游戏阶段动态调整普通食物价值
        if (base_value > 0 && base_value < 5) { // 只处理普通食物
            // 早期更看重普通食物(避免过分追求增长豆)
            if (state.remaining_ticks > 180) {
                num *= 1.2; // 提高20%
            }
            // 后期提高所有食物价值(加速得分)
            else if (state.remaining_ticks < 60) {
                num *= 1.3; // 提高30%
            }
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
        
        return num;
    }

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
        
        // 安全区收缩风险评估 - 使用新的辅助函数
        score += checkSafeZoneRisk(state, sx, sy);
        
        // 动态瓶颈区域检测（替换原有硬编码的特殊位置检测）
        bool is_bottleneck = false;
        int wall_count = 0;
        int danger_direction_count = 0;
        vector<pair<int, int>> exit_directions;
        
        // 检查周围有多少墙或障碍物
        for (auto dir : validDirections) {
            const auto [ny, nx] = Utils::nextPos({sy, sx}, dir);
            if (!Utils::boundCheck(ny, nx) || mp[ny][nx] == -4) {
                wall_count++; // 墙或边界
            } else if (mp2[ny][nx] == -5) {
                wall_count++; // 蛇身体部分，完全阻挡
            } else if (mp2[ny][nx] == -6 || mp2[ny][nx] == -7) {
                danger_direction_count++; // 危险方向（蛇头/尾可能移动区域）
            } else {
                // 记录可能的出口方向
                exit_directions.push_back({ny, nx});
            }
        }
        
        // 如果周围有大量障碍，且可移动方向很少，视为瓶颈
        if (wall_count >= 2 && exit_directions.size() <= 2) {
            is_bottleneck = true;
            
            // 检查出口方向是否安全
            for (const auto& [exit_y, exit_x] : exit_directions) {
                bool exit_safe = true;
                
                // 检查这个出口附近是否有敌方蛇
                for (const auto &snake : state.snakes) {
                    if (snake.id != MYID && snake.id != -1) {
                        const Point &enemy_head = snake.get_head();
                        int dist_to_exit = abs(enemy_head.y - exit_y) + abs(enemy_head.x - exit_x);
                        
                        // 如果敌方蛇距离出口很近，这个出口不安全
                        if (dist_to_exit <= 3) {
                            exit_safe = false;
                            break;
                        }
                    }
                }
                
                // 如果这个出口安全，瓶颈就不那么危险
                if (exit_safe) {
                    is_bottleneck = false;
                    break;
                }
            }
            
            // 如果是危险瓶颈(所有出口都不安全或没有出口)
            if (is_bottleneck) {
                // 危险程度取决于周围墙的数量和危险方向的数量
                int danger_level = wall_count * 500 + danger_direction_count * 300;
                score -= danger_level;
                
                // 极端情况：完全封闭或无安全出口
                if (exit_directions.empty() || (wall_count >= 3 && danger_direction_count >= 1)) {
                    return -2000; // 极度危险，几乎确定死亡
                }
            }
        }
        
        // 使用新的地形风险分析函数替代原有死胡同检测
        auto terrain_risk = analyzeTerrainRisk(state, sy, sx);
        
        // 如果是死胡同或瓶颈，将风险分数添加到总分中
        score += terrain_risk.risk_score;
        
        // 极度危险的地形，提前返回
        if (terrain_risk.risk_score <= -1800) {
            return terrain_risk.risk_score; // 直接返回极高风险值
        }
        
        // 处理陷阱的情况
        if (mp[sy][sx] == -2 && !terrain_risk.is_bottleneck) // 只有当不是瓶颈时才考虑陷阱
        {
            // 使用专门的陷阱评估函数
            score = evaluateTrap(state, sy, sx);
        }
        else if (mp[sy][sx] == -2 && terrain_risk.is_bottleneck && mp2[sy][sx] != -5 && mp2[sy][sx] != -6 && mp2[sy][sx] != -7)
        {
            // 拐角陷阱，但非紧急情况
            score = -500; // 非紧急情况下尽量避免
            
            // 检查是否处于紧急情况（四周都是障碍）
            bool is_emergency = true;
            const auto &head = state.get_self().get_head();
            for (auto dir : validDirections) {
                const auto [next_y, next_x] = Utils::nextPos({head.y, head.x}, dir);
                if (Utils::boundCheck(next_y, next_x)) {
                    if (mp[next_y][next_x] != -4 && mp2[next_y][next_x] != -5 &&
                        next_x >= state.current_safe_zone.x_min && next_x <= state.current_safe_zone.x_max && 
                        next_y >= state.current_safe_zone.y_min && next_y <= state.current_safe_zone.y_max) {
                        is_emergency = false;
                        break;
                    }
                }
            }
            
            if (is_emergency) {
                score = -100; // 紧急情况下接受陷阱
            }
            
            const double start = 150, end = 25, maxNum2 = 30;
            const int timeRest = state.remaining_ticks;
            
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
            if (state.remaining_ticks < 50)
            {
                maxLayer = 10; // 后期减小搜索范围
            }
            
            if (layer >= maxLayer)
            {
                break;
            }
            
            // 解析当前位置
            const auto [y, x] = Utils::str2idx(currentState);
            
            // 检查食物可达性
            bool can_reach = true;
            for (const auto &item : state.items) {
                if (item.pos.y == y && item.pos.x == x) {
                    // 使用通用的目标可达性检测函数
                    if (!Utils::isTargetReachable(state, item.pos, item.lifetime)) {
                        can_reach = false;
                        break;
                    }
                }
            }
            
            // 获取食物基础价值
            int base_value = mp[y][x];
            
            // 计算到蛇头的距离
            const auto &snake_head = state.get_self().get_head();
            int head_dist = abs(snake_head.y - y) + abs(snake_head.x - x);
            
            // 使用新的辅助函数评估食物价值
            double num = (mp[y][x] != 0 && mp[y][x] != -2 && !can_reach) ? 0 : 
                        evaluateFoodValue(state, y, x, base_value, head_dist);
            
            // 计算位置权重
            double weight;
            
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

    // 动态拐角检测函数，替换原有的硬编码版本
    int cornerEval(int y, int x, int fy, int fx)
    {
        // 如果坐标无效，立即返回
        if (!Utils::boundCheck(y, x)) {
            return -10000;
        }
      
        // 统计周围墙体和障碍物的分布
        int wall_count = 0;
        vector<pair<int, int>> adjacent_cells;
        vector<Direction> possible_exits;
      
        // 检查四个方向
        for (auto dir : validDirections) {
            const auto [ny, nx] = Utils::nextPos({y, x}, dir);
            if (!Utils::boundCheck(ny, nx) || mp[ny][nx] == -4 || mp2[ny][nx] == -5) {
                // 墙或蛇身
                wall_count++;
            } else {
                adjacent_cells.push_back({ny, nx});
                possible_exits.push_back(dir);
            }
        }
      
        // 拐角定义: 正好有两个墙，且两个墙相邻(形成直角)
        if (wall_count != 2 || adjacent_cells.size() != 2) {
            return -10000; // 不是拐角
        }
      
        // 确认墙是否相邻形成直角(两墙不在对角线上)
        bool is_diagonal = true;
        if (possible_exits.size() == 2) {
            // 检查剩余出口是否是对角线方向
            if ((possible_exits[0] == Direction::UP && possible_exits[1] == Direction::DOWN) ||
                (possible_exits[0] == Direction::DOWN && possible_exits[1] == Direction::UP) ||
                (possible_exits[0] == Direction::LEFT && possible_exits[1] == Direction::RIGHT) ||
                (possible_exits[0] == Direction::RIGHT && possible_exits[1] == Direction::LEFT)) {
                is_diagonal = false; // 出口在对角线方向
            }
        }
      
        if (!is_diagonal) {
            return -10000; // 不是典型的L型拐角
        }
      
        // 找出拐角的出口方向(两个相邻的开放方向)
        // 我们需要计算下一步会被迫走向哪个方向
        int next_y = -1, next_x = -1;
      
        // 确定进入拐角的方向
        Direction entry_dir;
        if (fy < y) entry_dir = Direction::UP;
        else if (fy > y) entry_dir = Direction::DOWN;
        else if (fx < x) entry_dir = Direction::LEFT;
        else entry_dir = Direction::RIGHT;
      
        // 确定出口方向(与入口不同的方向)
        for (auto dir : possible_exits) {
            // 不能往回走，所以出口不能是入口方向的反方向
            if ((entry_dir == Direction::UP && dir == Direction::DOWN) ||
                (entry_dir == Direction::DOWN && dir == Direction::UP) ||
                (entry_dir == Direction::LEFT && dir == Direction::RIGHT) ||
                (entry_dir == Direction::RIGHT && dir == Direction::LEFT)) {
                continue;
            }
          
            // 找到可能的出口
            const auto [ny, nx] = Utils::nextPos({y, x}, dir);
            next_y = ny;
            next_x = nx;
            break;
        }
      
        // 如果没找到有效出口，说明这是死胡同
        if (next_y == -1 || next_x == -1) {
            return -10000; // 不是标准拐角，可能是死胡同
        }
      
        // 检查出口位置的安全性
        if (mp2[next_y][next_x] == -5 || mp2[next_y][next_x] == -6) {
            return -5000; // 危险拐角：出口处有蛇身或蛇头威胁区域
        }
      
        if (mp[next_y][next_x] == -2) {
            return -50; // 陷阱拐角：出口处有陷阱
        }
      
        return 0; // 安全拐角
    }
    
    // 新增NAIVE陷阱评估函数
    double evaluateTrap(const GameState &state, int trap_y, int trap_x) {
        // 显式标记参数为已使用，避免警告
        (void)trap_y;
        (void)trap_x;
        
        const auto &self = state.get_self();
        
        // 基础分：默认陷阱非常危险
        double score = -1000;
        
        // 如果蛇当前分数很高，减轻陷阱惩罚
        if (self.score > 50) {
            score = -800; // 减轻惩罚
        } else if (self.score > 100) {
            score = -600; // 更进一步减轻惩罚
        }
        
        // 检测是否处于紧急情况（四周都是障碍）
        bool surrounded = true;
        const auto &head = self.get_head();
        for (auto dir : validDirections) {
            const auto [next_y, next_x] = Utils::nextPos({head.y, head.x}, dir);
            if (Utils::boundCheck(next_y, next_x)) {
                // 检查是否为障碍（墙、蛇身，但不包括陷阱）
                if (mp[next_y][next_x] != -4 && mp2[next_y][next_x] != -5 && 
                    next_x >= state.current_safe_zone.x_min && next_x <= state.current_safe_zone.x_max && 
                    next_y >= state.current_safe_zone.y_min && next_y <= state.current_safe_zone.y_max) {
                    surrounded = false;
                    break;
                }
            }
        }
        
        // 如果蛇即将死亡(四周都是障碍)，接受陷阱
        if (surrounded) {
            score = -100; // 显著减轻惩罚
        }
        
        return score;
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
            if (test == -50)
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

    // 评估目标的价值和安全性
    TargetEvaluation evaluateTarget(const GameState &state, const Point &target_pos, double base_value, bool is_chest) {
        const auto &head = state.get_self().get_head();
        int distance = abs(head.y - target_pos.y) + abs(head.x - target_pos.x);
      
        // 基础评估
        TargetEvaluation result = {
            target_pos,
            base_value,
            distance,
            true // 默认安全
        };
      
        // 安全性检查1: 安全区收缩
        if (Utils::willDisappearInShrink(state, target_pos.x, target_pos.y)) {
            result.is_safe = false;
            return result; // 目标将消失，直接返回
        }
      
        // 安全性检查2: 死胡同和瓶颈
        auto dead_end = analyzeDeadEnd(state, target_pos.y, target_pos.x);
        if (dead_end.is_dead_end) {
            // 对宝箱尤其严格判断死胡同风险
            if (is_chest && dead_end.risk_score < -500) {
                result.is_safe = false;
                result.value *= 0.3; // 大幅降低价值
            } else {
                // 根据风险程度降低价值
                result.value *= (1.0 + dead_end.risk_score / 1000);
            }
        }
      
        // 竞争因素
        int closest_competitor_dist = INT_MAX;
      
        for (const auto &snake : state.snakes) {
            if (snake.id != MYID) {
                int enemy_dist = abs(snake.get_head().y - target_pos.y) + 
                                abs(snake.get_head().x - target_pos.x);
              
                if (enemy_dist < distance) {
                    closest_competitor_dist = min(closest_competitor_dist, enemy_dist);
                  
                    // 对宝箱的竞争评估更严格
                    if (is_chest) {
                        result.value *= 0.5; // 竞争者更近，价值减半
                      
                        // 如果竞争者远远更近，宝箱几乎不可能获得
                        if (enemy_dist < distance / 2) {
                            result.is_safe = false;
                            result.value *= 0.2; // 进一步大幅降低价值
                        }
                    } else {
                        result.value *= 0.7; // 钥匙竞争惩罚较轻
                    }
                }
            }
        }
      
        return result;
    }
}

// 添加到judge函数之前
Direction chooseBestDirection(const GameState &state, const vector<Direction>& preferred_dirs = {}) {
                unordered_set<Direction> illegals = illegalMove(state);
    vector<Direction> legalMoves;
  
    // 首先尝试首选方向
    for (auto dir : preferred_dirs) {
        if (illegals.count(dir) == 0) {
            return dir; // 首选方向合法，直接返回
        }
    }
  
    // 没有可用的首选方向，收集所有合法移动
    for (auto dir : validDirections) {
        if (illegals.count(dir) == 0) {
            legalMoves.push_back(dir);
        }
    }
  
    // 没有合法移动
    if (legalMoves.empty()) {
        return Direction::UP; // 返回默认方向，外部会处理为护盾
    }
  
    // 评估每个合法移动
    double maxF = -4000;
    Direction best = legalMoves[0];
    const auto &head = state.get_self().get_head();
  
    for (auto dir : legalMoves) {
        const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
        double eval = Strategy::eval(state, y, x, head.y, head.x);
      
        if (eval > maxF) {
            maxF = eval;
            best = dir;
        }
    }
  
    return (maxF == -4000) ? Direction::UP : best;
}

// 食物优先级处理辅助函数
bool processFoodByPriority(const GameState &state, int minValue, int maxRange, Direction& moveDirection) {
    const auto &head = state.get_self().get_head();
    
    for (const auto &item : state.items) {
        // 根据价值过滤
        if (item.value < minValue || item.value == -2) {
            continue;  // 跳过低价值食物和陷阱
        }
        
        // 计算距离
            int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
        if (dist > maxRange) {
            continue;  // 跳过超出范围的食物
        }
        
        // 使用通用的目标可达性检测
        if (!Utils::isTargetReachable(state, item.pos, item.lifetime)) {
            continue; // 跳过不可达的食物
        }
        
        // 计算前往食物的首选方向
        vector<Direction> preferred_dirs;
        
        // 水平方向
        if (head.x > item.pos.x) preferred_dirs.push_back(Direction::LEFT);
        else if (head.x < item.pos.x) preferred_dirs.push_back(Direction::RIGHT);
        
        // 垂直方向
        if (head.y > item.pos.y) preferred_dirs.push_back(Direction::UP);
        else if (head.y < item.pos.y) preferred_dirs.push_back(Direction::DOWN);
        
        // 使用方向选择器
        Direction dir = chooseBestDirection(state, preferred_dirs);
        if (dir != Direction::UP || state.get_self().direction == 1) {
            moveDirection = dir;
            return true;
        }
    }
    
    return false;  // 没有找到满足条件的食物
}

int judge(const GameState &state)
{
    // 更新目标锁定状态
    lock_on_target(state);
    
    // 处理宝箱和钥匙的目标锁定
    if ((state.get_self().has_key && is_chest_target && current_target.x != -1 && current_target.y != -1) ||
        (!state.get_self().has_key && is_key_target && current_target.x != -1 && current_target.y != -1)) {
        
        // 计算前往目标的首选方向
        vector<Direction> preferred_dirs;
        const auto &head = state.get_self().get_head();
        
        // 水平方向
        if (head.x > current_target.x) preferred_dirs.push_back(Direction::LEFT);
        else if (head.x < current_target.x) preferred_dirs.push_back(Direction::RIGHT);
        
        // 垂直方向
        if (head.y > current_target.y) preferred_dirs.push_back(Direction::UP);
        else if (head.y < current_target.y) preferred_dirs.push_back(Direction::DOWN);
        
        Direction best_dir = chooseBestDirection(state, preferred_dirs);
        if (best_dir != Direction::UP || state.get_self().direction == 1) {
            return Utils::dir2num(best_dir);
        }
    }
    
    // 即时反应 - 处理近距离高价值目标
    const auto &head = state.get_self().get_head();

    // 距离分层常量定义
    const int VERY_CLOSE_RANGE = 4;  // 增加普通食物即时反应范围
    const int CLOSE_RANGE = 6;       // 近距离
    const int EXTENDED_RANGE = 10;   // 扩展检测范围
    
    // 使用辅助函数处理不同优先级的食物
            Direction move_dir;
            
    // 第一优先级：极近距离高价值尸体 (<=4格，价值>=5)
    if (processFoodByPriority(state, 5, VERY_CLOSE_RANGE, move_dir)) {
                return Utils::dir2num(move_dir);
            }
    
    // 特殊情况：极近距离的普通食物 (<=2格，任意价值>0)
    for (const auto &item : state.items) {
        // 只考虑普通食物
        if (item.value > 0 && item.value < 5) {
            int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
            if (dist <= 2) { // 非常近的食物
                // 使用通用的目标可达性检测
                if (!Utils::isTargetReachable(state, item.pos, item.lifetime)) {
                    continue; // 跳过不可达的食物
                }
                
                // 如果可达且安全，计算首选方向
                vector<Direction> preferred_dirs;
                
                // 水平方向
                if (head.x > item.pos.x) preferred_dirs.push_back(Direction::LEFT);
                else if (head.x < item.pos.x) preferred_dirs.push_back(Direction::RIGHT);
                
                // 垂直方向
                if (head.y > item.pos.y) preferred_dirs.push_back(Direction::UP);
                else if (head.y < item.pos.y) preferred_dirs.push_back(Direction::DOWN);
                
                Direction dir = chooseBestDirection(state, preferred_dirs);
                if (dir != Direction::UP || state.get_self().direction == 1) {
                    return Utils::dir2num(dir);
                }
            }
        }
    }
    
    // 第二优先级：近距离更高价值尸体 (<=6格，价值>=8)
    if (processFoodByPriority(state, 8, CLOSE_RANGE, move_dir)) {
        return Utils::dir2num(move_dir);
    }
    
    // 第三优先级：扩展距离极高价值尸体 (<=10格，价值>=10)
    if (processFoodByPriority(state, 10, EXTENDED_RANGE, move_dir)) {
        return Utils::dir2num(move_dir);
    }
    
    // 普通近距离食物检测
    if (processFoodByPriority(state, 0, VERY_CLOSE_RANGE, move_dir)) {
        return Utils::dir2num(move_dir);
    }
    
    // 默认行为 - 评估所有方向并选择最佳
    Direction best_dir = chooseBestDirection(state);
    return (best_dir == Direction::UP && state.get_self().direction != 1) ? 
           SHIELD_COMMAND : Utils::dir2num(best_dir);
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