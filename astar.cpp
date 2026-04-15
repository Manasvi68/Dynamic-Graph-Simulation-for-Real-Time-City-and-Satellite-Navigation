#include "astar.h"
#include <iostream>
#include <queue>
#include <cmath>

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double heuristicValue(int nodeA, int nodeB,
                             const vector<double>& posX, const vector<double>& posY) {
    double dx = posX[nodeA] - posX[nodeB];
    double dy = posY[nodeA] - posY[nodeB];
    return sqrt(dx * dx + dy * dy);
}

static double haversineKm(double lat1, double lon1, double lat2, double lon2) {
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dLon / 2) * sin(dLon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return 6371.0 * c;
}

PathResult runAstar(const CityGraph& graph, string startName, string goalName,
                    const vector<double>& posX, const vector<double>& posY) {
    PathResult result;
    result.found = false;
    result.totalCost = 0;
    result.nodesExplored = 0;

    int startId = graph.getNodeId(startName);
    int goalId = graph.getNodeId(goalName);
    int n = graph.getNodeCount();

    if (startId == -1 || goalId == -1) return result;

    vector<double> gScore(n, 1e18);
    gScore[startId] = 0;
    vector<int> parent(n, -1);
    vector<bool> visited(n, false);

    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> openSet;
    double startH = heuristicValue(startId, goalId, posX, posY);
    openSet.push({startH, startId});

    while (!openSet.empty()) {
        int currentNode = openSet.top().second;
        openSet.pop();

        if (visited[currentNode]) continue;
        visited[currentNode] = true;
        result.nodesExplored++;
        result.explorationOrder.push_back(currentNode);
        result.distAtExploration.push_back(gScore[currentNode]);

        if (currentNode == goalId) {
            result.found = true;
            result.totalCost = gScore[goalId];
            vector<int> path;
            int curr = goalId;
            while (curr != -1) {
                path.push_back(curr);
                curr = parent[curr];
            }
            int left = 0, right = path.size() - 1;
            while (left < right) {
                int temp = path[left];
                path[left] = path[right];
                path[right] = temp;
                left++; right--;
            }
            result.path = path;
            return result;
        }

        vector<Edge> neighbors = graph.getNeighbors(currentNode);
        for (int i = 0; i < (int)neighbors.size(); i++) {
            int neighborId = neighbors[i].to;
            double edgeWeight = neighbors[i].weight;
            double tentativeG = gScore[currentNode] + edgeWeight;

            if (!visited[neighborId] && tentativeG < gScore[neighborId]) {
                gScore[neighborId] = tentativeG;
                parent[neighborId] = currentNode;
                double h = heuristicValue(neighborId, goalId, posX, posY);
                openSet.push({tentativeG + h, neighborId});
            }
        }
    }
    return result;
}

PathResult runAstar(const CityGraph& graph, string startName, string goalName) {
    int n = graph.getNodeCount();
    int goalId = graph.getNodeId(goalName);
    if (goalId == -1) {
        PathResult r;
        r.found = false;
        r.totalCost = 0;
        r.nodesExplored = 0;
        return r;
    }

    double goalLat = graph.getNodeLat(goalId);
    double goalLng = graph.getNodeLng(goalId);

    bool hasCoords = (goalLat != 0.0 || goalLng != 0.0);

    if (!hasCoords) {
        vector<double> emptyX(n, 0.0);
        vector<double> emptyY(n, 0.0);
        return runAstar(graph, startName, goalName, emptyX, emptyY);
    }

    vector<double> posX(n, 0.0);
    vector<double> posY(n, 0.0);
    for (int i = 0; i < n; i++) {
        posX[i] = graph.getNodeLng(i);
        posY[i] = graph.getNodeLat(i);
    }

    PathResult result;
    result.found = false;
    result.totalCost = 0;
    result.nodesExplored = 0;

    int startId = graph.getNodeId(startName);
    if (startId == -1) return result;

    vector<double> gScore(n, 1e18);
    gScore[startId] = 0;
    vector<int> parent(n, -1);
    vector<bool> visited(n, false);

    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> openSet;
    double startH = haversineKm(graph.getNodeLat(startId), graph.getNodeLng(startId), goalLat, goalLng);
    openSet.push({startH, startId});

    while (!openSet.empty()) {
        int currentNode = openSet.top().second;
        openSet.pop();

        if (visited[currentNode]) continue;
        visited[currentNode] = true;
        result.nodesExplored++;
        result.explorationOrder.push_back(currentNode);
        result.distAtExploration.push_back(gScore[currentNode]);

        if (currentNode == goalId) {
            result.found = true;
            result.totalCost = gScore[goalId];
            vector<int> path;
            int curr = goalId;
            while (curr != -1) {
                path.push_back(curr);
                curr = parent[curr];
            }
            int left = 0, right = path.size() - 1;
            while (left < right) {
                int temp = path[left];
                path[left] = path[right];
                path[right] = temp;
                left++; right--;
            }
            result.path = path;
            return result;
        }

        vector<Edge> neighbors = graph.getNeighbors(currentNode);
        for (int i = 0; i < (int)neighbors.size(); i++) {
            int neighborId = neighbors[i].to;
            double tentativeG = gScore[currentNode] + neighbors[i].weight;

            if (!visited[neighborId] && tentativeG < gScore[neighborId]) {
                gScore[neighborId] = tentativeG;
                parent[neighborId] = currentNode;
                double h = haversineKm(graph.getNodeLat(neighborId), graph.getNodeLng(neighborId), goalLat, goalLng);
                openSet.push({tentativeG + h, neighborId});
            }
        }
    }
    return result;
}
