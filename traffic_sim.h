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
    //randomly pick a road and increase it's weight(simulates congestion)
    void randomCongestion();
    //randomly pick a road and remove it(simulates road closure)
    void randomClosure();
    //do one random simulation step(either congestion or closure)
    void stepOnce();
};
#endif