#include "dijkstra.h"
#include "graph.h"
#include <cmath>
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

PathResult runDijkstra(const CityGraph& graph, string startName, string goalName) {
    PathResult result;
    result.found = false;
    result.totalCost = 0;
    result.nodesExplored = 0;

    int startId = graph.getNodeId(startName);
    int goalId = graph.getNodeId(goalName);
    int n = graph.getNodeCount();

    // check if both nodes exist
    if (startId == -1 || goalId == -1) {
        cout << "Error: start or goal node doesn't exist." << endl;
        return result;
    }

    // dist[i] = shortest known distance from start to node i
    vector<double> dist(n, 1e18);  // start with "infinity"
    dist[startId] = 0;

    // parent[i] = which node leads to i on the shortest path
    vector<int> parent(n, -1);

    // visited[i] = true once we've finalized node i's distance
    vector<bool> visited(n, false);

    // min-heap priority queue: (distance, nodeId)
    // smallest distance comes out first
    // we use greater<> to make it a min-heap (default is max-heap)
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;

    pq.push({0.0, startId});

    while (!pq.empty()) {
        // grab the unvisited node with smallest distance
        double currentDist = pq.top().first;
        int currentNode = pq.top().second;
        pq.pop();

        // skip if already finalized
        if (visited[currentNode]) continue;

        visited[currentNode] = true;
        result.nodesExplored++;

        // if we just reached the goal, we're done!
        if (currentNode == goalId) {
            result.found = true;
            result.totalCost = dist[goalId];
            result.path = reconstructPath(parent, startId, goalId);
            return result;
        }

        // relax all neighbors
        vector<Edge> neighbors = graph.getNeighbors(currentNode);
        for (int i = 0; i < neighbors.size(); i++) {
            int neighborId = neighbors[i].to;
            double edgeWeight = neighbors[i].weight;

            if (!std::isfinite(edgeWeight)) continue;

            // can we reach this neighbor faster through currentNode?
            if (!visited[neighborId] && currentDist + edgeWeight < dist[neighborId]) {
                dist[neighborId] = currentDist + edgeWeight;
                parent[neighborId] = currentNode;
                pq.push({dist[neighborId], neighborId});
            }
        }
    }

    // if we get here, no path was found
    return result;
}

// ══════════ TEST MAIN ══════════
// compile: g++ -std=c++17 -o dijkstra_test graph.cpp dijkstra.cpp
// run: ./dijkstra_test

// int main() {
//     cout << "--- Testing Dijkstra's Algorithm ---\n" << endl;

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

//     city.printGraph();

//     // test 1: Home to Airport
//     cout << "=== Path: Home -> Airport ===" << endl;
//     PathResult r1 = runDijkstra(city, "Home", "Airport");
//     if (r1.found) {
//         cout << "Cost: " << r1.totalCost << endl;
//         cout << "Path: ";
//         for (int i = 0; i < r1.path.size(); i++) {
//             cout << city.getNodeName(r1.path[i]);
//             if (i < r1.path.size() - 1) cout << " -> ";
//         }
//         cout << endl;
//         cout << "Nodes explored: " << r1.nodesExplored << endl;
//     } else {
//         cout << "No path found!" << endl;
//     }

//     // test 2: Home to Park (should prefer Home->Airport->School->Park = 18 over Home->Park = 15)
//     cout << "\n=== Path: Home -> Park ===" << endl;
//     PathResult r2 = runDijkstra(city, "Home", "Park");
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

//     // test 3: non-existent path
//     cout << "\n=== Path: Park -> Home (check bidirectional) ===" << endl;
//     PathResult r3 = runDijkstra(city, "Park", "Home");
//     if (r3.found) {
//         cout << "Cost: " << r3.totalCost << endl;
//         cout << "Path: ";
//         for (int i = 0; i < r3.path.size(); i++) {
//             cout << city.getNodeName(r3.path[i]);
//             if (i < r3.path.size() - 1) cout << " -> ";
//         }
//         cout << endl;
//     } else {
//         cout << "No path found!" << endl;
//     }

//     cout << "\n--- Dijkstra tests done! ---" << endl;
//     return 0;
// }