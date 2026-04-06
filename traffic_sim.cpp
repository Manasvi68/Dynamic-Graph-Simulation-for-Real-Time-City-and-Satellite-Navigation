#include "traffic_sim.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;
TrafficSimulator::TrafficSimulator(CityGraph* g,Blockchain* bc){
graph=g;
blockchain=bc;
srand(time(0)); //seed random number generator
}
//pick a random road and make it heavier(simulate traffic building up)
void TrafficSimulator::randomCongestion(){
    const auto& adjList=graph->getAdjacencyList();
    const auto& nameMap=graph->getNameMap();
    if(adjList.empty()){
        cout<<"Graph is empty,can't simulate congestion."<<endl;
        return;
    }
    //collect all edges into a flat list so we can pick one randomly
    vector<pair<int,int>> allEdges; //(fromId, toId)
    for(auto it=adjList.begin(); it !=adjList.end(); it++){
        int fromId=it->first;
        const auto& edges=it->second;
        for(int i=0;i<edges.size();i++){
            allEdges.push_back({fromId,edges[i].to});
        }
    }
    if(allEdges.empty()){
        cout<<"No roads to congest."<<endl;
        return;
    }
    //pick a random edge
    int randomIndex=rand()% allEdges.size();
    int fromId=allEdges[randomIndex].first;
    int toId=allEdges[randomIndex].second;
    string fromName=graph->getNodeName(fromId);
    string toName =graph->getNodeName(toId);
    //find current weight
    double oldWeight=0;
    vector<Edge> neighbors=graph->getNeighbors(fromId);
    for(int i=0;i<neighbors.size();i++){
        if(neighbors[i].to==toId){
            oldWeight=neighbors[i].weight;
            break;
        }
    }
    //increase by 20-80%
    double increase=1.0+(rand()%60+20)/100.0;
    double newWeight=oldWeight*increase;
    graph->updateRoadWeight(fromName,toName,newWeight);
    //log to blockchain
    NetworkEvent event;
    event.type="CONGESTION";
    event.fromNode=fromName;
    event.toNode=toName;
    event.oldWeight=oldWeight;
    event.newWeight=newWeight;
    event.timestamp=to_string(time(0));
    blockchain->addBlockFromEvent(event);
    cout<<"Congestion: "<<fromName<<" -> "<<toName<<" weight "<< oldWeight<<" -> "<<newWeight<<endl;
}


// pick a random road and remove it (simulate road closure)
void TrafficSimulator::randomClosure() {
    const auto& adjList = graph->getAdjacencyList();

    if (adjList.empty()) return;

    // collect all edges
    vector<pair<int, int>> allEdges;
    for (auto it = adjList.begin(); it != adjList.end(); it++) {
        int fromId = it->first;
        const auto& edges = it->second;
        for (int i = 0; i < edges.size(); i++) {
            allEdges.push_back({fromId, edges[i].to});
        }
    }

    if (allEdges.empty()) {
        cout << "No roads to close." << endl;
        return;
    }

    int randomIndex = rand() % allEdges.size();
    int fromId = allEdges[randomIndex].first;
    int toId = allEdges[randomIndex].second;

    string fromName = graph->getNodeName(fromId);
    string toName = graph->getNodeName(toId);

    // find current weight before removing
    double oldWeight = 0;
    vector<Edge> neighbors = graph->getNeighbors(fromId);
    for (int i = 0; i < neighbors.size(); i++) {
        if (neighbors[i].to == toId) {
            oldWeight = neighbors[i].weight;
            break;
        }
    }

    graph->removeRoad(fromName, toName);

    // log to blockchain
    NetworkEvent event;
    event.type = "ROAD_CLOSED";
    event.fromNode = fromName;
    event.toNode = toName;
    event.oldWeight = oldWeight;
    event.newWeight = 0;
    event.timestamp = to_string(time(0));

    blockchain->addBlockFromEvent(event);

    cout << "Road closed: " << fromName << " -> " << toName << endl;
}



// do one random step — either congestion or closure
void TrafficSimulator::stepOnce() {
    // 70% chance congestion, 30% chance road closure
    int roll = rand() % 100;
    if (roll < 70) {
        randomCongestion();
    } else {
        randomClosure();
    }
}

// ══════════ TEST MAIN ══════════
// compile: g++ -std=c++17 -o sim_test graph.cpp blockchain.cpp traffic_sim.cpp
// run: ./sim_test

int main() {
    cout << "--- Testing Traffic Simulator ---\n" << endl;

    CityGraph city;
    city.addIntersection("Home");
    city.addIntersection("Market");
    city.addIntersection("Hospital");
    city.addIntersection("Airport");
    city.addIntersection("School");

    city.addRoad("Home", "Market", 5.0);
    city.addRoad("Home", "Hospital", 8.0);
    city.addRoad("Market", "Airport", 7.0);
    city.addRoad("Hospital", "Airport", 4.0);
    city.addRoad("Airport", "School", 6.0);

    Blockchain chain;
    TrafficSimulator sim(&city, &chain);

    cout << "Before simulation:" << endl;
    city.printGraph();

    // run 5 simulation steps
    for (int i = 0; i < 5; i++) {
        cout << "\n--- Step " << (i + 1) << " ---" << endl;
        sim.stepOnce();
    }

    cout << "\nAfter simulation:" << endl;
    city.printGraph();

    // show blockchain
    cout << "\nBlockchain has " << chain.getChain().size() << " blocks" << endl;
    cout << "Chain valid? " << (chain.isChainValid() ? "YES" : "NO") << endl;

    cout << "\n--- Traffic sim tests done! ---" << endl;
    return 0;
}