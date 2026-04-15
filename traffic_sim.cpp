#include "traffic_sim.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;

TrafficSimulator::TrafficSimulator(CityGraph* g, Blockchain* bc) {
    graph = g;
    blockchain = bc;
    srand((unsigned)time(0));
}

void TrafficSimulator::applyCondition(int fromId, int toId, const string& condition, double multiplier) {
    string fromName = graph->getNodeName(fromId);
    string toName = graph->getNodeName(toId);

    double oldWeight = 0;
    vector<Edge> neighbors = graph->getNeighbors(fromId);
    double baseWeight = 0;
    for (int i = 0; i < (int)neighbors.size(); i++) {
        if (neighbors[i].to == toId) {
            oldWeight = neighbors[i].weight;
            baseWeight = neighbors[i].baseWeight;
            break;
        }
    }
    if (baseWeight <= 0) baseWeight = oldWeight;

    double newWeight = baseWeight * multiplier;
    graph->updateRoadWeight(fromName, toName, newWeight);
    graph->setRoadCondition(fromName, toName, condition);

    NetworkEvent event;
    event.type = condition;
    event.fromNode = fromName;
    event.toNode = toName;
    event.oldWeight = oldWeight;
    event.newWeight = newWeight;
    event.timestamp = to_string(time(0));
    blockchain->addBlockFromEvent(event);

    cout << condition << ": " << fromName << " -> " << toName
         << " weight " << oldWeight << " -> " << newWeight << endl;
}

void TrafficSimulator::recoverRoad(int fromId, int toId) {
    string fromName = graph->getNodeName(fromId);
    string toName = graph->getNodeName(toId);

    double oldWeight = 0;
    double baseWeight = 0;
    vector<Edge> neighbors = graph->getNeighbors(fromId);
    for (int i = 0; i < (int)neighbors.size(); i++) {
        if (neighbors[i].to == toId) {
            oldWeight = neighbors[i].weight;
            baseWeight = neighbors[i].baseWeight;
            break;
        }
    }
    if (baseWeight <= 0) return;

    graph->updateRoadWeight(fromName, toName, baseWeight);
    graph->setRoadCondition(fromName, toName, "normal");

    NetworkEvent event;
    event.type = "RECOVERY";
    event.fromNode = fromName;
    event.toNode = toName;
    event.oldWeight = oldWeight;
    event.newWeight = baseWeight;
    event.timestamp = to_string(time(0));
    blockchain->addBlockFromEvent(event);

    cout << "RECOVERY: " << fromName << " -> " << toName << " back to " << baseWeight << endl;
}

void TrafficSimulator::stepOnce() {
    const auto& adjList = graph->getAdjacencyList();
    if (adjList.empty()) return;

    // collect all edges
    vector<pair<int,int>> allEdges;
    vector<pair<int,int>> nonNormalEdges;
    for (auto it = adjList.begin(); it != adjList.end(); it++) {
        int fromId = it->first;
        const auto& edges = it->second;
        for (int i = 0; i < (int)edges.size(); i++) {
            allEdges.push_back({fromId, edges[i].to});
            if (edges[i].condition != "normal") {
                nonNormalEdges.push_back({fromId, edges[i].to});
            }
        }
    }
    if (allEdges.empty()) return;

    // 20% chance: recover a non-normal road back to normal
    if (!nonNormalEdges.empty() && (rand() % 100) < 20) {
        int idx = rand() % nonNormalEdges.size();
        recoverRoad(nonNormalEdges[idx].first, nonNormalEdges[idx].second);
        return;
    }

    // pick a random edge and apply a condition
    int idx = rand() % allEdges.size();
    int fromId = allEdges[idx].first;
    int toId = allEdges[idx].second;

    int roll = rand() % 100;
    if (roll < 30) {
        // light_traffic: weight x1.2-1.4
        double mult = 1.2 + (rand() % 20) / 100.0;
        applyCondition(fromId, toId, "light_traffic", mult);
    } else if (roll < 50) {
        // heavy_traffic: weight x1.5-2.0
        double mult = 1.5 + (rand() % 50) / 100.0;
        applyCondition(fromId, toId, "heavy_traffic", mult);
    } else if (roll < 65) {
        // congestion: weight x2.5-4.0
        double mult = 2.5 + (rand() % 150) / 100.0;
        applyCondition(fromId, toId, "congestion", mult);
    } else if (roll < 75) {
        // accident: weight x5.0
        applyCondition(fromId, toId, "accident", 5.0);
    } else if (roll < 85) {
        // construction: weight x3.0
        applyCondition(fromId, toId, "construction", 3.0);
    } else {
        // closed: remove road
        string fromName = graph->getNodeName(fromId);
        string toName = graph->getNodeName(toId);

        double oldWeight = 0;
        vector<Edge> neighbors = graph->getNeighbors(fromId);
        for (int i = 0; i < (int)neighbors.size(); i++) {
            if (neighbors[i].to == toId) {
                oldWeight = neighbors[i].weight;
                break;
            }
        }

        graph->removeRoad(fromName, toName);

        NetworkEvent event;
        event.type = "ROAD_CLOSED";
        event.fromNode = fromName;
        event.toNode = toName;
        event.oldWeight = oldWeight;
        event.newWeight = 0;
        event.timestamp = to_string(time(0));
        blockchain->addBlockFromEvent(event);

        cout << "ROAD_CLOSED: " << fromName << " -> " << toName << endl;
    }
}
