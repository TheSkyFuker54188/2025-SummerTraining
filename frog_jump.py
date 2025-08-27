class FrogJumpProblem:
    def __init__(self, initial_state, goal_state):
        self.initial_state = initial_state
        self.goal_state = goal_state

    def get_actions(self, state):
        actions = []
        empty_pos = state.index('_')
        n = len(state)
        
        # 检查左边的青蛙是否可以跳
        if empty_pos > 0:
            frog = state[empty_pos - 1]
            if frog in ['L', 'R']:
                # 规则1: 直接跳到空位
                actions.append((empty_pos - 1, empty_pos))
                # 规则2: 如果隔一个是空位，可以跳过去
                if empty_pos - 2 >= 0 and state[empty_pos - 2] in ['L', 'R']:
                    actions.append((empty_pos - 2, empty_pos))
        
        # 检查右边的青蛙是否可以跳
        if empty_pos < n - 1:
            frog = state[empty_pos + 1]
            if frog in ['L', 'R']:
                # 规则1: 直接跳到空位
                actions.append((empty_pos + 1, empty_pos))
                # 规则2: 如果隔一个是空位，可以跳过去
                if empty_pos + 2 < n and state[empty_pos + 2] in ['L', 'R']:
                    actions.append((empty_pos + 2, empty_pos))
        
        return actions

    def apply_action(self, state, action):
        from_pos, to_pos = action
        new_state = list(state)
        # 交换位置
        new_state[from_pos], new_state[to_pos] = new_state[to_pos], new_state[from_pos]
        return tuple(new_state)

    def is_goal(self, state):
        return state == self.goal_state

    def solve(self):
        """使用BFS搜索解决方案路径"""
        from collections import deque
        
        queue = deque([self.initial_state])
        parent = {self.initial_state: None}
        action_to_parent = {self.initial_state: None}
        
        while queue:
            current_state = queue.popleft()
            
            if self.is_goal(current_state):
                # 重建路径
                path = []
                actions = []
                while current_state is not None:
                    path.append(current_state)
                    if action_to_parent[current_state] is not None:
                        actions.append(action_to_parent[current_state])
                    current_state = parent[current_state]
                path.reverse()
                actions.reverse()
                return path, actions
            
            for action in self.get_actions(current_state):
                new_state = self.apply_action(current_state, action)
                if new_state not in parent:
                    parent[new_state] = current_state
                    action_to_parent[new_state] = action
                    queue.append(new_state)
        
        return None, None  # 无解

    def print_state(self, state):
        """简单的可视化：打印河墩状态"""
        print(" ".join(f"[{frog}]" for frog in state))

    def visualize_jump(self, state, action):
        """可视化跳跃过程"""
        from_pos, to_pos = action
        frog = state[from_pos]
        direction = "→" if from_pos < to_pos else "←"
        print(f"青蛙 {frog} 从位置 {from_pos} {direction} 跳到位置 {to_pos}")
        self.print_state(state)
        new_state = self.apply_action(state, action)
        print("跳跃后:")
        self.print_state(new_state)
        return new_state

# 示例使用
if __name__ == "__main__":
    initial = ('L', 'L', 'L', '_', 'R', 'R', 'R')
    goal = ('R', 'R', 'R', '_', 'L', 'L', 'L')
    
    problem = FrogJumpProblem(initial, goal)
    
    print("初始状态:")
    problem.print_state(initial)
    print("目标状态:")
    problem.print_state(goal)
    
    # 获取初始状态的动作
    actions = problem.get_actions(initial)
    print("初始状态的可能动作:", actions)
    
    # 解决青蛙跳河问题
    print("\n搜索解决方案...")
    path, actions = problem.solve()
    
    if path:
        print(f"找到解决方案！需要 {len(actions)} 步")
        print("\n解决方案路径:")
        for i, (state, action) in enumerate(zip(path[:-1], actions)):
            print(f"\n第 {i+1} 步:")
            problem.visualize_jump(state, action)
        print(f"\n最终状态:")
        problem.print_state(path[-1])
        print("是否达到目标:", problem.is_goal(path[-1]))
    else:
        print("该问题无解！")
