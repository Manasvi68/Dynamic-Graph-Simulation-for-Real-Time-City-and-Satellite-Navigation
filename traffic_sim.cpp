#include "traffic_sim.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// w = baseDistance * (1 + interruptFactor), per project spec
static const double IF_NORMAL = 1.0;
static const double IF_TRAFFIC = 1.5;
static const double IF_CONSTRUCTION = 2.0;
static const double IF_ACCIDENT = 3.0;
static const double IF_BLOCKED = numeric_limits<double>::infinity();

static const char* eventTypeForFactor(double f) {
    if (!isfinite(f)) return "SIM_BLOCKED";
    if (f == IF_NORMAL) return "SIM_NORMAL";
    if (f == IF_TRAFFIC) return "SIM_TRAFFIC";
    if (f == IF_CONSTRUCTION) return "SIM_CONSTRUCTION";
    if (f == IF_ACCIDENT) return "SIM_ACCIDENT";
    return "SIM_INTERRUPT";
}

TrafficSimulator::TrafficSimulator(CityGraph* g, Blockchain* bc) {
    graph = g;
    blockchain = bc;
    srand(static_cast<unsigned>(time(nullptr)));
}

static bool hasDirectedEdge(const CityGraph* graph, int fromId, int toId) {
    vector<Edge> n = graph->getNeighbors(fromId);
    for (int i = 0; i < static_cast<int>(n.size()); i++) {
        if (n[i].to == toId) return true;
    }
    return false;
}

static double forwardWeight(const CityGraph* graph, int fromId, int toId) {
    vector<Edge> n = graph->getNeighbors(fromId);
    for (int i = 0; i < static_cast<int>(n.size()); i++) {
        if (n[i].to == toId) return n[i].weight;
    }
    return 0;
}

void TrafficSimulator::stepOnce() {
    const auto& adjList = graph->getAdjacencyList();
    if (adjList.empty()) {
        cout << "Graph is empty, can't simulate." << endl;
        return;
    }

    vector<pair<int, int>> roadKeys;
    set<pair<int, int>> seen;

    for (auto it = adjList.begin(); it != adjList.end(); ++it) {
        int u = it->first;
        const auto& edges = it->second;
        for (int i = 0; i < static_cast<int>(edges.size()); i++) {
            int v = edges[i].to;
            bool back = hasDirectedEdge(graph, v, u);
            if (back) {
                int lo = min(u, v);
                int hi = max(u, v);
                pair<int, int> key(lo, hi);
                if (seen.count(key)) continue;
                seen.insert(key);
                roadKeys.push_back(key);
            } else {
                pair<int, int> key(u, v);
                if (seen.count(key)) continue;
                seen.insert(key);
                roadKeys.push_back(key);
            }
        }
    }

    if (roadKeys.empty()) {
        cout << "No roads to update." << endl;
        return;
    }

    int pick = rand() % static_cast<int>(roadKeys.size());
    int a = roadKeys[pick].first;
    int b = roadKeys[pick].second;

    string fromName = graph->getNodeName(a);
    string toName = graph->getNodeName(b);

    double oldWeight = forwardWeight(graph, a, b);
    if (oldWeight == 0 && hasDirectedEdge(graph, b, a)) {
        oldWeight = forwardWeight(graph, b, a);
    }

    static const double kFactors[] = {IF_NORMAL, IF_TRAFFIC, IF_CONSTRUCTION, IF_ACCIDENT, IF_BLOCKED};
    double newFactor = kFactors[rand() % 5];

    graph->setRoadInterruptFactor(fromName, toName, newFactor);

    double newWeight = forwardWeight(graph, a, b);
    if (newWeight == 0 && hasDirectedEdge(graph, b, a)) {
        newWeight = forwardWeight(graph, b, a);
    }

    NetworkEvent event;
    event.type = eventTypeForFactor(newFactor);
    event.fromNode = fromName;
    event.toNode = toName;
    event.oldWeight = oldWeight;
    event.newWeight = newWeight;
    event.timestamp = to_string(time(nullptr));
    blockchain->addBlockFromEvent(event);

    cout << event.type << ": " << fromName << " -> " << toName << " weight " << oldWeight << " -> "
         << newWeight << endl;
}
