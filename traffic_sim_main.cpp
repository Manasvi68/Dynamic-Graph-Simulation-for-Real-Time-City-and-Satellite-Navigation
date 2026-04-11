#include "traffic_sim.h"
#include "graph.h"
#include "blockchain.h"
#include <iostream>

using namespace std;

int main() {
    cout << "--- Testing Traffic Simulator ---\n" << endl;

    CityGraph city;
    city.addIntersection("Home");
    city.addIntersection("Market");
    city.addIntersection("Hospital");
    city.addIntersection("Airport");
    city.addIntersection("School");

    city.addRoad("Home", "Market", 5.0);
    city.addRoad("Home", "Hospital", 8.0);
    city.addRoad("Market", "Airport", 7.0);
    city.addRoad("Hospital", "Airport", 4.0);
    city.addRoad("Airport", "School", 6.0);

    Blockchain chain;
    TrafficSimulator sim(&city, &chain);

    cout << "Before simulation:" << endl;
    city.printGraph();

    for (int i = 0; i < 5; i++) {
        cout << "\n--- Step " << (i + 1) << " ---" << endl;
        sim.stepOnce();
    }

    cout << "\nAfter simulation:" << endl;
    city.printGraph();

    cout << "\nBlockchain has " << chain.getChain().size() << " blocks" << endl;
    cout << "Chain valid? " << (chain.isChainValid() ? "YES" : "NO") << endl;

    cout << "\n--- Traffic sim tests done! ---" << endl;
    return 0;
}
