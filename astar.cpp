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

// simplified wrapper — no positions, so heuristic is always 0
// this makes A* behave exactly like Dijkstra (good for when we don't have coordinates)
PathResult runAstar(const CityGraph& graph, string startName, string goalName) {
    int n = graph.getNodeCount();
    vector<double> emptyX(n, 0.0);
    vector<double> emptyY(n, 0.0);
    return runAstar(graph, startName, goalName, emptyX, emptyY);
}

// ══════════ TEST MAIN ══════════
// compile: g++ -std=c++17 -o astar_test graph.cpp astar.cpp
// run: ./astar_test

// int main() {
//     cout << "--- Testing A* Algorithm ---\n" << endl;

//     CityGraph city;
//     city.addIntersection("Home");
//     city.addIntersection("Market");
//     city.addIntersection("Hospital");
//     city.addIntersection("Airport");
//     city.addIntersection("School");
//     city.addIntersection("Park");

//     city.addRoad("Home", "Market", 5.0);
//     city.addRoad("Home", "Hospital", 8.0);
//     city.addRoad("Market", "Airport", 7.0);
//     city.addRoad("Hospital", "Airport", 4.0);
//     city.addRoad("Airport", "School", 6.0);
//     city.addRoad("School", "Park", 3.0);
//     city.addRoad("Home", "Park", 15.0);

//     // test 1: A* without positions (h=0, should give same result as Dijkstra)
//     cout << "=== A* (no heuristic): Home -> Airport ===" << endl;
//     PathResult r1 = runAstar(city, "Home", "Airport");
//     if (r1.found) {
//         cout << "Cost: " << r1.totalCost << endl;
//         cout << "Path: ";
//         for (int i = 0; i < r1.path.size(); i++) {
//             cout << city.getNodeName(r1.path[i]);
//             if (i < r1.path.size() - 1) cout << " -> ";
//         }
//         cout << endl;
//         cout << "Nodes explored: " << r1.nodesExplored << endl;
//     }

//     // test 2: A* with positions (should explore fewer nodes)
//     cout << "\n=== A* (with positions): Home -> Airport ===" << endl;
//     vector<double> px = {0, 3, 0, 5, 7, 9};  // x coords for 6 nodes
//     vector<double> py = {0, 2, 5, 5, 3, 1};   // y coords for 6 nodes
//     PathResult r2 = runAstar(city, "Home", "Airport", px, py);
//     if (r2.found) {
//         cout << "Cost: " << r2.totalCost << endl;
//         cout << "Path: ";
//         for (int i = 0; i < r2.path.size(); i++) {
//             cout << city.getNodeName(r2.path[i]);
//             if (i < r2.path.size() - 1) cout << " -> ";
//         }
//         cout << endl;
//         cout << "Nodes explored: " << r2.nodesExplored << endl;
//     }

//     // test 3: Home -> Park
//     cout << "\n=== A* (no heuristic): Home -> Park ===" << endl;
//     PathResult r3 = runAstar(city, "Home", "Park");
//     if (r3.found) {
//         cout << "Cost: " << r3.totalCost << " | Path: ";
//         for (int i = 0; i < r3.path.size(); i++) {
//             cout << city.getNodeName(r3.path[i]);
//             if (i < r3.path.size() - 1) cout << " -> ";
//         }
//         cout << endl;
//     }

//     cout << "\n--- A* tests done! ---" << endl;
//     return 0;
// }