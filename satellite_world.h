#ifndef SATELLITE_WORLD_H
#define SATELLITE_WORLD_H

#include "graph.h"
#include "blockchain.h"
#include <vector>
#include <set>
#include <string>

using namespace std;

struct Body {
    string name;
    double centerX, centerY;
    double semiMajor;       // semi-major axis
    double eccentricity;    // 0 = circle, <1 = ellipse
    double inclination;     // tilt angle (radians) for visual variety
    double angle;           // current angle in radians
    double speed;           // angular speed (radians per step)
    double posX, posY;      // current position
};

class SatelliteWorld {
private:
    vector<Body> bodies;
    CityGraph internalGraph;
    Blockchain* blockchain;
    double maxLinkDistance;
    double planetRadius;    // for LOS occlusion check
    set<string> currentEdgeKeys;

    string makeEdgeKey(string a, string b);
    bool hasLineOfSight(int i, int j) const;

public:
    SatelliteWorld(Blockchain* bc, double linkDist);

    void addBody(string name, double cx, double cy, double semiMajor,
                 double angle, double speed, double eccentricity = 0.0,
                 double inclination = 0.0);

    void orbitStep();
    void rebuildTopology();

    CityGraph& getGraph();
    vector<double> getPositionsX() const;
    vector<double> getPositionsY() const;
    const vector<Body>& getBodies() const;

    void saveToFiles(string orbitFile) const;
    void loadFromFiles(string orbitFile);
    int getBodyCount() const;
};

#endif
