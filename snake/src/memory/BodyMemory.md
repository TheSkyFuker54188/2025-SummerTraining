# 使用Memory系统增强尸体追踪方案

Memory系统与尸体追踪是完美的搭配——通过将尸体信息持久化存储，我们能在不同游戏刻之间维持尸体追踪，即使这些尸体不在当前视野中。以下是整合方案：

## 1. 使用Memory替代静态变量

将原方案中的静态vector替换为基于Memory的存储：

```cpp
// 尸体信息序列化与反序列化
std::string serialize_corpses(const vector<CorpseRecord> &corpses) {
    std::stringstream ss;
    ss << "CORPSES:";
    for (size_t i = 0; i < corpses.size(); i++) {
        if (i > 0) ss << ";";
        ss << corpses[i].pos.x << "," << corpses[i].pos.y << "," 
           << corpses[i].value << "," << corpses[i].first_seen;
    }
    return ss.str();
}

vector<CorpseRecord> deserialize_corpses(const std::string &memory_data) {
    vector<CorpseRecord> result;
    if (memory_data.empty() || memory_data.substr(0, 8) != "CORPSES:") return result;
  
    std::string data = memory_data.substr(8);
    std::stringstream ss(data);
    std::string corpse_data;
  
    while (getline(ss, corpse_data, ';')) {
        std::stringstream corpse_ss(corpse_data);
        std::string value;
        vector<int> values;
      
        while (getline(corpse_ss, value, ',')) {
            values.push_back(std::stoi(value));
        }
      
        if (values.size() == 4) {
            result.push_back({
                {values[1], values[0]}, // Point{y, x}
                values[2],              // value
                values[3],              // first_seen
                false                   // default to not valid
            });
        }
    }
  
    return result;
}
```

## 2. 修改尸体追踪更新函数

```cpp
// 从Memory读取尸体记录并更新
void update_corpse_records(const GameState &state, const std::string &memory_data) {
    // 从Memory中恢复尸体记录
    vector<CorpseRecord> known_corpses = deserialize_corpses(memory_data);
  
    // 标记所有尸体为待验证
    for (auto &corpse : known_corpses) {
        corpse.still_valid = false;
    }
  
    // 更新已知尸体和添加新尸体
    for (const auto &item : state.items) {
        if (item.value >= 5) {  // 高价值尸体
            bool found = false;
            for (auto &corpse : known_corpses) {
                if (abs(corpse.pos.y - item.pos.y) <= 1 && abs(corpse.pos.x - item.pos.x) <= 1) {
                    corpse.pos = item.pos; // 更新位置
                    corpse.value = item.value; // 更新价值
                    corpse.still_valid = true;
                    found = true;
                    break;
                }
            }
            if (!found) {
                known_corpses.push_back({
                    item.pos, 
                    item.value, 
                    MAX_TICKS - state.remaining_ticks, 
                    true
                });
            }
        }
    }
  
    // 移除过期尸体 - 如果尸体超过20个tick未被验证则移除
    int current_tick = MAX_TICKS - state.remaining_ticks;
    known_corpses.erase(
        std::remove_if(known_corpses.begin(), known_corpses.end(),
                     [current_tick](const CorpseRecord &c) { 
                         return !c.still_valid && (current_tick - c.first_seen > 20); 
                     }),
        known_corpses.end()
    );
  
    // 返回更新后的尸体记录列表供决策使用
    return known_corpses;
}
```

## 3. 集成到主程序流程

```cpp
int judge(const GameState &state, const std::string &memory_data) {
    // 解析Memory数据，获取尸体记录
    vector<CorpseRecord> known_corpses = update_corpse_records(state, memory_data);
  
    // 使用当前尸体记录进行决策
    // ...现有决策代码...
  
    // 预测潜在尸体区域，添加到关注列表
    vector<Point> potential_areas = predict_potential_corpse_areas(state);
    for (const auto &area : potential_areas) {
        // 给预测区域添加较低权重，在BFS或评估中使用
        // ...
    }
  
    // 继续现有的评估和决策代码
    // ...
  
    // 准备Memory数据保存
    std::string memory_to_save = serialize_corpses(known_corpses);
  
    // 添加其他可能需要保存的信息（如当前锁定目标）
    if (current_target.y != -1 && current_target.x != -1) {
        memory_to_save += "|TARGET:" + 
                          std::to_string(current_target.x) + "," + 
                          std::to_string(current_target.y) + "," +
                          std::to_string(target_value) + "," +
                          std::to_string(target_lock_time);
    }
  
    // 返回决策值，决策完成后写入Memory
    int decision = Utils::dir2num(choice);
    cout << decision << endl;
    cout << memory_to_save << endl;  // 写入Memory系统
    return decision;
}
```

## 4. 读取Memory并初始化

```cpp
int main() {
    // 读取当前tick的所有游戏状态
    GameState current_state;
    std::string memory_data = "";
    read_game_state(current_state);
  
    // 读取Memory数据
    if (current_state.remaining_ticks < MAX_TICKS) {
        // 非首个tick，有Memory数据可读
        std::string line;
        if (std::getline(std::cin, line)) {
            memory_data = line;
        }
    }
  
    // 使用Memory数据进行决策
    int decision = judge(current_state, memory_data);
  
    return 0;
}
```

## 5. 增强功能：使用Memory锁定目标

```cpp
// 从Memory恢复锁定目标
void restore_locked_target(const std::string &memory_data) {
    size_t pos = memory_data.find("|TARGET:");
    if (pos != std::string::npos) {
        std::string target_data = memory_data.substr(pos + 8);
        std::stringstream ss(target_data);
        std::string value;
        vector<int> values;
      
        while (getline(ss, value, ',')) {
            values.push_back(std::stoi(value));
        }
      
        if (values.size() == 4) {
            current_target = {values[1], values[0]}; // {y, x}
            target_value = values[2];
            target_lock_time = values[3];
        }
    }
}
```

Memory系统使尸体追踪功能从"短期记忆"升级为"长期记忆"，能够记住更多尸体位置，并在整个游戏过程中持续追踪它们，大幅提高尸体收集效率。
