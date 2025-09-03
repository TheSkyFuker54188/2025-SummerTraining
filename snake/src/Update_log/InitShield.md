# 完整的两阶段初始护盾策略实现方案

下面是完整的初始护盾10刻优化策略，分为攻击获取和安全撤离两个阶段：

```cpp
// 在judge函数开始处添加初始护盾策略检测
int judge(const GameState &state) {
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
  
    // 原有的judge逻辑继续...
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
    if (enhancedFoodProcessByPriority(state, 0, 4, food_dir)) {
        return Utils::dir2num(food_dir);
    }
  
    // 最终回退 - 使用标准选择逻辑
    return Utils::dir2num(chooseBestDirection(state));
}
```

## 策略详解

### 阶段1: 攻击性获取(0-6刻)

**核心思想**：利用初始无敌时间最大化资源获取，无视安全性考虑。

1. **食物价值评估**：

   - 尸体：`item.value * 15` - 高价值
   - 增长豆：`8` - 中等价值
   - 普通食物：`1` - 低价值
2. **距离评分**：使用 `10.0 / (1.0 + dist * 0.5)`公式，创建指数衰减效果，使近距离食物极具吸引力
3. **直线路径选择**：

   - 优先垂直方向(实战中更容易快速调整)
   - 次选水平方向
   - 只检查墙壁和边界，其他障碍物(如蛇身)完全忽略
4. **应急方案**：如果第一选择不可行，尝试第二选择方向

### 阶段2: 安全撤离(7-9刻)

**核心思想**：为护盾消失做准备，寻找最安全且空旷的区域。

1. **多维评分系统**：

   ```
   总分 = 空旷度*2.0 + 出口数*1.5 - 蛇密度*3.0 + (有食物?20%加成:0)
   ```
2. **空旷度计算**：

   - BFS探索半径4步内的可达区域
   - 近距离点权重更高(5-depth)*0.5
   - 评估机动空间大小，避免被包围
3. **出口数量**：至少要有2个出口的位置才考虑
4. **蛇密度计算**：

   - 5步内的敌方蛇数量
   - 特别惩罚2步内的敌方蛇(+2额外分数)
   - 避开竞争激烈的区域
5. **食物加成**：如果位置有食物，评分提高20%，在同等安全性下优先吃食物
6. **安全性筛选**：

   - 只考虑至少有2个出口的位置
   - 如果没有足够安全的位置，退回到基本安全检查

## 整体策略总结

1. **前期(0-6刻)**：完全攻击性策略，直奔高价值资源

   - 尸体 > 增长豆 > 普通食物
   - 近 > 远
   - 无视障碍物
2. **后期(7-9刻)**：安全撤离策略，为护盾消失做准备

   - 出口多 > 出口少
   - 空旷区域 > 狭窄区域
   - 低蛇密度 > 高蛇密度
   - 有食物 > 无食物(同等安全性下)

这种两阶段策略能够最大化初始护盾的价值，在前期快速积累优势，同时确保在护盾消失后仍处于有利位置，为中期游戏打下坚实基础。
