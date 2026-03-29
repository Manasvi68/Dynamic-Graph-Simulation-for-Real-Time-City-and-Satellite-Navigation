#include "astar.h"
#include <iostream>
#include <queue>
#include <cmath>

using namespace std;

// calculates straight-line (Euclidean) distance between two nodes
// this is our heuristic for A* — it never overestimates the real distance
static double heuristicValue(int nodeA, int nodeB,
                             const vector<double>& posX, const vector<double>& posY) {
    double dx = posX[nodeA] - posX[nodeB];
    double dy = posY[nodeA] - posY[nodeB];
    return sqrt(dx * dx + dy * dy);
}

// full A* with Euclidean heuristic
PathResult runAstar(const CityGraph& graph, string startName, string goalName,
                    const vector<double>& posX, const vector<double>& posY) {
    PathResult result;
    result.found = false;
    result.totalCost = 0;
    result.nodesExplored = 0;

    int startId = graph.getNodeId(startName);
    int goalId = graph.getNodeId(goalName);
    int n = graph.getNodeCount();

    if (startId == -1 || goalId == -1) {
        cout << "Error: start or goal node doesn't exist." << endl;
        return result;
    }

    // g[i] = actual shortest distance from start to node i found so far
    vector<double> gScore(n, 1e18);
    gScore[startId] = 0;

    // parent pointers for path reconstruction
    vector<int> parent(n, -1);

    // visited nodes
    vector<bool> visited(n, false);

    // priority queue ordered by f = g + h (smallest f first)
    // pair is (fScore, nodeId)
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> openSet;

    double startH = heuristicValue(startId, goalId, posX, posY);
    openSet.push({startH, startId});  // f = 0 + h

    while (!openSet.empty()) {
        double currentF = openSet.top().first;
        int currentNode = openSet.top().second;
        openSet.pop();

        if (visited[currentNode]) continue;

        visited[currentNode] = true;
        result.nodesExplored++;

        // reached the goal?
        if (currentNode == goalId) {
            result.found = true;
            result.totalCost = gScore[goalId];

            // reconstruct path by walking parent pointers
            vector<int> path;
            int curr = goalId;
            while (curr != -1) {
                path.push_back(curr);
                curr = parent[curr];
            }
            // reverse the path
            int left = 0, right = path.size() - 1;
            while (left < right) {
                int temp = path[left];
                path[left] = path[right];
                path[right] = temp;
                left++;
                right--;
            }
            result.path = path;
            return result;
        }

        // explore neighbors
        vector<Edge> neighbors = graph.getNeighbors(currentNode);
        for (int i = 0; i < neighbors.size(); i++) {
            int neighborId = neighbors[i].to;
            double edgeWeight = neighbors[i].weight;

            double tentativeG = gScore[currentNode] + edgeWeight;

            if (!visited[neighborId] && tentativeG < gScore[neighborId]) {
                gScore[neighborId] = tentativeG;
                parent[neighborId] = currentNode;

                // f = g + h — actual cost so far + estimated cost to goal
                double h = heuristicValue(neighborId, goalId, posX, posY);
                double f = tentativeG + h;
                openSet.push({f, neighborId});
            }
        }
    }

    return result;  // no path found
}