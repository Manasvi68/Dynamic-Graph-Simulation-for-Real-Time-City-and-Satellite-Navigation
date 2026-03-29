#ifndef ASTAR_H
#define ASTAR_H

#include "graph.h"
#include "dijkstra.h"   // we reuse PathResult struct
#include <vector>
#include <string>

using namespace std;

// full A* with position-based Euclidean heuristic
// posX and posY hold the x,y coordinates of each node (indexed by node id)
PathResult runAstar(const CityGraph& graph, string startName, string goalName,
                    const vector<double>& posX, const vector<double>& posY);

PathResult runAstar(const CityGraph& graph, string startName, string goalName);

#endif