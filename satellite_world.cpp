#include "satellite_world.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iterator>

using namespace std;
using json = nlohmann::json;

// helper: consistent edge key for change detection
// always puts alphabetically smaller name first
string SatelliteWorld::makeEdgeKey(string a, string b) {
    if (a < b) return a + "-" + b;
    return b + "-" + a;
}

SatelliteWorld::SatelliteWorld(Blockchain* bc, double linkDist) {
    blockchain = bc;
    maxLinkDistance = linkDist;
}

void SatelliteWorld::addBody(string name, double cx, double cy, double r, double angle, double speed) {
    Body b;
    b.name = name;
    b.centerX = cx;
    b.centerY = cy;
    b.radius = r;
    b.angle = angle;
    b.speed = speed;
    // compute initial position from orbital parameters
    b.posX = cx + r * cos(angle);
    b.posY = cy + r * sin(angle);
    bodies.push_back(b);
}

void SatelliteWorld::orbitStep() {
    // move each satellite forward in its orbit
    for (int i = 0; i < bodies.size(); i++) {
        bodies[i].angle += bodies[i].speed;

        // keep angle in [0, 2*PI) range
        if (bodies[i].angle > 2.0 * M_PI) {
            bodies[i].angle -= 2.0 * M_PI;
        }

        // recompute position
        bodies[i].posX = bodies[i].centerX + bodies[i].radius * cos(bodies[i].angle);
        bodies[i].posY = bodies[i].centerY + bodies[i].radius * sin(bodies[i].angle);
    }

    // rebuild graph from new positions
    rebuildTopology();
}


void SatelliteWorld::rebuildTopology() {
    // clear and rebuild the graph from scratch
    internalGraph.clear();

    // add all satellites as nodes
    for (int i = 0; i < bodies.size(); i++) {
        internalGraph.addIntersection(bodies[i].name);
    }

    // build new edge set based on distances
    set<string> newEdgeKeys;

    for (int i = 0; i < bodies.size(); i++) {
        for (int j = i + 1; j < bodies.size(); j++) {
            double dx = bodies[i].posX - bodies[j].posX;
            double dy = bodies[i].posY - bodies[j].posY;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist <= maxLinkDistance) {
                // satellites are close enough — create a communication link
                internalGraph.addRoad(bodies[i].name, bodies[j].name, dist);
                newEdgeKeys.insert(makeEdgeKey(bodies[i].name, bodies[j].name));
            }
        }
    }

    // find which links were added (in new but not in old)
    set<string> addedLinks;
    set_difference(newEdgeKeys.begin(), newEdgeKeys.end(),
                   currentEdgeKeys.begin(), currentEdgeKeys.end(),
                   inserter(addedLinks, addedLinks.begin()));

    // find which links were removed (in old but not in new)
    set<string> removedLinks;
    set_difference(currentEdgeKeys.begin(), currentEdgeKeys.end(),
                   newEdgeKeys.begin(), newEdgeKeys.end(),
                   inserter(removedLinks, removedLinks.begin()));

    // log added links to blockchain
    for (auto it = addedLinks.begin(); it != addedLinks.end(); it++) {
        string key = *it;
        size_t dash = key.find('-');
        string from = key.substr(0, dash);
        string to = key.substr(dash + 1);

        NetworkEvent event;
        event.type = "SAT_LINK_UP";
        event.fromNode = from;
        event.toNode = to;
        event.oldWeight = 0;
        event.newWeight = 1;  // placeholder
        event.timestamp = to_string(time(0));
        blockchain->addBlockFromEvent(event);
    }

    // log removed links to blockchain
    for (auto it = removedLinks.begin(); it != removedLinks.end(); it++) {
        string key = *it;
        size_t dash = key.find('-');
        string from = key.substr(0, dash);
        string to = key.substr(dash + 1);

        NetworkEvent event;
        event.type = "SAT_LINK_DOWN";
        event.fromNode = from;
        event.toNode = to;
        event.oldWeight = 1;
        event.newWeight = 0;
        event.timestamp = to_string(time(0));
        blockchain->addBlockFromEvent(event);
    }

    // update current edge set for next comparison
    currentEdgeKeys = newEdgeKeys;
}

CityGraph& SatelliteWorld::getGraph() {
    return internalGraph;
}

vector<double> SatelliteWorld::getPositionsX() const {
    vector<double> px;
    for (int i = 0; i < bodies.size(); i++) {
        px.push_back(bodies[i].posX);
    }
    return px;
}

vector<double> SatelliteWorld::getPositionsY() const {
    vector<double> py;
    for (int i = 0; i < bodies.size(); i++) {
        py.push_back(bodies[i].posY);
    }
    return py;
}

int SatelliteWorld::getBodyCount() const {
    return bodies.size();
}

void SatelliteWorld::saveToFiles(string orbitFile) const {
    json j;
    j["maxLinkDistance"] = maxLinkDistance;

    json bodiesJson = json::array();
    for (int i = 0; i < bodies.size(); i++) {
        json b;
        b["name"] = bodies[i].name;
        b["centerX"] = bodies[i].centerX;
        b["centerY"] = bodies[i].centerY;
        b["radius"] = bodies[i].radius;
        b["angle"] = bodies[i].angle;
        b["speed"] = bodies[i].speed;
        b["posX"] = bodies[i].posX;
        b["posY"] = bodies[i].posY;
        bodiesJson.push_back(b);
    }
    j["bodies"] = bodiesJson;

    ofstream out(orbitFile);
    if (out.is_open()) {
        out << j.dump(2);
        out.close();
    }
}

void SatelliteWorld::loadFromFiles(string orbitFile) {
    ifstream in(orbitFile);
    if (!in.is_open()) {
        cout << "No orbit file found, starting fresh." << endl;
        return;
    }

    json j;
    in >> j;
    in.close();

    bodies.clear();
    if (j.contains("maxLinkDistance")) {
        maxLinkDistance = j["maxLinkDistance"];
    }

    if (j.contains("bodies")) {
        for (auto& bj : j["bodies"]) {
            Body b;
            b.name = bj["name"];
            b.centerX = bj["centerX"];
            b.centerY = bj["centerY"];
            b.radius = bj["radius"];
            b.angle = bj["angle"];
            b.speed = bj["speed"];
            b.posX = bj["posX"];
            b.posY = bj["posY"];
            bodies.push_back(b);
        }
    }

    // rebuild graph from loaded positions
    rebuildTopology();
}

// ══════════ TEST MAIN ══════════
// compile: g++ -std=c++17 -o sat_test graph.cpp blockchain.cpp satellite_world.cpp
// run: ./sat_test

int main() {
    cout << "--- Testing Satellite World ---\n" << endl;

    Blockchain chain;
    SatelliteWorld world(&chain, 15.0);  // satellites link if within 15 units

    // add 5 satellites with different orbits
    world.addBody("Sat-Alpha",   0,  0, 10, 0.0, 0.2);
    world.addBody("Sat-Beta",    0,  0, 12, 1.0, 0.15);
    world.addBody("Sat-Gamma",   5,  5,  8, 2.0, 0.25);
    world.addBody("Sat-Delta",  -5,  0, 15, 0.5, 0.1);
    world.addBody("Sat-Epsilon", 0, -5,  9, 3.0, 0.3);

    cout << "Initial topology:" << endl;
    world.getGraph().printGraph();

    // run 5 orbit steps
    for (int step = 0; step < 5; step++) {
        cout << "\n=== Orbit Step " << (step + 1) << " ===" << endl;
        world.orbitStep();
        world.getGraph().printGraph();
    }

    cout << "\nBlockchain has " << chain.getChain().size() << " blocks" << endl;
    cout << "Chain valid? " << (chain.isChainValid() ? "YES" : "NO") << endl;

    // save and reload
    world.saveToFiles("test_orbit.json");
    cout << "\nSaved orbit state." << endl;

    SatelliteWorld loaded(&chain, 15.0);
    loaded.loadFromFiles("test_orbit.json");
    cout << "Loaded orbit state:" << endl;
    loaded.getGraph().printGraph();

    cout << "\n--- Satellite world tests done! ---" << endl;
    return 0;
}
