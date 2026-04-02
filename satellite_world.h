#ifndef SATELLITE_WORLD_H
#define SATELLITE_WORLD_H

#include "graph.h"
#include "blockchain.h"
#include <vector>
#include <set>
#include <string>

using namespace std;

// a single orbiting body (satellite)
struct Body {
    string name;
    double centerX, centerY;    // center of orbit
    double radius;              // orbit radius
    double angle;               // current angle in radians
    double speed;               // angular speed (radians per step)
    double posX, posY;          // current position (computed from orbit)
};

class SatelliteWorld {
private:
    vector<Body> bodies;
    CityGraph internalGraph;        // the graph built from satellite positions
    Blockchain* blockchain;         // where we log link changes
    double maxLinkDistance;          // max distance for two satellites to form a link

    // current set of edges as "from-to" string keys for change detection
    set<string> currentEdgeKeys;

    // helper to make a consistent edge key regardless of direction
    string makeEdgeKey(string a, string b);

public:
    SatelliteWorld(Blockchain* bc, double linkDist);

    // add a new satellite with orbital parameters
    void addBody(string name, double cx, double cy, double r, double angle, double speed);

    // advance all orbits by one step and rebuild the graph
    void orbitStep();

    // rebuild the graph based on current satellite positions and distance threshold
    void rebuildTopology();

    // get the internal graph (for pathfinding and server)
    CityGraph& getGraph();

    // get current positions for A* heuristic
    vector<double> getPositionsX() const;
    vector<double> getPositionsY() const;

    // save/load orbital state
    void saveToFiles(string orbitFile) const;
    void loadFromFiles(string orbitFile);

    // get body count
    int getBodyCount() const;
};

#endif