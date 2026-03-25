#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

// represents a single road from one intersection to another
struct Edge {
    int to;         
    double weight;  
};

class CityGraph {
private:
    int nodeCount;  // how many intersections we have

    // maps node id -> list of outgoing edges
    unordered_map<int, vector<Edge>> adjacencyList;

    // two-way lookup between node ids and names
    unordered_map<int, string> idToName;
    unordered_map<string, int> nameToId;

public:
    CityGraph();

    // add a new intersection
    void addIntersection(string name);

    // add a road between two intersections, bidirectional hai
    void addRoad(string from, string to, double weight, bool oneWay = false);

    // remove a road between two intersections
    void removeRoad(string from, string to);

    // change the weight of an existing road
    void updateRoadWeight(string from, string to, double newWeight);

    // get all roads leaving a given node
    vector<Edge> getNeighbors(int nodeId) const;

    // lookup helpers
    int getNodeId(string name) const;
    string getNodeName(int id) const;
    bool hasIntersection(string name) const;
    bool hasRoad(string from, string to) const;
    int getNodeCount() const;

    // display the graph in console
    void printGraph() const;

    // save/load the entire city map as JSON
    void saveToJson(string filename) const;
    void loadFromJson(string filename);

    // wipe everything 
    void clear();

    // give read access to internal structures 
    const unordered_map<int, vector<Edge>>& getAdjacencyList() const;
    const unordered_map<int, string>& getNameMap() const;
    const unordered_map<string, int>& getIdMap() const;
};

#endif