/*
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stack>
#include <queue>
#include <cmath>  

using namespace std;

const int WALL = 0;    // 墙壁
const int ROAD = 1;    // 通路
const int VISITED = 2; // 已访问 (算法过程标记)
const int PATH = 3;    // 最终路径 

// 节点结构体
struct Node {
    int x, y;
    Node* parent; // 父指针，用于回溯路径

    // A* 专用属性
    int g; // 起点到当前的代价
    int h; // 到终点的预估代价
    int f; // f = g + h

    // 构造函数DFS/BFS
    Node(int _x, int _y, Node* _p = nullptr)
        : x(_x), y(_y), parent(_p), g(0), h(0), f(0) {
    }

    // 构造函数A* 
    Node(int _x, int _y, Node* _p, int _g, int _h)
        : x(_x), y(_y), parent(_p), g(_g), h(_h) {
        f = g + h;
    }

    // 重载运算符f值越小越前
    bool operator>(const Node& other) const {
        return f > other.f;
    }
};

class MazeSolver {
private:
    vector<vector<int>> mazeMap; // 原始地图数据
    vector<vector<int>> workMap; // 工作地图 
    int rows, cols;
    int startX, startY;
    int endX, endY;
    int totalPathCount = 0;

    // 方向数组：右 下 左 上
    int dx[4] = { 0, 1, 0, -1 };
    int dy[4] = { 1, 0, -1, 0 };

    // 存储所有动态分配的节点，最后统一释放
    vector<Node*> garbageCollection;

    // 检查地图是否已加载
    bool isMapLoaded() {
        if (mazeMap.empty() || rows == 0 || cols == 0) {
            cout << "错误：地图未加载！请先执行操作1！" << endl;
            return false;
        }
        return true;
    }

    // 判断坐标是否有效且不是墙
    bool isValid(int x, int y, const vector<vector<int>>& currentMap) const{
        return (x >= 0 && x < rows && y >= 0 && y < cols && currentMap[x][y] != WALL);
    }

    // 计算曼哈顿距离
    int calcH(int x, int y) const{
        return abs(x - endX) + abs(y - endY);
    }

    // 清理内存
    void clearMemory() {
        for (auto node : garbageCollection) {
            delete node;
        }
        garbageCollection.clear();
    }

    // 回溯路径 
    void markPath(Node* endNode) {
        Node* cur = endNode;
        while (cur != nullptr) {
            workMap[cur->x][cur->y] = PATH; // 标记为路径
            cur = cur->parent;
        }
    }

public:
    MazeSolver() : rows(0), cols(0), startX(0), startY(0), endX(0), endY(0) {}
    ~MazeSolver() { clearMemory(); }

    // 读取文件
    bool loadMap(const string& filename){
        ifstream file(filename);
        if(!file.is_open()){
            cout << "错误，无法打开文件！" << filename << endl;
            return false;
        }

        file >> rows >> cols;
        mazeMap.assign(rows, vector<int>(cols));
        char ch;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                file >> ch;
                cout << ch <<" ";
                if (ch == '.') mazeMap[i][j] = ROAD;
                else if (ch == '|') mazeMap[i][j] = WALL;
            }
            cout << endl;
        }
        file >> startX >> startY;
        file >> endX >> endY;
        file.close();

        cout << "地图加载成功! 大小: " << rows << "x" << cols << endl;
        return true;
    }

    // 打印迷宫
    void printResult(string algoName) {
        cout << "\n--- " << algoName << " 结果展示 ---" << endl;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (i == startX && j == startY) cout << "S "; // 起点
                else if (i == endX && j == endY) cout << "E "; // 终点
                else if (workMap[i][j] == WALL) cout << "| "; // 墙
                else if (workMap[i][j] == PATH) cout << "$ "; // 最终路径 
                else cout << ". "; //通路
            }
            cout << endl;
        }
        cout << "-------------------------" << endl;
    }

    // 重置工作地图
    void reset() {
        workMap = mazeMap; // 拷贝原始地图
        clearMemory();     // 清理上一次算法的内存
    }

    void solveDFS() {
        if (!isMapLoaded()) return;
        reset();
        stack<Node*> s;

        // 起点入栈
        Node* startNode = new Node(startX, startY);
        garbageCollection.push_back(startNode);
        s.push(startNode);
        workMap[startX][startY] = VISITED;

        bool found = false;

        while (!s.empty()) {
            Node* cur = s.top();
            s.pop();

            // 判断是否到达终点
            if (cur->x == endX && cur->y == endY) {
                markPath(cur);
                found = true;
                break;
            }
            // 尝试 4 个方向
            for (int i = 3; i >= 0; i--) {
                int nx = cur->x + dx[i];
                int ny = cur->y + dy[i];
                if (isValid(nx, ny, workMap) && workMap[nx][ny] != VISITED) {
                    workMap[nx][ny] = VISITED; // 标记已访问
                    Node* nextNode = new Node(nx, ny, cur); // 记录父节点
                    garbageCollection.push_back(nextNode);
                    s.push(nextNode);
                }
            }
        }

        if (found) printResult("深度优先搜索（DFS）");
        else cout << "DFS: 无解!" << endl;
    }

    void solveBFS() {
        if (!isMapLoaded()) return;
        reset();
        queue<Node*> q;
        // 起点入队
        Node* startNode = new Node(startX, startY);
        garbageCollection.push_back(startNode);
        q.push(startNode);
        workMap[startX][startY] = VISITED;

        bool found = false;

        while (!q.empty()) {
            Node* cur = q.front();
            q.pop();

            if (cur->x == endX && cur->y == endY) {
                markPath(cur);
                found = true;
                break;
            }

            for (int i = 0; i < 4; ++i) {
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

        if (found) printResult("广度优先搜索（BFS）");
        else cout << "BFS: 无解!" << endl;
    }

    void solveAStar() {
        if (!isMapLoaded()) return;
        reset();
        priority_queue<Node, vector<Node>, greater<Node>> pq; //自定义优先级需指明底层容器和比较函数

        // 记录每个点的最小G值，防止走回头路或绕远路
        vector<vector<int>> minG(rows, vector<int>(cols, 999999));

        int startH = calcH(startX, startY);
        // 起点入队
        Node* root = new Node(startX, startY, nullptr, 0, startH);
        garbageCollection.push_back(root);
        pq.push(*root); 
        minG[startX][startY] = 0;

        bool found = false;
        Node* finalNode = nullptr;

        while (!pq.empty()) {
            Node currentObj = pq.top(); // 取出F最小的
            pq.pop();
            Node* currPtr = nullptr;

            currPtr = new Node(currentObj);
            garbageCollection.push_back(currPtr);

            int cx = currPtr->x;
            int cy = currPtr->y;

            if (cx == endX && cy == endY) {
                finalNode = currPtr;
                found = true;
                break;
            }

            // 标记为已访问
            workMap[cx][cy] = VISITED;

            for (int i = 0; i < 4; i++) {
                int nx = cx + dx[i];
                int ny = cy + dy[i];
                if (isValid(nx, ny, workMap)) {
                    int newG = currPtr->g + 1;
                    // 如果新路径的G值更小，说明找到了更优路径
                    if (newG < minG[nx][ny]) {
                        minG[nx][ny] = newG;
                        int newH = calcH(nx, ny);
                        Node nextNode(nx, ny, currPtr, newG, newH);
                        pq.push(nextNode);   // priority_queue 只能进不能改，进顶替原来那个
                    }
                }
            }
        }

        if (found) {
            markPath(finalNode);
            printResult("A* 算法");
        }
        else {
            cout << "A*: 无解!" << endl;
        }
    }

    void solveAllPaths(int x = -1, int y = -1) {
        if (!isMapLoaded()) return;
        // 初始化 (总管逻辑)
        if (x == -1 || y == -1) {
            reset();
            totalPathCount = 0;  
            int limit = 10;     
            cout << "\n正在寻找所有路径..." << endl;
            // 启动递归
            solveAllPaths(startX, startY);
            // 总结汇报
            if (totalPathCount == 0) {
                cout << "没有找到任何通路" << endl;
            }
            else {
                cout << "\n搜索结束" << endl;
                if (totalPathCount >= limit) {
                    cout << "注意：迷宫路径较多，仅展示前 " << limit << " 条路径" << endl;
                }
                else {
                    cout << "共找到 " << totalPathCount << " 条通路" << endl;
                }
            }
            return; 
        }
        if (totalPathCount >= 10) {
            return;
        }
        workMap[x][y] = PATH; // 标记

        if (x == endX && y == endY) {
            totalPathCount++;
            printResult("路径 " + to_string(totalPathCount));

            workMap[x][y] = ROAD; // 回溯
            return;
        }

        for (int i = 0; i < 4; i++) {
            if (totalPathCount >= 10) break;
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (isValid(nx, ny, workMap) && workMap[nx][ny] != PATH) {
                solveAllPaths(nx, ny);
            }
        }
        // 回溯
        workMap[x][y] = ROAD;
    }
};

int main(){
    MazeSolver solver;
    string choice;

    while(true){
        cout << "\n================ 迷宫求解系统 ================" << endl;
        cout << "1. 读取迷宫地图" << endl;
        cout << "2. 深度优先搜索求解" << endl;
        cout << "3. 广度优先搜索求解" << endl;
        cout << "4. A*算法求解" << endl;
        cout << "5. 显示所有通路" << endl;
        cout << "Q. 退出系统" << endl;
        cout << "=============================================" << endl;
        cout << "请输入您的选择: ";
        cin >> choice;

        if(choice == "1"){
            string fname;
            cout << "请输入迷宫地图的文件名（如xxx.txt）: ";
            cin >> fname;
            solver.loadMap(fname);
        }
        else if (choice == "2") {
            solver.solveDFS();
        }
        else if (choice == "3") {
            solver.solveBFS();
        }
        else if (choice == "4") {
            solver.solveAStar();
        }
        else if (choice == "5") {
            solver.solveAllPaths(); 
        }
        else if (choice == "Q" || choice == "q") {
            cout << "退出程序" << endl;
            break;
        }
        else {
            cout << "无效输入，请重试!" << endl;
        }
    }
    return 0;
}
*/