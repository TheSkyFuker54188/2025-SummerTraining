#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <limits>
#include <algorithm>
#include <ctime>
#include <string>

// 最终版本：优化并添加GUI实现可视化人机对战

// 井字棋游戏状态
class TicTacToeState {
public:
    char board[3][3]; // 棋盘
    char currentPlayer; // 当前玩家 ('X' 或 'O')

    // 构造函数，初始化空棋盘
    TicTacToeState() {
        // 初始化棋盘
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                board[i][j] = ' ';
            }
        }
        currentPlayer = 'X'; // X先手
    }

    // 复制构造函数
    TicTacToeState(const TicTacToeState& other) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                board[i][j] = other.board[i][j];
            }
        }
        currentPlayer = other.currentPlayer;
    }

    // 判断是否是终止状态
    bool isTerminal() const {
        // 检查是否有玩家获胜
        if (checkWin('X') || checkWin('O')) {
            return true;
        }
        
        // 检查是否平局（棋盘已满）
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (board[i][j] == ' ') {
                    return false; // 还有空位，游戏没有结束
                }
            }
        }
        
        return true; // 棋盘已满，平局
    }

    // 检查玩家是否获胜
    bool checkWin(char player) const {
        // 检查行
        for (int i = 0; i < 3; i++) {
            if (board[i][0] == player && board[i][1] == player && board[i][2] == player) {
                return true;
            }
        }
        
        // 检查列
        for (int j = 0; j < 3; j++) {
            if (board[0][j] == player && board[1][j] == player && board[2][j] == player) {
                return true;
            }
        }
        
        // 检查对角线
        if (board[0][0] == player && board[1][1] == player && board[2][2] == player) {
            return true;
        }
        
        if (board[0][2] == player && board[1][1] == player && board[2][0] == player) {
            return true;
        }
        
        return false;
    }

    // 获取状态的评估值
    int evaluate() const {
        if (checkWin('X')) {
            return 10; // X获胜，返回正分
        } else if (checkWin('O')) {
            return -10; // O获胜，返回负分
        } else {
            return 0; // 平局或者游戏未结束
        }
    }

    // 获取所有可能的后继状态
    std::vector<TicTacToeState> getSuccessors() const {
        std::vector<TicTacToeState> successors;
        
        // 遍历棋盘，找到所有空位
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (board[i][j] == ' ') {
                    // 创建新状态
                    TicTacToeState newState(*this);
                    newState.board[i][j] = currentPlayer;
                    newState.currentPlayer = (currentPlayer == 'X') ? 'O' : 'X'; // 切换玩家
                    successors.push_back(newState);
                }
            }
        }
        
        return successors;
    }

    // 执行移动并返回移动是否有效
    bool makeMove(int row, int col) {
        if (row >= 0 && row < 3 && col >= 0 && col < 3 && board[row][col] == ' ') {
            board[row][col] = currentPlayer;
            currentPlayer = (currentPlayer == 'X') ? 'O' : 'X'; // 切换玩家
            return true;
        }
        return false;
    }
};

// 带Alpha-Beta剪枝的Minimax算法
int minimaxAlphaBeta(const TicTacToeState& state, int depth, int alpha, int beta, bool maximizingPlayer) {
    // 如果到达终止状态或深度限制，返回评估值
    if (depth == 0 || state.isTerminal()) {
        return state.evaluate();
    }

    if (maximizingPlayer) {
        // 最大化玩家
        int bestValue = std::numeric_limits<int>::min();
        std::vector<TicTacToeState> successors = state.getSuccessors();
        
        for (auto& child : successors) {
            int v = minimaxAlphaBeta(child, depth - 1, alpha, beta, false);
            bestValue = std::max(bestValue, v);
            alpha = std::max(alpha, bestValue);
            if (beta <= alpha) {
                break; // Beta剪枝
            }
        }
        
        return bestValue;
    } else {
        // 最小化玩家
        int bestValue = std::numeric_limits<int>::max();
        std::vector<TicTacToeState> successors = state.getSuccessors();
        
        for (auto& child : successors) {
            int v = minimaxAlphaBeta(child, depth - 1, alpha, beta, true);
            bestValue = std::min(bestValue, v);
            beta = std::min(beta, bestValue);
            if (beta <= alpha) {
                break; // Alpha剪枝
            }
        }
        
        return bestValue;
    }
}

// 找到最佳移动的位置（行、列）
std::pair<int, int> findBestMove(TicTacToeState& state, int depth) {
    int bestRow = -1;
    int bestCol = -1;
    int bestValue = std::numeric_limits<int>::min();
    
    // 遍历所有可能的移动
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (state.board[i][j] == ' ') {
                // 尝试这个移动
                TicTacToeState newState(state);
                newState.board[i][j] = state.currentPlayer;
                newState.currentPlayer = (state.currentPlayer == 'X') ? 'O' : 'X';
                
                // 评估这个移动
                int moveValue = minimaxAlphaBeta(newState, depth - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), false);
                
                // 如果这个移动比之前找到的最佳移动更好，则更新最佳移动
                if (moveValue > bestValue) {
                    bestValue = moveValue;
                    bestRow = i;
                    bestCol = j;
                }
            }
        }
    }
    
    return std::make_pair(bestRow, bestCol);
}

// 游戏类
class TicTacToeGame {
private:
    TicTacToeState gameState;
    sf::RenderWindow window;
    sf::Font font;
    sf::Text statusText;
    sf::RectangleShape grid[3][3];
    sf::Text gridText[3][3];
    sf::Text restartText;
    sf::RectangleShape restartButton;
    sf::Text difficultyText;
    sf::RectangleShape difficultyButton;
    int difficulty; // 难度等级（1-9，代表搜索深度）
    bool gameOver;
    std::string gameStatus;

public:
    TicTacToeGame() : window(sf::VideoMode(600, 700), "Tic-Tac-Toe - Minimax AI"), difficulty(5), gameOver(false) {
        // 创建默认字体
        font.loadFromFile("arial.ttf"); // 尝试加载Arial字体
        
        // 如果找不到arial.ttf，尝试其他常见系统字体
        if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf") && 
            !font.loadFromFile("C:/Windows/Fonts/calibri.ttf") &&
            !font.loadFromFile("C:/Windows/Fonts/segoeui.ttf")) {
            std::cout << "Warning: Could not load any system font, text may not display correctly." << std::endl;
        }

        // 初始化游戏状态
        initGame();
    }

    void initGame() {
        // 重置游戏状态
        gameState = TicTacToeState();
        gameOver = false;
        gameStatus = "Game in progress - Player O's turn";

        // 初始化网格
        float cellSize = 150.0f;
        float gridStartX = 75.0f;
        float gridStartY = 100.0f;

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                grid[i][j].setSize(sf::Vector2f(cellSize, cellSize));
                grid[i][j].setPosition(gridStartX + j * cellSize, gridStartY + i * cellSize);
                grid[i][j].setFillColor(sf::Color(220, 220, 220));
                grid[i][j].setOutlineThickness(2.0f);
                grid[i][j].setOutlineColor(sf::Color::Black);

                gridText[i][j].setFont(font);
                gridText[i][j].setCharacterSize(80);
                gridText[i][j].setPosition(gridStartX + j * cellSize + 50, gridStartY + i * cellSize + 25);
                gridText[i][j].setFillColor(sf::Color::Black);
                gridText[i][j].setString("");
            }
        }

        // 初始化状态文本
        statusText.setFont(font);
        statusText.setCharacterSize(24);
        statusText.setPosition(150.0f, 30.0f);
        statusText.setFillColor(sf::Color::Black);
        statusText.setString("Game Start: You are player O, AI is player X");

        // 初始化重新开始按钮
        restartButton.setSize(sf::Vector2f(200.0f, 50.0f));
        restartButton.setPosition(75.0f, 600.0f);
        restartButton.setFillColor(sf::Color(100, 180, 100));
        
        restartText.setFont(font);
        restartText.setCharacterSize(24);
        restartText.setPosition(105.0f, 610.0f);
        restartText.setFillColor(sf::Color::White);
        restartText.setString("Restart");

        // 初始化难度按钮
        difficultyButton.setSize(sf::Vector2f(200.0f, 50.0f));
        difficultyButton.setPosition(325.0f, 600.0f);
        difficultyButton.setFillColor(sf::Color(100, 100, 180));
        
        difficultyText.setFont(font);
        difficultyText.setCharacterSize(24);
        difficultyText.setPosition(335.0f, 610.0f);
        difficultyText.setString("Difficulty: " + std::to_string(difficulty));
        difficultyText.setFillColor(sf::Color::White);

        // 如果AI是先手，让AI先走一步
        if (gameState.currentPlayer == 'X') {
            makeAIMove();
        }
    }

    void makeAIMove() {
        // 添加思考时间的随机延迟，使游戏更自然
        int thinkTime = 300 + (rand() % 500);
        sf::sleep(sf::milliseconds(thinkTime));
        
        if (!gameOver) {
            auto [bestRow, bestCol] = findBestMove(gameState, difficulty);
            gameState.makeMove(bestRow, bestCol);
            updateGridText();
            
            gameStatus = "AI moved - Player O's turn";
            checkGameState();
        }
    }

    void updateGridText() {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (gameState.board[i][j] == 'X') {
                    gridText[i][j].setString("X");
                    gridText[i][j].setFillColor(sf::Color::Blue);
                    // 调整X的位置，使其居中
                    gridText[i][j].setPosition(
                        grid[i][j].getPosition().x + grid[i][j].getSize().x/2 - 25,
                        grid[i][j].getPosition().y + grid[i][j].getSize().y/2 - 40
                    );
                } else if (gameState.board[i][j] == 'O') {
                    gridText[i][j].setString("O");
                    gridText[i][j].setFillColor(sf::Color::Red);
                    // 调整O的位置，使其居中
                    gridText[i][j].setPosition(
                        grid[i][j].getPosition().x + grid[i][j].getSize().x/2 - 25,
                        grid[i][j].getPosition().y + grid[i][j].getSize().y/2 - 40
                    );
                }
            }
        }
    }

    void checkGameState() {
        if (gameState.isTerminal()) {
            gameOver = true;
            
            if (gameState.checkWin('X')) {
                gameStatus = "Game Over - AI wins!";
            } else if (gameState.checkWin('O')) {
                gameStatus = "Game Over - Player wins!";
            } else {
                gameStatus = "Game Over - It's a draw!";
            }
        }
    }

    void handleMouseClick(int x, int y) {
        // 处理重新开始按钮点击
        if (restartButton.getGlobalBounds().contains(x, y)) {
            initGame();
            return;
        }
        
        // 处理难度按钮点击
        if (difficultyButton.getGlobalBounds().contains(x, y)) {
            difficulty = (difficulty % 9) + 1; // 循环 1-9
            difficultyText.setString("Difficulty: " + std::to_string(difficulty));
            return;
        }
        
        // 如果游戏已经结束或不是玩家回合，忽略点击
        if (gameOver || gameState.currentPlayer != 'O') {
            return;
        }
        
        // 检查玩家点击的是哪个格子
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (grid[i][j].getGlobalBounds().contains(x, y) && gameState.board[i][j] == ' ') {
                    // 执行玩家移动
                    gameState.makeMove(i, j);
                    updateGridText();
                    
                    gameStatus = "Player moved - AI thinking...";
                    checkGameState();
                    
                    // 如果游戏没有结束，让AI移动
                    if (!gameOver) {
                        makeAIMove();
                    }
                    return;
                }
            }
        }
    }

    void run() {
        // 设置随机数种子
        srand(static_cast<unsigned int>(time(0)));
        
        // 游戏主循环
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
                
                if (event.type == sf::Event::MouseButtonPressed) {
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        handleMouseClick(event.mouseButton.x, event.mouseButton.y);
                    }
                }
            }
            
            // 更新状态文本
            statusText.setString(gameStatus);
            
            // 渲染
            window.clear(sf::Color::White);
            
            // 绘制棋盘网格
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    window.draw(grid[i][j]);
                    window.draw(gridText[i][j]);
                }
            }
            
            // 绘制界面元素
            window.draw(statusText);
            window.draw(restartButton);
            window.draw(restartText);
            window.draw(difficultyButton);
            window.draw(difficultyText);
            
            window.display();
        }
    }
};

int main() {
    // 创建并运行游戏
    TicTacToeGame game;
    game.run();
    
    return 0;
} 