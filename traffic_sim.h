#ifndef TRAFFIC_SIM_H
#define TRAFFIC_SIM_H
#include "graph.h"
#include "blockchain.h"
using namespace std;
class TrafficSimulator{
    private:
    CityGraph* graph;          //pointer to the city graph
    Blockchain* blockchain;    //pointer to the blockchain
    public:
    TrafficSimulator(CityGraph* g, Blockchain* bc);
    // pick a random undirected road and apply a random interrupt (Normal / Traffic / … / Blocked)
    void stepOnce();
};
#endif