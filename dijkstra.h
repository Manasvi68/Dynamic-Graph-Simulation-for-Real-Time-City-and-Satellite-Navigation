#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graph.h"
#include <vector>
#include <string>

using namespace std;

struct PathResult {
    vector<int> path;
    double totalCost;
    bool found;
    int nodesExplored;
    vector<int> explorationOrder;
    vector<double> distAtExploration;
};

PathResult runDijkstra(const CityGraph& graph, string startName, string goalName);

#endif
