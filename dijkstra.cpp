#include "dijkstra.h"
#include "graph.h"
#include <iostream>
#include <queue>
#include <climits>

using namespace std;

static vector<int> reconstructPath(vector<int>& parent, int startId, int goalId) {
    vector<int> path;
    int current = goalId;
    while (current != -1) {
        path.push_back(current);
        current = parent[current];
    }
    int left = 0, right = path.size() - 1;
    while (left < right) {
        int temp = path[left];
        path[left] = path[right];
        path[right] = temp;
        left++; right--;
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

    if (startId == -1 || goalId == -1) return result;

    vector<double> dist(n, 1e18);
    dist[startId] = 0;
    vector<int> parent(n, -1);
    vector<bool> visited(n, false);

    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;
    pq.push({0.0, startId});

    while (!pq.empty()) {
        double currentDist = pq.top().first;
        int currentNode = pq.top().second;
        pq.pop();

        if (visited[currentNode]) continue;
        visited[currentNode] = true;
        result.nodesExplored++;
        result.explorationOrder.push_back(currentNode);
        result.distAtExploration.push_back(dist[currentNode]);

        if (currentNode == goalId) {
            result.found = true;
            result.totalCost = dist[goalId];
            result.path = reconstructPath(parent, startId, goalId);
            return result;
        }

        vector<Edge> neighbors = graph.getNeighbors(currentNode);
        for (int i = 0; i < (int)neighbors.size(); i++) {
            int neighborId = neighbors[i].to;
            double edgeWeight = neighbors[i].weight;

            if (!visited[neighborId] && currentDist + edgeWeight < dist[neighborId]) {
                dist[neighborId] = currentDist + edgeWeight;
                parent[neighborId] = currentNode;
                pq.push({dist[neighborId], neighborId});
            }
        }
    }
    return result;
}
