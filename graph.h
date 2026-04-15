#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

struct Edge {
    int to;
    double weight;
    string condition;  // normal, light_traffic, heavy_traffic, accident, closed, congestion, construction
    double baseWeight; // original weight before traffic multipliers

    Edge() : to(0), weight(0), condition("normal"), baseWeight(0) {}
};

struct NodeInfo {
    int id;
    string name;
    double lat;
    double lng;
};

class CityGraph {
private:
    int nodeCount;
    unordered_map<int, vector<Edge>> adjacencyList;
    unordered_map<int, string> idToName;
    unordered_map<string, int> nameToId;
    unordered_map<int, double> nodeLat;
    unordered_map<int, double> nodeLng;

public:
    CityGraph();

    void addIntersection(string name, double lat = 0.0, double lng = 0.0);
    void addRoad(string from, string to, double weight, bool oneWay = false);
    void removeRoad(string from, string to);
    void updateRoadWeight(string from, string to, double newWeight);
    void setRoadCondition(string from, string to, string condition);
    string getRoadCondition(string from, string to) const;

    vector<Edge> getNeighbors(int nodeId) const;
    int getNodeId(string name) const;
    string getNodeName(int id) const;
    double getNodeLat(int id) const;
    double getNodeLng(int id) const;
    bool hasIntersection(string name) const;
    bool hasRoad(string from, string to) const;
    int getNodeCount() const;

    void printGraph() const;
    void saveToJson(string filename) const;
    void loadFromJson(string filename);
    void clear();

    const unordered_map<int, vector<Edge>>& getAdjacencyList() const;
    const unordered_map<int, string>& getNameMap() const;
    const unordered_map<string, int>& getIdMap() const;
};

#endif
