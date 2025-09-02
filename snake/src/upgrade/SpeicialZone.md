# 批次2: 死胡同检测与评估

**目标**: 识别死胡同和可能导致蛇被困的区域，避免蛇陷入无法脱身的局面。

**代码改动**:

```cpp
// 在Strategy::bfs函数中，添加死胡同检测代码(在现有评分逻辑之前)

// 死胡同检测
bool is_dead_end = false;
int max_depth = 0;  // 死胡同的深度

// 简单的方向性检查 - 判断该点周围空间结构
if (Utils::boundCheck(sy, sx)) {
    // 检查上下左右四个方向的墙体数量
    int horizontal_walls = 0;
    int vertical_walls = 0;
  
    // 左右方向
    if (!Utils::boundCheck(sy, sx-1) || mp[sy][sx-1] == -4 || mp2[sy][sx-1] == -5) horizontal_walls++;
    if (!Utils::boundCheck(sy, sx+1) || mp[sy][sx+1] == -4 || mp2[sy][sx+1] == -5) horizontal_walls++;
  
    // 上下方向
    if (!Utils::boundCheck(sy-1, sx) || mp[sy-1][sx] == -4 || mp2[sy-1][sx] == -5) vertical_walls++;
    if (!Utils::boundCheck(sy+1, sx) || mp[sy+1][sx] == -4 || mp2[sy+1][sx] == -5) vertical_walls++;
  
    // 判断是否为潜在死胡同入口
    if ((horizontal_walls == 2 && vertical_walls >= 1) || 
        (vertical_walls == 2 && horizontal_walls >= 1)) {
    
        // 确定死胡同的方向
        int open_dir_y = 0, open_dir_x = 0;
        if (horizontal_walls < 2) {
            // 水平方向开放
            open_dir_y = 0;
            open_dir_x = (!Utils::boundCheck(sy, sx-1) || mp[sy][sx-1] == -4 || mp2[sy][sx-1] == -5) ? 1 : -1;
        } else {
            // 垂直方向开放
            open_dir_y = (!Utils::boundCheck(sy-1, sx) || mp[sy-1][sx] == -4 || mp2[sy-1][sx] == -5) ? 1 : -1;
            open_dir_x = 0;
        }
    
        // 沿着开放方向探测死胡同深度
        int depth = 1;
        int curr_y = sy + open_dir_y;
        int curr_x = sx + open_dir_x;
    
        // 最多探测8格深度
        while (depth < 8 && Utils::boundCheck(curr_y, curr_x) &&
               mp[curr_y][curr_x] != -4 && mp2[curr_y][curr_x] != -5) {
        
            // 检查当前位置是否形成新的出口
            int exits = 0;
            for (auto dir : validDirections) {
                const auto [next_y, next_x] = Utils::nextPos({curr_y, curr_x}, dir);
            
                // 跳过来时的方向
                if (next_y == curr_y - open_dir_y && next_x == curr_x - open_dir_x) continue;
            
                // 检查是否是有效出口
                if (Utils::boundCheck(next_y, next_x) && 
                    mp[next_y][next_x] != -4 && mp2[next_y][next_x] != -5) {
                    exits++;
                }
            }
        
            // 如果找到新出口，这不是死胡同
            if (exits > 0) {
                depth = 0; // 重置深度，表示不是死胡同
                break;
            }
        
            // 继续沿当前方向探索
            depth++;
            curr_y += open_dir_y;
            curr_x += open_dir_x;
        }
    
        // 如果深度大于等于蛇的长度的一半，可能会被困住
        if (depth > 0) {
            is_dead_end = true;
            max_depth = depth;
        
            // 检查我的蛇长度，如果死胡同太短可能被困
            int my_length = 0;
            for (const auto &snake : state.snakes) {
                if (snake.id == MYID) {
                    my_length = snake.length;
                    break;
                }
            }
        
            // 如果死胡同深度小于蛇长度的一半，可能无法调头
            if (max_depth < my_length / 2) {
                score -= (my_length / 2 - max_depth) * 400; // 惩罚分数
            
                // 极短死胡同且蛇较长时，给予极高惩罚
                if (max_depth <= 2 && my_length >= 8) {
                    return -1800; // 几乎必死
                }
            }
        }
    }
}
```

## 批次3: 安全区收缩危险预警

**目标**: 加强对安全区收缩形成的危险边界的评估，特别是即将收缩和刚收缩区域。

**代码改动**:

```cpp
// 在Strategy::bfs函数中强化现有的安全区收缩风险评估(682-705行)

// 安全区收缩风险评估
int current_tick = MAX_TICKS - state.remaining_ticks;
int ticks_to_shrink = state.next_shrink_tick - current_tick;

// 如果即将收缩（小于20个tick）或刚收缩完（5个tick内）
bool near_shrink_boundary = false;
double boundary_danger = 0.0;

// 检查是否靠近当前安全区边界
int dist_to_current_boundary = min({
    abs(sx - state.current_safe_zone.x_min),
    abs(sx - state.current_safe_zone.x_max),
    abs(sy - state.current_safe_zone.y_min),
    abs(sy - state.current_safe_zone.y_max)
});

// 检查是否靠近下个安全区边界
int dist_to_next_boundary = INT_MAX;
if (ticks_to_shrink >= 0) {
    dist_to_next_boundary = min({
        abs(sx - state.next_safe_zone.x_min),
        abs(sx - state.next_safe_zone.x_max),
        abs(sy - state.next_safe_zone.y_min),
        abs(sy - state.next_safe_zone.y_max)
    });
}

// 处理即将收缩情况
if (ticks_to_shrink >= 0 && ticks_to_shrink <= 20) {
    // 检查位置是否在下一个安全区内
    if (sx < state.next_safe_zone.x_min || sx > state.next_safe_zone.x_max ||
        sy < state.next_safe_zone.y_min || sy > state.next_safe_zone.y_max) {
    
        // 安全区外位置价值大幅降低，危险度随收缩时间临近增加
        if (ticks_to_shrink <= 5) {
            // 极度危险，几乎立即放弃
            score -= 5000;
            return score; // 直接返回，避免后续评估
        } else if (ticks_to_shrink <= 10) {
            // 很危险，强烈避免
            score -= 3000; 
        } else {
            // 有风险，尽量避免
            score -= 1000;
        }
    } 
    // 位置在下一个安全区内，但靠近边界
    else if (dist_to_next_boundary <= 3) {
        // 虽然在安全区内，但靠近新边界也有风险
        near_shrink_boundary = true;
        boundary_danger = 200.0 * (4 - dist_to_next_boundary); // 最高600，最低200
        score -= boundary_danger;
    
        // 检查是否是收缩后形成的瓶颈
        int wall_or_danger_count = 0;
        for (auto dir : validDirections) {
            const auto [ny, nx] = Utils::nextPos({sy, sx}, dir);
            // 检查是否超出下一安全区
            if (!Utils::boundCheck(ny, nx) || 
                ny < state.next_safe_zone.y_min || ny > state.next_safe_zone.y_max ||
                nx < state.next_safe_zone.x_min || nx > state.next_safe_zone.x_max ||
                mp[ny][nx] == -4 || mp2[ny][nx] == -5) {
                wall_or_danger_count++;
            }
        }
    
        // 如果收缩会导致这里变成死胡同或极度受限区域
        if (wall_or_danger_count >= 3) {
            score -= 1500; // 收缩后将形成极度危险区域
        }
    }
} 
// 刚收缩完成的情况(收缩后5个tick内)
else if (ticks_to_shrink < 0 && ticks_to_shrink >= -5) {
    // 靠近当前边界依然危险
    if (dist_to_current_boundary <= 2) {
        score -= 800; // 刚收缩完成，边界附近仍有危险
    }
}
// 一般情况下的边界检查
else if (dist_to_current_boundary <= 2) {
    score -= 200 * (3 - dist_to_current_boundary); // 一般边界风险
}
```

## 批次4: 转向限制风险评估

**目标**: 考虑蛇只能在特定位置转弯的规则限制，避免因转向问题陷入危险。

**代码改动**:

```cpp
// 在Utils命名空间中添加转弯点检查函数
namespace Utils {
    // 其他现有函数...
  
    // 检查坐标是否是有效转弯点(根据规则，坐标为20*N+3的位置可以转弯)
    bool isTurningPoint(int x) {
        return (x % 20 == 3);
    }
  
    // 计算到最近转弯点的距离
    int distanceToNearestTurningPoint(int x) {
        int mod = x % 20;
        if (mod == 3) return 0; // 已在转弯点
    
        // 计算到前后两个转弯点的距离
        int dist_forward = (mod < 3) ? (3 - mod) : (23 - mod);
        int dist_backward = (mod > 3) ? (mod - 3) : (mod + 17);
    
        return min(dist_forward, dist_backward);
    }
}

// 在Strategy::eval函数中添加转弯风险评估(在cornerEval后)
double eval(const GameState &state, int y, int x, int fy, int fx)
{
    // 现有代码...
    int test = cornerEval(y, x, fy, fx);
    if (test != -10000)
    {
        // 处理拐角
        // ...现有代码...
    }
  
    // 添加: 转向限制风险评估
    bool needs_turn = false;
    Direction current_direction;
  
    // 计算当前移动方向
    if (x > fx) current_direction = Direction::RIGHT;
    else if (x < fx) current_direction = Direction::LEFT;
    else if (y > fy) current_direction = Direction::DOWN;
    else current_direction = Direction::UP;
  
    // 获取目标坐标
    int target_x = -1, target_y = -1;
    double highest_value = -1;
  
    // 查找当前评估点附近的最高价值目标
    for (const auto &item : state.items) {
        if (item.value <= 0) continue; // 跳过非正价值物品
    
        int dist = abs(y - item.pos.y) + abs(x - item.pos.x);
        if (dist <= 5) { // 只考虑附近的目标
            double adjusted_value = item.value * (6.0 - dist); // 距离越近价值越高
            if (adjusted_value > highest_value) {
                highest_value = adjusted_value;
                target_y = item.pos.y;
                target_x = item.pos.x;
            }
        }
    }
  
    // 如果找到了目标，分析是否需要转向
    if (target_x != -1 && target_y != -1) {
        Direction target_direction;
    
        // 确定到目标的主要方向
        if (abs(x - target_x) > abs(y - target_y)) {
            // 水平方向为主
            target_direction = (x < target_x) ? Direction::RIGHT : Direction::LEFT;
        } else {
            // 垂直方向为主
            target_direction = (y < target_y) ? Direction::DOWN : Direction::UP;
        }
    
        // 如果需要转向但不在转弯点
        if (current_direction != target_direction) {
            needs_turn = true;
        
            // 检查当前位置是否可以转弯
            bool can_turn_here = Utils::isTurningPoint(x);
        
            if (!can_turn_here) {
                // 计算到最近转弯点的距离
                int dist_to_turn = Utils::distanceToNearestTurningPoint(x);
            
                // 根据转弯难度调整评分
                if (dist_to_turn >= 4) {
                    // 转弯点太远，可能错过目标
                    score -= 350;
                } else {
                    // 轻微惩罚
                    score -= dist_to_turn * 50;
                }
            
                // 特别处理：如果这一步会导致错过转弯点且需要转弯去追高价值目标
                if (current_direction == Direction::LEFT || current_direction == Direction::RIGHT) {
                    int next_x = (current_direction == Direction::RIGHT) ? x + 1 : x - 1;
                
                    // 检查当前位置是否比下一步更接近转弯点
                    if (Utils::distanceToNearestTurningPoint(x) < Utils::distanceToNearestTurningPoint(next_x) &&
                        highest_value >= 8.0) { // 高价值目标
                        score -= 500; // 严重惩罚，避免错过重要转弯点
                    }
                }
            } else {
                // 已在转弯点，适当奖励
                score += 150;
            }
        }
    }
  
    // 安全区评分和BFS评估...
    double safe_zone_score = safeZoneCenterScore(state, y, x);
    double bfs_score = bfs(y, x, fy, fx, state);
  
    return bfs_score + safe_zone_score;
}
```

## 批次5: 宝箱周围区域危险评估

**目标**: 加强对宝箱周围区域的评估，避免蛇在未持钥匙时接近宝箱区域。

**代码改动**:

```cpp
// 在illegalMove函数中补充宝箱危险区域评估(403-406行附近)

// 检查宝箱 - 无钥匙时视为绝对障碍
if (!self.has_key) {
    for (const auto &chest : state.chests) {
        // 直接宝箱位置是绝对障碍
        if (y == chest.pos.y && x == chest.pos.x) {
            illegals.insert(dir);
        }
        // 宝箱周围1格也是危险区域(在紧急模式下才考虑通过)
        else if (!emergency_mode && 
                abs(y - chest.pos.y) + abs(x - chest.pos.x) <= 1) {
            illegals.insert(dir);
        }
    }
}

// 在Strategy::bfs函数中添加宝箱危险区域评估(与食物评估部分前)
// 宝箱危险区域评估
if (!state.chests.empty() && !state.get_self().has_key) {
    for (const auto &chest : state.chests) {
        int dist_to_chest = abs(sy - chest.pos.y) + abs(sx - chest.pos.x);
    
        // 距离宝箱越近风险越高
        if (dist_to_chest <= 3) {
            // 直接接触宝箱是致命的
            if (dist_to_chest == 0) {
                return -5000; // 致命障碍物
            }
            // 距离1格内高度危险
            else if (dist_to_chest == 1) {
                score -= 2000;
            }
            // 距离2格有风险
            else if (dist_to_chest == 2) {
                score -= 800;
            }
            // 距离3格轻微风险
            else {
                score -= 300;
            }
        }
    }
}

// 相反，如果持有钥匙，鼓励接近宝箱
if (!state.chests.empty() && state.get_self().has_key) {
    for (const auto &chest : state.chests) {
        int dist_to_chest = abs(sy - chest.pos.y) + abs(sx - chest.pos.x);
    
        // 接近宝箱有额外奖励
        if (dist_to_chest <= 5) {
            // 靠近宝箱奖励递增
            score += (6 - dist_to_chest) * 200;
        }
    }
}
```

这些优化方案分批次实施，每批次专注于一个功能点，可以逐步改进蛇的安全性能。每个方案都针对规则中的特定风险点进行了细致处理，并考虑了各种边缘情况。建议按照批次顺序逐步实施，每次观察蛇的行为变化再进行下一步优化。
