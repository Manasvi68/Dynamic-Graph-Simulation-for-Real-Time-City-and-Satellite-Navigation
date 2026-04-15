#include "graph.h"
#include "json.hpp"
#include <iostream>
#include <fstream>

using namespace std;
using json = nlohmann::json;

CityGraph::CityGraph() {
    nodeCount = 0;
}

void CityGraph::addIntersection(string name, double lat, double lng) {
    if (nameToId.find(name) != nameToId.end()) {
        return;
    }
    int id = nodeCount;
    nameToId[name] = id;
    idToName[id] = name;
    nodeLat[id] = lat;
    nodeLng[id] = lng;
    adjacencyList[id] = {};
    nodeCount++;
}

int CityGraph::getNodeId(string name) const {
    auto it = nameToId.find(name);
    if (it != nameToId.end()) return it->second;
    return -1;
}

string CityGraph::getNodeName(int id) const {
    auto it = idToName.find(id);
    if (it != idToName.end()) return it->second;
    return "";
}

double CityGraph::getNodeLat(int id) const {
    auto it = nodeLat.find(id);
    if (it != nodeLat.end()) return it->second;
    return 0.0;
}

double CityGraph::getNodeLng(int id) const {
    auto it = nodeLng.find(id);
    if (it != nodeLng.end()) return it->second;
    return 0.0;
}

void CityGraph::addRoad(string from, string to, double weight, bool oneWay) {
    int fromId = getNodeId(from);
    int toId = getNodeId(to);

    if (fromId == -1 || toId == -1) {
        cout << "Can't add road: one or both intersections don't exist." << endl;
        return;
    }

    Edge forwardEdge;
    forwardEdge.to = toId;
    forwardEdge.weight = weight;
    forwardEdge.baseWeight = weight;
    forwardEdge.condition = "normal";
    adjacencyList[fromId].push_back(forwardEdge);

    if (!oneWay) {
        Edge reverseEdge;
        reverseEdge.to = fromId;
        reverseEdge.weight = weight;
        reverseEdge.baseWeight = weight;
        reverseEdge.condition = "normal";
        adjacencyList[toId].push_back(reverseEdge);
    }
}

void CityGraph::removeRoad(string from, string to) {
    int fromId = getNodeId(from);
    int toId = getNodeId(to);
    if (fromId == -1 || toId == -1) return;

    auto& edgesFrom = adjacencyList[fromId];
    for (int i = 0; i < (int)edgesFrom.size(); i++) {
        if (edgesFrom[i].to == toId) {
            edgesFrom.erase(edgesFrom.begin() + i);
            break;
        }
    }
    auto& edgesTo = adjacencyList[toId];
    for (int i = 0; i < (int)edgesTo.size(); i++) {
        if (edgesTo[i].to == fromId) {
            edgesTo.erase(edgesTo.begin() + i);
            break;
        }
    }
}

void CityGraph::updateRoadWeight(string from, string to, double newWeight) {
    int fromId = getNodeId(from);
    int toId = getNodeId(to);
    if (fromId == -1 || toId == -1) return;

    auto& edgesFrom = adjacencyList[fromId];
    for (int i = 0; i < (int)edgesFrom.size(); i++) {
        if (edgesFrom[i].to == toId) {
            edgesFrom[i].weight = newWeight;
            break;
        }
    }
    auto& edgesTo = adjacencyList[toId];
    for (int i = 0; i < (int)edgesTo.size(); i++) {
        if (edgesTo[i].to == fromId) {
            edgesTo[i].weight = newWeight;
            break;
        }
    }
}

void CityGraph::setRoadCondition(string from, string to, string condition) {
    int fromId = getNodeId(from);
    int toId = getNodeId(to);
    if (fromId == -1 || toId == -1) return;

    auto& edgesFrom = adjacencyList[fromId];
    for (int i = 0; i < (int)edgesFrom.size(); i++) {
        if (edgesFrom[i].to == toId) {
            edgesFrom[i].condition = condition;
            break;
        }
    }
    auto& edgesTo = adjacencyList[toId];
    for (int i = 0; i < (int)edgesTo.size(); i++) {
        if (edgesTo[i].to == fromId) {
            edgesTo[i].condition = condition;
            break;
        }
    }
}

string CityGraph::getRoadCondition(string from, string to) const {
    int fromId = getNodeId(from);
    int toId = getNodeId(to);
    if (fromId == -1 || toId == -1) return "normal";

    auto it = adjacencyList.find(fromId);
    if (it == adjacencyList.end()) return "normal";
    const auto& edges = it->second;
    for (int i = 0; i < (int)edges.size(); i++) {
        if (edges[i].to == toId) return edges[i].condition;
    }
    return "normal";
}

vector<Edge> CityGraph::getNeighbors(int nodeId) const {
    auto it = adjacencyList.find(nodeId);
    if (it != adjacencyList.end()) return it->second;
    return {};
}

bool CityGraph::hasIntersection(string name) const {
    return nameToId.find(name) != nameToId.end();
}

bool CityGraph::hasRoad(string from, string to) const {
    int fromId = getNodeId(from);
    int toId = getNodeId(to);
    if (fromId == -1 || toId == -1) return false;
    auto it = adjacencyList.find(fromId);
    if (it == adjacencyList.end()) return false;
    const auto& edges = it->second;
    for (int i = 0; i < (int)edges.size(); i++) {
        if (edges[i].to == toId) return true;
    }
    return false;
}

int CityGraph::getNodeCount() const {
    return nodeCount;
}

void CityGraph::printGraph() const {
    cout << "\n=== City Graph (" << nodeCount << " intersections) ===" << endl;
    for (auto it = adjacencyList.begin(); it != adjacencyList.end(); it++) {
        int nodeId = it->first;
        string nodeName = getNodeName(nodeId);
        const auto& edges = it->second;
        cout << "  [" << nodeId << "] " << nodeName << " (" << getNodeLat(nodeId) << "," << getNodeLng(nodeId) << ") -> ";
        if (edges.empty()) {
            cout << "(no roads)";
        } else {
            for (int i = 0; i < (int)edges.size(); i++) {
                string destName = getNodeName(edges[i].to);
                cout << destName << "(" << edges[i].weight << "," << edges[i].condition << ")";
                if (i < (int)edges.size() - 1) cout << ", ";
            }
        }
        cout << endl;
    }
    cout << "================================\n" << endl;
}

void CityGraph::saveToJson(string filename) const {
    json j;
    j["nodeCount"] = nodeCount;

    json nodesJson = json::array();
    for (auto it = idToName.begin(); it != idToName.end(); it++) {
        json nodeObj;
        nodeObj["id"] = it->first;
        nodeObj["name"] = it->second;
        nodeObj["lat"] = getNodeLat(it->first);
        nodeObj["lng"] = getNodeLng(it->first);
        nodesJson.push_back(nodeObj);
    }
    j["nodes"] = nodesJson;

    json edgesJson = json::array();
    for (auto it = adjacencyList.begin(); it != adjacencyList.end(); it++) {
        int fromId = it->first;
        const auto& edges = it->second;
        for (int i = 0; i < (int)edges.size(); i++) {
            json edgeObj;
            edgeObj["from"] = fromId;
            edgeObj["to"] = edges[i].to;
            edgeObj["weight"] = edges[i].weight;
            edgeObj["baseWeight"] = edges[i].baseWeight;
            edgeObj["condition"] = edges[i].condition;
            edgesJson.push_back(edgeObj);
        }
    }
    j["edges"] = edgesJson;

    ofstream outFile(filename);
    if (outFile.is_open()) {
        outFile << j.dump(2);
        outFile.close();
    }
}

void CityGraph::loadFromJson(string filename) {
    ifstream inFile(filename);
    if (!inFile.is_open()) return;

    json j;
    inFile >> j;
    inFile.close();

    adjacencyList.clear();
    idToName.clear();
    nameToId.clear();
    nodeLat.clear();
    nodeLng.clear();
    nodeCount = 0;

    if (j.contains("nodes")) {
        for (auto& nodeObj : j["nodes"]) {
            int id = nodeObj["id"];
            string name = nodeObj["name"];
            double lat = nodeObj.value("lat", 0.0);
            double lng = nodeObj.value("lng", 0.0);
            idToName[id] = name;
            nameToId[name] = id;
            nodeLat[id] = lat;
            nodeLng[id] = lng;
            adjacencyList[id] = {};
            nodeCount++;
        }
    }

    if (j.contains("edges")) {
        for (auto& edgeObj : j["edges"]) {
            Edge e;
            e.to = edgeObj["to"];
            e.weight = edgeObj["weight"];
            e.baseWeight = edgeObj.value("baseWeight", e.weight);
            e.condition = edgeObj.value("condition", string("normal"));
            int fromId = edgeObj["from"];
            adjacencyList[fromId].push_back(e);
        }
    }
}

void CityGraph::clear() {
    adjacencyList.clear();
    idToName.clear();
    nameToId.clear();
    nodeLat.clear();
    nodeLng.clear();
    nodeCount = 0;
}

const unordered_map<int, vector<Edge>>& CityGraph::getAdjacencyList() const {
    return adjacencyList;
}

const unordered_map<int, string>& CityGraph::getNameMap() const {
    return idToName;
}

const unordered_map<string, int>& CityGraph::getIdMap() const {
    return nameToId;
}
