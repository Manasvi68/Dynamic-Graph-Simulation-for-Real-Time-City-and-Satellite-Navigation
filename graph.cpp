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