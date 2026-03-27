#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graph.h"
#include <vector>
#include <string>

using namespace std;

// holds the result of a shortest path search
struct PathResult {
    vector<int> path;       // node ids in order from start to goal
    double totalCost;       // total travel time along the path
    bool found;            
    int nodesExplored;     
};

// finds shortest path from startName to goalName using Dijkstra's algorithm
PathResult runDijkstra(const CityGraph& graph, string startName, string goalName);

#endif