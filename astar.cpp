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