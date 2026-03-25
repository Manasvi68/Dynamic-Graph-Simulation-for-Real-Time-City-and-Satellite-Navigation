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