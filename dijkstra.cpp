#include "dijkstra.h"
#include <iostream>
#include <queue>
#include <climits>

using namespace std;

static vector<int> reconstructPath(vector<int>& parent, int startId, int goalId) {
    vector<int> path;
    int current = goalId;

    // walk backwards from goal to start using parent pointers
    while (current != -1) {
        path.push_back(current);
        current = parent[current];
    }

    // path is currently goal→start, we need start→goal
    int left = 0;
    int right = path.size() - 1;
    while (left < right) {
        int temp = path[left];
        path[left] = path[right];
        path[right] = temp;
        left++;
        right--;
    }

    return path;
}