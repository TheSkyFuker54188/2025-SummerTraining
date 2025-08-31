#include <cstdio>
#include <cstring>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <vector>
#include <unordered_set>
#include <random>
#include <chrono>
#include <queue>

constexpr int maxN = 100;
constexpr int WIDTH = 40;
constexpr int HEIGHT = 30;
const std::string ID = "2023202300";
constexpr char ID1 = '0';

std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());

// 指示所有可能的方向

enum class Direction
{
    LEFT,
    UP,
    RIGHT,
    DOWN
};

const std::vector<Direction> validDirections{Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT};

namespace Utils
{
    // 检查是否越界
    bool boundCheck(int x, int y)
    {
        return (x >= 0) && (x < HEIGHT) && (y >= 0) && (y < WIDTH);
    }

    // 获取下一个位置，在 dir 方向上
    std::pair<int, int> nextPos(std::pair<int, int> pos, Direction dir)
    {
        switch (dir)
        {
        case Direction::LEFT:
            return std::make_pair(pos.first, pos.second - 1);
        case Direction::RIGHT:
            return std::make_pair(pos.first, pos.second + 1);
        case Direction::UP:
            return std::make_pair(pos.first - 1, pos.second);
        default:
            return std::make_pair(pos.first + 1, pos.second);
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

    // 随机地从一个 vector 中抽取一个元素
    template <typename T>
    T randomChoice(const std::vector<T> &vec)
    {
        std::uniform_int_distribution<> distrib(0, vec.size() - 1);
        const int randomIndex = distrib(gen);
        return vec[randomIndex];
    }

    // 将一个坐标转化为 string
    std::string idx2str(const std::pair<int, int> idx)
    {
        std::string a = std::to_string(idx.first);
        if (a.length() == 1)
        {
            a = "0" + a;
        }
        std::string b = std::to_string(idx.second);
        if (b.length() == 1)
        {
            b = "0" + b;
        }
        return a + b;
    }

    // 将一个 string 转化为数字坐标
    std::pair<int, int> str2idx(const std::string &str)
    {
        std::string substr1{str[0], str[1]};
        std::string substr2{str[2], str[3]};
        return std::make_pair(std::stoi(substr1), std::stoi(substr2));
    }
}

// mp 完全基于输入
// mp2 用来指示蛇的情况
int mp[HEIGHT][WIDTH], timeRest, totSnake;
int mp2[HEIGHT][WIDTH];

class Snake
{
public:
    int len, score, shieldCold, shieldLast;
    std::vector<std::pair<int, int>> pos;
    std::pair<int, int> head;
    std::pair<int, int> tail;
    Direction dir;

    void build(const int tag)
    {
        int tmpDir;
        std::cin >> len >> score >> tmpDir >> shieldCold >> shieldLast;
        switch (tmpDir)
        {
        case 0:
            dir = Direction::LEFT;
            break;
        case 1:
            dir = Direction::UP;
            break;
        case 2:
            dir = Direction::RIGHT;
            break;
        default:
            dir = Direction::DOWN;
            break;
        }
        for (int i = 0; i < len; i += 1)
        {
            int x, y;
            std::cin >> x >> y;
            if (Utils::boundCheck(x, y))
            {
                mp2[x][y] = tag;
                pos.emplace_back(x, y);
            }
        }
        // 策略：拒绝一切可能会死的步，通过预测蛇头与蛇尾的延伸方向
        head = pos[0];
        tail = pos[pos.size() - 1];
        if (tag != 10)
        {
            for (auto possible_dir : validDirections)
            {
                const auto [u, v] = Utils::nextPos(head, possible_dir);
                if (Utils::boundCheck(u, v))
                {
                    if (mp2[u][v] != -5)
                    {
                        mp2[u][v] = -6;
                    }
                }
            }
            for (auto possible_dir : validDirections)
            {
                const auto [u, v] = Utils::nextPos(tail, possible_dir);
                if (Utils::boundCheck(u, v))
                {
                    if (mp2[u][v] != -5 && mp2[u][v] != -6)
                    {
                        mp2[u][v] = -7;
                    }
                }
            }
        }
    }
};

Snake self, others[maxN];

// Builds the current state from std::cin
// 其他蛇的头和身子表示为 -5
void build()
{
    memset(mp2, 0, sizeof(mp2));
    int toolCount, snakeCount;
    std::cin >> timeRest >> toolCount;
    for (int i = 0; i < toolCount; i += 1)
    {
        int x, y, v;
        std::cin >> x >> y >> v;
        if (Utils::boundCheck(x, y))
        {
            mp[x][y] = v;
        }
    }
    std::cin >> snakeCount;
    for (int i = 0; i < snakeCount; i += 1)
    {
        std::string name;
        std::cin >> name;
        bool flag = false;
        if (name[name.size() - 1] == ID1)
        {
            if (name == ID)
            {
                // This is my snake.
                self.build(10);
                flag = true;
            }
        }
        if (!flag)
        {
            others[totSnake].build(-5);
            totSnake += 1;
        }
    }
}

// 这个函数返回所有的走了就必死的步
std::unordered_set<Direction> illegalMove()
{
    std::unordered_set<Direction> illegals;

    auto addReverse = [&]()
    {
        switch (self.dir)
        {
        case Direction::DOWN:
            illegals.insert(Direction::UP);
            break;
        case Direction::UP:
            illegals.insert(Direction::DOWN);
            break;
        case Direction::LEFT:
            illegals.insert(Direction::RIGHT);
            break;
        default:
            illegals.insert(Direction::LEFT);
            break;
        }
    };

    // 自己方向的反方向不能走
    addReverse();

    // 碰到其他蛇的身子不能走，但是开了护盾除外
    // 墙绝对不能走
    for (auto dir : validDirections)
    {
        const auto [u, v] = Utils::nextPos(self.head, dir);
        if (!Utils::boundCheck(u, v))
        {
            illegals.insert(dir);
        }
        else
        {
            if (mp[u][v] == -4)
            {
                illegals.insert(dir);
            }
            else if (mp2[u][v] == -5 || mp2[u][v] == -6 || mp2[u][v] == -7)
            {
                // 如果护盾时间快要结束，并且开不了护盾了
                // 一般护盾都是不值得的
                if (self.shieldLast < 2)
                {
                    illegals.insert(dir);
                }
            }
        }
    }

    // 如果四种方向都不行，试图放宽限制，可以开护盾
    if (illegals.size() == 4)
    {
        illegals.clear();

        // 自己方向的反方向不能走
        addReverse();

        // 碰到其他蛇的身子不能走，但是开了护盾除外
        // 墙绝对不能走
        for (auto dir : validDirections)
        {
            const auto [u, v] = Utils::nextPos(self.head, dir);
            if (!Utils::boundCheck(u, v))
            {
                illegals.insert(dir);
            }
            else
            {
                if (mp[u][v] == -4)
                {
                    illegals.insert(dir);
                }
                else if (mp2[u][v] == -5)
                {
                    // 如果护盾时间快要结束，并且开不了护盾了
                    // 一般护盾都是不值得的
                    if (!(self.shieldLast >= 2 || (self.shieldCold == 0 && self.score > 20) ||
                          timeRest >= 255 - 9 + 2))
                    {
                        illegals.insert(dir);
                    }
                    else if (self.shieldCold == 0 && self.score > 20)
                    {
                        std::cout << "4 ";
                    }
                }
            }
        }
    }

    return illegals;
}

namespace Strategy
{
    // 计算其他蛇离某个目标的距离，如果太近就放弃这个地方
    std::pair<int, int> count(int x, int y)
    {
        const int target = std::abs(self.head.first - x) + std::abs(self.head.second - y);
        int tot = 0;
        int minimized = target;
        for (int i = 0; i < totSnake; i += 1)
        {
            const int current = std::abs(others[i].head.first - x) + std::abs(others[i].head.second - y);
            if (current < target)
            {
                minimized = std::min(minimized, current);
                tot += 1;
            }
        }
        return std::make_pair(tot, minimized);
    }

    double bfs(int sx, int sy, int fx, int fy)
    {
        double score = 0;
        // 特殊检查：如果在关口位置，小心进入
        bool flag = false;
        if (sx == 9 && sy == 20)
        {
            const int t = mp2[18 - fx][20];
            flag = true;
            if (t == -5 || t == -6 || t == -7)
            {
                return -1000;
            }
        }
        if (sx == 21 && sy == 20)
        {
            const int t = mp2[42 - fx][20];
            flag = true;
            if (t == -5 || t == -6 || t == -7)
            {
                return -1000;
            }
        }
        if (sx == 15 && sy == 14)
        {
            const int t = mp2[15][28 - fy];
            flag = true;
            if (t == -5 || t == -6 || t == -7)
            {
                return -1000;
            }
        }
        if (sx == 15 && sy == 26)
        {
            const int t = mp2[15][52 - fy];
            flag = true;
            if (t == -5 || t == -6 || t == -7)
            {
                return -1000;
            }
        }
        if (mp[sx][sy] == -2 && !flag)
        {
            score = -1000;
        }
        const double start = 150, end = 25, maxNum = 5, maxNum2 = 30;
        if (mp[sx][sy] == -2 && flag && mp2[sx][sy] != -5 && mp2[sx][sy] != -6 && mp2[sx][sy] != -7)
        {
            score = -26;
            if (start > timeRest && timeRest >= end)
            {
                const double r = maxNum2 * (timeRest - start) * (timeRest - start);
                score += r / ((start - end) * (start - end));
            }
            return score;
        }
        else
        {
            if (start > timeRest && timeRest >= end)
            {
                const double r = maxNum * (timeRest - start) * (timeRest - start);
                score -= (std::fabs(sx - 14.5) + std::fabs(sy - 19.5)) * r / ((start - end) * (start - end));
            }
        }
        // 希望引入一个权重，用来将蛇“吸引”到中心
        // 权重不能太大
        // 学习北大的绩点制度（大雾）
        // 我们的估价函数是一个二次函数
        // 峰值在 end, 从 start 开始产生影响
        // 峰值影响系数为 maxNum
        // 经过 (start, 0) 和 (end, maxNum) 并以前者为顶点的二次函数
        // 形如 y = maxNum * (x - start) * (x - start) / ((start - end) * (start - end))

        std::unordered_set<std::string> visited;
        std::queue<std::pair<std::string, int>> q;
        std::string initial = Utils::idx2str(std::make_pair(sx, sy));
        q.emplace(initial, 1);
        visited.insert(initial);
        while (!q.empty())
        {
            auto [currentState, layer] = q.front();
            q.pop();
            // You look only x
            // 视野逐步缩小...
            // 视野峰值是 maxEye, 最小值是 minEye
            // 以 (start, maxEye) 和 (end, minEye) 为上面的点
            // 且以 start 为对称轴
            // 形如 y = a(x - start) * (x - start) + maxEye
            // (end - start) * (end - start) * a = - maxEye
            // 最大 layer: 20
            // 最小 layer: 10
            double maxLayer = 12;
            if (timeRest < 50)
            {
                maxLayer = 10;
            }
            /*
            const double maxEye = 15, minEye = 9;
            const double startEyeEffect = 120, endEyeEffect = 20;
            double maxLayer;
            if (timeRest >= startEyeEffect) {
                maxLayer = maxEye;
            } else if (timeRest < endEyeEffect) {
                maxLayer = minEye;
            } else {
                const double a = -maxEye / ((endEyeEffect - startEyeEffect) * (endEyeEffect - startEyeEffect));
                maxLayer = a * (timeRest - start) * (timeRest - start) + maxEye;
            }*/
            if (layer >= maxLayer)
            {
                break;
            }
            const auto [u, v] = Utils::str2idx(currentState);
            double num = mp[u][v] * 1.0;
            if (mp[u][v] == -1)
            {
                num = 1.8;
            }
            if (mp[u][v] == -2)
            {
                num = -0.5;
            }
            double weight;
            auto [tot, mini] = count(u, v);
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
                weight = 3 - 0.5 * std::min(6, layer - 9);
            }
            if (mp[u][v] != -2)
            {
                if (tot >= 4)
                {
                    weight = 0.5;
                }
                if (tot >= 3 && mini <= 3)
                {
                    weight = 0.5;
                }
                if (tot >= 2 && mini <= 2)
                {
                    weight = 0.5;
                }
                if (tot >= 1 && mini <= 1)
                {
                    weight = 0.5;
                }
            }
            score += num * weight;
            for (auto dir : validDirections)
            {
                const auto [x, y] = Utils::nextPos(std::make_pair(u, v), dir);
                if (!Utils::boundCheck(x, y))
                {
                    // Invalid move.
                }
                else
                {
                    if (mp[x][y] == -4 || mp2[x][y] == -5)
                    {
                        // 有墙的话应该提供一些罚
                        if (layer == 1 && mp[x][y] == -4)
                        {
                            score -= 10;
                        }
                        // Invalid move.
                    }
                    else
                    {
                        std::string neighbour = Utils::idx2str(std::make_pair(x, y));
                        if (visited.find(neighbour) == visited.end())
                        {
                            q.emplace(neighbour, layer + 1);
                            visited.insert(neighbour);
                        }
                    }
                }
            }
        }
        return score;
    }

    // 一旦你选择进入了拐角，你就必须接受...
    // X S
    // X P Q
    // X X X
    // S 选择了 P，下一步就要选择 Q 了...
    int cornerEval(int x, int y, int fx, int fy)
    {
        // 从 fx fy 走到 x, y
        // 首先判断是不是 corner
        std::string target = Utils::idx2str({x, y});
        std::unordered_set<std::string> corners = {"0917", "1018", "1015", "1116", "1214", "1315",
                                                   "0923", "1022", "1025", "1124", "1226", "1325",
                                                   "1814", "1715", "2015", "1916", "2117", "2018",
                                                   "2123", "2022", "2025", "1924", "1826", "1725"};
        if (corners.count(target) == 0)
        {
            return -10000;
        }
        int checkWall = 0, qx, qy;
        if (mp[x - 1][y] == -4 || x - 1 == fx)
        {
            // 你的上面是墙
            checkWall += 100;
        }
        if (mp[x + 1][y] == -4 || x + 1 == fx)
        {
            checkWall -= 100;
        }
        if (mp[x][y - 1] == -4 || y - 1 == fy)
        {
            checkWall += 1;
        }
        if (mp[x][y + 1] == -4 || y + 1 == fy)
        {
            checkWall -= 1;
        }
        switch (checkWall)
        {
        case 1:
            qx = x;
            qy = y + 1;
            break;
        case -1:
            qx = x;
            qy = y - 1;
            break;
        case 100:
            qx = x + 1;
            qy = y;
            break;
        default:
            qx = x - 1;
            qy = y;
            break;
        }
        if (mp2[qx][qy] == -5 || mp2[qx][qy] == -6)
        {
            return -5000;
        }
        if (mp[qx][qy] == -2)
        {
            return 1;
        }
        return 0;
    }

    // 计算这一步的价值
    double eval(int x, int y, int fx, int fy)
    {
        int test = cornerEval(x, y, fx, fy);
        if (test != -10000)
        {
            // 你在 corner 了
            if (test == -5000)
            {
                return -50000;
            }
            if (test == 1)
            {
                mp[x][y] = -9;
            }
        }
        return bfs(x, y, fx, fy);
    }
}

// This function should return a number between 0 and 4.
int judge()
{
    std::unordered_set<Direction> illegals = illegalMove();
    std::vector<Direction> legalMoves;
    for (auto dir : validDirections)
    {
        if (illegals.count(dir) == 0)
        {
            legalMoves.emplace_back(dir);
        }
    }
    if (legalMoves.empty())
    {
        return 4;
    }
    double maxF = -4000;
    Direction choice = legalMoves[0];
    for (auto dir : legalMoves)
    {
        const auto [u, v] = Utils::nextPos(self.head, dir);
        double eval = Strategy::eval(u, v, self.head.first, self.head.second);
        if (eval > maxF)
        {
            maxF = eval;
            choice = dir;
        }
        // 如果相同，有 1/p 的几率不改换
        if (eval == maxF)
        {
            constexpr int p = 2;
            std::uniform_int_distribution<> distrib(0, p - 1);
            const int random = distrib(gen);
            if (random == 0)
            {
                choice = dir;
            }
        }
    }
    if (maxF == -4000)
    {
        return 4;
    }
    return Utils::dir2num(choice);
}

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    build();
    std::cout << judge() << std::endl;
    return 0;
}
