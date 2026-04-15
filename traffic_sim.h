#ifndef TRAFFIC_SIM_H
#define TRAFFIC_SIM_H
#include "graph.h"
#include "blockchain.h"
using namespace std;

class TrafficSimulator {
private:
    CityGraph* graph;
    Blockchain* blockchain;

public:
    TrafficSimulator(CityGraph* g, Blockchain* bc);
    void stepOnce();

private:
    void applyCondition(int fromId, int toId, const string& condition, double multiplier);
    void recoverRoad(int fromId, int toId);
};
#endif
