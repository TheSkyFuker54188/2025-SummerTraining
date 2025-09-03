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
constexpr int MAXN = 40; constexpr int MAXM = 30;
constexpr int MAX_TICKS = 256; constexpr int MYID = 2023202295; // 此处替换为你的学号！
// 空位置标识（如果需要）
// constexpr int EmptyIdx = -1;
constexpr int SHIELD_COMMAND = 4; // 护盾指令值

enum class Direction { LEFT, UP, RIGHT, DOWN };

const vector<Direction> validDirections{Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT};

// 游戏地图状态: map_item用于物品, map_snake用于蛇的位置
int map_item[MAXM][MAXN], map_snake[MAXM][MAXN];

// 将类型定义放到最前面
struct Point { 
    int y, x;

    bool operator<(const Point& other) const {
        return y < other.y || (y == other.y && x < other.x);
    }
    
    bool operator==(const Point& other) const {
        return y == other.y && x == other.x;
    }
};

struct Item { Point pos; int value; int lifetime; };

struct Snake {
  int id; int length; int score; int direction;
  int shield_cd; int shield_time; bool has_key;
  vector<Point> body;

  const Point &get_head() const { return body.front(); }
};

struct Chest { Point pos; int score; };

struct Key { Point pos; int holder_id; int remaining_time; };

struct SafeZoneBounds { int x_min, y_min, x_max, y_max; };

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
    // 前向声明所有需要的函数
    int cornerEval(int y, int x, int fy, int fx);
    double safeZoneCenterScore(const GameState &state, int y, int x);
    double bfs(int sy, int sx, int fy, int fx, const GameState &state);

    struct TargetEvaluation { Point position; double value; int distance; bool is_safe; };
    TargetEvaluation evaluateTarget(const GameState &state, const Point &target_pos, double base_value, bool is_chest);
    struct DeadEndAnalysis; struct TerrainAnalysis;
    
    // 添加这些函数的声明
    int countSafeExits(const GameState &state, const Point &pos);
    Point findBestNextStep(const GameState &state, const Point &pos);
    vector<vector<int>> makeMapSnapshot(const GameState &state);
    double simulateMovementRisk(const GameState &state, int start_y, int start_x, int steps);
    
    // 新增路径安全评估框架相关声明
    struct PathSafetyEvaluation {
        double safety_score;    // 综合安全评分
        bool is_safe;           // 是否安全
        int safe_steps;         // 安全步数
        vector<pair<int, int>> risky_points; // 危险点位置
        
        // 默认构造为不安全路径
        PathSafetyEvaluation() : safety_score(-1000), is_safe(false), safe_steps(0) {}
    };
    
    PathSafetyEvaluation evaluatePathSafety(const GameState &state, const Point &start, const Point &target, int look_ahead = 8);
    vector<Point> findPath(const GameState &state, const Point &start, const Point &target);
    double checkPointSafeZoneRisk(const GameState &state, const Point &point, int step_idx);
    pair<double, bool> checkTerrainRisk(const GameState &state, const Point &point);
    double checkSnakeDensityRisk(const GameState &state, const Point &point, int step_idx);
    double checkSpecialTerrainRisk(const GameState &state, const Point &point);
    
    // 前向声明
    double evaluateSafeZoneDensity(const GameState &state, int x, int y);
    double evaluateStrategicPosition(const GameState &state, int y, int x);
    TargetEvaluation evaluateChestKeyTarget(const GameState &state, const Point &target_pos, double base_value, bool is_chest);
    
    // 前向声明evalWithAdaptiveConfig函数
    double evalWithAdaptiveConfig(const GameState &state, int y, int x, int fy, int fx);
    
    // 添加风险适应性配置结构体，用于游戏不同阶段的风险调整
    struct RiskAdaptiveConfig {
        double terrain_risk_weight;       // 地形风险权重
        double snake_density_risk_weight; // 蛇密度风险权重
        double food_value_weight;         // 食物价值权重
        double path_safety_threshold;     // 路径安全阈值
        double shield_activation_threshold; // 护盾激活阈值
        
        // 默认构造函数 - 均衡配置
        RiskAdaptiveConfig() 
            : terrain_risk_weight(1.0), 
              snake_density_risk_weight(1.0),
              food_value_weight(1.0),
              path_safety_threshold(-1200),
              shield_activation_threshold(-800) {}
    };
    
    // 根据游戏阶段和蛇状态获取适应性配置
    RiskAdaptiveConfig getAdaptiveConfig(const GameState &state) {
        RiskAdaptiveConfig config;
        const auto &self = state.get_self();
        int current_tick = MAX_TICKS - state.remaining_ticks;
        
        // 游戏阶段划分
        bool is_early_game = current_tick < 80;
        bool is_mid_game = current_tick >= 80 && current_tick < 180;
        bool is_late_game = current_tick >= 180;
        
        // 安全区状态
        int ticks_to_shrink = state.next_shrink_tick - current_tick;
        bool is_near_shrink = ticks_to_shrink >= 0 && ticks_to_shrink <= 20;
        
        // 蛇状态
        bool is_high_score = self.score >= 100;
        bool is_mid_score = self.score >= 50 && self.score < 100;
        bool has_shield = self.shield_time > 0;
        bool can_use_shield = self.shield_cd == 0 && self.score >= 20;
        
        // 游戏早期：更激进地追求食物，风险评估较宽松
        if (is_early_game) {
            config.terrain_risk_weight = 0.8;       // 降低地形风险权重
            config.snake_density_risk_weight = 0.9; // 稍微降低蛇密度风险
            config.food_value_weight = 1.3;         // 提高食物价值
            config.path_safety_threshold = -1400;   // 接受更高风险的路径
            config.shield_activation_threshold = -1000; // 更高的护盾激活阈值
        }
        // 游戏中期：逐渐重视安全性
        else if (is_mid_game) {
            if (is_high_score) { // 高分时更保守
                config.terrain_risk_weight = 1.2;
                config.snake_density_risk_weight = 1.2;
                config.food_value_weight = 0.9;
                config.path_safety_threshold = -1000;
                config.shield_activation_threshold = -700;
            } else {
                // 默认配置适用
            }
            
            // 安全区即将收缩时，调整策略
            if (is_near_shrink) {
                config.terrain_risk_weight *= 0.9;    // 降低地形风险权重
                config.snake_density_risk_weight *= 1.3; // 更重视蛇密度风险
                config.path_safety_threshold -= 200;  // 接受更高风险的路径
            }
        }
        // 游戏后期：非常重视安全性
        else if (is_late_game) {
            config.terrain_risk_weight = 1.4;         // 提高地形风险权重
            config.snake_density_risk_weight = 1.5;   // 更重视蛇密度
            config.food_value_weight = 0.8;           // 降低食物价值权重
            config.path_safety_threshold = -900;      // 更严格的安全路径标准
            config.shield_activation_threshold = -600; // 更容易激活护盾
            
            // 安全区即将收缩时，特别调整
            if (is_near_shrink) {
                config.terrain_risk_weight *= 0.8;     // 降低地形风险权重
                config.snake_density_risk_weight *= 1.5; // 大幅增加蛇密度风险权重
                config.path_safety_threshold -= 300;   // 接受更高风险的路径
                config.shield_activation_threshold -= 200; // 更容易激活护盾
            }
        }
        
        // 根据蛇状态进行额外调整
        if (has_shield || can_use_shield) {
            // 有护盾或可以使用护盾时更激进
            config.terrain_risk_weight *= 0.9;
            config.snake_density_risk_weight *= 0.9;
            config.food_value_weight *= 1.1;
            config.path_safety_threshold -= 200;
        }
        
        if (is_high_score) {
            // 高分时整体更保守
            config.terrain_risk_weight *= 1.1;
            config.snake_density_risk_weight *= 1.1;
            config.food_value_weight *= 0.9;
            config.path_safety_threshold += 100;
        } else if (!is_mid_score && !is_early_game) {
            // 低分且不在早期，更积极寻找食物
            config.food_value_weight *= 1.2;
            config.path_safety_threshold -= 100;
        }
        
        return config;
    }
    
    // 增强版评估函数，使用自适应配置
    double evalWithAdaptiveConfig(const GameState &state, int y, int x, int fy, int fx) {
        // 获取自适应配置
        RiskAdaptiveConfig config = getAdaptiveConfig(state);
        
        // 基础评估
        int test = cornerEval(y, x, fy, fx);
        if (test != -10000) { // 是拐角位置
            if (test == -5000) return -50000; // 危险拐角，强烈避免
            if (test == -50) map_item[y][x] = -9; // 标记陷阱拐角
        }
        
        // 安全区评分 - 考虑边界和蛇密度
        double safe_zone_score = safeZoneCenterScore(state, y, x);
        
        // 战略位置评估 - 中后期更关注终点安全区
        double strategic_score = evaluateStrategicPosition(state, y, x);
        
        // BFS评估
        double bfs_score = bfs(y, x, fy, fx, state);
        
        // 根据配置调整各部分权重
        safe_zone_score *= config.snake_density_risk_weight;
        strategic_score *= config.terrain_risk_weight;
        
        // 食物价值单独调整
        double food_value = 0;
        if (map_item[y][x] > 0 || map_item[y][x] == -1) { // 如果是食物或增长豆
            food_value = map_item[y][x] * 50 * config.food_value_weight; // 提取并调整食物价值
        }
        
        // 整合评分
        return bfs_score + safe_zone_score + strategic_score + food_value;
    }

    // 严格限制护盾使用条件，只在必死无疑的情况下使用
    bool shouldUseShield(const GameState &state, double path_safety_score, double target_value, 
                         bool is_emergency = false, bool is_survival = false) {
        const auto &self = state.get_self();
        
        // 检查基本条件
        if (self.shield_cd != 0 || self.score < 20) {
            return false; // 无法使用护盾
        }
        
        // 最后5tick内不使用护盾
        if (state.remaining_ticks <= 5) {
                return false;
        }
        
        // 1. 仅在生死攸关情形使用
        if (is_survival) {
            return true; // 生死存亡时才使用
        }
        
        // 2. 安全区收缩导致的绝境
        if (is_emergency) {
            int current_tick = MAX_TICKS - state.remaining_ticks;
            int ticks_to_shrink = state.next_shrink_tick - current_tick;
            
            // 计算到安全区的距离
            const auto &head = self.get_head();
            int dist_to_safe_zone = INT_MAX;
            
            if (head.x < state.next_safe_zone.x_min) {
                dist_to_safe_zone = min(dist_to_safe_zone, state.next_safe_zone.x_min - head.x);
            } else if (head.x > state.next_safe_zone.x_max) {
                dist_to_safe_zone = min(dist_to_safe_zone, head.x - state.next_safe_zone.x_max);
            }
            
            if (head.y < state.next_safe_zone.y_min) {
                dist_to_safe_zone = min(dist_to_safe_zone, state.next_safe_zone.y_min - head.y);
            } else if (head.y > state.next_safe_zone.y_max) {
                dist_to_safe_zone = min(dist_to_safe_zone, head.y - state.next_safe_zone.y_max);
            }
            
            // 仅当肯定无法安全到达安全区时才使用护盾
            if (dist_to_safe_zone <= ticks_to_shrink - 1) {
                return false; // 能安全到达，不使用护盾
            }
            
            // 如果无法到达且即将收缩，认为是绝境
            if (ticks_to_shrink <= 2) {
                return true;
            }
        }
        
        // 3. 其他情况不使用护盾，哪怕是高价值目标
        return false;
    }
}

namespace Utils
{
    // 检查是否越界
    bool boundCheck(int y, int x) { return (y >= 0) && (y < MAXM) && (x >= 0) && (x < MAXN); }

    // 获取下一个位置，在 dir 方向上
    pair<int, int> nextPos(pair<int, int> pos, Direction dir)
    {
        switch (dir)
        {
        case Direction::LEFT: return make_pair(pos.first, pos.second - 1);
        case Direction::RIGHT: return make_pair(pos.first, pos.second + 1);
        case Direction::UP: return make_pair(pos.first - 1, pos.second);
        default: return make_pair(pos.first + 1, pos.second);
        }
    }

    // 将方向转换为数字
    int dir2num(const Direction dir)
    {
        switch (dir)
        {
        case Direction::LEFT: return 0;
        case Direction::UP: return 1;
        case Direction::RIGHT: return 2;
        default: return 3;
        }
    }

    // 判断食物是否可以及时到达 - 直接使用坐标和生命周期
    bool canReachFoodInTime(int head_y, int head_x, int food_y, int food_x, int lifetime) 
    {
        // 计算曼哈顿距离
        int dist = abs(head_y - food_y) + abs(head_x - food_x);
        
        // 考虑转弯限制，估计实际所需时间
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
        if (willDisappearInShrink(state, target.x, target.y)) return false;
      
        // 检查能否及时到达
        if (lifetime < INT_MAX) return canReachFoodInTime(head.y, head.x, target.y, target.x, lifetime);
      
        return true;
    }

    // 将坐标转化为string (用于拐角检测)
    string idx2str(const pair<int, int> idx)
    {
        string a = to_string(idx.first);
        if (a.length() == 1) a = "0" + a;
        string b = to_string(idx.second);
        if (b.length() == 1) b = "0" + b;
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

// 这行已移动到文件顶部，此处删除
// 游戏地图状态现在在文件顶部声明

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
        
        // 检查钥匙剩余时间
        int key_remaining_time = INT_MAX;
        for (const auto &key : state.keys) {
            if (key.holder_id == MYID) {
                key_remaining_time = key.remaining_time;
                break;
            }
        }
        
        // 计算当前游戏阶段
        int current_tick = MAX_TICKS - state.remaining_ticks;
        int ticks_to_shrink = state.next_shrink_tick - current_tick;
        bool is_near_shrink = (ticks_to_shrink >= 0 && ticks_to_shrink <= 20);
      
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
                // 使用专门的宝箱评估函数
                auto eval = Strategy::evaluateChestKeyTarget(state, chest.pos, chest.score, true);
                
                // 钥匙剩余时间是否足够到达宝箱的考虑
                bool has_enough_time = (key_remaining_time >= eval.distance + 5);
                
                // 如果钥匙时间不够，降低宝箱价值
                if (!has_enough_time) {
                    eval.value *= (double)key_remaining_time / (eval.distance + 5);
                    eval.is_safe = false; // 时间不够就不安全
                }
                
                // 优先选择安全的宝箱
                if (eval.is_safe) {
                    if (best_chest.position.x == -1 || !best_chest.is_safe || eval.value > best_chest.value) {
                    best_chest = eval;
                    }
                } 
                // 安全区收缩前的紧急决策
                else if (is_near_shrink) {
                    // 判断宝箱是否在下一个安全区内
                    bool chest_in_next_zone = (chest.pos.x >= state.next_safe_zone.x_min && 
                                             chest.pos.x <= state.next_safe_zone.x_max &&
                                             chest.pos.y >= state.next_safe_zone.y_min && 
                                             chest.pos.y <= state.next_safe_zone.y_max);
                    
                    // 如果宝箱在下一个安全区，提高价值
                    if (chest_in_next_zone) {
                        eval.value *= 1.5;
                    }
                    // 否则大幅降低价值
                    else {
                        eval.value *= 0.3;
                    }
                    
                    // 收缩前紧急获取宝箱
                    if (eval.value > best_chest.value || (chest_in_next_zone && !best_chest.is_safe)) {
                    best_chest = eval;
                    }
                }
                // 次优先：不安全但高价值的宝箱
                else if (!best_chest.is_safe && eval.value > best_chest.value) {
                    // 获取完整的路径安全评估
                    auto path_safety = Strategy::evaluatePathSafety(state, head, chest.pos, 15);
                    
                    // 安全评估更全面，考虑路径风险和预期收益
                    double risk_reward_ratio = eval.value / (1.0 - min(1.0, -path_safety.safety_score / 2000.0));
                    
                    // 如果风险收益比较高，或者路径风险可控
                    if (path_safety.safety_score > -1000 || risk_reward_ratio > best_chest.value * 1.2) {
                        best_chest = eval;
                    }
                }
            }
            
            // 如果找到了合适的宝箱
            if (best_chest.position.x != -1) {
                current_target = best_chest.position;
                target_value = best_chest.value;
                
                // 根据路径安全性和紧急程度调整锁定时间
                int base_lock_time = min(best_chest.distance + 5, 25);
                
                // 获取完整的路径安全评估
                auto path_safety = Strategy::evaluatePathSafety(state, head, best_chest.position);
                
                // 如果路径安全且钥匙时间充足
                if (best_chest.is_safe && key_remaining_time > best_chest.distance + 5) {
                    target_lock_time = base_lock_time; // 标准锁定时间
                } 
                // 钥匙即将消失
                else if (key_remaining_time < best_chest.distance + 3) {
                    target_lock_time = max(1, key_remaining_time - 2); // 紧急情况
                }
                // 路径不安全
                else if (path_safety.safety_score < -800) {
                    target_lock_time = min(base_lock_time, 12); // 高风险路径，缩短锁定时间
                } else {
                    target_lock_time = min(base_lock_time, 18); // 中等风险路径
                }
                
                is_chest_target = true;
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
        
        // 计算当前游戏阶段信息
        int current_tick = MAX_TICKS - state.remaining_ticks;
        int ticks_to_shrink = state.next_shrink_tick - current_tick;
        bool is_near_shrink = (ticks_to_shrink >= 0 && ticks_to_shrink <= 20);
        bool is_late_game = (state.remaining_ticks < 100);
      
        // 如果没有锁定钥匙或锁定时间结束，寻找新的钥匙
        if (!is_key_target || target_lock_time <= 0 || current_target.x == -1 || current_target.y == -1) {
            // 评估所有钥匙
            Strategy::TargetEvaluation best_key = {
                {-1, -1}, // position
                -1,       // value
                INT_MAX,  // distance
                false     // is_safe
            };
            
            // 如果有宝箱，先收集宝箱信息
            vector<Chest> accessible_chests;
            Point best_chest_pos = {-1, -1};
            int best_chest_value = 0;
            
            for (const auto &chest : state.chests) {
                // 检查宝箱是否在安全区内
                bool in_next_zone = is_near_shrink && 
                    chest.pos.x >= state.next_safe_zone.x_min && 
                    chest.pos.x <= state.next_safe_zone.x_max &&
                    chest.pos.y >= state.next_safe_zone.y_min && 
                    chest.pos.y <= state.next_safe_zone.y_max;
                
                // 检查路径安全性
                auto chest_path_safety = Strategy::evaluatePathSafety(state, head, chest.pos);
                
                // 如果宝箱在下一个安全区且路径相对安全，记录为可接近的宝箱
                if ((chest_path_safety.safety_score > -1200 || in_next_zone) && 
                    (chest.score > best_chest_value || best_chest_pos.x == -1)) {
                    accessible_chests.push_back(chest);
                    best_chest_pos = chest.pos;
                    best_chest_value = chest.score;
                }
            }
            
            for (const auto &key : state.keys) {
                // 只考虑地图上的钥匙
                if (key.holder_id == -1) {
                    // 使用专门的钥匙评估函数
                    auto eval = Strategy::evaluateChestKeyTarget(state, key.pos, 40.0, false);
                    
                    // 安全区收缩调整
                    if (is_near_shrink) {
                        bool in_next_zone = key.pos.x >= state.next_safe_zone.x_min && 
                                          key.pos.x <= state.next_safe_zone.x_max &&
                                          key.pos.y >= state.next_safe_zone.y_min && 
                                          key.pos.y <= state.next_safe_zone.y_max;
                        
                        // 如果钥匙不在下一个安全区，大幅降低价值
                        if (!in_next_zone) {
                            eval.value *= 0.2;
                            eval.is_safe = false;
                        }
                    }
                    
                    // 考虑宝箱与钥匙的关联性
                    if (!accessible_chests.empty()) {
                        // 计算钥匙到最佳宝箱的距离
                        int key_to_chest_dist = INT_MAX;
                        if (best_chest_pos.x != -1) {
                            key_to_chest_dist = abs(key.pos.y - best_chest_pos.y) + 
                                               abs(key.pos.x - best_chest_pos.x);
                                               
                            // 如果钥匙离宝箱较近，提高价值
                            if (key_to_chest_dist < 15) {
                                eval.value *= 1.0 + (15 - key_to_chest_dist) * 0.05; // 最多提高75%
                            }
                            
                            // 评估从钥匙到宝箱的路径安全性
                            auto key_to_chest_safety = Strategy::evaluatePathSafety(
                                state, key.pos, best_chest_pos);
                                
                            // 如果钥匙到宝箱的路径安全，进一步提高价值
                            if (key_to_chest_safety.is_safe) {
                                eval.value *= 1.2;
                            } 
                            // 如果路径非常不安全，降低价值
                            else if (key_to_chest_safety.safety_score < -1500) {
                                eval.value *= 0.6;
                                eval.is_safe = false;
                            }
                        }
                    }
                    
                    // 后期游戏更看重钥匙
                    if (is_late_game) {
                        eval.value *= 1.3;
                    }
                    
                    // 优先考虑安全的钥匙
                    if (eval.is_safe) {
                        if (best_key.position.x == -1 || !best_key.is_safe || eval.value > best_key.value) {
                        best_key = eval;
                        }
                    } 
                    // 次优先：安全区收缩时更紧急地寻找钥匙
                    else if (is_near_shrink && ticks_to_shrink <= 10) {
                        // 即将收缩，更愿意接受风险
                        auto path_safety = Strategy::evaluatePathSafety(state, head, key.pos);
                        
                        // 如果风险可控，考虑这个钥匙
                        if (path_safety.safety_score > -1500 || 
                            (eval.distance < best_key.distance * 0.7 && eval.value > best_key.value * 0.8)) {
                        best_key = eval;
                        }
                    }
                    // 次优先：不安全但更接近且价值高的钥匙
                    else if (!best_key.is_safe && 
                            (eval.value > best_key.value * 1.2 || 
                             (eval.value >= best_key.value * 0.9 && eval.distance < best_key.distance * 0.7))) {
                        
                        // 使用高级路径安全评估
                        auto path_safety = Strategy::evaluatePathSafety(state, head, key.pos, 12);
                        
                        // 风险收益比分析
                        double risk_reward_ratio = eval.value / (1.0 - min(1.0, -path_safety.safety_score / 2000.0));
                        
                        // 如果风险收益比高或者路径风险可控
                        if (path_safety.safety_score > -900 || 
                            risk_reward_ratio > best_key.value * 1.1 ||
                            (best_key.position.x != -1 && 
                             path_safety.safety_score > Strategy::evaluatePathSafety(
                                 state, head, best_key.position).safety_score * 1.2)) {
                            best_key = eval;
                        }
                    }
                }
            }
            
            // 如果找到了合适的钥匙
            if (best_key.position.x != -1) {
                current_target = best_key.position;
                target_value = best_key.value;
                
                // 路径安全分析
                auto path_safety = Strategy::evaluatePathSafety(state, head, best_key.position, 10);
                
                // 根据路径安全性和紧急程度调整锁定时间
                int base_lock_time;
                
                // 安全区收缩前的紧急情况
                if (is_near_shrink && ticks_to_shrink <= 10) {
                    base_lock_time = min(best_key.distance + 3, 15);
                } 
                // 普通情况
                else {
                    base_lock_time = min(best_key.distance + 5, 20);
                }
                
                // 安全路径
                if (best_key.is_safe) {
                    target_lock_time = base_lock_time;
                } 
                // 不安全路径
                else {
                    target_lock_time = min(base_lock_time, 
                        (path_safety.safety_score < -900) ? 8 : 12); 
                }
                
                is_key_target = true;
                
                // 检查是否有宝箱，如果有，预留时间从钥匙到宝箱
                if (!accessible_chests.empty() && best_chest_pos.x != -1) {
                    // 计算从钥匙到宝箱所需的时间
                    int key_to_chest_dist = abs(best_key.position.y - best_chest_pos.y) + 
                                          abs(best_key.position.x - best_chest_pos.x);
                    
                    // 根据路径安全性确定是否预留额外时间
                    auto key_to_chest_safety = Strategy::evaluatePathSafety(
                        state, best_key.position, best_chest_pos);
                    
                    // 路径安全才预留时间
                    if (key_to_chest_safety.is_safe || key_to_chest_safety.safety_score > -800) {
                        // 考虑收缩时间的影响
                        if (is_near_shrink) {
                            target_lock_time = min(ticks_to_shrink - 2, 
                                                best_key.distance + key_to_chest_dist + 3);
                        } else {
                            target_lock_time = min(25, best_key.distance + key_to_chest_dist + 5);
                        }
                    }
                }
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
                    current_target = key.pos; // 更新为精确位置
                    break;
                }
            }
            
            if (!key_exists) { // 钥匙已被拿走或消失
                current_target = {-1, -1}; target_value = 0; is_key_target = false;
            }
        }
    }
    // 如果没有钥匙和宝箱相关目标，重置目标锁定
    else if (is_key_target || is_chest_target) {
        current_target = {-1, -1}; target_value = 0; 
        is_key_target = false; is_chest_target = false;
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
            current_target = {-1, -1}; target_value = 0;
        }
    }
}

void read_game_state(GameState &s) {
  cin >> s.remaining_ticks;

  // 初始化地图状态
  memset(map_item, 0, sizeof(map_item)); memset(map_snake, 0, sizeof(map_snake));

  // 读取物品信息
  int item_count;
  cin >> item_count;
  s.items.resize(item_count);
  for (int i = 0; i < item_count; ++i) {
    cin >> s.items[i].pos.y >> s.items[i].pos.x >>
        s.items[i].value >> s.items[i].lifetime;
    
    // 更新map_item地图
    if (Utils::boundCheck(s.items[i].pos.y, s.items[i].pos.x)) {
      map_item[s.items[i].pos.y][s.items[i].pos.x] = s.items[i].value;
    }
  }

  // 读取蛇信息
  int snake_count;
  cin >> snake_count;
  s.snakes.resize(snake_count);
  unordered_map<int, int> id2idx; id2idx.reserve(snake_count * 2);

  for (int i = 0; i < snake_count; ++i) {
    auto &sn = s.snakes[i];
    cin >> sn.id >> sn.length >> sn.score >> sn.direction >> sn.shield_cd >>
        sn.shield_time;
    sn.body.resize(sn.length);
    for (int j = 0; j < sn.length; ++j) {
      cin >> sn.body[j].y >> sn.body[j].x;
      
      // 更新map_snake地图 - 蛇的身体部分
      if (Utils::boundCheck(sn.body[j].y, sn.body[j].x)) {
        map_snake[sn.body[j].y][sn.body[j].x] = sn.id == MYID ? 10 : -5;
      }
    }
    
    // 标记蛇头周围的危险区域
    if (sn.id != MYID) {
      auto &head = sn.body[0];
      for (auto dir : validDirections) {
        const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
        if (Utils::boundCheck(y, x) && map_snake[y][x] != -5) {
          map_snake[y][x] = -6; // 蛇头可能移动区域
        }
      }
      
      // 蛇尾部可能移动区域
      auto &tail = sn.body[sn.length - 1];
      for (auto dir : validDirections) {
        const auto [y, x] = Utils::nextPos({tail.y, tail.x}, dir);
        if (Utils::boundCheck(y, x) && map_snake[y][x] != -5 && map_snake[y][x] != -6) {
          map_snake[y][x] = -7; // 尾部可能移动区域
        }
      }
    }
    
    if (sn.id == MYID) s.self_idx = i;
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
      if (it != id2idx.end()) s.snakes[it->second].has_key = true;
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

// 基本非法移动检查，返回不能移动的方向
unordered_set<Direction> basicIllegalCheck(const GameState &state) {
    unordered_set<Direction> illegals;
    const Snake &self = state.get_self();
    const Point &head = self.get_head();
    
    // 确定反方向（不能往回走）
    switch (self.direction) {
    case 3: illegals.insert(Direction::UP); break; // DOWN
    case 1: illegals.insert(Direction::DOWN); break; // UP
    case 0: illegals.insert(Direction::RIGHT); break; // LEFT
    default: illegals.insert(Direction::LEFT); break; // RIGHT
    }
    
    // 检查四个方向
    for (auto dir : validDirections) {
        if (illegals.count(dir) > 0) continue; // 跳过已标记非法方向
        
        const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
        
        // 检查是否越界
        if (!Utils::boundCheck(y, x)) { illegals.insert(dir); continue; }
        
        // 安全区检查
        if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max ||
            y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max) {
            // 只有当护盾时间足够时才允许离开安全区
            if (self.shield_time <= 1) { illegals.insert(dir); continue; }
        }
        
        // 墙检查 - 任何情况下都不能穿墙
        if (map_item[y][x] == -4) { illegals.insert(dir); continue; }
        
        // 陷阱检查 - 常规情况下不去陷阱
        if (map_item[y][x] == -2) { illegals.insert(dir); continue; }
        
        // 蛇身检查 - 常规情况下不能穿过蛇身
        if (map_snake[y][x] == -5 && self.shield_time < 2) { illegals.insert(dir); continue; }
        
        // 危险区域检查 - 蛇头/尾可能移动区域
        if ((map_snake[y][x] == -6 || map_snake[y][x] == -7) && self.shield_time < 2) { 
            illegals.insert(dir); continue; 
        }
    }
    
    return illegals;
}

// 紧急情况下的非法移动检查，放宽条件
unordered_set<Direction> emergencyIllegalCheck(const GameState &state) {
    unordered_set<Direction> illegals;
    const Snake &self = state.get_self();
    const Point &head = self.get_head();
    
    // 确定反方向（不能往回走）
    switch (self.direction) {
    case 3: illegals.insert(Direction::UP); break; // DOWN
    case 1: illegals.insert(Direction::DOWN); break; // UP
    case 0: illegals.insert(Direction::RIGHT); break; // LEFT
    default: illegals.insert(Direction::LEFT); break; // RIGHT
    }
    
    // 放宽条件检查四个方向
    for (auto dir : validDirections) {
        if (illegals.count(dir) > 0) continue; // 跳过已标记为非法的方向
        
        const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
        
        // 检查是否越界 - 紧急情况下仍不能越界
        if (!Utils::boundCheck(y, x)) { illegals.insert(dir); continue; }
        
        // 安全区检查 - 紧急情况下仍要求安全区内
        if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max || 
            y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max) {
            // 只有当护盾时间足够时才允许离开安全区
            if (self.shield_time <= 1) { illegals.insert(dir); continue; }
        }
        
        // 墙检查 - 紧急情况下仍不能穿墙
        if (map_item[y][x] == -4) { illegals.insert(dir); continue; }
        
        // 蛇身检查 - 紧急情况下放宽条件
        if (map_snake[y][x] == -5) {
            // 如果有足够护盾或能开护盾，可以考虑穿过
            if (!(self.shield_time >= 2 || (self.shield_cd == 0 && self.score > 20) ||
                state.remaining_ticks >= 255 - 9 + 2)) { 
                illegals.insert(dir); continue; 
            }
        }
        
        // 紧急情况下不再检查陷阱和危险区域
    }
    
    return illegals;
}

// 添加默认参数值为3
unordered_set<Direction> illegalMove(const GameState &state, int look_ahead = 3)
{
    // 先执行基本检查
    unordered_set<Direction> illegals = basicIllegalCheck(state);
    const Snake &self = state.get_self();
    
    // 如果基本检查已将所有方向标记为非法，启用紧急模式
    if (illegals.size() == 4) return emergencyIllegalCheck(state);
    
    // 前瞻性评估剩余合法方向
    const Point &head = self.get_head();
    for (auto dir : validDirections) {
        if (illegals.count(dir) > 0) continue; // 跳过已标记为非法的方向
        
        const auto [ny, nx] = Utils::nextPos({head.y, head.x}, dir);
        
        // 模拟前进look_ahead步，评估是否会陷入险境
        // 直接使用向前声明的函数
        double future_risk = -500; // 默认风险值
        
        // 基本风险评估 - 通过计算安全出口的数量
        int safe_exits = 0;
        for (auto exit_dir : validDirections) {
            const auto [exit_y, exit_x] = Utils::nextPos({ny, nx}, exit_dir);
            // 检查是否是有效出口(在界内，不是墙，不是蛇身，在安全区)
            if (Utils::boundCheck(exit_y, exit_x) && 
                map_item[exit_y][exit_x] != -4 && map_snake[exit_y][exit_x] != -5 &&
                exit_x >= state.current_safe_zone.x_min && exit_x <= state.current_safe_zone.x_max && 
                exit_y >= state.current_safe_zone.y_min && exit_y <= state.current_safe_zone.y_max) {
                safe_exits++;
            }
        }
        
        // 出口越少，风险越高
        if (safe_exits == 0) future_risk = -2000;  // 死路
        else if (safe_exits == 1) future_risk = -1200;  // 只有一个出口，高风险
        else if (safe_exits == 2) future_risk = -800;   // 两个出口，中等风险
        else future_risk = -200;  // 多个出口，相对安全
        
        // 如果前瞻风险过高，标记为非法
        if (future_risk < -1000) illegals.insert(dir);
    }
    
    // 如果前瞻性检查将所有方向标记为非法，恢复到基本合法方向
    if (illegals.size() == 4) illegals = basicIllegalCheck(state);
    
    return illegals;
}

namespace Strategy
{
    // 前向声明evaluateTrap函数，确保在bfs函数之前可见
    double evaluateTrap(const GameState &state, int trap_y, int trap_x);
    
    // 添加新函数前向声明
    int countSafeExits(const GameState &state, const Point &pos);
    Point findBestNextStep(const GameState &state, const Point &pos);
    vector<vector<int>> makeMapSnapshot(const GameState &state);
    double simulateMovementRisk(const GameState &state, int start_y, int start_x, int steps);

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
                    if (dist <= max_distance) return true; // 发现敌方蛇在附近
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
                if (!Utils::boundCheck(ny, nx) || map_item[ny][nx] == -4 || map_snake[ny][nx] == -5) continue;
                
                string next = Utils::idx2str({ny, nx});
                if (visited.find(next) == visited.end()) { visited.insert(next); q.push({{ny, nx}, depth + 1}); }
            }
        }
        
        // 统计每个深度的宽度
        vector<int> width_at_depth(max_depth + 1, 0);
        for (int i = 0; i < MAXM; i++) {
            for (int j = 0; j < MAXN; j++) {
                if (reachable[i][j] >= 0) width_at_depth[reachable[i][j]]++;
            }
        }
        
        // 计算最大宽度和瓶颈宽度
        for (int d = 0; d <= max_depth; d++) {
            max_width = max(max_width, width_at_depth[d]);
            if (width_at_depth[d] > 0) bottleneck_width = min(bottleneck_width, width_at_depth[d]);
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
        for (const auto &snake : state.snakes) if (snake.id == MYID) { my_length = snake.length; break; }
            
            // 如果死胡同太窄不能调头，进一步增加风险
            if (my_length > max_width * 2) {
                result.risk_score -= 1000;
                
                // 极短死胡同且蛇较长时，给予极高惩罚
                if (max_depth <= 2 && my_length >= 8) result.risk_score = -1800; // 几乎必死
            }
        }
        
        // 检测瓶颈：有收缩点且收缩点宽度小
        if (bottleneck_width <= 2 && max_width > 3) {
            result.is_bottleneck = true;
            result.risk_score -= 300 * (4 - bottleneck_width);
            
            // 检查瓶颈附近是否有其他蛇
            if (checkEnemyNearArea(state, visited, 3)) result.risk_score -= 500;
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
        if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20 && 
            (x < state.next_safe_zone.x_min || x > state.next_safe_zone.x_max ||
             y < state.next_safe_zone.y_min || y > state.next_safe_zone.y_max)) {
            // 安全区外位置价值大幅降低，危险度随收缩时间临近增加
            if (ticks_to_shrink <= 5) score = -5000;       // 极度危险
            else if (ticks_to_shrink <= 10) score = -3000; // 很危险
            else score = -1000;                            // 有风险
        }
        
        return score;
    }
    
    // 检查食物是否能及时到达并计算价值
    double evaluateFoodValue(const GameState &state, int y, int x, int base_value, int distance) {
        double num = 0;
        
        // 根据食物类型和距离评估价值
        if (base_value > 0) { // 普通食物
            // 动态评估尸体价值 - 基于距离
            if (base_value >= 10) { // 极高价值的尸体
                if (distance <= 6) num = base_value * 200 + 200;      // 非常近
                else if (distance <= 10) num = base_value * 120 + 120; // 较近
                else num = base_value * 80 + 80;                      // 远距离
            } else if (base_value >= 5) { // 中等价值的尸体
                if (distance <= 4) num = base_value * 100 + 80; // 非常近
                else num = base_value * 70 + 40;               // 标准价值
            } else if (base_value > 0) { // 普通食物
                if (distance <= 3) num = base_value * 45;      // 非常近
                else if (distance <= 5) num = base_value * 35; // 较近
                else num = base_value * 30;                   // 远距离
            }
        } else if (base_value == -1) { // 增长豆
            // 根据游戏阶段动态调整增长豆价值
            if (state.remaining_ticks > 180) num = 30;      // 早期价值高
            else if (state.remaining_ticks > 100) num = 20; // 中期价值中等
            else num = 10;                                 // 后期价值低
        } else if (base_value == -2) { // 陷阱
            num = -0.5;
        }
        
        // 根据游戏阶段动态调整普通食物价值
        if (base_value > 0 && base_value < 5) { // 只处理普通食物
            if (state.remaining_ticks > 180) num *= 1.2;      // 早期提高20%
            else if (state.remaining_ticks < 60) num *= 1.3;  // 后期提高30%
        }
        
        // 基于安全区阶段的食物价值动态调整
        double safeZoneFactor = 1.0;
        int zone_current_tick = MAX_TICKS - state.remaining_ticks;
        int zone_ticks_to_shrink = state.next_shrink_tick - zone_current_tick;
        
        // 分析当前所处安全区阶段
        if (zone_current_tick < 80) {
            safeZoneFactor = 1.0; // 早期阶段 - 标准评估
        } else if (zone_current_tick < 160) { // 第一次收缩阶段
            if (zone_ticks_to_shrink >= 0 && zone_ticks_to_shrink <= 25) {
                if (x >= state.next_safe_zone.x_min && x <= state.next_safe_zone.x_max &&
                    y >= state.next_safe_zone.y_min && y <= state.next_safe_zone.y_max) {
                    safeZoneFactor = 1.3; // 提升30%价值
                }
            }
        } else if (zone_current_tick < 220) { // 第二次收缩阶段
            if (zone_ticks_to_shrink >= 0 && zone_ticks_to_shrink <= 25) {
                if (x >= state.next_safe_zone.x_min && x <= state.next_safe_zone.x_max &&
                    y >= state.next_safe_zone.y_min && y <= state.next_safe_zone.y_max) {
                    safeZoneFactor = 1.5; // 提升50%价值
                }
            }
        } else { // 最终收缩阶段
            if (zone_ticks_to_shrink >= 0 && zone_ticks_to_shrink <= 25) {
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
        
        for (const auto &snake : state.snakes) {
            if (snake.id == MYID) continue;
                
            const int current = abs(snake.get_head().y - y) + abs(snake.get_head().x - x);
            if (current < target) {
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
                    if (item.value > 0) { // 普通食物
                        total_value += item.value; count++;
                    } else if (item.value == -1) { // 增长豆
                        int growth_value = (state.remaining_ticks > 176) ? 3 : 2;
                        total_value += growth_value; count++;
                    }
                }
            }
        return (count > 0) ? (total_value / count) * count : 0;
    }

    // 安全区边界评分函数（考虑蛇密度和安全区收缩）
    double safeZoneCenterScore(const GameState &state, int y, int x)
    {
        double score = 0;
        
        // 检查位置是否在安全区内，并根据到边界的距离给予适当评分
        SafeZoneBounds zone = state.current_safe_zone;
        
        // 当安全区即将收缩时，使用下一个安全区
        int current_tick = MAX_TICKS - state.remaining_ticks;
        int ticks_to_shrink = state.next_shrink_tick - current_tick;
        if (ticks_to_shrink >= 0 && ticks_to_shrink <= 25) {
            zone = state.next_safe_zone;
            
            // 如果在收缩前位于下一个安全区内，加分
            if (x >= state.next_safe_zone.x_min && x <= state.next_safe_zone.x_max &&
                y >= state.next_safe_zone.y_min && y <= state.next_safe_zone.y_max) {
                score += 100; // 给予明显加分，鼓励提前进入下一个安全区
                
                // 收缩越近，加分越高
                if (ticks_to_shrink <= 5) score += 200;
                else if (ticks_to_shrink <= 10) score += 100;
            }
        }
        
        // 计算到安全区边界的最小距离
        int dist_to_border = min({
            x - zone.x_min,
            zone.x_max - x,
            y - zone.y_min,
            zone.y_max - y
        });
        
        // 如果距离边界太近(<=3格)，给予轻微惩罚
        if (dist_to_border <= 3) {
            score -= 5.0 * (3 - dist_to_border);
        }
        
        // 评估该位置的蛇密度
        int nearby_snakes = 0;
        int close_snakes = 0;
        const int NEARBY_RADIUS = 7;  // 较大范围
        const int CLOSE_RADIUS = 3;   // 较小范围
        
        for (const auto &snake : state.snakes) {
            if (snake.id == MYID || snake.id == -1) continue; // 跳过自己和无效蛇
            
            const Point &enemy_head = snake.get_head();
            int dist = abs(enemy_head.y - y) + abs(enemy_head.x - x);
            
            if (dist <= CLOSE_RADIUS) {
                close_snakes++;
            }
            
            if (dist <= NEARBY_RADIUS) {
                nearby_snakes++;
            }
        }
        
        // 蛇密度评分 - 蛇密度越高，惩罚越大
        double density_penalty = close_snakes * 100 + (nearby_snakes - close_snakes) * 30;
        
        // 如果有蛇在很近的位置，给予额外惩罚
        if (close_snakes > 0) {
            density_penalty *= 1.5;
        }
        
        score -= density_penalty;
        
        // 如果是即将收缩的安全区边缘，考虑额外风险
        if (ticks_to_shrink >= 0 && ticks_to_shrink <= 10 && dist_to_border <= 2) {
            score -= 300; // 安全区即将收缩且靠近边缘，额外惩罚
        }
        
        return score;
    }

    // BFS搜索评估函数
    double bfs(int sy, int sx, int fy, int fx, const GameState &state)
    {
        double score = 0;
        
        // 安全区收缩风险和蛇密度评估 - 使用新的安全区密度评估函数
        score += evaluateSafeZoneDensity(state, sx, sy);
        
        // 动态瓶颈区域检测（替换原有硬编码的特殊位置检测）
        bool is_bottleneck = false;
        int wall_count = 0;
        int danger_direction_count = 0;
        vector<pair<int, int>> exit_directions;
        
        // 检查周围有多少墙或障碍物
        for (auto dir : validDirections) {
            const auto [ny, nx] = Utils::nextPos({sy, sx}, dir);
            if (!Utils::boundCheck(ny, nx) || map_item[ny][nx] == -4) {
                wall_count++; // 墙或边界
            } else if (map_snake[ny][nx] == -5) {
                wall_count++; // 蛇身体部分，完全阻挡
            } else if (map_snake[ny][nx] == -6 || map_snake[ny][nx] == -7) {
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
        if (terrain_risk.risk_score <= -1800) return terrain_risk.risk_score;
        
        // 处理陷阱的情况
        if (map_item[sy][sx] == -2 && !terrain_risk.is_bottleneck) { // 只有当不是瓶颈时才考虑陷阱
            // 使用专门的陷阱评估函数
            score = evaluateTrap(state, sy, sx);
        } else if (map_item[sy][sx] == -2 && terrain_risk.is_bottleneck && 
                   map_snake[sy][sx] != -5 && map_snake[sy][sx] != -6 && map_snake[sy][sx] != -7) {
            // 拐角陷阱，但非紧急情况
            score = -500; // 非紧急情况下尽量避免
            
            // 检查是否处于紧急情况（四周都是障碍）
            bool is_emergency = true;
            const auto &head = state.get_self().get_head();
            for (auto dir : validDirections) {
                const auto [next_y, next_x] = Utils::nextPos({head.y, head.x}, dir);
                if (Utils::boundCheck(next_y, next_x)) {
                    if (map_item[next_y][next_x] != -4 && map_snake[next_y][next_x] != -5 &&
                        next_x >= state.current_safe_zone.x_min && next_x <= state.current_safe_zone.x_max && 
                        next_y >= state.current_safe_zone.y_min && next_y <= state.current_safe_zone.y_max) {
                        is_emergency = false;
                        break;
                    }
                }
            }
            
            if (is_emergency) score = -100; // 紧急情况下接受陷阱
            
            const double start = 150, end = 25, maxNum2 = 30;
            const int timeRest = state.remaining_ticks;
            
            if (start > timeRest && timeRest >= end) {
                const double r = maxNum2 * (timeRest - start) * (timeRest - start);
                score += r / ((start - end) * (start - end));
            }
            
            return score;
        } else {
            // 在第二次收缩前考虑食物密度
            if (state.remaining_ticks > 76) { // 1-160刻
                // 计算当前位置区域的食物密度
                double density_score = calculateFoodDensity(state, sy, sx, 5);
                
                // 根据游戏进程调整密度权重
                double density_weight = 0.0;
                if (state.remaining_ticks > 176) density_weight = 0.8; // 早期更看重食物密集区
                else density_weight = 0.5; // 中期适当关注
                
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
        
        while (!q.empty()) {
            auto [currentState, layer] = q.front();
            q.pop();
            
            // 设定视野搜索范围
            double maxLayer = 12;
            if (state.remaining_ticks < 50) maxLayer = 10; // 后期减小搜索范围
            
            if (layer >= maxLayer) break;
            
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
            int base_value = map_item[y][x];
            
            // 计算到蛇头的距离
                const auto &snake_head = state.get_self().get_head();
                int head_dist = abs(snake_head.y - y) + abs(snake_head.x - x);
                
            // 使用新的辅助函数评估食物价值
            double num = (map_item[y][x] != 0 && map_item[y][x] != -2 && !can_reach) ? 0 : 
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
            if (map_item[y][x] != -2) { // 不是陷阱
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
                            if (map_item[y][x] >= 10) { // 极高价值尸体
                                competition_factor *= (self_distance <= 6) ? 0.95f : 0.85f;
                            } else if (map_item[y][x] >= 5) { // 高价值尸体
                                competition_factor *= (self_distance <= 4) ? 0.90f : 0.80f;
                            } else {
                                competition_factor *= 0.8f; // 从0.7提高到0.8，减轻普通食物竞争惩罚
                            }
                        }
                        // 如果敌方蛇距离相近，轻微降低价值
                        else if (dist <= self_distance + 2) {
                            if (map_item[y][x] >= 8) { // 对高分尸体，竞争性调整
                                competition_factor *= (self_distance <= 6) ? 0.98f : 0.95f;
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
            for (auto dir : validDirections) {
                const auto [next_y, next_x] = Utils::nextPos(make_pair(y, x), dir);
                if (!Utils::boundCheck(next_y, next_x)) continue; // 越界
                
                // 检查障碍物
                if (map_item[next_y][next_x] == -4 || map_snake[next_y][next_x] == -5) {
                    // 墙或蛇身
                    if (layer == 1 && map_item[next_y][next_x] == -4) score -= 10; // 靠近墙的惩罚
                } else {
                    // 加入待访问队列
                    string neighbour = Utils::idx2str(make_pair(next_y, next_x));
                    if (visited.find(neighbour) == visited.end()) {
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
        if (!Utils::boundCheck(y, x)) return -10000;
      
        // 统计周围墙体和障碍物的分布
        int wall_count = 0; vector<pair<int, int>> adjacent_cells; vector<Direction> possible_exits;
      
        // 检查四个方向
        for (auto dir : validDirections) {
            const auto [ny, nx] = Utils::nextPos({y, x}, dir);
            if (!Utils::boundCheck(ny, nx) || map_item[ny][nx] == -4 || map_snake[ny][nx] == -5) {
                wall_count++; // 墙或蛇身
            } else {
                adjacent_cells.push_back({ny, nx});
                possible_exits.push_back(dir);
            }
        }
      
        // 拐角定义: 正好有两个墙，且两个墙相邻(形成直角)
        if (wall_count != 2 || adjacent_cells.size() != 2) return -10000; // 不是拐角
        
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
      
        if (!is_diagonal) return -10000; // 不是典型的L型拐角
      
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
        if (map_snake[next_y][next_x] == -5 || map_snake[next_y][next_x] == -6) {
            return -5000; // 危险拐角：出口处有蛇身或蛇头威胁区域
        }
      
        if (map_item[next_y][next_x] == -2) {
            return -50; // 陷阱拐角：出口处有陷阱
        }
      
        return 0; // 安全拐角
    }
    
    // 新增NAIVE陷阱评估函数
    double evaluateTrap(const GameState &state, int trap_y, int trap_x) {
        // 不再需要这些参数标记，因为我们实际会使用它们
        // (void)trap_y;
        // (void)trap_x;
        
    const auto &self = state.get_self();
        double score = -1000; // 默认高风险
        
        // 1. 检查陷阱周围的出口数量
        int exits = 0;
        vector<pair<int, int>> exit_points;
        
        for (auto dir : validDirections) {
            const auto [y, x] = Utils::nextPos({trap_y, trap_x}, dir);
            // 有效出口：在界内，不是墙，不是蛇身，不是另一个陷阱
            if (Utils::boundCheck(y, x) && 
                map_item[y][x] != -4 && map_snake[y][x] != -5 && map_item[y][x] != -2) {
                exits++;
                exit_points.push_back({y, x});
            }
        }
        
        // 更多出口 = 更安全
        score += exits * 200;
        
        // 2. 评估各出口的安全程度
        int safe_exits = 0;
        for (const auto &exit : exit_points) {
            // 分析出口的地形风险
            auto exit_terrain = analyzeTerrainRisk(state, exit.first, exit.second);
            
            // 如果出口不是死胡同或高风险瓶颈，视为安全出口
            if (!exit_terrain.is_dead_end && exit_terrain.risk_score > -500) {
                safe_exits++;
                score += 100; // 每个安全出口加分
            }
            
            // 额外检查：出口附近是否有敌方蛇
            bool enemy_near_exit = false;
            for (const auto &snake : state.snakes) {
                if (snake.id != MYID && snake.id != -1) {
                    const Point &enemy_head = snake.get_head();
                    int dist_to_exit = abs(enemy_head.y - exit.first) + abs(enemy_head.x - exit.second);
                    
                    // 如果敌方蛇距离出口很近，这个出口不安全
                    if (dist_to_exit <= 2) {
                        enemy_near_exit = true;
                        break;
                    }
                }
            }
            
            // 如果出口附近有敌方蛇，降低分数
            if (enemy_near_exit) {
                score -= 200;
            } else {
                score += 50; // 无敌方蛇的出口更安全
            }
        }
        
        // 如果没有安全出口，严重降低分数
        if (safe_exits == 0 && exits > 0) score -= 500; // 有出口但都不安全
        else if (exits == 0) score -= 1000; // 完全没有出口
        
        // 根据蛇当前状态调整风险容忍度
        if (self.score > 50) score += 200; // 分数较高时，提高风险容忍度
        if (self.score > 100) score += 200; // 分数很高时，进一步提高风险容忍度
        
        // 紧急情况下接受较高风险
        bool is_emergency = true;
        const auto &head = self.get_head();
        for (auto dir : validDirections) {
            const auto [next_y, next_x] = Utils::nextPos({head.y, head.x}, dir);
            if (Utils::boundCheck(next_y, next_x)) {
                // 检查是否为障碍（墙、蛇身，但不包括陷阱）
                if (map_item[next_y][next_x] != -4 && map_snake[next_y][next_x] != -5 && 
                    next_x >= state.current_safe_zone.x_min && next_x <= state.current_safe_zone.x_max && 
                    next_y >= state.current_safe_zone.y_min && next_y <= state.current_safe_zone.y_max) {
                    is_emergency = false;
                    break;
                }
            }
        }
        
        // 如果蛇即将死亡(四周都是障碍)，接受陷阱
        if (is_emergency) score = max(score, -300.0); // 紧急情况下提高风险容忍
        
        return score;
    }

    // 综合评估函数 - 结合BFS、拐角评估和安全区边界考虑
    double eval(const GameState &state, int y, int x, int fy, int fx)
    {
        int test = cornerEval(y, x, fy, fx);
        if (test != -10000) { // 是拐角位置
            if (test == -5000) return -50000; // 危险拐角，强烈避免
            if (test == -50) map_item[y][x] = -9; // 标记陷阱拐角
        }
        
        // 安全区评分 - 考虑边界和蛇密度
        double safe_zone_score = safeZoneCenterScore(state, y, x);
        
        // 新增：战略位置评估 - 中后期更关注终点安全区
        double strategic_score = evaluateStrategicPosition(state, y, x);
        
        // BFS评估
        double bfs_score = bfs(y, x, fy, fx, state);
        
        // 整合评分
        return bfs_score + safe_zone_score + strategic_score;
    }

    // 评估目标的价值和安全性
    TargetEvaluation evaluateTarget(const GameState &state, const Point &target_pos, double base_value, bool is_chest) {
        // 宝箱和钥匙使用专门的评估函数
        if (is_chest || base_value >= 40.0) {
            return evaluateChestKeyTarget(state, target_pos, base_value, is_chest);
        }
        
        const auto &head = state.get_self().get_head();
        int distance = abs(head.y - target_pos.y) + abs(head.x - target_pos.x);
      
        // 基础评估
        TargetEvaluation result = {
            target_pos, base_value, distance, true // 默认安全
        };
      
        // 安全性检查1: 安全区收缩
        if (Utils::willDisappearInShrink(state, target_pos.x, target_pos.y)) {
            result.is_safe = false;
            result.value *= 0.2;
            return result; // 目标将消失，直接返回
        }
      
        // 新增: 评估通往目标的路径安全性
        int look_ahead = min(distance, 8); // 路径前瞻步数，不超过距离
        auto path_safety = evaluatePathSafety(state, head, target_pos, look_ahead);
        
        // 根据路径安全性调整目标价值
        if (!path_safety.is_safe) {
                result.is_safe = false;
            
            // 根据危险程度降低价值
            double danger_ratio = 1.0 - min(1.0, -path_safety.safety_score / 2000.0);
            result.value *= danger_ratio;
            
            // 如果是极度危险的路径，几乎放弃该目标
            if (path_safety.safety_score < -1500) {
                result.value *= 0.1;
            }
        }
        
        // 安全路径但仅能安全前进少数几步，也视为不安全
        if (result.is_safe && path_safety.safe_steps < min(distance, 3)) {
            result.is_safe = false;
            result.value *= 0.5;
        }
      
        // 竞争因素
        int closest_competitor_dist = INT_MAX;
      
        for (const auto &snake : state.snakes) {
            if (snake.id != MYID && snake.id != -1) {
                int enemy_dist = abs(snake.get_head().y - target_pos.y) + 
                                abs(snake.get_head().x - target_pos.x);
              
                if (enemy_dist < distance) {
                    closest_competitor_dist = min(closest_competitor_dist, enemy_dist);
                    result.value *= 0.7; // 竞争惩罚
                }
            }
        }
      
        return result;
    }

    // 专门评估宝箱和钥匙目标的价值和安全性
    TargetEvaluation evaluateChestKeyTarget(const GameState &state, const Point &target_pos, double base_value, bool is_chest) {
        const auto &head = state.get_self().get_head();
        int distance = abs(head.y - target_pos.y) + abs(head.x - target_pos.x);
        const auto &self = state.get_self();
        
        // 基础评估
        TargetEvaluation result = {
            target_pos, base_value, distance, true // 默认安全
        };
        
        // 安全性检查1: 安全区收缩
        if (Utils::willDisappearInShrink(state, target_pos.x, target_pos.y)) {
                            result.is_safe = false;
            result.value *= 0.1; // 宝箱/钥匙如果在收缩区，几乎不考虑
            return result; // 目标将消失，直接返回
        }
        
        // 安全性检查2: 路径安全性评估
        int look_ahead = min(max(distance, 10), 15); // 宝箱和钥匙需要更长的前瞻
        auto path_safety = evaluatePathSafety(state, head, target_pos, look_ahead);
        
        // 根据路径安全性调整目标价值
        if (!path_safety.is_safe) {
            result.is_safe = false;
            
            // 宝箱和钥匙路径安全性要求更高
            double danger_ratio = 1.0 - min(1.0, -path_safety.safety_score / 1500.0);
            result.value *= danger_ratio;
            
            // 高风险路径大幅降低价值
            if (path_safety.safety_score < -1200) {
                result.value *= 0.05;
            }
        }
        
        // 安全路径但仅能安全前进少数几步，对宝箱和钥匙更严格评估
        if (result.is_safe && path_safety.safe_steps < min(distance + 2, 5)) {
            result.is_safe = false;
            result.value *= 0.3;
        }
        
        // 检查这个位置是否在安全区内，且离边缘足够远
        int current_tick = MAX_TICKS - state.remaining_ticks;
        int ticks_to_shrink = state.next_shrink_tick - current_tick;
        SafeZoneBounds zone = (ticks_to_shrink >= 0 && ticks_to_shrink <= 15) ? 
                             state.next_safe_zone : state.current_safe_zone;
        
        // 计算到安全区边界的最小距离
        int dist_to_border = min({
            target_pos.x - zone.x_min,
            zone.x_max - target_pos.x,
            target_pos.y - zone.y_min,
            zone.y_max - target_pos.y
        });
        
        // 宝箱或钥匙太靠近边界也不安全
        if (dist_to_border < 3) {
            if (dist_to_border == 0) {
                result.is_safe = false;
                result.value *= 0.3;
                    } else {
                result.value *= (0.7 + 0.1 * dist_to_border); // 0.8 或 0.9
            }
        }
        
        // 竞争因素 - 对宝箱和钥匙的竞争评估更加细致
        vector<pair<int, int>> competitors; // 存储(距离, 蛇ID)
        
        for (const auto &snake : state.snakes) {
            if (snake.id != MYID && snake.id != -1) {
                const Point &enemy_head = snake.get_head();
                int enemy_dist = abs(enemy_head.y - target_pos.y) + abs(enemy_head.x - target_pos.x);
                
                // 记录所有竞争者
                if (enemy_dist <= distance * 1.5) { // 只考虑距离较近的竞争者
                    competitors.push_back({enemy_dist, snake.id});
                }
            }
        }
        
        // 根据竞争情况调整价值
        if (!competitors.empty()) {
            // 按距离排序
            sort(competitors.begin(), competitors.end());
            
            int closest_competitor_dist = competitors[0].first;
            
            // 如果敌人比我们更近
            if (closest_competitor_dist < distance) {
                double competition_factor = (double)closest_competitor_dist / distance;
                
                // 宝箱竞争更严格评估
                if (is_chest) {
                    if (competition_factor < 0.5) {
                        // 竞争者远远更近，几乎放弃
                        result.is_safe = false;
                        result.value *= 0.1;
                    } else if (competition_factor < 0.8) {
                        // 竞争者明显更近
                        result.is_safe = false;
                        result.value *= 0.3;
                    } else {
                        // 竞争者稍微更近
                        result.value *= 0.6;
                    }
                    
                    // 特殊情况：我们有护盾，增加价值
                    if (self.shield_time > 0 || self.shield_cd == 0) {
                        result.value *= 1.5;
                    }
                } else {
                    // 钥匙竞争评估稍宽松
                    if (competition_factor < 0.6) {
                        // 竞争者明显更近
                        result.value *= 0.4;
                    } else if (competition_factor < 0.9) {
                        // 竞争者稍微更近
                        result.value *= 0.7;
                    }
                }
            }
            
            // 多个竞争者更加危险
            if (competitors.size() > 1) {
                result.value *= pow(0.9, competitors.size() - 1);
            }
        }
        
        // 针对宝箱的特殊评估
        if (is_chest) {
            // 如果持有钥匙但剩余时间不多，提高宝箱价值
            for (const auto &key : state.keys) {
                if (key.holder_id == MYID) {
                    if (key.remaining_time <= distance + 3) {
                        result.value *= 2.0; // 紧急情况加倍价值
                    } else if (key.remaining_time <= distance * 1.5) {
                        result.value *= 1.5; // 中等紧急
                    }
                    break;
                }
            }
            
            // 游戏后期，宝箱价值更高
            if (state.remaining_ticks < 100) {
                result.value *= 1.3;
            }
        } 
        // 钥匙特殊评估
        else {
            // 如果有宝箱在附近，提高钥匙价值
            int min_chest_dist = INT_MAX;
            for (const auto &chest : state.chests) {
                int chest_dist = abs(chest.pos.y - target_pos.y) + abs(chest.pos.x - target_pos.x);
                min_chest_dist = min(min_chest_dist, chest_dist);
            }
            
            // 钥匙附近有宝箱，提高价值
            if (min_chest_dist < 15) {
                result.value *= 1.0 + (15 - min_chest_dist) * 0.05; // 最多提高75%
            }
            
            // 游戏后期，钥匙价值更高
            if (state.remaining_ticks < 100) {
                result.value *= 1.2;
            }
        }
      
        return result;
    }

    // 计算位置的安全出口数量
    int countSafeExits(const GameState &state, const Point &pos) {
        int safe_exits = 0;
        
        for (auto dir : validDirections) {
            const auto [ny, nx] = Utils::nextPos({pos.y, pos.x}, dir);
            
            // 检查是否是有效出口(在界内，不是墙，不是蛇身，在安全区)
            if (Utils::boundCheck(ny, nx) && 
                map_item[ny][nx] != -4 && map_snake[ny][nx] != -5 &&
                nx >= state.current_safe_zone.x_min && nx <= state.current_safe_zone.x_max && 
                ny >= state.current_safe_zone.y_min && ny <= state.current_safe_zone.y_max) {
                safe_exits++;
            }
        }
        
        return safe_exits;
    }
    
    // 找出当前位置的最佳下一步方向
    Point findBestNextStep(const GameState &state, const Point &pos) {
        Point best_next = pos; // 默认原地不动
        double best_score = -10000;
        
        for (auto dir : validDirections) {
            const auto [ny, nx] = Utils::nextPos({pos.y, pos.x}, dir);
            
            // 检查位置有效性
            if (!Utils::boundCheck(ny, nx) || map_item[ny][nx] == -4 || map_snake[ny][nx] == -5) continue; // 无效位置
            
            // 检查位置是否在安全区内
            if (nx < state.current_safe_zone.x_min || nx > state.current_safe_zone.x_max || 
                ny < state.current_safe_zone.y_min || ny > state.current_safe_zone.y_max) continue; // 安全区外
            
            // 简单评估该位置
            double score = 0;
            
            // 评估出口数量
            int exits = countSafeExits(state, {ny, nx});
            score += exits * 100; // 出口越多越好
            
            // 避免陷阱
            if (map_item[ny][nx] == -2) score -= 300; // 陷阱惩罚
            
            // 避开敌方蛇头附近
            for (const auto &snake : state.snakes) {
                if (snake.id != MYID && snake.id != -1) {
                    const Point &enemy_head = snake.get_head();
                    int dist = abs(enemy_head.y - ny) + abs(enemy_head.x - nx);
                    
                    // 距离敌方蛇头越近越危险
                    if (dist <= 2) score -= (3 - dist) * 500; // 越近惩罚越大
                }
            }
            
            // 更新最佳选择
            if (score > best_score) {
                best_score = score;
                best_next = {ny, nx};
            }
        }
        
        return best_next;
    }
    
    // 创建地图快照用于模拟移动
    vector<vector<int>> makeMapSnapshot(const GameState &state) {
        // 创建地图副本
        vector<vector<int>> map_copy(MAXM, vector<int>(MAXN, 0));
        
        // 复制墙体
        for (int i = 0; i < MAXM; i++) {
            for (int j = 0; j < MAXN; j++) {
                if (map_item[i][j] == -4) map_copy[i][j] = -4; // 墙
            }
        }
        
        // 复制蛇身体
        for (const auto &snake : state.snakes) {
            for (const auto &part : snake.body) {
                if (Utils::boundCheck(part.y, part.x)) {
                    map_copy[part.y][part.x] = -5; // 蛇身
                }
            }
        }
        
        return map_copy;
    }
    
    // 模拟移动风险评估
    double simulateMovementRisk(const GameState &state, int start_y, int start_x, int steps) {
        double risk = 0;
        Point current = {start_y, start_x};
        
        // 创建地图快照
        auto map_copy = makeMapSnapshot(state);
        
        for (int i = 0; i < steps; i++) {
            // 计算该位置的可行出口数量
            int exits = countSafeExits(state, current);
            
            // 出口越少，风险越高
            if (exits == 0) return -2000;  // 死路
            if (exits == 1) risk -= 500;   // 只有一个出口，高风险
            if (exits == 2) risk -= 100;   // 两个出口，中等风险
            
            // 检查当前位置是否有陷阱
            if (map_item[current.y][current.x] == -2) risk -= 200; // 陷阱惩罚
            
            // 检查当前位置是否安全
            auto terrain = analyzeTerrainRisk(state, current.y, current.x);
            if (terrain.is_dead_end) risk -= 800; // 死胡同惩罚
            if (terrain.is_bottleneck) risk -= 300; // 瓶颈惩罚
            
            // 模拟选择最佳下一步
            Point next_pos = findBestNextStep(state, current);
            
            // 如果没有找到合适的下一步或者原地踏步
            if (next_pos.y == current.y && next_pos.x == current.x) {
                risk -= 1000; // 严重惩罚
                break; // 中断模拟
            }
            
            // 更新当前位置
            current = next_pos;
            
            // 更新地图快照，标记已访问位置
            map_copy[current.y][current.x] = -5; // 标记为已访问
        }
        
        return risk;
    }
    
    // A*寻路算法实现
    vector<Point> findPath(const GameState &state, const Point &start, const Point &target) {
        // 启发式函数：计算曼哈顿距离
        auto h = [](const Point &p, const Point &target) {
            return abs(p.y - target.y) + abs(p.x - target.x);
        };
        
        // 优先队列用于A*算法，按f值（g+h）排序
        // 存储格式：{f值, {g值, {点位置y, 点位置x}}}
        priority_queue<pair<int, pair<int, Point>>, vector<pair<int, pair<int, Point>>>, greater<>> pq;
        
        // 记录已经访问的节点和每个点的前驱节点
        unordered_map<string, bool> visited;
        unordered_map<string, Point> parent;
        
        // 初始化起点
        pq.push({h(start, target), {0, start}});
        
        // A*搜索
        while (!pq.empty()) {
            auto [f, gp] = pq.top();
            auto [g, current] = gp;
            pq.pop();
            
            // 生成当前点的唯一标识
            string current_id = Utils::idx2str({current.y, current.x});
            
            // 如果已访问，跳过
            if (visited[current_id]) continue;
            visited[current_id] = true;
            
            // 如果到达目标，构建路径并返回
            if (current.y == target.y && current.x == target.x) {
                vector<Point> path;
                Point p = current;
                while (!(p.y == start.y && p.x == start.x)) {
                    path.push_back(p);
                    string p_id = Utils::idx2str({p.y, p.x});
                    p = parent[p_id];
                }
                reverse(path.begin(), path.end());
                return path;
            }
            
            // 探索四个方向
            for (auto dir : validDirections) {
                const auto [ny, nx] = Utils::nextPos({current.y, current.x}, dir);
                
                // 检查是否越界
                if (!Utils::boundCheck(ny, nx)) continue;
                
                // 检查是否为墙或蛇身
                if (map_item[ny][nx] == -4 || map_snake[ny][nx] == -5) continue;
                
                // 新的点
                Point next = {ny, nx};
                string next_id = Utils::idx2str({ny, nx});
                
                // 如果已访问，跳过
                if (visited[next_id]) continue;
                
                // 计算新的g值（距离起点的成本）
                int new_g = g + 1;
                
                // 额外考虑陷阱成本
                if (map_item[ny][nx] == -2) new_g += 3;
                
                // 考虑蛇头威胁区域成本
                if (map_snake[ny][nx] == -6) new_g += 2;
                
                // 计算f值（总估计成本）
                int new_f = new_g + h(next, target);
                
                // 更新队列
                pq.push({new_f, {new_g, next}});
                parent[next_id] = current;
            }
        }
        
        // 无法找到路径
        return {};
    }
    
    // 检查点位的安全区收缩风险
    double checkPointSafeZoneRisk(const GameState &state, const Point &point, int step_idx) {
        int x = point.x, y = point.y;
        double risk = 0.0;
        
        int current_tick = MAX_TICKS - state.remaining_ticks;
        int ticks_to_shrink = state.next_shrink_tick - current_tick;
        
        // 考虑步数和收缩时间的关系
        // 如果我们预计在安全区收缩之前能到达该点，风险较低
        int estimated_arrival_tick = current_tick + step_idx;
        
        // 如果点在当前安全区外，立即给高风险
        if (x < state.current_safe_zone.x_min || x > state.current_safe_zone.x_max ||
            y < state.current_safe_zone.y_min || y > state.current_safe_zone.y_max) {
            return -5000;  // 极高风险
        }
        
        // 如果即将收缩（20个tick内）
        if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20) {
            // 如果预计到达时间晚于收缩时间，且点在下一个安全区外
            if (estimated_arrival_tick >= state.next_shrink_tick && 
                (x < state.next_safe_zone.x_min || x > state.next_safe_zone.x_max ||
                 y < state.next_safe_zone.y_min || y > state.next_safe_zone.y_max)) {
                
                // 安全区外位置风险大幅增加
                risk = -3000;
                
                // 收缩越近风险越高
                if (ticks_to_shrink <= 5) risk -= 2000;
                else if (ticks_to_shrink <= 10) risk -= 1000;
            }
            // 如果点在下一个安全区外但预计能在收缩前到达
            else if (x < state.next_safe_zone.x_min || x > state.next_safe_zone.x_max ||
                     y < state.next_safe_zone.y_min || y > state.next_safe_zone.y_max) {
                
                // 风险值取决于到达时间和收缩时间的差距
                int margin = state.next_shrink_tick - estimated_arrival_tick;
                if (margin <= 3) risk = -1500;  // 时间很紧，高风险
                else if (margin <= 10) risk = -500;  // 中等风险
                else risk = -200;  // 较低风险
            }
            // 如果点在下一个安全区内，考虑收缩后的蛇密度风险
            else {
                // 增加对蛇密度的考虑
                // 使用新的安全区密度评估函数
                double density_risk = evaluateSafeZoneDensity(state, x, y);
                
                // 只保留蛇密度风险部分，基本安全区风险已经在上面处理
                density_risk -= checkSafeZoneRisk(state, x, y);
                
                // 把蛇密度风险考虑进总体风险
                risk += density_risk;
            }
        }
        
        return risk;
    }
    
    // 检查点位的地形风险（死胡同、瓶颈）
    pair<double, bool> checkTerrainRisk(const GameState &state, const Point &point) {
        // 复用已有的地形分析函数
        TerrainAnalysis terrain = analyzeTerrainRisk(state, point.y, point.x);
        
        double risk = terrain.risk_score;
        bool is_dangerous = terrain.is_dead_end || terrain.is_bottleneck;
        
        return {risk, is_dangerous};
    }
    
    // 检查点位的蛇密度风险 - 增强版，考虑收缩影响
    double checkSnakeDensityRisk(const GameState &state, const Point &point, int step_idx) {
        double risk = 0.0;
        int x = point.x, y = point.y;
        
        // 计算当前tick和收缩时间
        int current_tick = MAX_TICKS - state.remaining_ticks;
        int ticks_to_shrink = state.next_shrink_tick - current_tick;
        int estimated_arrival_tick = current_tick + step_idx;
        bool will_be_shrunk = estimated_arrival_tick >= state.next_shrink_tick;
        
        // 计算安全区面积和收缩比例
        double safe_zone_area_factor = 1.0;
        if (will_be_shrunk) {
            int current_area = (state.current_safe_zone.x_max - state.current_safe_zone.x_min + 1) *
                              (state.current_safe_zone.y_max - state.current_safe_zone.y_min + 1);
            int next_area = (state.next_safe_zone.x_max - state.next_safe_zone.x_min + 1) *
                           (state.next_safe_zone.y_max - state.next_safe_zone.y_min + 1);
            safe_zone_area_factor = (double)current_area / next_area;
            
            // 限制在合理范围内
            safe_zone_area_factor = min(3.0, max(1.0, safe_zone_area_factor));
        }
        
        // 计算附近的敌方蛇数量
        int nearby_snakes = 0;
        int very_close_snakes = 0;
        
        // 动态调整检测范围 - 收缩后范围缩小
        int close_radius = will_be_shrunk ? 2 : 3;
        int nearby_radius = will_be_shrunk ? 4 : 6;
        
        for (const auto &snake : state.snakes) {
            if (snake.id != MYID && snake.id != -1) {
                const Point &enemy_head = snake.get_head();
                
                // 检查敌方蛇是否在将来的安全区内
                bool enemy_in_future_safe_zone = 
                    enemy_head.x >= state.next_safe_zone.x_min && 
                    enemy_head.x <= state.next_safe_zone.x_max &&
                    enemy_head.y >= state.next_safe_zone.y_min && 
                    enemy_head.y <= state.next_safe_zone.y_max;
                
                // 如果收缩后点和敌方蛇都在安全区内，增加风险
                if (will_be_shrunk && 
                    x >= state.next_safe_zone.x_min && x <= state.next_safe_zone.x_max &&
                    y >= state.next_safe_zone.y_min && y <= state.next_safe_zone.y_max &&
                    enemy_in_future_safe_zone) {
                    
                    int dist = abs(enemy_head.y - y) + abs(enemy_head.x - x);
                    
                    // 收缩后距离变近
                    int adjusted_dist = (int)(dist / sqrt(safe_zone_area_factor));
                    
                    // 敌方蛇头在非常近的位置
                    if (adjusted_dist <= close_radius) very_close_snakes++;
                    
                    // 敌方蛇头在较近的位置
                    if (adjusted_dist <= nearby_radius) nearby_snakes++;
                }
                else {
                    // 正常计算距离
                    int dist = abs(enemy_head.y - y) + abs(enemy_head.x - x);
                    
                    // 敌方蛇头在非常近的位置
                    if (dist <= close_radius) very_close_snakes++;
                    
                    // 敌方蛇头在较近的位置
                    if (dist <= nearby_radius) nearby_snakes++;
                }
            }
        }
        
        // 计算基础风险值
        if (very_close_snakes > 0) {
            // 非常近的敌方蛇，高风险
            risk -= 800 * very_close_snakes;
        }
        
        if (nearby_snakes > 0) {
            // 近距离敌方蛇，中等风险
            risk -= 200 * nearby_snakes;
        }
        
        // 考虑步数因素 - 越早遇到敌方蛇风险越高
        if (risk < 0 && step_idx <= 3) {
            risk *= 1.5; // 前3步遇到敌方蛇，风险增加50%
        }
        
        // 收缩因素 - 如果即将收缩且蛇密度高，风险更高
        if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20) {
            // 计算所有蛇在未来安全区内的比例
            int total_snakes = 0;
            int snakes_in_future_zone = 0;
            
            for (const auto &snake : state.snakes) {
                if (snake.id != -1) {
                    total_snakes++;
                    const Point &snake_head = snake.get_head();
                    if (snake_head.x >= state.next_safe_zone.x_min && 
                        snake_head.x <= state.next_safe_zone.x_max &&
                        snake_head.y >= state.next_safe_zone.y_min && 
                        snake_head.y <= state.next_safe_zone.y_max) {
                        snakes_in_future_zone++;
                    }
                }
            }
            
            // 如果未来安全区内蛇的比例高，增加风险
            if (total_snakes > 0) {
                double density_factor = (double)snakes_in_future_zone / total_snakes * safe_zone_area_factor;
                
                // 密度越高，风险越大
                if (density_factor > 1.5) {
                    risk -= (density_factor - 1.5) * 300;
                }
                
                // 收缩时间越近，风险越大
                if (ticks_to_shrink <= 5) {
                    risk *= 1.5;
                } else if (ticks_to_shrink <= 10) {
                    risk *= 1.3;
                }
            }
        }
        
        return risk;
    }
    
    // 检查点位的特殊地形风险（如陷阱）
    double checkSpecialTerrainRisk(const GameState &state, const Point &point) {
        double risk = 0.0;
        int x = point.x, y = point.y;
        
        // 陷阱检查
        if (map_item[y][x] == -2) {
            // 使用现有的陷阱评估函数
            risk = evaluateTrap(state, y, x);
            
            // 确保评估值是负数（表示风险）
            if (risk > 0) risk = 0;
        }
        
        return risk;
    }
    
    // 路径安全评估主函数
    PathSafetyEvaluation evaluatePathSafety(const GameState &state, const Point &start, const Point &target, int look_ahead) {
        PathSafetyEvaluation result;
        
        // 使用A*算法规划路径
        vector<Point> path = findPath(state, start, target);
        if (path.empty()) {
            return result; // 无法到达，返回默认不安全评估
        }
        
        // 初始化为安全路径
        result.safety_score = 0;
        result.is_safe = true;
        result.safe_steps = path.size();
        
        // 评估路径上的每个点
        for (int i = 0; i < min((int)path.size(), look_ahead); i++) {
            const Point &point = path[i];
            double point_safety = 0;
            
            // 1. 检查安全区收缩风险
            double safe_zone_risk = checkPointSafeZoneRisk(state, point, i);
            point_safety += safe_zone_risk;
            
            // 2. 检查死胡同/瓶颈风险
            auto [terrain_risk, is_dangerous_terrain] = checkTerrainRisk(state, point);
            point_safety += terrain_risk;
            
            // 3. 检查蛇密度风险 - 增强版，考虑收缩因素
            double snake_density_risk = checkSnakeDensityRisk(state, point, i);
            
            // 对于安全区收缩相关的风险，进行更全面的评估
            int current_tick = MAX_TICKS - state.remaining_ticks;
            int ticks_to_shrink = state.next_shrink_tick - current_tick;
            
            // 如果路径点接近收缩时间，增加额外的蛇密度评估
            if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20 && i >= ticks_to_shrink) {
                // 使用evaluateSafeZoneDensity评估收缩后的蛇密度风险
                double zone_density_risk = evaluateSafeZoneDensity(state, point.x, point.y);
                
                // 结合两种风险评估
                snake_density_risk = min(snake_density_risk, zone_density_risk);
            }
            
            point_safety += snake_density_risk;
            
            // 4. 特殊地形风险(如陷阱)
            double special_terrain_risk = checkSpecialTerrainRisk(state, point);
            point_safety += special_terrain_risk;
            
            // 综合评估点位安全性
            result.safety_score += point_safety;
            
            // 如果某个点极度危险，记录并降低整体安全性
            if (point_safety < -500) {
                result.risky_points.push_back({point.y, point.x});
                result.safe_steps = min(result.safe_steps, i);
                
                // 如果是前3步就有高危险，整条路径视为不安全
                if (i < 3 && point_safety < -1000) {
                    result.is_safe = false;
                }
            }
        }
        
        // 路径整体安全性评估
        if (result.safety_score < -2000) {
            result.is_safe = false;
        }
        
        return result;
    }

    // 添加评估安全区密度函数 - 计算收缩后蛇密度的变化
    double evaluateSafeZoneDensity(const GameState &state, int x, int y) {
        double score = 0;
        
        // 使用现有函数评估基本安全区风险
        score += checkSafeZoneRisk(state, x, y);
        
        // 计算当前蛇密度
        int current_tick = MAX_TICKS - state.remaining_ticks;
        int ticks_to_shrink = state.next_shrink_tick - current_tick;
        
        // 如果即将收缩（20个tick内）
        if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20) {
            // 检查位置是否在下一个安全区内
            if (x >= state.next_safe_zone.x_min && x <= state.next_safe_zone.x_max &&
                y >= state.next_safe_zone.y_min && y <= state.next_safe_zone.y_max) {
                
                // 计算当前安全区和下一个安全区的面积
                int current_area = (state.current_safe_zone.x_max - state.current_safe_zone.x_min + 1) *
                                  (state.current_safe_zone.y_max - state.current_safe_zone.y_min + 1);
                int next_area = (state.next_safe_zone.x_max - state.next_safe_zone.x_min + 1) *
                               (state.next_safe_zone.y_max - state.next_safe_zone.y_min + 1);
                
                // 收缩比例
                double shrink_ratio = (double)next_area / current_area;
                
                // 计算收缩后该点附近的蛇密度
                int current_snakes = 0;
                int future_snakes = 0;
                int proximity_radius = 5; // 附近区域半径
                
                for (const auto &snake : state.snakes) {
                    if (snake.id == -1) continue; // 跳过无效蛇
                    
                    const Point &snake_head = snake.get_head();
                    int dist = abs(snake_head.y - y) + abs(snake_head.x - x);
                    
                    // 计算当前密度
                    if (dist <= proximity_radius) {
                        current_snakes++;
                    }
                    
                    // 预测收缩后的密度（如果蛇在下一个安全区内）
                    if (snake_head.x >= state.next_safe_zone.x_min && 
                        snake_head.x <= state.next_safe_zone.x_max &&
                        snake_head.y >= state.next_safe_zone.y_min && 
                        snake_head.y <= state.next_safe_zone.y_max) {
                        
                        // 收缩后距离会变小，按比例计算
                        int future_dist = (int)(dist * sqrt(shrink_ratio));
                        if (future_dist <= proximity_radius) {
                            future_snakes++;
                        }
                    }
                }
                
                // 密度增加带来的风险
                double density_risk = (future_snakes - current_snakes) * 200;
                
                // 如果收缩后蛇密度显著增加，增加额外风险
                if (future_snakes > current_snakes + 1) {
                    density_risk += (future_snakes - current_snakes - 1) * 300;
                }
                
                // 收缩时间越近，风险越高
                if (ticks_to_shrink <= 5) {
                    density_risk *= 2;
                } else if (ticks_to_shrink <= 10) {
                    density_risk *= 1.5;
                }
                
                score -= density_risk;
            }
        }
        
        return score;
    }
    
    // 新增：评估战略位置函数 - 为中后期游戏提供更智能的位置评估
    double evaluateStrategicPosition(const GameState &state, int y, int x) {
        double score = 0;
        int current_tick = MAX_TICKS - state.remaining_ticks;
        
        // 游戏阶段判断
        bool is_mid_game = current_tick >= 100 && current_tick < 180;
        bool is_late_game = current_tick >= 180;
        
        // 如果是游戏初期，直接返回0（不影响）
        if (!is_mid_game && !is_late_game) {
            return score;
        }
        
                 // 获取当前和将来的安全区
         SafeZoneBounds current_zone = state.current_safe_zone;
         SafeZoneBounds next_zone = state.next_safe_zone;
         SafeZoneBounds final_zone = state.final_safe_zone;
         
         // 计算当前点到最终安全区中心的距离
         int final_center_x = (final_zone.x_min + final_zone.x_max) / 2;
         int final_center_y = (final_zone.y_min + final_zone.y_max) / 2;
         int dist_to_final_center = abs(y - final_center_y) + abs(x - final_center_x);
         
         // 计算各个安全区的面积
         int current_area = (current_zone.x_max - current_zone.x_min + 1) * 
                          (current_zone.y_max - current_zone.y_min + 1);
         // 注意：next_area在下面的代码中未使用，先计算final_area
         int final_area = (final_zone.x_max - final_zone.x_min + 1) * 
                        (final_zone.y_max - final_zone.y_min + 1);
        
        // 中期策略：开始关注下一个安全区
        if (is_mid_game) {
            // 距离下一个安全区中心越近越好
            int next_center_x = (next_zone.x_min + next_zone.x_max) / 2;
            int next_center_y = (next_zone.y_min + next_zone.y_max) / 2;
            int dist_to_next_center = abs(y - next_center_y) + abs(x - next_center_x);
            
            // 在下一个安全区内的位置更好
            if (x >= next_zone.x_min && x <= next_zone.x_max &&
                y >= next_zone.y_min && y <= next_zone.y_max) {
                score += 150;
                
                // 在下一个安全区内，离中心越近越好（避免边缘）
                double center_bonus = 100 - min(100.0, dist_to_next_center * 15.0);
                score += center_bonus;
            }
            // 如果在当前安全区但不在下一个安全区，看距离有多远
            else if (x >= current_zone.x_min && x <= current_zone.x_max &&
                     y >= current_zone.y_min && y <= current_zone.y_max) {
                // 计算到下一个安全区边界的最短距离
                int dist_to_next_zone = max(0, min({
                    x - next_zone.x_min,
                    next_zone.x_max - x,
                    y - next_zone.y_min,
                    next_zone.y_max - y
                }));
                
                // 距离越远，惩罚越大
                score -= dist_to_next_zone * 50;
            }
            
            // 中期也开始考虑最终安全区
            // 但权重较小
            if (dist_to_final_center < current_area / 10) {
                score += 50; // 额外奖励离最终安全区中心近的位置
            }
        }
        
        // 后期策略：高度重视最终安全区
        if (is_late_game) {
            // 在最终安全区内的位置极其重要
            if (x >= final_zone.x_min && x <= final_zone.x_max &&
                y >= final_zone.y_min && y <= final_zone.y_max) {
                score += 500;
                
                // 在最终安全区内，离中心越近越好
                double center_bonus = 300 - min(300.0, dist_to_final_center * 30.0);
                score += center_bonus;
                
                // 额外考虑蛇密度
                int snakes_in_final_zone = 0;
                for (const auto &snake : state.snakes) {
                    if (snake.id != -1) {
                        const Point &snake_head = snake.get_head();
                        if (snake_head.x >= final_zone.x_min && 
                            snake_head.x <= final_zone.x_max &&
                            snake_head.y >= final_zone.y_min && 
                            snake_head.y <= final_zone.y_max) {
                            snakes_in_final_zone++;
                        }
                    }
                }
                
                // 密度越高风险越大
                double density = (double)snakes_in_final_zone / final_area * 100;
                if (density > 10) { // 10%阈值
                    score -= (density - 10) * 20;
                }
            }
            // 不在最终安全区，但在下一个安全区
            else if (x >= next_zone.x_min && x <= next_zone.x_max &&
                     y >= next_zone.y_min && y <= next_zone.y_max) {
                score += 200;
                
                // 优先考虑向最终安全区移动的方向
                int dist_to_final_zone = min({
                    abs(x - final_zone.x_min),
                    abs(x - final_zone.x_max),
                    abs(y - final_zone.y_min),
                    abs(y - final_zone.y_max)
                });
                
                // 距离最终安全区越近越好
                score -= dist_to_final_zone * 30;
            }
            // 既不在最终也不在下一个安全区
            else {
                // 高风险情况
                score -= 500;
            }
        }
        
        return score;
    }
}

// 添加到judge函数之前
Direction chooseBestDirection(const GameState &state, const vector<Direction>& preferred_dirs = {}) {
    unordered_set<Direction> illegals = illegalMove(state);
    vector<Direction> legalMoves;
  
    // 首先尝试首选方向
    for (auto dir : preferred_dirs) {
        if (illegals.count(dir) == 0) return dir; // 首选方向合法，直接返回
    }
  
    // 没有可用的首选方向，收集所有合法移动
    for (auto dir : validDirections) {
        if (illegals.count(dir) == 0) legalMoves.push_back(dir);
    }
  
    // 没有合法移动
    if (legalMoves.empty()) return Direction::UP; // 返回默认方向，外部会处理为护盾
  
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

// 添加增强版食物处理函数的前向声明
bool enhancedFoodProcessByPriority(const GameState &state, int minValue, int maxRange, Direction& moveDirection);

// 这个函数已被enhancedFoodProcessByPriority替代，保留为兼容性考虑
bool processFoodByPriority(const GameState &state, int minValue, int maxRange, Direction& moveDirection) {
    return enhancedFoodProcessByPriority(state, minValue, maxRange, moveDirection);
}

// 增强版食物优先级处理函数，考虑安全性因素
bool enhancedFoodProcessByPriority(const GameState &state, int minValue, int maxRange, Direction& moveDirection) {
    const auto &head = state.get_self().get_head();
    const auto &self = state.get_self();
    
    // 获取自适应配置
    Strategy::RiskAdaptiveConfig config = Strategy::getAdaptiveConfig(state);
    
    // 收集候选食物
    vector<pair<Item, double>> candidate_foods;
    
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
        
        // 评估路径安全性
        auto path_safety = Strategy::evaluatePathSafety(state, head, item.pos, min(dist, 8));
        
        // 计算食物的综合价值
        double adjusted_value = item.value;
        
        // 根据路径安全性调整价值 - 使用改进的风险评估方法
        if (!path_safety.is_safe) {
            // 1. 改进: 动态风险因子计算
            double dynamic_risk_threshold = config.path_safety_threshold * 
                                          (1 + 0.5 * (self.length > 10 ? 0.5 : 0)); // 蛇越长，风险容忍越高
            
            // 改进: 风险因子最多降低80%
            double risk_factor = 1.0 - min(0.8, -path_safety.safety_score / 2000.0);
            adjusted_value *= risk_factor;
            
            // 梯度惩罚而非二元决策
            if (path_safety.safety_score < dynamic_risk_threshold) {
                // 使用平滑过渡函数而非硬阈值
                double severity = min(1.0, (dynamic_risk_threshold - path_safety.safety_score) / 500.0);
                adjusted_value *= (0.8 - 0.6 * severity); // 从80%降至20%，而非直接20%
            }
        }
        
        // 安全区收缩检查
        int current_tick = MAX_TICKS - state.remaining_ticks;
        int ticks_to_shrink = state.next_shrink_tick - current_tick;
        bool is_near_shrink = (ticks_to_shrink >= 0 && ticks_to_shrink <= 20);
        
        if (is_near_shrink) {
            bool in_next_zone = item.pos.x >= state.next_safe_zone.x_min && 
                              item.pos.x <= state.next_safe_zone.x_max &&
                              item.pos.y >= state.next_safe_zone.y_min && 
                              item.pos.y <= state.next_safe_zone.y_max;
            
            if (!in_next_zone) {
                adjusted_value *= 0.3; // 不在下一安全区的食物价值大幅降低
            } else {
                adjusted_value *= 1.2; // 在下一安全区的食物价值提高
            }
        }
        
        // 2. 改进: 食物密集区识别
        int nearby_food_count = 0;
        for (const auto &other_item : state.items) {
            if (&other_item != &item && // 不是同一个食物
                abs(other_item.pos.y - item.pos.y) + abs(other_item.pos.x - item.pos.x) <= 4) {
                nearby_food_count++;
            }
        }
        
        // 调整距离权重，较近食物更高权重
        double distance_weight = self.length < 6 ? 0.8 : 0.6; // 短蛇更重视近距离食物
        double distance_factor = pow(1.0 - (double)dist / maxRange, 1.5); // 使用指数函数增强近距离偏好
        if (dist == 1) distance_factor *= 1.8; // 紧邻食物(dist=1)显著提升权重
        double cluster_bonus = nearby_food_count * 0.15; // 食物密集区加分
        
        // 3. 改进: 竞争分析
        double competition_penalty = 0;
        
        // 紧邻食物无竞争惩罚，确保优先获取
        if (dist == 1) {
            competition_penalty = 0;
        } else {
            for (const auto &snake : state.snakes) {
                if (snake.id != MYID && snake.id != -1) {
                    int enemy_dist = abs(snake.get_head().y - item.pos.y) + abs(snake.get_head().x - item.pos.x);
                    if (enemy_dist < dist * 1.2) { // 敌蛇更近或距离相近
                    // 评估我方蛇在竞争中的优势
                    bool has_length_advantage = self.length > snake.length + 2;
                    bool has_position_advantage = Strategy::countSafeExits(state, item.pos) >= 2;
                    
                    // 根据优势调整惩罚
                    if (!has_length_advantage && !has_position_advantage)
                        competition_penalty += 0.4; // 重大劣势
                    else if (has_length_advantage || has_position_advantage)
                        competition_penalty += 0.2; // 部分劣势
                }
            }
        }
        }
        
        // 应用竞争惩罚
        double competition_factor = max(0.3, 1.0 - competition_penalty);
        
        // 最终价值计算
        double final_value = adjusted_value * (1.0 + distance_factor * distance_weight + cluster_bonus) * competition_factor;
        
        // 收集候选食物
        candidate_foods.push_back({item, final_value});
    }
    
    // 如果没有候选食物
    if (candidate_foods.empty()) {
        return false;
    }
    
    // 按调整后的价值排序
    sort(candidate_foods.begin(), candidate_foods.end(), 
         [](const pair<Item, double> &a, const pair<Item, double> &b) {
             return a.second > b.second;
         });
    
    // 选择最佳食物
    const Item &best_food = candidate_foods[0].first;
        
        // 计算前往食物的首选方向
        vector<Direction> preferred_dirs;
        
            // 计算曼哈顿距离的各分量
    int dx = abs(head.x - best_food.pos.x);
    int dy = abs(head.y - best_food.pos.y);
    
    // 优先选择能消除更多距离的方向
    if (dx >= dy) {
        // 水平距离更大或相等，先处理水平
        if (head.x > best_food.pos.x) preferred_dirs.push_back(Direction::LEFT);
        else if (head.x < best_food.pos.x) preferred_dirs.push_back(Direction::RIGHT);
      
        if (head.y > best_food.pos.y) preferred_dirs.push_back(Direction::UP);
        else if (head.y < best_food.pos.y) preferred_dirs.push_back(Direction::DOWN);
    } else {
        // 垂直距离更大，先处理垂直
        if (head.y > best_food.pos.y) preferred_dirs.push_back(Direction::UP);
        else if (head.y < best_food.pos.y) preferred_dirs.push_back(Direction::DOWN);
      
        if (head.x > best_food.pos.x) preferred_dirs.push_back(Direction::LEFT);
        else if (head.x < best_food.pos.x) preferred_dirs.push_back(Direction::RIGHT);
    }
    
    // 获取路径安全性评估
    auto best_path_safety = Strategy::evaluatePathSafety(state, head, best_food.pos);
    
    // 使用优化后的护盾决策 - 根据食物价值和路径安全性决定是否使用护盾
    bool should_use_shield = false;
    
    // 护盾策略：仅在必死情况下使用
    unordered_set<Direction> illegals = illegalMove(state);
    if (illegals.size() >= 3) { // 至少有3个方向不可走
        // 使用新的护盾评估函数，但仅考虑生存因素
        should_use_shield = Strategy::shouldUseShield(
            state, 
            -2000, // 极高风险
            0,     // 不考虑食物价值
            false, // 不是安全区紧急情况
            true   // 是生死攸关情形
        );
    }
    
    // 使用增强版方向选择逻辑
    Direction dir;
    
    // 尝试使用A*寻路
    auto path = Strategy::findPath(state, head, best_food.pos);
    if (!path.empty()) {
        // 获取下一步位置
        const Point &next_pos = path[0];
        
        // 计算移动方向
        if (next_pos.y < head.y) dir = Direction::UP;
        else if (next_pos.y > head.y) dir = Direction::DOWN;
        else if (next_pos.x < head.x) dir = Direction::LEFT;
        else dir = Direction::RIGHT;
        
        // 检查移动是否合法
        unordered_set<Direction> illegals = illegalMove(state);
        if (illegals.count(dir) == 0) {
            moveDirection = dir;
            
            // 如果应该使用护盾，将其信息传递出去
            if (should_use_shield) {
                moveDirection = Direction::UP; // 特殊标记
                return false; // 特殊返回值，需要在外部处理护盾
            }
            
            return true;
        }
    }
    
    // 对于紧邻食物，A*失败时尝试直接移动
    if (abs(head.y - best_food.pos.y) + abs(head.x - best_food.pos.x) == 1 && !preferred_dirs.empty()) {
        dir = preferred_dirs[0]; // 紧邻食物时直接使用首选方向
      
        // 检查该方向是否合法
        unordered_set<Direction> check_illegals = illegalMove(state);
        if (check_illegals.count(dir) > 0) {
            // 如果不合法，退回到传统方法
            dir = chooseBestDirection(state, preferred_dirs);
        }
    } else {
        // 非紧邻食物，使用传统方法
        dir = chooseBestDirection(state, preferred_dirs);
    }
    
    if (dir != Direction::UP || state.get_self().direction == 1) {
        moveDirection = dir;
        
        // 只在必死情况下使用护盾，且不在最后5tick
        if (should_use_shield && state.remaining_ticks > 5) {
            // 检查是否确实无路可走
            unordered_set<Direction> all_illegals = illegalMove(state);
            if (all_illegals.size() >= 3) { // 只有1个方向可走或没有方向可走
            moveDirection = Direction::UP; // 特殊标记
            return false; // 特殊返回值，需要在外部处理护盾
            }
        }
        
        return true;
    }
    
    return false;  // 没有找到满足条件的食物
}

// 前向声明统一食物处理函数
bool processFoodWithDynamicPriority(const GameState &state, Direction& moveDirection);

// 前向声明初始护盾阶段处理函数
int handleInitialShieldPhase(const GameState &state, int current_tick);

// 修改judge函数，整合所有安全性改进
int judge(const GameState &state)
{
    // 更新目标锁定状态
    lock_on_target(state);
  
    // 获取自适应配置
    Strategy::RiskAdaptiveConfig config = Strategy::getAdaptiveConfig(state);
  
    const auto &self = state.get_self();
    const auto &head = self.get_head();
  
    // 获取当前游戏阶段信息
    int current_tick = MAX_TICKS - state.remaining_ticks;
    
    // 初始护盾阶段判定 - 游戏前10刻且有护盾
    bool is_initial_shield_phase = (current_tick < 10 && self.shield_time > 0);
  
    if (is_initial_shield_phase) {
        return handleInitialShieldPhase(state, current_tick);
    }
  
    int ticks_to_shrink = state.next_shrink_tick - current_tick;
    bool is_near_shrink = (ticks_to_shrink >= 0 && ticks_to_shrink <= 20);
    bool is_emergency_shrink = is_near_shrink && ticks_to_shrink <= 5;
    
    // 处理宝箱和钥匙的目标锁定
    bool is_special_target = (self.has_key && is_chest_target && current_target.x != -1 && current_target.y != -1) ||
                            (!self.has_key && is_key_target && current_target.x != -1 && current_target.y != -1);
    
    // 特殊目标逻辑
    if (is_special_target) {
        bool is_chest = self.has_key && is_chest_target;
        
        // 评估目标安全性，确保我们追求的是值得的目标
        Strategy::TargetEvaluation target_eval = Strategy::evaluateChestKeyTarget(
            state, current_target, is_chest ? 100.0 : 40.0, is_chest);
        
        // 如果目标评估非常不安全，或者价值降低了太多，考虑放弃
        if (target_eval.value < (is_chest ? 20.0 : 10.0)) {
            // 这种情况下，让蛇执行常规觅食行为
            current_target = {-1, -1};
            target_value = 0;
            if (is_chest) is_chest_target = false;
            else is_key_target = false;
        } 
        else {
            // 使用路径安全评估框架进行增强的路径规划
            auto path_safety = Strategy::evaluatePathSafety(state, head, current_target);
            auto path = Strategy::findPath(state, head, current_target);
            
            // 计算特殊目标的实际价值
            int expected_gain = is_chest ? 100 : 40; // 宝箱或钥匙的基础价值
            
            // 根据路径安全程度决定是否使用护盾 - 使用优化的护盾决策
            bool should_use_shield = false;
            
            // 如果路径不安全但必须获取目标（如钥匙即将消失）
            if (!path_safety.is_safe && path_safety.safety_score > config.shield_activation_threshold) {
                // 使用新的护盾评估函数
                should_use_shield = Strategy::shouldUseShield(
                    state, 
                    path_safety.safety_score, 
                    expected_gain,
                    false, // 不是安全区紧急情况
                    false  // 不是生死攸关情形
                );
            }
            
            // 如果找到了路径
            if (!path.empty()) {
                // 获取下一步位置
                const Point &next_pos = path[0];
                
                // 计算移动方向
                Direction move_dir;
                if (next_pos.y < head.y) move_dir = Direction::UP;
                else if (next_pos.y > head.y) move_dir = Direction::DOWN;
                else if (next_pos.x < head.x) move_dir = Direction::LEFT;
                else move_dir = Direction::RIGHT;
                
                // 检查移动是否合法
                unordered_set<Direction> illegals = illegalMove(state);
                
                // 如果下一步是合法的
                if (illegals.count(move_dir) == 0) {
                    // 即使有宝箱/钥匙，也只在必死情况下使用护盾，且不在最后5tick
                    if (should_use_shield && self.shield_cd == 0 && self.score >= 20 && state.remaining_ticks > 5) {
                        // 检查是否确实是生存必要的情况
                        bool is_truly_necessary = false;
                        
                        // 计算其他方向是否全部导致死亡
                        unordered_set<Direction> illegals = illegalMove(state);
                        int legal_dirs = 4 - illegals.size();
                        
                        if (legal_dirs <= 1) {
                            is_truly_necessary = true; // 只有这一个方向或者无路可走
                        }
                        
                        if (is_truly_necessary) {
                        return SHIELD_COMMAND;
                        }
                    }
                    return Utils::dir2num(move_dir);
                }
                
                // 如果A*路径的第一步不合法，回退到传统方法
        // 计算前往目标的首选方向
        vector<Direction> preferred_dirs;
        
                // 添加首选方向
        if (head.x > current_target.x) preferred_dirs.push_back(Direction::LEFT);
        else if (head.x < current_target.x) preferred_dirs.push_back(Direction::RIGHT);
                if (head.y > current_target.y) preferred_dirs.push_back(Direction::UP);
                else if (head.y < current_target.y) preferred_dirs.push_back(Direction::DOWN);
                
                Direction best_dir = chooseBestDirection(state, preferred_dirs);
                if (best_dir != Direction::UP || state.get_self().direction == 1) {
                    // 只在必死情况下使用护盾
                    if (should_use_shield && self.shield_cd == 0 && self.score >= 20 && state.remaining_ticks > 5) {
                        // 检查是否确实无路可走
                        unordered_set<Direction> all_illegals = illegalMove(state);
                        if (all_illegals.size() >= 3) { // 至少有3个方向不可走
                        return SHIELD_COMMAND;
                        }
                    }
                    return Utils::dir2num(best_dir);
                }
            } else {
                // 路径不存在，但仍然尝试向目标方向移动
                vector<Direction> preferred_dirs;
                
                // 确定方向优先级 - 同时考虑水平和垂直方向
                if (head.x > current_target.x) preferred_dirs.push_back(Direction::LEFT);
                else if (head.x < current_target.x) preferred_dirs.push_back(Direction::RIGHT);
        if (head.y > current_target.y) preferred_dirs.push_back(Direction::UP);
        else if (head.y < current_target.y) preferred_dirs.push_back(Direction::DOWN);
        
                // 使用增强的方向选择
        Direction best_dir = chooseBestDirection(state, preferred_dirs);
        if (best_dir != Direction::UP || state.get_self().direction == 1) {
                    // 只在必死情况下使用护盾
                    if (should_use_shield && self.shield_cd == 0 && self.score >= 20 && state.remaining_ticks > 5) {
                        // 检查是否确实无路可走
                        unordered_set<Direction> all_illegals = illegalMove(state);
                        if (all_illegals.size() >= 3) { // 至少有3个方向不可走
                        return SHIELD_COMMAND;
                        }
                    }
            return Utils::dir2num(best_dir);
                }
            }
        }
    }
    
    // 使用统一的动态优先级食物处理系统
    Direction food_dir = Direction::UP;  // 初始化为默认值
            
    // 4. 改进: 使用统一的食物处理函数，根据蛇长度和游戏阶段动态调整策略
    if (processFoodWithDynamicPriority(state, food_dir)) {
        return Utils::dir2num(food_dir);
    }
    
    // 如果新的食物处理系统未返回结果，尝试使用普通近距离食物检测作为备用策略
    if (enhancedFoodProcessByPriority(state, 0, 4, food_dir)) {
        return Utils::dir2num(food_dir);
    }
    
    // 安全区收缩策略
    if (is_near_shrink) {
        // 如果即将收缩且蛇不在下一个安全区
        if (!Utils::willDisappearInShrink(state, head.x, head.y)) {
            // 判断蛇是否在下一个安全区内
            bool snake_in_next_zone = (head.x >= state.next_safe_zone.x_min && 
                                     head.x <= state.next_safe_zone.x_max &&
                                     head.y >= state.next_safe_zone.y_min && 
                                     head.y <= state.next_safe_zone.y_max);
            
            if (!snake_in_next_zone) {
                // 计算到下一个安全区的最短距离方向
                vector<Direction> safe_zone_dirs;
                
                // 水平方向
                if (head.x < state.next_safe_zone.x_min) {
                    safe_zone_dirs.push_back(Direction::RIGHT);
                } else if (head.x > state.next_safe_zone.x_max) {
                    safe_zone_dirs.push_back(Direction::LEFT);
                }
                
                // 垂直方向
                if (head.y < state.next_safe_zone.y_min) {
                    safe_zone_dirs.push_back(Direction::DOWN);
                } else if (head.y > state.next_safe_zone.y_max) {
                    safe_zone_dirs.push_back(Direction::UP);
                }
                
                // 如果有明确方向指向安全区
                if (!safe_zone_dirs.empty()) {
                    Direction safe_dir = chooseBestDirection(state, safe_zone_dirs);
                    
                    // 计算到安全区的最短距离
                    int dist_to_safe_zone = INT_MAX;
                    
                    if (head.x < state.next_safe_zone.x_min) {
                        dist_to_safe_zone = min(dist_to_safe_zone, state.next_safe_zone.x_min - head.x);
                    } else if (head.x > state.next_safe_zone.x_max) {
                        dist_to_safe_zone = min(dist_to_safe_zone, head.x - state.next_safe_zone.x_max);
                    }
                    
                    if (head.y < state.next_safe_zone.y_min) {
                        dist_to_safe_zone = min(dist_to_safe_zone, state.next_safe_zone.y_min - head.y);
                    } else if (head.y > state.next_safe_zone.y_max) {
                        dist_to_safe_zone = min(dist_to_safe_zone, head.y - state.next_safe_zone.y_max);
                    }
                    
                    // 安全区收缩极端情况：只在收缩点到达时且无法安全到达时使用护盾
                    bool should_use_shield = false;
                    if (is_emergency_shrink && ticks_to_shrink <= 1) {
                        // 计算是否真的无法到达安全区
                        if (dist_to_safe_zone > ticks_to_shrink + 1) {
                            // 检查是否有合法路径
                            unordered_set<Direction> illegals = illegalMove(state);
                            if (illegals.size() >= 3) { // 几乎无路可走
                                should_use_shield = true;
                            }
                        }
                    }
                    
                    if (should_use_shield && self.shield_cd == 0 && self.score >= 20 && state.remaining_ticks > 5) {
                        return SHIELD_COMMAND;
                    }
                    
                    if (safe_dir != Direction::UP || state.get_self().direction == 1) {
                        return Utils::dir2num(safe_dir);
                    }
                }
            }
        }
    }
    
    // 最后尝试再次使用动态优先级系统，但降低阈值
    if (processFoodWithDynamicPriority(state, food_dir)) {
        return Utils::dir2num(food_dir);
    }
    
    // 默认行为 - 使用自适应配置评估所有方向并选择最佳
    unordered_set<Direction> illegals = illegalMove(state);
    vector<Direction> legalMoves;
    
    for (auto dir : validDirections) {
        if (illegals.count(dir) == 0) legalMoves.push_back(dir);
    }
    
    if (legalMoves.empty()) {
        // 如果没有合法移动但可以开护盾 - 生死攸关情况，但仍检查是否在最后5tick
        if (self.shield_cd == 0 && self.score >= 20 && state.remaining_ticks > 5) {
            return SHIELD_COMMAND;
        }
        return Utils::dir2num(Direction::UP); // 默认上方向
    }
    
    double maxF = -4000;
    Direction best = legalMoves[0];
    
    for (auto dir : legalMoves) {
        const auto [y, x] = Utils::nextPos({head.y, head.x}, dir);
        
        // 使用自适应配置的评估函数
        double eval = Strategy::evalWithAdaptiveConfig(state, y, x, head.y, head.x);
        
        if (eval > maxF) {
            maxF = eval;
            best = dir;
        }
    }
    
    return Utils::dir2num(best);
}

// 添加统一的食物处理函数，实现动态优先级和智能食物评估
bool processFoodWithDynamicPriority(const GameState &state, Direction& moveDirection) {
    const auto &self = state.get_self();
    const auto &head = self.get_head();
    
    // 获取自适应配置
    Strategy::RiskAdaptiveConfig config = Strategy::getAdaptiveConfig(state);
    
    // 定义优先级结构
    struct FoodPriority {
        int minValue;
        int maxRange;
        double priority_weight;
    };
    
    // 动态调整优先级参数
    int game_phase = MAX_TICKS - state.remaining_ticks;
    bool late_game = (game_phase > 180);
    
    // 动态优先级配置
    vector<FoodPriority> priorities;
    
    // 调整优先级配置，提高尸体优先级
    if (self.length < 8) { // 短蛇
        if (late_game) {
            priorities = {
                {8, 6, 1.4},    // 高价值尸体(>=8)搜索范围6格，权重1.4
                {5, 4, 1.2},    // 中价值尸体(>=5)搜索范围4格，权重1.2
                {2, 2, 1.1},    // 低价值尸体(>=2)搜索范围2格，权重1.1
                {0, 5, 0.6}     // 普通食物搜索范围5格，权重0.6
            };
        } else {
            // 前中期短蛇对尸体更积极
            priorities = {
                {8, 7, 1.5},    // 高价值尸体权重更高
                {4, 5, 1.3},    // 扩大中价值尸体搜索范围
                {1, 3, 1.1},    // 极近距离任意食物
                {0, 6, 0.7}     // 普通食物
            };
        }
    } else { // 长蛇
        if (late_game) {
            priorities = {
                {10, 5, 1.3},   // 高价值尸体
                {6, 3, 1.1},    // 中价值尸体
                {3, 2, 0.9},    // 低价值尸体
                {0, 4, 0.5}     // 普通食物
            };
        } else {
            priorities = {
                {8, 6, 1.3},    // 高价值尸体
                {5, 4, 1.1},    // 中价值尸体
                {2, 3, 0.9},    // 低价值尸体
                {0, 5, 0.6}     // 普通食物
            };
        }
    }
    
    // 统一处理所有食物
    vector<pair<Item, double>> all_candidates;
    
    for (const auto &item : state.items) {
        // 基础过滤
        if (item.value == -2) continue; // 跳过陷阱
        
        // 计算距离
        int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
        int effective_max_range = 12; // 默认最大搜索范围
        
        // 适度扩大尸体搜索范围
        if (item.value > 0) { // 尸体
            if (item.value >= 10)
                effective_max_range = 16; // 高价值尸体搜索范围扩大到16
            else
                effective_max_range = 14; // 普通尸体搜索范围扩大到14
              
            // 游戏阶段调整 - 中期最积极寻找尸体
            if (game_phase > 100 && game_phase < 180)
                effective_max_range += 1;
        }
        
        if (dist > effective_max_range) continue; // 应用调整后的搜索范围
        
        // 使用通用的目标可达性检测
        if (!Utils::isTargetReachable(state, item.pos, item.lifetime)) {
            continue; // 跳过不可达的食物
        }
        
        // 评估路径安全性
        auto path_safety = Strategy::evaluatePathSafety(state, head, item.pos, min(dist, 8));
        
            // 计算食物的基础价值 - 适度提升尸体价值
    double base_value;
    if (item.value > 0) { // 尸体
        // 根据游戏阶段动态调整尸体价值系数
        double corpse_multiplier;
        if (game_phase < 100) 
            corpse_multiplier = 8.0;      // 早期
        else if (game_phase < 180) 
            corpse_multiplier = 12.0;     // 中期更看重尸体
        else 
            corpse_multiplier = 10.0;     // 后期略微降低
          
        // 根据蛇长度微调 - 短蛇更需要快速成长
        if (self.length < 8) corpse_multiplier *= 1.2;
      
        // 根据尸体价值进一步调整 - 高价值尸体更值得争取
        if (item.value >= 10)
            corpse_multiplier *= 1.25;
          
        base_value = item.value * corpse_multiplier;
    } else if (item.value == -1) {
        base_value = 3; // 增长豆价值保持不变
    } else {
        base_value = 1; // 普通食物
    }
        
        // 1. 改进: 动态风险因子计算
        double dynamic_risk_threshold = config.path_safety_threshold * 
                                       (1 + 0.5 * (self.length > 10 ? 0.5 : 0)); // 蛇越长，风险容忍越高
        
        double adjusted_value = base_value;
        
        // 根据路径安全性调整价值
        if (!path_safety.is_safe) {
            // 为尸体适度提高风险容忍度
            double risk_threshold_factor = 1.0;
            double risk_penalty_factor = 1.0;
          
            if (item.value > 0) { // 尸体
                // 根据尸体价值调整风险容忍度
                if (item.value >= 10) {
                    risk_threshold_factor = 1.2; // 高价值尸体阈值提高20%
                    risk_penalty_factor = 0.85;  // 风险惩罚降低15%
                } else {
                    risk_threshold_factor = 1.1; // 普通尸体阈值提高10%
                    risk_penalty_factor = 0.9;   // 风险惩罚降低10%
                }
              
                // 考虑蛇长度 - 短蛇更保守
                if (self.length < 8) {
                    risk_threshold_factor = max(1.0, risk_threshold_factor - 0.05);
                    risk_penalty_factor = min(1.0, risk_penalty_factor + 0.05);
                }
              
                // 考虑游戏阶段 - 后期更保守
                if (game_phase > 180) {
                    risk_threshold_factor = max(1.0, risk_threshold_factor - 0.1);
                    risk_penalty_factor = min(1.0, risk_penalty_factor + 0.1);
                }
            }
          
            // 应用调整后的风险评估
            double adjusted_risk_threshold = dynamic_risk_threshold * risk_threshold_factor;
            double risk_factor = 1.0 - min(0.8 * risk_penalty_factor, -path_safety.safety_score / 2000.0);
            adjusted_value *= risk_factor;
          
            // 梯度惩罚
            if (path_safety.safety_score < adjusted_risk_threshold) {
                double severity = min(1.0, (adjusted_risk_threshold - path_safety.safety_score) / 500.0);
                adjusted_value *= (0.8 - 0.6 * severity * risk_penalty_factor);
            }
        }
        
        // 安全区收缩检查
        int current_tick = MAX_TICKS - state.remaining_ticks;
        int ticks_to_shrink = state.next_shrink_tick - current_tick;
        bool is_near_shrink = (ticks_to_shrink >= 0 && ticks_to_shrink <= 20);
        
        if (is_near_shrink) {
            bool in_next_zone = item.pos.x >= state.next_safe_zone.x_min && 
                               item.pos.x <= state.next_safe_zone.x_max &&
                               item.pos.y >= state.next_safe_zone.y_min && 
                               item.pos.y <= state.next_safe_zone.y_max;
            
            if (!in_next_zone) {
                adjusted_value *= 0.3; // 不在下一安全区的食物价值大幅降低
            } else {
                adjusted_value *= 1.2; // 在下一安全区的食物价值提高
            }
        }
        
        // 2. 改进: 食物密集区识别
        int nearby_food_count = 0;
        for (const auto &other_item : state.items) {
            if (&other_item != &item && // 不是同一个食物
                abs(other_item.pos.y - item.pos.y) + abs(other_item.pos.x - item.pos.x) <= 4) {
                nearby_food_count++;
            }
        }
        
        // 调整距离权重，较近食物更高权重
        double distance_weight = self.length < 6 ? 0.8 : 0.6; // 短蛇更重视近距离食物
        double distance_factor = pow(1.0 - (double)dist / 12.0, 1.5); // 使用指数函数增强近距离偏好
        if (dist == 1) distance_factor *= 1.8; // 紧邻食物(dist=1)显著提升权重
        double cluster_bonus = nearby_food_count * 0.15; // 食物密集区加分
        
        // 3. 改进: 竞争分析 - 对尸体优化竞争评估
        double competition_penalty = 0;
        for (const auto &snake : state.snakes) {
            if (snake.id != MYID && snake.id != -1) {
                int enemy_dist = abs(snake.get_head().y - item.pos.y) + abs(snake.get_head().x - item.pos.x);
              
                // 针对尸体的特殊竞争评估
                if (item.value > 0) { // 尸体
                    // 紧邻尸体无竞争惩罚，确保优先获取
                    if (dist == 1) {
                        competition_penalty = 0; 
                        continue; // 跳过后续竞争评估
                    }
                    // 计算距离差异 - 正值表示我们更近
                    int distance_advantage = enemy_dist - dist;
                  
                    // 评估优势因素
                    bool has_length_advantage = self.length >= snake.length;
                    bool has_shield_advantage = self.shield_time > 0 && self.shield_time >= 3;
                    bool is_high_value = item.value >= 8;
                  
                    // 如果我方更近或距离相近
                    if (distance_advantage >= 0) {
                        // 我方占优，几乎不惩罚
                        competition_penalty += 0.05;
                    }
                    // 敌方略微更近
                    else if (distance_advantage >= -2) {
                        // 如果有优势或尸体价值高，适度惩罚
                        if (has_length_advantage || has_shield_advantage || is_high_value) {
                            competition_penalty += 0.15;
                        } else {
                            competition_penalty += 0.3;
                        }
                    }
                    // 敌方明显更近
                    else if (distance_advantage >= -4) {
                        // 如果同时具有长度和护盾优势，或是极高价值尸体，仍然考虑争夺
                        if ((has_length_advantage && has_shield_advantage) || item.value >= 10) {
                            competition_penalty += 0.4;
                        } else {
                            competition_penalty += 0.6;
                        }
                    }
                    // 敌方远远更近，几乎放弃
                    else {
                        competition_penalty += 0.8;
                    }
                } else { // 非尸体食物保持原有逻辑
                    if (enemy_dist < dist * 1.2) { // 敌蛇更近或距离相近
                        // 评估我方蛇在竞争中的优势
                        bool has_length_advantage = self.length > snake.length + 2;
                        bool has_position_advantage = Strategy::countSafeExits(state, item.pos) >= 2;
                      
                        // 根据优势调整惩罚
                        if (!has_length_advantage && !has_position_advantage)
                            competition_penalty += 0.4; // 重大劣势
                        else if (has_length_advantage || has_position_advantage)
                            competition_penalty += 0.2; // 部分劣势
                    }
                }
            }
        }
        
        // 应用竞争惩罚 - 确保至少保留30%价值
        double competition_factor = max(0.3, 1.0 - competition_penalty);
        
        // 最终价值计算
        double final_value = adjusted_value * (1.0 + distance_factor * distance_weight + cluster_bonus) * competition_factor;
        
        // 应用动态优先级
        double priority_multiplier = 0.5; // 默认低优先级
        for (const auto &p : priorities) {
            if (item.value >= p.minValue && dist <= p.maxRange) {
                priority_multiplier = p.priority_weight;
                break;
            }
        }
        
        // 将计算好的食物加入统一列表
        all_candidates.push_back({item, final_value * priority_multiplier});
    }
    
    // 如果没有候选食物
    if (all_candidates.empty()) {
        return false;
    }
    
    // 排序并选择最佳食物
    sort(all_candidates.begin(), all_candidates.end(), 
         [](const pair<Item, double> &a, const pair<Item, double> &b) {
             return a.second > b.second;
         });
    
    // 选择最佳食物
    const Item &best_food = all_candidates[0].first;
    
            // 计算前往食物的首选方向
        vector<Direction> preferred_dirs;
        
        // 计算曼哈顿距离的各分量
        int dx = abs(head.x - best_food.pos.x);
        int dy = abs(head.y - best_food.pos.y);
        
        // 优先选择能消除更多距离的方向
        if (dx >= dy) {
            // 水平距离更大或相等，先处理水平
            if (head.x > best_food.pos.x) preferred_dirs.push_back(Direction::LEFT);
            else if (head.x < best_food.pos.x) preferred_dirs.push_back(Direction::RIGHT);
          
            if (head.y > best_food.pos.y) preferred_dirs.push_back(Direction::UP);
            else if (head.y < best_food.pos.y) preferred_dirs.push_back(Direction::DOWN);
        } else {
            // 垂直距离更大，先处理垂直
            if (head.y > best_food.pos.y) preferred_dirs.push_back(Direction::UP);
            else if (head.y < best_food.pos.y) preferred_dirs.push_back(Direction::DOWN);
          
            if (head.x > best_food.pos.x) preferred_dirs.push_back(Direction::LEFT);
            else if (head.x < best_food.pos.x) preferred_dirs.push_back(Direction::RIGHT);
        }
    
    // 护盾策略：仅在必死情况下使用
    bool should_use_shield = false;
    
    // 检查是否处于绝境（几乎无路可走）
    unordered_set<Direction> illegals = illegalMove(state);
    if (illegals.size() >= 3) { // 至少有3个方向不可走
        // 使用新的护盾评估函数，但仅考虑生存因素
        should_use_shield = Strategy::shouldUseShield(
            state, 
            -2000, // 极高风险
            0,     // 不考虑食物价值
            false, // 不是安全区紧急情况
            true   // 是生死攸关情形
        );
    }
    
    // 使用增强版方向选择逻辑
    Direction dir;
    
    // 尝试使用A*寻路
    auto path = Strategy::findPath(state, head, best_food.pos);
    if (!path.empty()) {
        // 获取下一步位置
        const Point &next_pos = path[0];
        
        // 计算移动方向
        if (next_pos.y < head.y) dir = Direction::UP;
        else if (next_pos.y > head.y) dir = Direction::DOWN;
        else if (next_pos.x < head.x) dir = Direction::LEFT;
        else dir = Direction::RIGHT;
        
        // 检查移动是否合法
        if (illegals.count(dir) == 0) {
            moveDirection = dir;
            
            // 只在必死情况下使用护盾，且不在最后5tick
            if (should_use_shield && self.shield_cd == 0 && self.score >= 20 && state.remaining_ticks > 5) {
                // 检查是否确实无路可走
                unordered_set<Direction> all_illegals = illegalMove(state);
                if (all_illegals.size() >= 3) { // 只有1个方向可走或没有方向可走
                    moveDirection = Direction::UP; // 特殊标记
                    return false; // 特殊返回值，需要在外部处理护盾
                }
            }
            
            return true;
        }
    }
    
    // A*失败时退回到传统方法
    dir = chooseBestDirection(state, preferred_dirs);
    if (dir != Direction::UP || state.get_self().direction == 1) {
        moveDirection = dir;
        
        // 只在必死情况下使用护盾，且不在最后5tick
        if (should_use_shield && self.shield_cd == 0 && self.score >= 20 && state.remaining_ticks > 5) {
            // 检查是否确实无路可走
            unordered_set<Direction> all_illegals = illegalMove(state);
            if (all_illegals.size() >= 3) { // 只有1个方向可走或没有方向可走
                moveDirection = Direction::UP; // 特殊标记
                return false; // 特殊返回值，需要在外部处理护盾
            }
        }
        
        return true;
    }
    
    return false;  // 没有找到满足条件的食物
}

// 初始护盾阶段处理函数
int handleInitialShieldPhase(const GameState &state, int current_tick) {
    const auto &self = state.get_self();
    const auto &head = self.get_head();
  
    // 阶段1: 攻击性获取(前7刻) - 直奔最高价值目标
    if (current_tick < 7) {
        // 构建加权评分的候选食物列表
        vector<pair<Item, double>> candidate_foods;
      
        for (const auto &item : state.items) {
            // 跳过陷阱
            if (item.value == -2) continue;
          
            int dist = abs(head.y - item.pos.y) + abs(head.x - item.pos.x);
          
            // 计算基础价值
            double base_value = 0;
            if (item.value > 0) {  // 尸体
                base_value = item.value * 15;  // 尸体价值权重高
            } else if (item.value == -1) {  // 增长豆
                base_value = 8;  // 增长豆也有价值
            } else {
                base_value = 1;  // 普通食物基础价值低
            }
          
            // 距离因子 - 越近越好，指数衰减
            double distance_factor = 10.0 / (1.0 + dist * 0.5);
          
            // 计算最终评分
            double score = base_value * distance_factor;
          
            // 排除过远的食物
            if (dist <= 12) {  // 最大搜索半径
                candidate_foods.push_back({item, score});
            }
        }
      
        // 如果找到了候选食物，选择评分最高的
        if (!candidate_foods.empty()) {
            // 按评分排序
            sort(candidate_foods.begin(), candidate_foods.end(),
                [](const pair<Item, double> &a, const pair<Item, double> &b) {
                    return a.second > b.second;
                });
          
            const Item &best_food = candidate_foods[0].first;
          
            // 直线路径策略 - 完全不考虑安全性，直奔目标
            vector<Direction> preferred_dirs;
          
            // 先考虑垂直方向(优先垂直移动)
            if (head.y > best_food.pos.y) preferred_dirs.push_back(Direction::UP);
            else if (head.y < best_food.pos.y) preferred_dirs.push_back(Direction::DOWN);
          
            // 再考虑水平方向
            if (head.x > best_food.pos.x) preferred_dirs.push_back(Direction::LEFT);
            else if (head.x < best_food.pos.x) preferred_dirs.push_back(Direction::RIGHT);
          
            // 有方向就走，无视障碍
            if (!preferred_dirs.empty()) {
                Direction best_dir = preferred_dirs[0];
              
                // 检查第一选择是否会导致出界或撞墙
                const auto [ny, nx] = Utils::nextPos({head.y, head.x}, best_dir);
                if (!Utils::boundCheck(ny, nx) || map_item[ny][nx] == -4) {
                    // 如果有第二选择且合法，使用第二选择
                    if (preferred_dirs.size() > 1) {
                        const auto [ny2, nx2] = Utils::nextPos({head.y, head.x}, preferred_dirs[1]);
                        if (Utils::boundCheck(ny2, nx2) && map_item[ny2][nx2] != -4) {
                            best_dir = preferred_dirs[1];
                        }
                    }
                }
              
                return Utils::dir2num(best_dir);
            }
        }
    }
    // 阶段2: 安全撤离(后3刻) - 前往空旷安全区域
    else {
        // 评估每个方向的安全性和空旷度
        struct DirectionScore {
            Direction dir;
            int exits;        // 出口数量
            double openness;  // 空旷度评分
            int snake_count;  // 周围蛇的数量
            bool has_food;    // 是否有食物
          
            // 综合评分
            double score() const {
                double base = openness * 2.0 + exits * 1.5 - snake_count * 3.0;
                return has_food ? base * 1.2 : base;  // 有食物加成
            }
        };
      
        vector<DirectionScore> dir_scores;
      
        // 评估所有可能的方向
        for (auto dir : validDirections) {
            const auto [ny, nx] = Utils::nextPos({head.y, head.x}, dir);
          
            // 基础合法性检查
            if (!Utils::boundCheck(ny, nx) || map_item[ny][nx] == -4) continue;
            if (nx < state.current_safe_zone.x_min || nx > state.current_safe_zone.x_max ||
                ny < state.current_safe_zone.y_min || ny > state.current_safe_zone.y_max) continue;
          
            // 计算出口数量 - 护盾消失后的安全移动选择
            int exits = 0;
            for (auto exit_dir : validDirections) {
                const auto [ey, ex] = Utils::nextPos({ny, nx}, exit_dir);
                if (Utils::boundCheck(ey, ex) && map_item[ey][ex] != -4 && map_snake[ey][ex] != -5) {
                    exits++;
                }
            }
          
            // 计算空旷度 - 从该点BFS探索几步，计算可达点的数量
            double openness = 0;
            unordered_set<string> visited;
            queue<pair<pair<int, int>, int>> q; // 位置和距离
            q.push({{ny, nx}, 1});
            visited.insert(Utils::idx2str({ny, nx}));
          
            while (!q.empty()) {
                auto [pos, depth] = q.front();
                q.pop();
              
                if (depth > 4) continue; // 只探索4步以内
              
                // 深度越浅的点贡献越大
                openness += (5 - depth) * 0.5;
              
                // 继续探索
                for (auto next_dir : validDirections) {
                    const auto [next_y, next_x] = Utils::nextPos(pos, next_dir);
                    string key = Utils::idx2str({next_y, next_x});
                  
                    if (Utils::boundCheck(next_y, next_x) && 
                        map_item[next_y][next_x] != -4 && // 不是墙
                        visited.find(key) == visited.end()) { // 未访问过
                      
                        q.push({{next_y, next_x}, depth + 1});
                        visited.insert(key);
                    }
                }
            }
          
            // 计算附近蛇的数量 - 避开竞争区域
            int snake_count = 0;
            for (const auto &snake : state.snakes) {
                if (snake.id != MYID && snake.id != -1) { // 不是自己且有效
                    int dist = abs(snake.get_head().y - ny) + abs(snake.get_head().x - nx);
                    if (dist <= 5) { // 5步以内的蛇
                        snake_count++;
                        // 特别惩罚非常近的蛇
                        if (dist <= 2) {
                            snake_count += 2;
                        }
                    }
                }
            }
          
            // 检查是否有食物 - 在安全前提下优先考虑有食物的位置
            bool has_food = map_item[ny][nx] > 0 || map_item[ny][nx] == -1;
          
            // 只有当出口至少2个时才考虑该方向
            if (exits >= 2) {
                dir_scores.push_back({dir, exits, openness, snake_count, has_food});
            }
        }
      
        // 如果有可行方向，选择综合评分最高的
        if (!dir_scores.empty()) {
            // 按综合评分排序
            sort(dir_scores.begin(), dir_scores.end(), 
                [](const DirectionScore &a, const DirectionScore &b) {
                    return a.score() > b.score();
                });
          
            // 选择评分最高的方向
            return Utils::dir2num(dir_scores[0].dir);
        }
      
        // 如果没有足够安全的位置，回退到基本安全检查
        for (auto dir : validDirections) {
            const auto [ny, nx] = Utils::nextPos({head.y, head.x}, dir);
            if (Utils::boundCheck(ny, nx) && map_item[ny][nx] != -4 &&
                nx >= state.current_safe_zone.x_min && nx <= state.current_safe_zone.x_max &&
                ny >= state.current_safe_zone.y_min && ny <= state.current_safe_zone.y_max) {
                return Utils::dir2num(dir); // 任何在界内且不是墙的位置
            }
        }
    }
  
    // 如果所有特殊策略都无法应用，回退到标准行为
    Direction food_dir = Direction::UP;
    if (processFoodWithDynamicPriority(state, food_dir)) {
        return Utils::dir2num(food_dir);
    }
  
    // 最终回退 - 使用标准选择逻辑
    return Utils::dir2num(chooseBestDirection(state));
}

int main() {
  // 读取当前 tick 的所有游戏状态
  GameState current_state;
  read_game_state(current_state);

  int decision = judge(current_state);

  // 输出决策
  cout << decision << endl;
  
  return 0;
}