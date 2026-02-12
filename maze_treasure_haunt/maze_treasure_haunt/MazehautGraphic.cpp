#include <graphics.h>       // EasyX图形库头文件
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stack>
#include <queue>
#include <cmath>
#include <windows.h>        
#include <commdlg.h>        
#include <algorithm>
#include <ctime>  

using namespace std;

// 窗口和侧边栏大小定义
const int WIN_WIDTH = 1200;
const int WIN_HEIGHT = 750;
const int SIDEBAR_WIDTH = 300;
const int MAP_AREA_WIDTH = WIN_WIDTH - SIDEBAR_WIDTH;

// 颜色定义
const COLORREF C_BG = RGB(40, 44, 52);         // 全局背景
const COLORREF C_PANEL = RGB(33, 37, 43);      // 侧边栏背景
const COLORREF C_WALL = RGB(70, 75, 85);       // 墙
const COLORREF C_ROAD = RGB(255, 255, 255);    // 路
const COLORREF C_START = RGB(0, 255, 0);       // 起点
const COLORREF C_END = RGB(255, 0, 0);         // 终点
const COLORREF C_PATH = RGB(255, 215, 0);      // 最终路径
const COLORREF C_VISITED = RGB(184, 184, 184); // 已访问
const COLORREF C_CURRENT = RGB(227, 157, 235); // 当前探路点

// 按钮颜色定义
const COLORREF C_BTN_IDLE = RGB(64, 169, 255);
const COLORREF C_BTN_HOVER = RGB(105, 192, 255);
const COLORREF C_BTN_TEXT = RGB(255, 255, 255);

// 地图状态常量
const int WALL = 0;
const int ROAD = 1;
const int VISITED = 2;
const int PATH = 3;

// 动画速度 (毫秒)
const int DELAY_TIME = 100;

// 打开Windows文件选择窗口
string openFileDialog() {
    OPENFILENAME ofn;
    char szFile[260];
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetHWnd();
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Map_Files\0*.txt\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileName(&ofn) == TRUE) return string(ofn.lpstrFile);
    return "";
}

// 节点结构体
struct Node {
    int x, y;
    Node* parent;
    int g, h, f; 

    Node(int _x, int _y, Node* _p = nullptr) : x(_x), y(_y), parent(_p), g(0), h(0), f(0) {}
    Node(int _x, int _y, Node* _p, int _g, int _h) : x(_x), y(_y), parent(_p), g(_g), h(_h) { f = g + h; }

	// 重载大于号，用于优先队列比较
    bool operator>(const Node& other) const { return f > other.f; }
};

// 按钮类
class Button {
public:
    int x, y, w, h;
    string text;
    int id;
    bool hover;
    Button(int _x, int _y, int _w, int _h, string _text, int _id)
        : x(_x), y(_y), w(_w), h(_h), text(_text), id(_id), hover(false) {
    }
    // 绘制按钮
    void draw() {
        setfillcolor(hover ? C_BTN_HOVER : C_BTN_IDLE);
        setlinecolor(C_PANEL);
        fillroundrect(x, y, x + w, y + h, 10, 10);
        setbkmode(TRANSPARENT);
        settextcolor(C_BTN_TEXT);
        settextstyle(18, 0, "微软雅黑");
        int tx = x + (w - textwidth(text.c_str())) / 2;
        int ty = y + (h - textheight(text.c_str())) / 2;
        outtextxy(tx, ty, text.c_str());
    }
    // 检测鼠标是否在按钮范围内
    bool checkHover(int mx, int my) {
        bool last = hover;
        hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
        return last != hover;
    }
    bool isClicked(int mx, int my) { return hover; }
};

// 迷宫求解类
class MazeSolver {
private:
    vector<vector<int>> mazeMap; // 原始地图
    vector<vector<int>> workMap; // 工作地图
    int rows, cols;
    int startX, startY, endX, endY;

    int totalPathCount = 0;
    int visitedCount = 0;   // 累计已访问节点数
    int currentPathLen = 0; // 当前路径长度

    // 日志信息
    string currentMsg = "欢迎使用迷宫求解系统";
    string currentSubMsg = "请加载或生成地图";

    int cellSize;           // 迷宫格子大小
	int offsetX, offsetY;   // 地图居中偏移量
    // 寻宝方向：下右上左
	int dx[4] = { 0, 1, 0, -1 };     
    int dy[4] = { 1, 0, -1, 0 };
    // 内存管理数组
    vector<Node*> garbageCollection; 

    // 绘制单个格子
    void drawCell(int r, int c, COLORREF color) {
        int left = offsetX + c * cellSize;
        int top = offsetY + r * cellSize;
        setfillcolor(color);
        setlinecolor(C_BG);
        fillrectangle(left, top, left + cellSize, top + cellSize);
    }

    // 动态计算地图布局
    void calcLayout() {
        if (rows == 0 || cols == 0) return;
        int availW = MAP_AREA_WIDTH - 40;
        int availH = WIN_HEIGHT - 40;
        int sizeByW = availW / cols;
        int sizeByH = availH / rows;
        cellSize = (sizeByW < sizeByH) ? sizeByW : sizeByH;
        if (cellSize > 80) cellSize = 80;
        if (cellSize < 5) cellSize = 5;
        int totalMapW = cols * cellSize;
        int totalMapH = rows * cellSize;
        offsetX = (MAP_AREA_WIDTH - totalMapW) / 2;
        offsetY = (WIN_HEIGHT - totalMapH) / 2;
    }

    // 曼哈顿距离
    int calcH(int x, int y) const { return abs(x - endX) + abs(y - endY); }

    // 侧边栏日志信息绘制
    void drawSidePanel() {
        int panelX = MAP_AREA_WIDTH + 15;
        int panelY = 535;                // 按钮下方
        int panelW = SIDEBAR_WIDTH - 30;
        int panelH = WIN_HEIGHT - panelY - 15;

        // 清除旧内容
        setfillcolor(C_PANEL);
        solidrectangle(panelX, panelY, panelX + panelW, panelY + panelH);

        // 分割线绘制
        setlinecolor(RGB(60, 65, 75));
        line(panelX, panelY, panelX + panelW, panelY);

        setbkmode(TRANSPARENT);
        settextstyle(18, 0, "微软雅黑");

        // 显示数据统计（第一行）
        int line1_Y = panelY + 15;
        settextcolor(RGB(170, 170, 170));
        outtextxy(panelX, line1_Y, "已访问:");
        settextcolor(RGB(100, 220, 100)); 
        char buf1[32];
        sprintf_s(buf1, "%d", visitedCount);
        outtextxy(panelX + 60, line1_Y, buf1);
        settextcolor(RGB(170, 170, 170));
        outtextxy(panelX + 130, line1_Y, "步数:");
        char buf2[32];
        if (currentPathLen > 0) {
            sprintf_s(buf2, "%d", currentPathLen);
            settextcolor(C_PATH); 
        }else {
            sprintf_s(buf2, "--");
            settextcolor(RGB(100, 100, 100));
        }
        outtextxy(panelX + 180, line1_Y, buf2);

        // 显示日志（第二行）
        int line2_Y = line1_Y + 35;
        setlinecolor(RGB(50, 55, 60));
        line(panelX, line2_Y - 5, panelX + panelW, line2_Y - 5);
        settextcolor(RGB(120, 180, 255)); 
        outtextxy(panelX, line2_Y, "系统消息:");
        settextcolor(RGB(255, 255, 255)); 
        settextstyle(20, 0, "微软雅黑");
        outtextxy(panelX, line2_Y + 25, currentMsg.c_str());

        // 副消息颜色设置
        if (!currentSubMsg.empty()) {
            if (currentSubMsg.find("注意") != string::npos || currentSubMsg.find("失败") != string::npos) settextcolor(C_END);
            else settextcolor(RGB(150, 155, 165));
            settextstyle(18, 0, "微软雅黑");
            outtextxy(panelX, line2_Y + 55, currentSubMsg.c_str());
        }
    }
public:
    MazeSolver() : rows(0), cols(0), cellSize(30), offsetX(0), offsetY(0) {}
    ~MazeSolver() { clearMemory(); }

    // 更新日志
    void log(string msg, string sub = "") {
        currentMsg = msg;
        currentSubMsg = sub;
        drawSidePanel();
    }

    // 更新统计
    void updateStats(int visited, int pathLen) {
        visitedCount = visited;
        currentPathLen = pathLen;
        drawSidePanel();
    }

    // 清理所有动态分配的节点
    void clearMemory() {
        for (auto node : garbageCollection) delete node;
        garbageCollection.clear();
    }

	// 绘制初始地图
    void drawInitialMap() {
        if (mazeMap.empty()) return;
        setfillcolor(C_BG);
        solidrectangle(0, 0, MAP_AREA_WIDTH, WIN_HEIGHT);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                drawCell(i, j, (mazeMap[i][j] == WALL) ? C_WALL : C_ROAD);
            }
        }
        drawCell(startX, startY, C_START);
        drawCell(endX, endY, C_END);
        drawSidePanel();
    }

    // 加载文件
    void loadMapGUI() {
        string fname = openFileDialog();
        if (fname.empty()) return;
        ifstream file(fname);
        if (!file.is_open()) { log("错误", "读取文件失败！"); return; }
        file >> rows >> cols;
        mazeMap.assign(rows, vector<int>(cols));
        char ch;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                file >> ch;
                if(ch=='|') mazeMap[i][j] = WALL;
                else if(ch=='.') mazeMap[i][j] = ROAD;
            }
        }
        file >> startX >> startY >> endX >> endY;
        file.close();

        calcLayout();
        workMap = mazeMap;
        log("地图加载成功", "请选择想要演示的地图搜索算法");
        drawInitialMap();
    }

    // 随机生成地图
    void generateMaze() {
        rows = 21; cols = 29;       // 使用奇数尺寸，保证围墙完整
        mazeMap.assign(rows, vector<int>(cols, WALL));
        stack<Node*> s;
        int sx = 1, sy = 1;         // 起点
        mazeMap[sx][sy] = ROAD;
        s.push(new Node(sx, sy)); 
        srand((unsigned)time(NULL));// 初始化随机种子
        while (!s.empty()) {
            Node* cur = s.top();
            vector<int> dirs;
            // 检查四周距离为2的位置是否为墙
            if (cur->x + 2 < rows - 1 && mazeMap[cur->x + 2][cur->y] == WALL) dirs.push_back(0); // 下
            if (cur->y + 2 < cols - 1 && mazeMap[cur->x][cur->y + 2] == WALL) dirs.push_back(1); // 右
            if (cur->x - 2 > 0 && mazeMap[cur->x - 2][cur->y] == WALL) dirs.push_back(2); // 上
            if (cur->y - 2 > 0 && mazeMap[cur->x][cur->y - 2] == WALL) dirs.push_back(3); // 左
         
            if (!dirs.empty()) {
                int dir = dirs[rand() % dirs.size()];    // 随机选一个方向
                int nx = cur->x, ny = cur->y;
                int mx = cur->x, my = cur->y;
                switch (dir) {
                    case 0: nx += 2; mx += 1; break;
                    case 1: ny += 2; my += 1; break;
                    case 2: nx -= 2; mx -= 1; break;
                    case 3: ny -= 2; my -= 1; break;
                }
                mazeMap[mx][my] = ROAD; // 打通墙
                mazeMap[nx][ny] = ROAD; // 打通目标
                s.push(new Node(nx, ny));
            }else{
                delete cur; 
                s.pop(); // 回溯
            }
        }
        startX = 1; startY = 1;
        endX = rows - 2; endY = cols - 2;

        calcLayout();
        workMap = mazeMap;
        log("生成完毕", "随机可解迷宫已就绪");
        drawInitialMap();
    }
    // 重置地图
    void reset() {
        if (mazeMap.empty()) { log("地图数据未加载"); return; }
        workMap = mazeMap;
        clearMemory();
        updateStats(0, 0);       // 数据归零
        drawInitialMap();
        log("状态已重置");
    }
    // 检查地图是否加载
    bool isReady() {
        if (mazeMap.empty()) { log("请先加载地图!"); return false; }
        return true;
    }
    // 检查节点是否有效
    bool isValid(int x, int y, const vector<vector<int>>& m) const {
        return (x >= 0 && x < rows && y >= 0 && y < cols && m[x][y] != WALL);
    }
    // 绘制回溯路径
    void markPath(Node* endNode) {
        Node* cur = endNode;
        int len = 0;
        while (cur != nullptr) {
            len++;
            workMap[cur->x][cur->y] = PATH;
            if (!((cur->x == startX && cur->y == startY) || (cur->x == endX && cur->y == endY))) {
                drawCell(cur->x, cur->y, C_PATH);
                Sleep(DELAY_TIME / 2);
            }
            cur = cur->parent;
        }
        drawCell(startX, startY, C_START);
        drawCell(endX, endY, C_END);
        updateStats(visitedCount, len); // 更新最终路径长度
        log("搜索完成", "路径已显示");
    }

    // DFS求解
    void solveDFS() {
        if (!isReady()) return;
        reset();
        log("运行 DFS...", "深度优先搜索中");
        stack<Node*> s;
        Node* start = new Node(startX, startY);
        garbageCollection.push_back(start);
        s.push(start);
        workMap[startX][startY] = VISITED;
        bool found = false;
        while (!s.empty()) {
            Node* cur = s.top(); s.pop();
            updateStats(++visitedCount, 0); 
            // 绘制当前格子
            if (!(cur->x == startX && cur->y == startY))
                drawCell(cur->x, cur->y, C_CURRENT);
            Sleep(DELAY_TIME);
            // 找到终点
            if (cur->x == endX && cur->y == endY) { markPath(cur); found = true; break; }
            // 标记已访问
            if (!(cur->x == startX && cur->y == startY))
                drawCell(cur->x, cur->y, C_VISITED);
            // 倒序入栈，保证处理DFS顺序
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
        if (!found) log("DFS 无解");
    }

    // BFS
    void solveBFS() {
        if (!isReady()) return;
        reset();
        log("运行 BFS...", "广度优先搜索中");
        queue<Node*> q;
        Node* start = new Node(startX, startY);
        garbageCollection.push_back(start);
        q.push(start);
        workMap[startX][startY] = VISITED;
        bool found = false;
        while (!q.empty()) {
            Node* cur = q.front(); q.pop();
            updateStats(++visitedCount, 0);
            if (!(cur->x == startX && cur->y == startY))
                drawCell(cur->x, cur->y, C_CURRENT);
            Sleep(DELAY_TIME);
            if (cur->x == endX && cur->y == endY) { markPath(cur); found = true; break; }
            if (!(cur->x == startX && cur->y == startY)) {
                drawCell(cur->x, cur->y, C_VISITED);
            }
            for (int i = 0; i < 4; i++) {
                int nx = cur->x + dx[i], ny = cur->y + dy[i];
                if (isValid(nx, ny, workMap) && workMap[nx][ny] != VISITED) {
                    workMap[nx][ny] = VISITED;
                    Node* next = new Node(nx, ny, cur);
                    garbageCollection.push_back(next);
                    q.push(next);
                }
            }
        }
        if (!found) log("BFS 无解");
    }

    // A* 
    void solveAStar() {
        if (!isReady()) return;
        reset();
        log("运行 A*...", "启发式搜索中");
        priority_queue<Node, vector<Node>, greater<Node>> pq;
        vector<vector<int>> minG(rows, vector<int>(cols, 999999));
        Node* root = new Node(startX, startY, nullptr, 0, calcH(startX, startY));
        garbageCollection.push_back(root);
        pq.push(*root);
        minG[startX][startY] = 0;
        bool found = false;
        while (!pq.empty()) {
            Node currentObj = pq.top();        
            pq.pop();               // 取出 F 值最小的
            Node* currPtr = new Node(currentObj);      // 把临时的栈变量变成永久的堆变量（currentObj是局部变量，currPtr只要不delete就一直存在）
            garbageCollection.push_back(currPtr);
            updateStats(++visitedCount, 0);
            int cx = currPtr->x; int cy = currPtr->y;
            if (!(cx == startX && cy == startY))
                drawCell(cx, cy, C_CURRENT);
            Sleep(DELAY_TIME);
            if (cx == endX && cy == endY) { markPath(currPtr); found = true; break; }
            workMap[cx][cy] = VISITED;
            if (!(cx == startX && cy == startY))
                drawCell(cx, cy, C_VISITED);
            for (int i = 0; i < 4; i++) {
                int nx = cx + dx[i]; int ny = cy + dy[i];
                if (isValid(nx, ny, workMap)) {
                    int newG = currPtr->g + 1;
                    if (newG < minG[nx][ny]) {       // 只有发现更短的路径才更新
                        minG[nx][ny] = newG;
                        Node nextNode(nx, ny, currPtr, newG, calcH(nx, ny));    // 父指针指向堆内存的稳固指针
                        pq.push(nextNode);
                    }
                }
            }
        }
        if (!found) log("A* 无解");
    }

    // 寻找所有路径 
    void solveAllPathsHelper(int x, int y, int currentStep) {
        if (totalPathCount >= 10) return;
        visitedCount++;
        updateStats(visitedCount, currentStep);
        workMap[x][y] = PATH;
        // 找到终点
        if (x == endX && y == endY) {
            totalPathCount++;
            drawCell(x, y, C_END);
            char buffer[50];
            sprintf_s(buffer, "发现第 %d 条路径", totalPathCount);
            log("找到路径!", buffer);
            Sleep(DELAY_TIME * 15); 
            workMap[x][y] = ROAD;   // 回溯
            return;
        }
        // 绘制路径点
        if (!(x == startX && y == startY)) {
            drawCell(x, y, C_PATH);
            Sleep(DELAY_TIME);
        }
        for (int i = 0; i < 4; i++) {
            if (totalPathCount >= 10) break;
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (isValid(nx, ny, workMap) && workMap[nx][ny] != PATH) {
                solveAllPathsHelper(nx, ny, currentStep + 1);
            }
        }
        // 回溯
        workMap[x][y] = ROAD;
        if (!((x == startX && y == startY) || (x == endX && y == endY))) {
            drawCell(x, y, C_VISITED);
            updateStats(visitedCount, currentStep - 1);
        }
    }
    void solveAllPaths() {
        if (!isReady()) return;
        reset();
        totalPathCount = 0;
        visitedCount = 0;
        log("寻找所有路径...");
        solveAllPathsHelper(startX, startY, 0);
        if (totalPathCount == 0) log("搜索结束", "该迷宫无解");
        else if (totalPathCount >= 10) log("搜索结束", "当前路径过多，仅演示前10条");
        else log("搜索结束", "所有路径已显示");
    }
};

int main() {
    initgraph(WIN_WIDTH, WIN_HEIGHT);     // 初始化 EasyX 窗口
    setbkcolor(C_BG);
    cleardevice();
    srand((unsigned)time(NULL));
    // 绘制侧边栏背景
    setfillcolor(C_PANEL);
    solidrectangle(MAP_AREA_WIDTH, 0, WIN_WIDTH, WIN_HEIGHT);
    settextcolor(RGB(220, 220, 220));
    settextstyle(28, 0, "微软雅黑");
    outtextxy(MAP_AREA_WIDTH + 40, 40, "算法控制台");

    MazeSolver solver;

    // 按钮布局
    int btnW = 220;
    int btnH = 50;
    int btnX = MAP_AREA_WIDTH + (SIDEBAR_WIDTH - btnW) / 2;
    int startY = 90;
    int gap = 55; 

    vector<Button> btns = {
        Button(btnX, startY, btnW, btnH, "加载地图文件", 1),
        Button(btnX, startY + gap, btnW, btnH, "随机生成地图", 2),
        Button(btnX, startY + gap * 2, btnW, btnH, "DFS (深度优先搜索)", 3),
        Button(btnX, startY + gap * 3, btnW, btnH, "BFS (广度优先搜索)", 4),
        Button(btnX, startY + gap * 4, btnW, btnH, "A* (启发式搜索)", 5),
        Button(btnX, startY + gap * 5, btnW, btnH, "寻找所有路径", 6),
        Button(btnX, startY + gap * 6, btnW, btnH, "重置状态", 7),
        Button(btnX, startY + gap * 7, btnW, btnH, "退出程序", 8)
    };
    for (auto& btn : btns) btn.draw();
    solver.log("欢迎使用迷宫求解系统", "请加载或生成地图");

    while (true) {
        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            bool redraw = false;
            // 鼠标悬停检测
            for (auto& btn : btns) if (btn.checkHover(msg.x, msg.y)) redraw = true;
            if (redraw) {
                BeginBatchDraw();
                for (auto& btn : btns) btn.draw();
                EndBatchDraw();
            }
            // 鼠标点击处理
            if (msg.uMsg == WM_LBUTTONDOWN) {
                int id = -1;
                for (auto& btn : btns) if (btn.isClicked(msg.x, msg.y)) id = btn.id;
                switch (id) {
                case 1: solver.loadMapGUI(); break;
                case 2: solver.generateMaze(); break;
                case 3: solver.solveDFS(); break;
                case 4: solver.solveBFS(); break;
                case 5: solver.solveAStar(); break;
                case 6: solver.solveAllPaths(); break;
                case 7: solver.reset(); break;
                case 8: closegraph(); return 0;
                }
            }
        }
        Sleep(10);
    }
    return 0;
}