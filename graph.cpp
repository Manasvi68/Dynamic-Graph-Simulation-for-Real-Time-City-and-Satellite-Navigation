#include "graph.h"
#include "json.hpp"
#include <cmath>
#include <iostream>
#include <fstream>
#include <limits>

using namespace std;

namespace {

constexpr double kNormalInterrupt = 1.0;

void recomputeEdgeWeight(Edge& e) {
    if (!std::isfinite(e.interruptFactor)) {
        e.weight = numeric_limits<double>::infinity();
    } else {
        e.weight = e.baseDistance * (1.0 + e.interruptFactor);
    }
}

}  // namespace
using json = nlohmann::json;

// ── constructor ──
CityGraph::CityGraph() {
    nodeCount = 0;
    // everything else (maps, adjacency list) starts empty automatically
}

// ── add a new intersection ──
void CityGraph::addIntersection(string name) {
    // don't add the same intersection twice
    if (nameToId.find(name) != nameToId.end()) {
        cout << "Intersection '" << name << "' already exists, skipping." << endl;
        return;
    }

    int id = nodeCount;
    nameToId[name] = id;
    idToName[id] = name;
    adjacencyList[id] = {};  // empty edge list for now
    nodeCount++;
}

// ── get numeric id from name ──
int CityGraph::getNodeId(string name) const {
    auto it = nameToId.find(name);
    if (it != nameToId.end()) {
        return it->second;
    }
    return -1;  // not found
}

// ── get name from numeric id ──
string CityGraph::getNodeName(int id) const {
    auto it = idToName.find(id);
    if (it != idToName.end()) {
        return it->second;
    }
    return "";  // not found
}

// ── add a road between two intersections ──
void CityGraph::addRoad(string from, string to, double weight, bool oneWay) {
    int fromId = getNodeId(from);
    int toId = getNodeId(to);

    // both intersections must exist
    if (fromId == -1 || toId == -1) {
        cout << "Can't add road: one or both intersections don't exist." << endl;
        return;
    }

    // add forward edge: from -> to
    Edge forwardEdge;
    forwardEdge.to = toId;
    forwardEdge.interruptFactor = kNormalInterrupt;
    forwardEdge.baseDistance = weight / (1.0 + kNormalInterrupt);
    recomputeEdgeWeight(forwardEdge);
    adjacencyList[fromId].push_back(forwardEdge);

    // if it's not one-way, also add reverse edge: to -> from
    if (!oneWay) {
        Edge reverseEdge;
        reverseEdge.to = fromId;
        reverseEdge.interruptFactor = kNormalInterrupt;
        reverseEdge.baseDistance = forwardEdge.baseDistance;
        recomputeEdgeWeight(reverseEdge);
        adjacencyList[toId].push_back(reverseEdge);
    }
}

// ── remove a road between two intersections ──
void CityGraph::removeRoad(string from, string to) {
    int fromId = getNodeId(from);
    int toId = getNodeId(to);

    if (fromId == -1 || toId == -1) {
        cout << "Can't remove road: intersection not found." << endl;
        return;
    }

    // remove edge from -> to
    auto& edgesFrom = adjacencyList[fromId];
    for (int i = 0; i < edgesFrom.size(); i++) {
        if (edgesFrom[i].to == toId) {
            edgesFrom.erase(edgesFrom.begin() + i);
            break;
        }
    }

    // also remove reverse edge to -> from (in case it was bidirectional)
    auto& edgesTo = adjacencyList[toId];
    for (int i = 0; i < edgesTo.size(); i++) {
        if (edgesTo[i].to == fromId) {
            edgesTo.erase(edgesTo.begin() + i);
            break;
        }
    }
}

// ── update the weight of an existing road ──
void CityGraph::updateRoadWeight(string from, string to, double newWeight) {
    int fromId = getNodeId(from);
    int toId = getNodeId(to);

    if (fromId == -1 || toId == -1) {
        cout << "Can't update road: intersection not found." << endl;
        return;
    }

    bool found = false;

    double fForward = kNormalInterrupt;
    double baseForward = 0;

    // read forward edge state, then set effective travel cost while keeping interrupt semantics
    auto& edgesFrom = adjacencyList[fromId];
    for (int i = 0; i < edgesFrom.size(); i++) {
        if (edgesFrom[i].to == toId) {
            fForward = edgesFrom[i].interruptFactor;
            baseForward = edgesFrom[i].baseDistance;
            found = true;
            break;
        }
    }

    if (!found) {
        cout << "Road from " << from << " to " << to << " not found." << endl;
        return;
    }

    if (!isfinite(fForward)) {
        // was blocked: interpret manual weight as new nominal cost under normal interrupt
        fForward = kNormalInterrupt;
        baseForward = newWeight / (1.0 + kNormalInterrupt);
    } else if (isfinite(newWeight) && newWeight > 0) {
        baseForward = newWeight / (1.0 + fForward);
    }

    for (int i = 0; i < edgesFrom.size(); i++) {
        if (edgesFrom[i].to == toId) {
            edgesFrom[i].interruptFactor = fForward;
            edgesFrom[i].baseDistance = baseForward;
            recomputeEdgeWeight(edgesFrom[i]);
            break;
        }
    }

    auto& edgesTo = adjacencyList[toId];
    for (int i = 0; i < edgesTo.size(); i++) {
        if (edgesTo[i].to == fromId) {
            edgesTo[i].interruptFactor = fForward;
            edgesTo[i].baseDistance = baseForward;
            recomputeEdgeWeight(edgesTo[i]);
            break;
        }
    }
}

void CityGraph::setRoadInterruptFactor(string from, string to, double interruptFactor) {
    int fromId = getNodeId(from);
    int toId = getNodeId(to);

    if (fromId == -1 || toId == -1) {
        cout << "Can't set interrupt: intersection not found." << endl;
        return;
    }

    double base = 0;
    bool haveForward = false;

    auto& edgesFrom = adjacencyList[fromId];
    for (int i = 0; i < edgesFrom.size(); i++) {
        if (edgesFrom[i].to == toId) {
            base = edgesFrom[i].baseDistance;
            haveForward = true;
            break;
        }
    }

    if (!haveForward) {
        cout << "Road from " << from << " to " << to << " not found." << endl;
        return;
    }

    for (int i = 0; i < edgesFrom.size(); i++) {
        if (edgesFrom[i].to == toId) {
            edgesFrom[i].baseDistance = base;
            edgesFrom[i].interruptFactor = interruptFactor;
            recomputeEdgeWeight(edgesFrom[i]);
            break;
        }
    }

    auto& edgesTo = adjacencyList[toId];
    for (int i = 0; i < edgesTo.size(); i++) {
        if (edgesTo[i].to == fromId) {
            edgesTo[i].baseDistance = base;
            edgesTo[i].interruptFactor = interruptFactor;
            recomputeEdgeWeight(edgesTo[i]);
            break;
        }
    }
}

// get all roads leaving a node
vector<Edge> CityGraph::getNeighbors(int nodeId) const {
    auto it=adjacencyList.find(nodeId);
    if(it !=adjacencyList.end()) {
        return it->second;
    }
    return{};
}
//check if an intersection exists by name
bool CityGraph::hasIntersection(string name) const {
    return nameToId.find(name) !=nameToId.end();
}
//check if a direct road exists between two intersections
bool CityGraph::hasRoad(string from, string to) const {
    int fromId=getNodeId(from);
    int toId=getNodeId(to);
    if(fromId == -1 || toId==-1) return false;
    auto it=adjacencyList.find(fromId);
    if(it == adjacencyList.end()) return false;
    const auto& edges =it->second;
    for(int i=0;i<edges.size();i++){
        if(edges[i].to ==toId){
            return true;
        }
    }
    return false;
}
//how many instersections in the city
int CityGraph::getNodeCount() const {
    return nodeCount;
}

// ── display the whole graph in console ──
void CityGraph::printGraph() const {
    cout << "\n=== City Graph (" << nodeCount << " intersections) ===" << endl;

    for (auto it = adjacencyList.begin(); it != adjacencyList.end(); it++) {
        int nodeId = it->first;
        string nodeName = getNodeName(nodeId);
        const auto& edges = it->second;

        cout << "  [" << nodeId << "] " << nodeName << " → ";

        if (edges.empty()) {
            cout << "(no roads)";
        } else {
            for (int i = 0; i < edges.size(); i++) {
                string destName = getNodeName(edges[i].to);
                cout << destName << "(" << edges[i].weight << ")";
                if (i < edges.size() - 1) cout << ", ";
            }
        }
        cout << endl;
    }
    cout << "================================\n" << endl;
}

// ── save the entire city map to a JSON file ──
void CityGraph::saveToJson(string filename) const {
    json j;
    j["nodeCount"] = nodeCount;

    // save all node names
    json nodesJson = json::array();
    for (auto it = idToName.begin(); it != idToName.end(); it++) {
        json nodeObj;
        nodeObj["id"] = it->first;
        nodeObj["name"] = it->second;
        nodesJson.push_back(nodeObj);
    }
    j["nodes"] = nodesJson;

    // save all edges
    json edgesJson = json::array();
    for (auto it = adjacencyList.begin(); it != adjacencyList.end(); it++) {
        int fromId = it->first;
        const auto& edges = it->second;
        for (int i = 0; i < edges.size(); i++) {
            json edgeObj;
            edgeObj["from"] = fromId;
            edgeObj["to"] = edges[i].to;
            if (!isfinite(edges[i].weight)) {
                edgeObj["weight"] = nullptr;
            } else {
                edgeObj["weight"] = edges[i].weight;
            }
            edgeObj["baseDistance"] = edges[i].baseDistance;
            if (!isfinite(edges[i].interruptFactor)) {
                edgeObj["interruptFactor"] = "infinity";
            } else {
                edgeObj["interruptFactor"] = edges[i].interruptFactor;
            }
            edgesJson.push_back(edgeObj);
        }
    }
    j["edges"] = edgesJson;

    // write to file
    ofstream outFile(filename);
    if (outFile.is_open()) {
        outFile << j.dump(2);  // pretty print with indent of 2
        outFile.close();
    } else {
        cout << "Error: couldn't open " << filename << " for writing." << endl;
    }
}

// ── load city map from a JSON file ──
void CityGraph::loadFromJson(string filename) {
    ifstream inFile(filename);
    if (!inFile.is_open()) {
        cout << "Couldn't open " << filename << ", starting with empty graph." << endl;
        return;
    }

    json j;
    inFile >> j;
    inFile.close();

    // clear existing data first
    adjacencyList.clear();
    idToName.clear();
    nameToId.clear();
    nodeCount = 0;

    // load nodes
    if (j.contains("nodes")) {
        for (auto& nodeObj : j["nodes"]) {
            int id = nodeObj["id"];
            string name = nodeObj["name"];
            idToName[id] = name;
            nameToId[name] = id;
            adjacencyList[id] = {};
            nodeCount++;
        }
    }

    // load edges
    if (j.contains("edges")) {
        for (auto& edgeObj : j["edges"]) {
            int fromId = edgeObj["from"];
            int toId = edgeObj["to"];

            Edge e;
            e.to = toId;

            double f = kNormalInterrupt;
            if (edgeObj.contains("interruptFactor")) {
                if (edgeObj["interruptFactor"].is_string() &&
                    edgeObj["interruptFactor"].get<string>() == "infinity") {
                    f = numeric_limits<double>::infinity();
                } else if (edgeObj["interruptFactor"].is_number()) {
                    f = edgeObj["interruptFactor"].get<double>();
                }
            }

            if (edgeObj.contains("baseDistance") && edgeObj["baseDistance"].is_number()) {
                e.baseDistance = edgeObj["baseDistance"].get<double>();
                e.interruptFactor = f;
            } else {
                double legacyW = edgeObj.value("weight", 0.0);
                e.interruptFactor = kNormalInterrupt;
                e.baseDistance = legacyW / (1.0 + kNormalInterrupt);
            }

            recomputeEdgeWeight(e);
            adjacencyList[fromId].push_back(e);
        }
    }
}

// ── wipe everything ──
void CityGraph::clear() {
    adjacencyList.clear();
    idToName.clear();
    nameToId.clear();
    nodeCount = 0;
}

// ── accessors for read-only access to internals ──
const unordered_map<int, vector<Edge>>& CityGraph::getAdjacencyList() const {
    return adjacencyList;
}

const unordered_map<int, string>& CityGraph::getNameMap() const {
    return idToName;
}

const unordered_map<string, int>& CityGraph::getIdMap() const {
    return nameToId;
}

// ══════════ TEST MAIN ══════════
// compile: g++ -std=c++17 -o graph_test graph.cpp
// run: ./graph_test

// int graph_main() {
//     cout << "--- Testing CityGraph ---\n" << endl;

//     CityGraph city;

//     // add 6 intersections
//     city.addIntersection("Home");
//     city.addIntersection("Market");
//     city.addIntersection("Hospital");
//     city.addIntersection("Airport");
//     city.addIntersection("School");
//     city.addIntersection("Park");

//     // add some roads
//     city.addRoad("Home", "Market", 5.0);
//     city.addRoad("Home", "Hospital", 8.0);
//     city.addRoad("Market", "Airport", 7.0);
//     city.addRoad("Hospital", "Airport", 4.0);
//     city.addRoad("Airport", "School", 6.0);
//     city.addRoad("School", "Park", 3.0);
//     city.addRoad("Home", "Park", 15.0);  // long route
//     city.addRoad("Market", "School", 9.0, true);  // one-way road

//     city.printGraph();

//     // test lookups
//     cout << "Home id: " << city.getNodeId("Home") << endl;
//     cout << "Node 3 name: " << city.getNodeName(3) << endl;
//     cout << "Has 'Hospital'? " << (city.hasIntersection("Hospital") ? "yes" : "no") << endl;
//     cout << "Has 'Stadium'? " << (city.hasIntersection("Stadium") ? "yes" : "no") << endl;
//     cout << "Road Home->Market? " << (city.hasRoad("Home", "Market") ? "yes" : "no") << endl;
//     cout << "Road Park->Home? " << (city.hasRoad("Park", "Home") ? "yes" : "no") << endl;
//     cout << "Total intersections: " << city.getNodeCount() << endl;

//     // test update
//     cout << "\nUpdating Home->Market weight to 12.0..." << endl;
//     city.updateRoadWeight("Home", "Market", 12.0);
//     city.printGraph();

//     // test remove
//     cout << "Removing road Home->Hospital..." << endl;
//     city.removeRoad("Home", "Hospital");
//     city.printGraph();

//     // test save/load
//     cout << "Saving graph to test_city.json..." << endl;
//     city.saveToJson("test_city.json");

//     CityGraph loadedCity;
//     cout << "Loading from test_city.json..." << endl;
//     loadedCity.loadFromJson("test_city.json");
//     loadedCity.printGraph();

//     // test clear
//     cout << "Clearing graph..." << endl;
//     city.clear();
//     cout << "Node count after clear: " << city.getNodeCount() << endl;

//     cout << "\n--- All graph tests passed! ---" << endl;
//     return 0;
// }
