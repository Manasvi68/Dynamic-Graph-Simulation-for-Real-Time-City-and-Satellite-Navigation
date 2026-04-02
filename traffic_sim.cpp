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
    vector<Edge> neighbours=graph->getNeighbours(fromId);
    for(int i=0;i<neighbours.size();i++){
        if(neighbours[i].to==toId){
            oldWeight=neighbours[i].weight;
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


