# 设计师
_Exported on 8/29/2025 at 20:55:59 GMT+8 from Cursor (1.2.4)_

---

**User**

研究整个项目，以便于后续上手实现

（说中文）

---

**Cursor**

# 贪吃蛇对战项目分析

## 项目概述
这是一个多人对战贪吃蛇游戏，玩家需要编写AI算法控制蛇在地图上移动、收集食物并与其他玩家竞争。

## 关键组件

1. **游戏规则** (docs/rule.md)
   - 地图：40×30格
   - 游戏时长：256个游戏刻(tick)，分为早期、中期和后期
   - 蛇初始长度：5格
   - 物品类型：普通食物(1-5分)、增长豆、陷阱、宝箱、钥匙
   - 护盾系统：消耗20分激活，持续5个游戏刻，冷却30个游戏刻
   - 安全区会随时间收缩，离开安全区会死亡

2. **通信接口** (docs/interface.md)
   - 输入：通过stdin接收游戏状态
   - 输出：通过stdout提交决策(0-左, 1-上, 2-右, 3-下, 4-激活护盾)
   - Memory系统：可在游戏刻之间保存状态信息

3. **算法模板** (docs/file/snake_algorithm_template.cpp)
   - 基本数据结构：Point, Item, Snake, Chest, Key等
   - 游戏状态读取函数

4. **当前实现** (src/snake.cpp)
   - 基础框架已搭建，但目前只实现了随机方向选择

5. **实现方案** (src/basic_plan.md)
   - 详细的类设计：MapAnalyzer, PathFinder, SnakeAI
   - 核心算法：地图分析、路径规划、目标选择、决策逻辑
   - 分阶段实现计划

## 核心算法组件

1. **地图分析**
   - 危险度矩阵：标记安全区、蛇身、可能移动位置的危险程度
   - 安全区收缩预测

2. **路径规划**
   - A*算法实现，考虑蛇只能在整格点(坐标为20倍数+3)改变方向

3. **目标选择**
   - 基于价值/距离比率选择最优目标
   - 考虑安全因素、特殊物品(钥匙、宝箱)的优先级

4. **决策逻辑**
   - 路径有效性检查
   - 护盾使用策略
   - 安全方向选择

5. **记忆系统**
   - 保存当前目标、路径、决策等信息
   - 在游戏刻之间传递状态

## 实现步骤建议

按照basic_plan.md中的分阶段计划进行：

1. **阶段一：基础功能**
   - 安全移动：避免撞墙和撞蛇
   - 简单目标选择：就近吃食物
   - 安全区边界检测

2. **阶段二：核心功能**
   - 地图分析系统
   - A*路径规划算法
   - 目标价值评估系统
   - 护盾基础策略

3. **阶段三：高级功能**
   - 宝箱和钥匙策略
   - 智能护盾使用
   - 对手行为预测
   - 安全区收缩应对

4. **阶段四：优化和测试**
   - 算法性能优化
   - 特殊情况处理
   - 参数调优
   - 全面测试

## 特别注意事项

1. 蛇只能在坐标为20倍数+3的点改变方向
2. 需要提前规划应对安全区收缩
3. 护盾是关键资源，需要智能使用
4. Memory系统容量限制为4KB，需合理规划

基于src/basic_plan.md提供的详细实现方案，可以逐步完善src/snake.cpp文件，实现一个高效的贪吃蛇AI算法。

---

**User**

@game.js @snake_algorithm_template.cpp @interface.md @rule.md 细读这4个文件，以对项目产生更深而具体的理解，为将来实现打下基础

---

**Cursor**

# 贪吃蛇对战项目深度分析

## 一、游戏规则解析 (rule.md)

### 1. 游戏基本设定
- **地图尺寸**: 40×30格
- **游戏时长**: 256个游戏刻(tick)
- **初始蛇长**: 5格

### 2. 游戏阶段
- **早期(1-80)**: 低分食物(1-3分)80%，增长豆20%
- **中期(81-200)**: 引入高分食物和陷阱，第一个宝箱出现
- **后期(201-256)**: 高风险高回报，第二个宝箱可能出现

### 3. 关键机制
- **蛇的成长**:
  - 每累计20分增加1格长度
  - 增长豆直接增加2格长度

- **护盾系统**:
  - 初始护盾持续10个tick
  - 消耗20分激活，持续5个tick，冷却30个tick
  - 免疫碰撞死亡(除墙壁外)

- **安全区收缩**:
  | 游戏阶段 | 刻范围  | 最终边界        |
  | -------- | ------- | --------------- |
  | 早期     | 1-80    | (0,0)->(39,29)  |
  | 中期     | 81-100  | (3,2)->(36,27)  |
  | 中期     | 161-180 | (7,5)->(32,24)  |
  | 后期     | 221-240 | (10,7)->(29,22) |

- **宝箱与钥匙**:
  - 持有钥匙才能开启宝箱获得高分
  - 无钥匙触碰宝箱立即死亡(护盾无效)
  - 钥匙持有30个tick后自动脱落

## 二、通信接口 (interface.md)

### 1. 数据交互
- **输入**: 通过stdin接收游戏状态
- **输出**: 通过stdout提交决策(0-左,1-上,2-右,3-下,4-护盾)

### 2. 游戏状态格式
```
剩余游戏刻
物品数量
y1 x1 v1 t1  // 坐标、价值、剩余时间
...
蛇总数
id1 len1 score1 dir1 shield_cd1 shield_time1
y1_1 x1_1    // 蛇身体坐标
...
宝箱总数
y_chest1 x_chest1 score1
...
钥匙总数
y_key1 x_key1 holder_id1 remaining_time1
...
安全区边界信息
```

### 3. 物品值含义
- **1至5**: 普通食物(得分)
- **-1**: 增长豆
- **-2**: 陷阱
- **-3**: 钥匙
- **-5**: 宝箱

### 4. Memory系统
- 每回合可存储状态信息(上限4KB)
- 在决策后通过额外输出保存
- 下一回合通过输入末尾获取

### 5. 资源限制
- CPU时间: 1秒
- 内存: 1GB
- 墙钟时间: 5秒

## 三、游戏引擎分析 (game.js)

### 1. 核心机制
- **移动系统**:
  - 蛇每次移动speed=2个单位
  - 只能在整格点(坐标为20的倍数+3)改变方向
  ```javascript
  if ((otherSnakeX[i][0] - 3) % 20 == 0 && (otherSnakeY[i][0] - 3) % 20 == 0) {
      // 只有在整格点才能改变方向
  }
  ```

- **碰撞检测**:
  ```javascript
  function check_death() {
      // 检查蛇头是否与其他蛇身体、障碍物碰撞
  }
  
  function check_wall() {
      // 检查是否撞墙或离开安全区
  }
  ```

- **食物与豆子**:
  ```javascript
  function eat_bonus() {
      // 处理吃食物、增长豆、陷阱等
  }
  
  function update_bonus() {
      // 随时间更新食物生成位置，逐渐向中心生成
  }
  ```

- **安全区收缩**:
  ```javascript
  // 安全区随游戏进程向中心收缩
  var centerX = 13 + width / 2;
  var centerY = 13 + height / 2;
  var maxOffsetX = Math.max(100, centerX - time*1.5); // 随时间减小生成范围
  ```

### 2. 关键数据结构
- **地图表示**: `map[]` 一维数组表示二维地图(0空,1物品,2墙)
- **蛇身体**: `otherSnakeX[][],otherSnakeY[][]` 分别存储各节身体坐标
- **蛇属性**: `otherSnakeDirection,otherSnakeScore,otherSnakeShieldTimes`等

## 四、算法模板解析 (snake_algorithm_template.cpp)

### 1. 关键数据结构
```cpp
struct Point { int y, x; };

struct Item {
  Point pos;
  int value;   // 物品类型/价值
  int lifetime; // 剩余时间
};

struct Snake {
  int id;
  int length;
  int score;
  int direction;  // 0左,1上,2右,3下
  int shield_cd;
  int shield_time;
  bool has_key;
  vector<Point> body; // 蛇身体坐标，头部在front
};

struct GameState {
  int remaining_ticks;
  vector<Item> items;
  vector<Snake> snakes;
  vector<Chest> chests;
  vector<Key> keys;
  SafeZoneBounds current_safe_zone;
  // ...其他信息
};
```

### 2. 核心功能
- **状态读取**: `read_game_state()` 从stdin解析游戏状态
- **决策提交**: 将0-4的整数输出到stdout
- **内存读写**: 通过cin额外读取上回合存储的数据

## 五、实现关键点

### 1. 技术挑战
- **方向转换限制**: 只能在整格点(坐标为20倍数+3)改变方向，路径规划必须考虑这一点
- **安全区收缩预测**: 需提前应对安全区收缩，避免被困
- **碰撞风险评估**: 对其他蛇可能的移动路径进行预测
- **护盾策略**: 在关键时刻使用护盾避免死亡
- **宝箱钥匙策略**: 判断何时去拿钥匙、何时开启宝箱

### 2. AI架构设计
- **地图分析模块**: 危险度矩阵、安全路径评估
- **路径规划模块**: 基于A*算法，考虑转向限制
- **目标选择模块**: 评估食物价值/风险比
- **决策逻辑模块**: 整合各模块信息，输出最终决策
- **记忆系统**: 在游戏刻之间传递状态和计划

### 3. 优先实现顺序
1. **安全移动基础**: 避免撞墙和基本碰撞
2. **简单食物追踪**: 就近吃安全食物
3. **路径规划**: 实现考虑整格点的A*算法
4. **高级策略**: 护盾使用、宝箱开启、对手预测

## 六、实现建议

1. **先实现通信机制**: 确保能正确读取状态并输出决策
2. **逐步优化算法**: 从简单避障开始，逐步添加高级功能
3. **分模块开发**: 按地图分析、路径规划、决策逻辑等模块分开开发
4. **充分利用Memory**: 存储路径计划和目标信息
5. **注重高效性**: 确保算法在1秒内完成计算

这些深入分析为实现一个强大的贪吃蛇AI奠定了基础，关键在于理解蛇的移动机制、转向限制、碰撞规则和安全区变化，以及平衡风险和收益的策略制定。

```plaintext
剩余游戏刻
物品数量
y1 x1 v1 t1  // 坐标、价值、剩余时间
...
蛇总数
id1 len1 score1 dir1 shield_cd1 shield_time1
y1_1 x1_1    // 蛇身体坐标
...
宝箱总数
y_chest1 x_chest1 score1
...
钥匙总数
y_key1 x_key1 holder_id1 remaining_time1
...
安全区边界信息
```

```javascript
  if ((otherSnakeX[i][0] - 3) % 20 == 0 && (otherSnakeY[i][0] - 3) % 20 == 0) {
      // 只有在整格点才能改变方向
  }
```

```javascript
  function check_death() {
      // 检查蛇头是否与其他蛇身体、障碍物碰撞
  }
  
  function check_wall() {
      // 检查是否撞墙或离开安全区
  }
```

```javascript
  function eat_bonus() {
      // 处理吃食物、增长豆、陷阱等
  }
  
  function update_bonus() {
      // 随时间更新食物生成位置，逐渐向中心生成
  }
```

```javascript
  // 安全区随游戏进程向中心收缩
  var centerX = 13 + width / 2;
  var centerY = 13 + height / 2;
  var maxOffsetX = Math.max(100, centerX - time*1.5); // 随时间减小生成范围
```

```cpp
struct Point { int y, x; };

struct Item {
  Point pos;
  int value;   // 物品类型/价值
  int lifetime; // 剩余时间
};

struct Snake {
  int id;
  int length;
  int score;
  int direction;  // 0左,1上,2右,3下
  int shield_cd;
  int shield_time;
  bool has_key;
  vector<Point> body; // 蛇身体坐标，头部在front
};

struct GameState {
  int remaining_ticks;
  vector<Item> items;
  vector<Snake> snakes;
  vector<Chest> chests;
  vector<Key> keys;
  SafeZoneBounds current_safe_zone;
  // ...其他信息
};
```