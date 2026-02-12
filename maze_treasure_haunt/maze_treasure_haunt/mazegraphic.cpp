/*
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stack>
#include <queue>
#include <cmath>
#include <conio.h>   
#include <graphics.h> 

using namespace std;

const int CELL_SIZE = 60; // 每个格子的大小
const int DELAY_TIME = 100; // 动画延时 (毫秒)

// 定义颜色 
const COLORREF COLOR_WALL = RGB(50, 50, 50);       // 深灰墙壁
const COLORREF COLOR_ROAD = RGB(255, 255, 255);    //白色通路
const COLORREF COLOR_START = RGB(0, 255, 0);       // 绿色起点
const COLORREF COLOR_END = RGB(255, 0, 0);         // 红色终点
const COLORREF COLOR_VISITED = RGB(173, 216, 230); // 浅蓝 (探索过的区域)
const COLORREF COLOR_PATH = RGB(255, 215, 0);      // 金色 (最终路径)
const COLORREF COLOR_CURRENT = RGB(255, 105, 180); // 粉色 (当前的探头)

const int WALL = 0;
const int ROAD = 1;
const int VISITED = 2;
const int PATH = 3;

struct Node {
    int x, y;
    Node* parent;
    int g, h, f;
    // 通用构造
    Node(int _x, int _y, Node* _p = nullptr)
        : x(_x), y(_y), parent(_p), g(0), h(0), f(0) {
    }
    // A*构造
    Node(int _x, int _y, Node* _p, int _g, int _h)
        : x(_x), y(_y), parent(_p), g(_g), h(_h) {
        f = g + h;
    }
    // 最小堆重载
    bool operator>(const Node& other) const { return f > other.f; }
};

class MazeSolver {
private:
    vector<vector<int>> mazeMap;
    vector<vector<int>> workMap;
    int rows, cols;
    int startX, startY;
    int endX, endY;
    int totalPathCount = 0;

    // 方向：右 下 左 上
    int dx[4] = { 0, 1, 0, -1 };
    int dy[4] = { 1, 0, -1, 0 };

    vector<Node*> garbageCollection;

    // 绘制单个格子
    void drawCell(int r, int c, COLORREF color) {
        int left = c * CELL_SIZE;
        int top = r * CELL_SIZE;
        int right = left + CELL_SIZE;
        int bottom = top + CELL_SIZE;

        setfillcolor(color);
        setlinecolor(RGB(200, 200, 200)); // 格子边框颜色
        fillrectangle(left, top, right, bottom);
    }

    void initGraphics() {
        int width = cols * CELL_SIZE;
        int height = rows * CELL_SIZE;

        initgraph(width, height);

        // 设置背景
        setbkcolor(WHITE);
        cleardevice();
    }

    // 绘制完整初始地图
    void drawInitialMap() {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (mazeMap[i][j] == WALL) drawCell(i, j, COLOR_WALL);
                else drawCell(i, j, COLOR_ROAD);
            }
        }
        // 画起点终点
        drawCell(startX, startY, COLOR_START);
        drawCell(endX, endY, COLOR_END);
    }

    bool isMapLoaded() {
        if (mazeMap.empty()) {
            cout << "错误：地图未加载！" << endl;
            return false;
        }
        return true;
    }

    bool isValid(int x, int y, const vector<vector<int>>& currentMap) const {
        return (x >= 0 && x < rows && y >= 0 && y < cols && currentMap[x][y] != WALL);
    }

    int calcH(int x, int y) const { return abs(x - endX) + abs(y - endY); }

    void clearMemory() {
        for (auto node : garbageCollection) delete node;
        garbageCollection.clear();
    }

    void markPath(Node* endNode) {
        Node* cur = endNode;
        // 回溯并绘制路径
        while (cur != nullptr) {
            workMap[cur->x][cur->y] = PATH;

            // 排除起点和终点，避免颜色覆盖
            if (!((cur->x == startX && cur->y == startY) || (cur->x == endX && cur->y == endY))) {
                drawCell(cur->x, cur->y, COLOR_PATH);
                Sleep(100); // 绘制路径时的动画效果
            }
            cur = cur->parent;
        }
        // 补画起点终点
        drawCell(startX, startY, COLOR_START);
        drawCell(endX, endY, COLOR_END);
    }

public:
    MazeSolver() : rows(0), cols(0), startX(0), startY(0), endX(0), endY(0) {}
    ~MazeSolver() { clearMemory(); closegraph(); }

    bool loadMap(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cout << "无法打开文件！" << endl;
            return false;
        }
        file >> rows >> cols;
        mazeMap.assign(rows, vector<int>(cols));

        char ch;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                file >> ch;
                mazeMap[i][j] = (ch == '|') ? WALL : ROAD;
            }
        }
        file >> startX >> startY >> endX >> endY;
        file.close();

        cout << "地图加载成功！启动图形窗口..." << endl;

        // 加载完立即初始化窗口
        initGraphics();
        drawInitialMap();
        return true;
    }

    void reset() {
        workMap = mazeMap;
        clearMemory();
        // 重绘地图，清除之前的痕迹
        drawInitialMap();
    }

    void solveDFS() {
        if (!isMapLoaded()) return;
        reset();
        stack<Node*> s;
        Node* startNode = new Node(startX, startY);
        garbageCollection.push_back(startNode);
        s.push(startNode);
        workMap[startX][startY] = VISITED;

        bool found = false;
        while (!s.empty()) {
            Node* cur = s.top();
            s.pop();

            // 绘制当前正在处理的点 (粉色)
            if (!(cur->x == startX && cur->y == startY))
                drawCell(cur->x, cur->y, COLOR_CURRENT);
            Sleep(DELAY_TIME);

            if (cur->x == endX && cur->y == endY) {
                markPath(cur);
                found = true;
                break;
            }

            // 处理完变回已访问色 (浅蓝)
            if (!(cur->x == startX && cur->y == startY))
                drawCell(cur->x, cur->y, COLOR_VISITED);

            for (int i = 3; i >= 0; i--) {
                int nx = cur->x + dx[i];
                int ny = cur->y + dy[i];
                if (isValid(nx, ny, workMap) && workMap[nx][ny] != VISITED) {
                    workMap[nx][ny] = VISITED;
                    Node* nextNode = new Node(nx, ny, cur);
                    garbageCollection.push_back(nextNode);
                    s.push(nextNode);
                }
            }
        }
        if (!found) cout << "DFS 无解" << endl;
    }

    void solveBFS() {
        if (!isMapLoaded()) return;
        reset();
        queue<Node*> q;
        Node* startNode = new Node(startX, startY);
        garbageCollection.push_back(startNode);
        q.push(startNode);
        workMap[startX][startY] = VISITED;

        bool found = false;
        while (!q.empty()) {
            Node* cur = q.front();
            q.pop();

            if (!(cur->x == startX && cur->y == startY))
                drawCell(cur->x, cur->y, COLOR_CURRENT);
            Sleep(DELAY_TIME); 

            if (cur->x == endX && cur->y == endY) {
                markPath(cur);
                found = true;
                break;
            }

            if (!(cur->x == startX && cur->y == startY))
                drawCell(cur->x, cur->y, COLOR_VISITED);

            for (int i = 0; i < 4; i++) {
                int nx = cur->x + dx[i];
                int ny = cur->y + dy[i];
                if (isValid(nx, ny, workMap) && workMap[nx][ny] != VISITED) {
                    workMap[nx][ny] = VISITED;
                    Node* nextNode = new Node(nx, ny, cur);
                    garbageCollection.push_back(nextNode);
                    q.push(nextNode);
                }
            }
        }
        if (!found) cout << "BFS 无解" << endl;
    }

    void solveAStar() {
        if (!isMapLoaded()) return;
        reset();
        priority_queue<Node, vector<Node>, greater<Node>> pq;
        vector<vector<int>> minG(rows, vector<int>(cols, 999999));

        Node* root = new Node(startX, startY, nullptr, 0, calcH(startX, startY));
        garbageCollection.push_back(root);
        pq.push(*root);
        minG[startX][startY] = 0;

        bool found = false;
        while (!pq.empty()) {
            Node currentObj = pq.top();
            pq.pop();

            // 恢复指针逻辑
            Node* currPtr = new Node(currentObj);
            garbageCollection.push_back(currPtr);

            int cx = currPtr->x;
            int cy = currPtr->y;

            if (!(cx == startX && cy == startY))
                drawCell(cx, cy, COLOR_CURRENT);
            Sleep(DELAY_TIME);

            if (cx == endX && cy == endY) {
                markPath(currPtr);
                found = true;
                break;
            }

            // 动画展示"已探索区域"
            workMap[cx][cy] = VISITED;
            if (!(cx == startX && cy == startY))
                drawCell(cx, cy, COLOR_VISITED);

            for (int i = 0; i < 4; i++) {
                int nx = cx + dx[i];
                int ny = cy + dy[i];
                if (isValid(nx, ny, workMap)) { 
                    int newG = currPtr->g + 1;
                    if (newG < minG[nx][ny]) {
                        minG[nx][ny] = newG;
                        Node nextNode(nx, ny, currPtr, newG, calcH(nx, ny));
                        pq.push(nextNode);
                    }
                }
            }
        }
        if (!found) cout << "A* 无解" << endl;
    }

    void solveAllPaths(int x = -1, int y = -1) {
        if (!isMapLoaded()) return;
        if (x == -1) {
            reset();
            totalPathCount = 0;
            int limit = 10; 
            cout << "正在寻找所有路径..." << endl;

            solveAllPaths(startX, startY);

            if (totalPathCount == 0) {
                cout << "没有找到任何通路。" << endl;
            }
            else {
                if (totalPathCount >= limit) {
                    cout << "注意：路径迷宫路径较多，仅展示前 " << limit << " 条路径" << endl;
                }
                else {
                    cout << "共找到 " << totalPathCount << " 条通路。" << endl;
                }
            }
            return; 
        }

        if (totalPathCount >= 10) return;

        workMap[x][y] = PATH; // 标记当前点

        if (x == endX && y == endY) {
            // 如果是终点，直接染红
            drawCell(x, y, COLOR_END);
        }
        else if (!(x == startX && y == startY)) {
            drawCell(x, y, COLOR_PATH);
            Sleep(DELAY_TIME);
        }

        // 判断是否到达终点
        if (x == endX && y == endY) {
            totalPathCount++;
            cout << ">>> 发现第 " << totalPathCount << " 条路径! <<<" << endl;
            Sleep(1000);

            workMap[x][y] = ROAD;

            drawCell(x, y, COLOR_END);
            return;
        }

        // 遍历四个方向
        for (int i = 0; i < 4; i++) {
            if (totalPathCount >= 10) break;
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (isValid(nx, ny, workMap) && workMap[nx][ny] != PATH) {
                solveAllPaths(nx, ny);
            }
        }
        workMap[x][y] = ROAD;

        // 擦除颜色，表示退回
        if (!((x == startX && y == startY) || (x == endX && y == endY))) {
            drawCell(x, y, COLOR_ROAD);
        }
    }
};

int main() {
    MazeSolver solver;
    string choice;

    while (true) {
        cout << "\n================ 迷宫求解系统 ================" << endl;
        cout << "1. 读取迷宫地图" << endl;
        cout << "2. 深度优先搜索求解演示" << endl;
        cout << "3. 广度优先搜索求解求解" << endl;
        cout << "4. A*算法求解演示" << endl;
        cout << "5. 显示所有通路" << endl;
        cout << "Q. 退出系统" << endl;
        cout << "-==============================================" << endl;
        cout << "请输入您的选择: ";
        cin >> choice;

        if (choice == "1") {
            string fname;
            cout << "请输入迷宫文件名 (例如 maze.txt): ";
            cin >> fname;
            solver.loadMap(fname);
        }
        else if (choice == "2") solver.solveDFS();
        else if (choice == "3") solver.solveBFS();
        else if (choice == "4") solver.solveAStar();
        else if (choice == "5") solver.solveAllPaths();
        else if (choice == "Q" || choice == "q") {
            cout << "退出程序。" << endl;
            break;
        }
        else {
            cout << "无效输入，请重试。" << endl;
        }
    }
    return 0;
}
*/