#include "graph.h"
#include "json.hpp"
#include <iostream>
#include <fstream>

using namespace std;
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
    forwardEdge.weight = weight;
    adjacencyList[fromId].push_back(forwardEdge);

    // if it's not one-way, also add reverse edge: to -> from
    if (!oneWay) {
        Edge reverseEdge;
        reverseEdge.to = fromId;
        reverseEdge.weight = weight;
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

    // update forward edge
    auto& edgesFrom = adjacencyList[fromId];
    for (int i = 0; i < edgesFrom.size(); i++) {
        if (edgesFrom[i].to == toId) {
            edgesFrom[i].weight = newWeight;
            found = true;
            break;
        }
    }

    // update reverse edge too (if bidirectional road exists)
    auto& edgesTo = adjacencyList[toId];
    for (int i = 0; i < edgesTo.size(); i++) {
        if (edgesTo[i].to == fromId) {
            edgesTo[i].weight = newWeight;
            break;
        }
    }

    if (!found) {
        cout << "Road from " << from << " to " << to << " not found." << endl;
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
            edgeObj["weight"] = edges[i].weight;
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
            double weight = edgeObj["weight"];

            Edge e;
            e.to = toId;
            e.weight = weight;
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

int graph_main() {
    cout << "--- Testing CityGraph ---\n" << endl;

    CityGraph city;

    // add 6 intersections
    city.addIntersection("Home");
    city.addIntersection("Market");
    city.addIntersection("Hospital");
    city.addIntersection("Airport");
    city.addIntersection("School");
    city.addIntersection("Park");

    // add some roads
    city.addRoad("Home", "Market", 5.0);
    city.addRoad("Home", "Hospital", 8.0);
    city.addRoad("Market", "Airport", 7.0);
    city.addRoad("Hospital", "Airport", 4.0);
    city.addRoad("Airport", "School", 6.0);
    city.addRoad("School", "Park", 3.0);
    city.addRoad("Home", "Park", 15.0);  // long route
    city.addRoad("Market", "School", 9.0, true);  // one-way road

    city.printGraph();

    // test lookups
    cout << "Home id: " << city.getNodeId("Home") << endl;
    cout << "Node 3 name: " << city.getNodeName(3) << endl;
    cout << "Has 'Hospital'? " << (city.hasIntersection("Hospital") ? "yes" : "no") << endl;
    cout << "Has 'Stadium'? " << (city.hasIntersection("Stadium") ? "yes" : "no") << endl;
    cout << "Road Home->Market? " << (city.hasRoad("Home", "Market") ? "yes" : "no") << endl;
    cout << "Road Park->Home? " << (city.hasRoad("Park", "Home") ? "yes" : "no") << endl;
    cout << "Total intersections: " << city.getNodeCount() << endl;

    // test update
    cout << "\nUpdating Home->Market weight to 12.0..." << endl;
    city.updateRoadWeight("Home", "Market", 12.0);
    city.printGraph();

    // test remove
    cout << "Removing road Home->Hospital..." << endl;
    city.removeRoad("Home", "Hospital");
    city.printGraph();

    // test save/load
    cout << "Saving graph to test_city.json..." << endl;
    city.saveToJson("test_city.json");

    CityGraph loadedCity;
    cout << "Loading from test_city.json..." << endl;
    loadedCity.loadFromJson("test_city.json");
    loadedCity.printGraph();

    // test clear
    cout << "Clearing graph..." << endl;
    city.clear();
    cout << "Node count after clear: " << city.getNodeCount() << endl;

    cout << "\n--- All graph tests passed! ---" << endl;
    return 0;
}
