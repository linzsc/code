#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <string>
using namespace std;

// 定义网格的大小和方向
const int dx[] = {1, 0, -1, 0, 1, -1, -1, 1}; // 8个方向：右、下、左、上、右下、左下、左上、右上
const int dy[] = {0, 1, 0, -1, 1, -1, 1, -1}; 

struct Node {
    int x, y; // 节点坐标
    int g, h; // g值（从起点到当前点的实际距离），h值（从当前点到终点的估算距离）
    Node* parent; // 父节点指针

    Node(int _x, int _y, int _g, int _h, Node* _parent = nullptr) 
        : x(_x), y(_y), g(_g), h(_h), parent(_parent) {}

    // 用于优先队列的比较函数
    bool operator>(const Node& other) const {
        return (g + h) > (other.g + other.h); // f = g + h
    }
};

// 检查坐标是否在网格内
bool isValid(int x, int y, int rows, int cols, const vector<vector<int>>& grid) {
    return x >= 0 && x < rows && y >= 0 && y < cols && grid[x][y] != 1; // 1代表障碍物
}

// 寻找跳点
bool isJumpPoint(int x, int y, int dir, const vector<vector<int>>& grid) {
    int nx = x + dx[dir];
    int ny = y + dy[dir];
    if (!isValid(nx, ny, grid.size(), grid[0].size(), grid)) {
        return false;
    }
    // 检查是否跳到一个非障碍点
    if (dir < 4) { // 直行方向
        return true;
    }
    return true; // 对于对角线方向也可以根据需要改进
}

// JPS的跳跃算法
void jump(int x, int y, int dir, const vector<vector<int>>& grid, vector<vector<bool>>& visited, queue<Node*>& toVisit) {
    if (!isValid(x, y, grid.size(), grid[0].size(), grid) || visited[x][y]) {
        return;
    }
    visited[x][y] = true;
    toVisit.push(new Node(x, y, 0, 0)); // 此处可以设置合适的g和h
    // 寻找跳点，跳到下一个有效的点
    for (int i = 0; i < 8; i++) {
        if (isJumpPoint(x, y, i, grid)) {
            jump(x + dx[i], y + dy[i], i, grid, visited, toVisit);
        }
    }
}

// JPS路径搜索
vector<Node*> JPS(const vector<vector<int>>& grid, Node* start, Node* end) {
    int rows = grid.size();
    int cols = grid[0].size();
    vector<vector<bool>> visited(rows, vector<bool>(cols, false));
    priority_queue<Node*, vector<Node*>, greater<Node*>> openList;
    openList.push(start);

    while (!openList.empty()) {
        Node* current = openList.top();
        openList.pop();
        
        if (current->x == end->x && current->y == end->y) {
            vector<Node*> path;
            while (current != nullptr) {
                path.push_back(current);
                current = current->parent;
            }
            reverse(path.begin(), path.end());
            return path;
        }

        for (int i = 0; i < 8; i++) {
            int nx = current->x + dx[i];
            int ny = current->y + dy[i];
            if (isValid(nx, ny, rows, cols, grid) && !visited[nx][ny]) {
                visited[nx][ny] = true;
                openList.push(new Node(nx, ny, current->g + 1, 0, current)); // 更新g值
            }
        }
    }
    return {}; // 无法找到路径
}

// 主程序
int main() {
    // 示例网格，0为通路，1为障碍物
    vector<vector<int>> grid = {
        {0, 0, 0, 0, 0},
        {0, 1, 1, 1, 0},
        {0, 0, 0, 1, 0},
        {0, 1, 0, 0, 0},
        {0, 0, 0, 0, 0}
    };
    
    Node* start = new Node(0, 0, 0, 0);
    Node* end = new Node(4, 4, 0, 0);

    vector<Node*> path = JPS(grid, start, end);

    if (!path.empty()) {
        cout << "Path found: ";
        for (Node* node : path) {
            cout << "(" << node->x << ", " << node->y << ") ";
        }
        cout << endl;
    } else {
        cout << "No path found!" << endl;
    }

    return 0;
}
