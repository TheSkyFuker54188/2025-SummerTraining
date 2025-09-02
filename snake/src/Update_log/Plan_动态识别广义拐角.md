# 拐角检测优化方案

现有实现中的cornerEval函数使用了硬编码的拐角位置集合，这种方法缺乏通用性且难以维护。以下是一个更为通用和高效的优化方案。

## 问题分析

当前实现的问题：

- 使用硬编码位置集合("0917", "1018"等)识别拐角
- 无法适应不同地图或未预设的拐角情况
- 维护成本高，每增加一个拐角都需要手动添加

## 优化方案：基于地形特征的动态拐角检测

```cpp
// 替换现有的cornerEval函数
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
```

## 改进点

1. **动态识别拐角**：

   - 基于地形特征(墙和出口的分布)识别拐角，而非依赖硬编码位置
   - 明确定义拐角：两面墙成直角，两个出口相邻
2. **更准确的危险评估**：

   - 考虑从何处进入拐角，确定必经的出口方向
   - 针对出口方向进行安全性分析，而非整个拐角位置
3. **与原有判断保持一致**：

   - 仍然返回-5000表示危险拐角，-50表示陷阱拐角，0表示安全拐角
   - 保持与原函数相同的对外接口，不影响其他代码
4. **优点**：

   - 适用于任何地图布局，无需硬编码位置
   - 更精确地识别真正的危险拐角
   - 代码更加简洁清晰，易于理解和维护

这个优化方案巧妙地将硬编码的拐角集合替换为基于地形特征的动态检测算法，同时保持了原有功能的判断逻辑，是对现有代码缺陷的针对性、简明且高效的改进。
