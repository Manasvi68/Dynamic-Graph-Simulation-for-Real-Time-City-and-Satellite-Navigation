#include "satellite_world.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <iterator>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;
using json = nlohmann::json;

static const double SPEED_OF_LIGHT_SCALED = 50.0; // scaled for visualization

string SatelliteWorld::makeEdgeKey(string a, string b) {
    if (a < b) return a + "-" + b;
    return b + "-" + a;
}

bool SatelliteWorld::hasLineOfSight(int i, int j) const {
    if (planetRadius <= 0) return true;

    double ax = bodies[i].posX, ay = bodies[i].posY;
    double bx = bodies[j].posX, by = bodies[j].posY;
    double dx = bx - ax, dy = by - ay;
    double len2 = dx*dx + dy*dy;
    if (len2 == 0) return true;

    // closest point on line segment AB to origin (planet center at 0,0)
    double t = -(ax*dx + ay*dy) / len2;
    t = max(0.0, min(1.0, t));
    double cx = ax + t*dx;
    double cy = ay + t*dy;
    double dist2 = cx*cx + cy*cy;
    return dist2 > planetRadius * planetRadius;
}

SatelliteWorld::SatelliteWorld(Blockchain* bc, double linkDist) {
    blockchain = bc;
    maxLinkDistance = linkDist;
    planetRadius = 3.0;
}

void SatelliteWorld::addBody(string name, double cx, double cy, double semiMajor,
                              double angle, double speed, double eccentricity,
                              double inclination) {
    Body b;
    b.name = name;
    b.centerX = cx;
    b.centerY = cy;
    b.semiMajor = semiMajor;
    b.eccentricity = eccentricity;
    b.inclination = inclination;
    b.angle = angle;
    b.speed = speed;

    double semiMinor = semiMajor * sqrt(1.0 - eccentricity * eccentricity);
    b.posX = cx + semiMajor * cos(angle) * cos(inclination) - semiMinor * sin(angle) * sin(inclination);
    b.posY = cy + semiMajor * cos(angle) * sin(inclination) + semiMinor * sin(angle) * cos(inclination);
    bodies.push_back(b);
}

void SatelliteWorld::orbitStep() {
    for (int i = 0; i < (int)bodies.size(); i++) {
        bodies[i].angle += bodies[i].speed;
        if (bodies[i].angle > 2.0 * M_PI) bodies[i].angle -= 2.0 * M_PI;

        double a = bodies[i].semiMajor;
        double e = bodies[i].eccentricity;
        double inc = bodies[i].inclination;
        double semiMinor = a * sqrt(1.0 - e * e);
        double theta = bodies[i].angle;

        bodies[i].posX = bodies[i].centerX + a * cos(theta) * cos(inc) - semiMinor * sin(theta) * sin(inc);
        bodies[i].posY = bodies[i].centerY + a * cos(theta) * sin(inc) + semiMinor * sin(theta) * cos(inc);
    }
    rebuildTopology();
}

void SatelliteWorld::rebuildTopology() {
    internalGraph.clear();
    for (int i = 0; i < (int)bodies.size(); i++) {
        internalGraph.addIntersection(bodies[i].name);
    }

    set<string> newEdgeKeys;
    for (int i = 0; i < (int)bodies.size(); i++) {
        for (int j = i + 1; j < (int)bodies.size(); j++) {
            double dx = bodies[i].posX - bodies[j].posX;
            double dy = bodies[i].posY - bodies[j].posY;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist <= maxLinkDistance && hasLineOfSight(i, j)) {
                internalGraph.addRoad(bodies[i].name, bodies[j].name, dist);
                newEdgeKeys.insert(makeEdgeKey(bodies[i].name, bodies[j].name));
            }
        }
    }

    set<string> addedLinks;
    set_difference(newEdgeKeys.begin(), newEdgeKeys.end(),
                   currentEdgeKeys.begin(), currentEdgeKeys.end(),
                   inserter(addedLinks, addedLinks.begin()));

    set<string> removedLinks;
    set_difference(currentEdgeKeys.begin(), currentEdgeKeys.end(),
                   newEdgeKeys.begin(), newEdgeKeys.end(),
                   inserter(removedLinks, removedLinks.begin()));

    for (auto it = addedLinks.begin(); it != addedLinks.end(); it++) {
        string key = *it;
        size_t dash = key.find('-');
        NetworkEvent event;
        event.type = "SAT_LINK_UP";
        event.fromNode = key.substr(0, dash);
        event.toNode = key.substr(dash + 1);
        event.oldWeight = 0;
        event.newWeight = 1;
        event.timestamp = to_string(time(0));
        blockchain->addBlockFromEvent(event);
    }

    for (auto it = removedLinks.begin(); it != removedLinks.end(); it++) {
        string key = *it;
        size_t dash = key.find('-');
        NetworkEvent event;
        event.type = "SAT_LINK_DOWN";
        event.fromNode = key.substr(0, dash);
        event.toNode = key.substr(dash + 1);
        event.oldWeight = 1;
        event.newWeight = 0;
        event.timestamp = to_string(time(0));
        blockchain->addBlockFromEvent(event);
    }

    currentEdgeKeys = newEdgeKeys;
}

CityGraph& SatelliteWorld::getGraph() { return internalGraph; }

vector<double> SatelliteWorld::getPositionsX() const {
    vector<double> px;
    for (int i = 0; i < (int)bodies.size(); i++) px.push_back(bodies[i].posX);
    return px;
}

vector<double> SatelliteWorld::getPositionsY() const {
    vector<double> py;
    for (int i = 0; i < (int)bodies.size(); i++) py.push_back(bodies[i].posY);
    return py;
}

const vector<Body>& SatelliteWorld::getBodies() const { return bodies; }

int SatelliteWorld::getBodyCount() const { return bodies.size(); }

void SatelliteWorld::saveToFiles(string orbitFile) const {
    json j;
    j["maxLinkDistance"] = maxLinkDistance;
    j["planetRadius"] = planetRadius;

    json bodiesJson = json::array();
    for (int i = 0; i < (int)bodies.size(); i++) {
        json b;
        b["name"] = bodies[i].name;
        b["centerX"] = bodies[i].centerX;
        b["centerY"] = bodies[i].centerY;
        b["semiMajor"] = bodies[i].semiMajor;
        b["eccentricity"] = bodies[i].eccentricity;
        b["inclination"] = bodies[i].inclination;
        b["angle"] = bodies[i].angle;
        b["speed"] = bodies[i].speed;
        b["posX"] = bodies[i].posX;
        b["posY"] = bodies[i].posY;
        bodiesJson.push_back(b);
    }
    j["bodies"] = bodiesJson;

    ofstream out(orbitFile);
    if (out.is_open()) { out << j.dump(2); out.close(); }
}

void SatelliteWorld::loadFromFiles(string orbitFile) {
    ifstream in(orbitFile);
    if (!in.is_open()) return;

    json j;
    in >> j;
    in.close();

    bodies.clear();
    if (j.contains("maxLinkDistance")) maxLinkDistance = j["maxLinkDistance"];
    if (j.contains("planetRadius")) planetRadius = j["planetRadius"];

    if (j.contains("bodies")) {
        for (auto& bj : j["bodies"]) {
            Body b;
            b.name = bj["name"];
            b.centerX = bj["centerX"];
            b.centerY = bj["centerY"];
            b.semiMajor = bj.value("semiMajor", bj.value("radius", 10.0));
            b.eccentricity = bj.value("eccentricity", 0.0);
            b.inclination = bj.value("inclination", 0.0);
            b.angle = bj["angle"];
            b.speed = bj["speed"];
            b.posX = bj["posX"];
            b.posY = bj["posY"];
            bodies.push_back(b);
        }
    }
    rebuildTopology();
}
