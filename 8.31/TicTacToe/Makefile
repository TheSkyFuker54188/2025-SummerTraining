# 井字棋游戏的Makefile

# 编译器设置
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# SFML设置
SFML_LIBS = -lsfml-graphics -lsfml-window -lsfml-system

# 所有目标
all: tictactoe_v1 tictactoe_v2 tictactoe_v3

# 版本1: 初始版本
tictactoe_v1: tictactoe_v1.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# 版本2: 完善版本
tictactoe_v2: tictactoe_v2.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# 版本3: GUI版本
tictactoe_v3: tictactoe_v3.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(SFML_LIBS)

# 清理编译文件
clean:
	rm -f tictactoe_v1 tictactoe_v2 tictactoe_v3

.PHONY: all clean 