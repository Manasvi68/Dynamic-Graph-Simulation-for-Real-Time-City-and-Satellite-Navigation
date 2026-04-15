#include "graph.h"
#include "dijkstra.h"
#include "astar.h"
#include "blockchain.h"
#include "traffic_sim.h"
#include "satellite_world.h"
#include "json.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <cstdlib>

using namespace std;
using json = nlohmann::json;

CityGraph cityGraph;
Blockchain blockchain;
TrafficSimulator* trafficSim = nullptr;
SatelliteWorld* satWorld = nullptr;
string currentMode = "city";

const int PORT = 8080;
const string STATIC_DIR_REACT = "frontend/dist";
const string STATIC_DIR_FALLBACK = "web";
const string GRAPH_VERSION = "jodhpur_v1";

struct HttpRequest {
    string method;
    string path;
    string query;
    string body;
};

HttpRequest parseRequest(const string& raw);
void sendResponse(SOCKET client, int statusCode, const string& contentType, const string& body);
void sendJson(SOCKET client, const json& j);
void serveStaticFile(SOCKET client, const string& urlPath);
void handleApiRequest(SOCKET client, const HttpRequest& req);

HttpRequest parseRequest(const string& raw) {
    HttpRequest req;
    req.method = "";
    req.path = "";
    req.query = "";
    req.body = "";

    size_t firstSpace = raw.find(' ');
    if (firstSpace == string::npos) return req;
    req.method = raw.substr(0, firstSpace);

    size_t secondSpace = raw.find(' ', firstSpace + 1);
    if (secondSpace == string::npos) return req;

    string fullPath = raw.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    size_t questionMark = fullPath.find('?');
    if (questionMark != string::npos) {
        req.path = fullPath.substr(0, questionMark);
        req.query = fullPath.substr(questionMark + 1);
    } else {
        req.path = fullPath;
    }

    size_t bodyStart = raw.find("\r\n\r\n");
    if (bodyStart != string::npos) {
        req.body = raw.substr(bodyStart + 4);
    }
    return req;
}

static string getQueryParam(const string& query, const string& key) {
    string search = key + "=";
    size_t start = query.find(search);
    if (start == string::npos) return "";
    start += search.length();
    size_t end = query.find('&', start);
    if (end == string::npos) return query.substr(start);
    return query.substr(start, end - start);
}

static string urlDecode(const string& str) {
    string result;
    for (int i = 0; i < (int)str.length(); i++) {
        if (str[i] == '%' && i + 2 < (int)str.length()) {
            string hex = str.substr(i + 1, 2);
            char c = (char)strtol(hex.c_str(), nullptr, 16);
            result += c;
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

void sendResponse(SOCKET client, int statusCode, const string& contentType, const string& body) {
    string statusText = "OK";
    if (statusCode == 404) statusText = "Not Found";
    if (statusCode == 400) statusText = "Bad Request";
    if (statusCode == 500) statusText = "Internal Server Error";

    ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    string responseStr = response.str();
    ::send(client, responseStr.c_str(), static_cast<int>(responseStr.size()), 0);
}

void sendJson(SOCKET client, const json& j) {
    sendResponse(client, 200, "application/json", j.dump(2));
}

static string guessContentType(const string& path) {
    if (path.find(".html") != string::npos) return "text/html";
    if (path.find(".css") != string::npos) return "text/css";
    if (path.find(".js") != string::npos) return "application/javascript";
    if (path.find(".json") != string::npos) return "application/json";
    if (path.find(".png") != string::npos) return "image/png";
    if (path.find(".jpg") != string::npos) return "image/jpeg";
    if (path.find(".svg") != string::npos) return "image/svg+xml";
    if (path.find(".ico") != string::npos) return "image/x-icon";
    return "text/plain";
}

void serveStaticFile(SOCKET client, const string& urlPath) {
    string staticDir = STATIC_DIR_REACT;
    ifstream testFile(staticDir + "/index.html");
    if (!testFile.is_open()) {
        staticDir = STATIC_DIR_FALLBACK;
    } else {
        testFile.close();
    }

    string filePath = staticDir;
    if (urlPath == "/" || urlPath.empty()) {
        filePath += "/index.html";
    } else {
        filePath += urlPath;
    }

    ifstream file(filePath, ios::binary);
    if (!file.is_open()) {
        filePath = staticDir + "/index.html";
        file.open(filePath, ios::binary);
        if (!file.is_open()) {
            sendResponse(client, 404, "text/plain", "File not found");
            return;
        }
    }

    ostringstream contents;
    contents << file.rdbuf();
    file.close();
    sendResponse(client, 200, guessContentType(filePath), contents.str());
}

void handleApiRequest(SOCKET client, const HttpRequest& req) {

    // GET /api/graph
    if (req.method == "GET" && req.path == "/api/graph") {
        json j;

        CityGraph* activeGraph;
        if (currentMode == "satellite" && satWorld != nullptr) {
            activeGraph = &(satWorld->getGraph());
        } else {
            activeGraph = &cityGraph;
        }

        j["mode"] = currentMode;
        j["nodeCount"] = activeGraph->getNodeCount();

        json nodesArr = json::array();
        const auto& nameMap = activeGraph->getNameMap();
        for (auto it = nameMap.begin(); it != nameMap.end(); it++) {
            json nodeObj;
            nodeObj["id"] = it->first;
            nodeObj["name"] = it->second;
            nodeObj["lat"] = activeGraph->getNodeLat(it->first);
            nodeObj["lng"] = activeGraph->getNodeLng(it->first);

            if (currentMode == "satellite" && satWorld != nullptr) {
                auto px = satWorld->getPositionsX();
                auto py = satWorld->getPositionsY();
                const auto& allBodies = satWorld->getBodies();
                if (it->first < (int)px.size()) {
                    nodeObj["posX"] = px[it->first];
                    nodeObj["posY"] = py[it->first];
                }
                if (it->first < (int)allBodies.size()) {
                    json orbitParams;
                    orbitParams["semiMajor"] = allBodies[it->first].semiMajor;
                    orbitParams["eccentricity"] = allBodies[it->first].eccentricity;
                    orbitParams["inclination"] = allBodies[it->first].inclination;
                    orbitParams["angle"] = allBodies[it->first].angle;
                    orbitParams["speed"] = allBodies[it->first].speed;
                    orbitParams["centerX"] = allBodies[it->first].centerX;
                    orbitParams["centerY"] = allBodies[it->first].centerY;
                    nodeObj["orbitParams"] = orbitParams;
                }
            }
            nodesArr.push_back(nodeObj);
        }
        j["nodes"] = nodesArr;

        json edgesArr = json::array();
        const auto& adjList = activeGraph->getAdjacencyList();
        for (auto it = adjList.begin(); it != adjList.end(); it++) {
            int fromId = it->first;
            const auto& edges = it->second;
            for (int i = 0; i < (int)edges.size(); i++) {
                json edgeObj;
                edgeObj["from"] = fromId;
                edgeObj["fromName"] = activeGraph->getNodeName(fromId);
                edgeObj["to"] = edges[i].to;
                edgeObj["toName"] = activeGraph->getNodeName(edges[i].to);
                edgeObj["weight"] = edges[i].weight;
                edgeObj["baseWeight"] = edges[i].baseWeight;
                edgeObj["condition"] = edges[i].condition;
                if (currentMode == "satellite") {
                    edgeObj["signalDelay"] = edges[i].weight / 50.0;
                }
                edgesArr.push_back(edgeObj);
            }
        }
        j["edges"] = edgesArr;

        sendJson(client, j);
        return;
    }

    // GET /api/blockchain
    if (req.method == "GET" && req.path == "/api/blockchain") {
        json j = json::array();
        const auto& chain = blockchain.getChain();
        for (int i = 0; i < (int)chain.size(); i++) {
            json blockObj;
            blockObj["index"] = chain[i].index;
            blockObj["hash"] = chain[i].hash;
            blockObj["previousHash"] = chain[i].previousHash;
            blockObj["timestamp"] = chain[i].timestamp;
            try {
                blockObj["data"] = json::parse(chain[i].data);
            } catch (...) {
                blockObj["data"] = chain[i].data;
            }
            j.push_back(blockObj);
        }
        sendJson(client, j);
        return;
    }

    // GET /api/mode
    if (req.method == "GET" && req.path == "/api/mode") {
        json j;
        j["mode"] = currentMode;
        sendJson(client, j);
        return;
    }

    // GET /api/path
    if (req.method == "GET" && req.path == "/api/path") {
        string fromName = urlDecode(getQueryParam(req.query, "from"));
        string toName = urlDecode(getQueryParam(req.query, "to"));
        string algo = getQueryParam(req.query, "algo");

        if (fromName.empty() || toName.empty()) {
            sendResponse(client, 400, "application/json", "{\"error\":\"Missing from or to parameter\"}");
            return;
        }
        if (algo.empty()) algo = "dijkstra";

        CityGraph* activeGraph;
        if (currentMode == "satellite" && satWorld != nullptr) {
            activeGraph = &(satWorld->getGraph());
        } else {
            activeGraph = &cityGraph;
        }

        PathResult result;
        if (algo == "astar") {
            if (currentMode == "satellite" && satWorld != nullptr) {
                result = runAstar(*activeGraph, fromName, toName,
                                  satWorld->getPositionsX(), satWorld->getPositionsY());
            } else {
                result = runAstar(*activeGraph, fromName, toName);
            }
        } else {
            result = runDijkstra(*activeGraph, fromName, toName);
        }

        json j;
        j["found"] = result.found;
        j["totalCost"] = result.totalCost;
        j["nodesExplored"] = result.nodesExplored;
        j["algorithm"] = algo;

        json pathArr = json::array();
        for (int i = 0; i < (int)result.path.size(); i++) {
            json step;
            step["id"] = result.path[i];
            step["name"] = activeGraph->getNodeName(result.path[i]);
            pathArr.push_back(step);
        }
        j["path"] = pathArr;

        json explorationArr = json::array();
        for (int i = 0; i < (int)result.explorationOrder.size(); i++) {
            json eo;
            eo["id"] = result.explorationOrder[i];
            eo["name"] = activeGraph->getNodeName(result.explorationOrder[i]);
            eo["dist"] = result.distAtExploration[i];
            explorationArr.push_back(eo);
        }
        j["explorationOrder"] = explorationArr;

        // Compute a simple "second best" route by blocking each edge in the chosen path once.
        if (result.found && result.path.size() >= 2) {
            PathResult secondBest;
            secondBest.found = false;
            secondBest.totalCost = 0;
            secondBest.nodesExplored = 0;

            for (int i = 0; i < (int)result.path.size() - 1; i++) {
                CityGraph variant = *activeGraph;
                int a = result.path[i];
                int b = result.path[i + 1];
                string aName = activeGraph->getNodeName(a);
                string bName = activeGraph->getNodeName(b);
                variant.removeRoad(aName, bName);

                PathResult alt;
                if (algo == "astar") {
                    if (currentMode == "satellite" && satWorld != nullptr) {
                        alt = runAstar(variant, fromName, toName, satWorld->getPositionsX(), satWorld->getPositionsY());
                    } else {
                        alt = runAstar(variant, fromName, toName);
                    }
                } else {
                    alt = runDijkstra(variant, fromName, toName);
                }

                if (alt.found && (!secondBest.found || alt.totalCost < secondBest.totalCost)) {
                    secondBest = alt;
                }
            }

            if (secondBest.found) {
                json secondPath = json::array();
                for (int i = 0; i < (int)secondBest.path.size(); i++) {
                    json step;
                    step["id"] = secondBest.path[i];
                    step["name"] = activeGraph->getNodeName(secondBest.path[i]);
                    secondPath.push_back(step);
                }
                j["secondBest"] = {
                    {"found", true},
                    {"totalCost", secondBest.totalCost},
                    {"nodesExplored", secondBest.nodesExplored},
                    {"path", secondPath}
                };
            } else {
                j["secondBest"] = {{"found", false}};
            }
        }

        sendJson(client, j);
        return;
    }

    // POST /api/sim/step
    if (req.method == "POST" && req.path == "/api/sim/step") {
        if (currentMode == "satellite" && satWorld != nullptr) {
            satWorld->orbitStep();
            satWorld->getGraph().saveToJson("satellite_graph.json");
            satWorld->saveToFiles("satellite_orbit.json");
        } else {
            if (trafficSim != nullptr) {
                trafficSim->stepOnce();
            }
            cityGraph.saveToJson("city_graph.json");
        }
        blockchain.saveToJson("blockchain.json");

        json j;
        j["status"] = "ok";
        j["mode"] = currentMode;
        sendJson(client, j);
        return;
    }

    // POST /api/road/update
    if (req.method == "POST" && req.path == "/api/road/update") {
        json body;
        try { body = json::parse(req.body); }
        catch (...) {
            sendResponse(client, 400, "application/json", "{\"error\":\"Invalid JSON body\"}");
            return;
        }

        string from = body.value("from", "");
        string to = body.value("to", "");
        double weight = body.value("weight", 0.0);

        if (from.empty() || to.empty() || weight <= 0) {
            sendResponse(client, 400, "application/json", "{\"error\":\"Need from, to, and weight > 0\"}");
            return;
        }

        CityGraph* activeGraph = (currentMode == "satellite" && satWorld)
                                  ? &(satWorld->getGraph()) : &cityGraph;

        double oldWeight = 0;
        int fromId = activeGraph->getNodeId(from);
        if (fromId != -1) {
            vector<Edge> neighbors = activeGraph->getNeighbors(fromId);
            for (int i = 0; i < (int)neighbors.size(); i++) {
                if (neighbors[i].to == activeGraph->getNodeId(to)) {
                    oldWeight = neighbors[i].weight;
                    break;
                }
            }
        }

        activeGraph->updateRoadWeight(from, to, weight);

        NetworkEvent event;
        event.type = "ROAD_UPDATE";
        event.fromNode = from;
        event.toNode = to;
        event.oldWeight = oldWeight;
        event.newWeight = weight;
        event.timestamp = to_string(time(0));
        blockchain.addBlockFromEvent(event);

        cityGraph.saveToJson("city_graph.json");
        blockchain.saveToJson("blockchain.json");

        json j;
        j["status"] = "ok";
        sendJson(client, j);
        return;
    }

    // POST /api/road/close
    if (req.method == "POST" && req.path == "/api/road/close") {
        json body;
        try { body = json::parse(req.body); }
        catch (...) {
            sendResponse(client, 400, "application/json", "{\"error\":\"Invalid JSON body\"}");
            return;
        }

        string from = body.value("from", "");
        string to = body.value("to", "");

        CityGraph* activeGraph = (currentMode == "satellite" && satWorld)
                                  ? &(satWorld->getGraph()) : &cityGraph;

        double oldWeight = 0;
        int fromId = activeGraph->getNodeId(from);
        if (fromId != -1) {
            vector<Edge> neighbors = activeGraph->getNeighbors(fromId);
            for (int i = 0; i < (int)neighbors.size(); i++) {
                if (neighbors[i].to == activeGraph->getNodeId(to)) {
                    oldWeight = neighbors[i].weight;
                    break;
                }
            }
        }

        activeGraph->removeRoad(from, to);

        NetworkEvent event;
        event.type = "ROAD_CLOSED";
        event.fromNode = from;
        event.toNode = to;
        event.oldWeight = oldWeight;
        event.newWeight = 0;
        event.timestamp = to_string(time(0));
        blockchain.addBlockFromEvent(event);

        cityGraph.saveToJson("city_graph.json");
        blockchain.saveToJson("blockchain.json");

        json j;
        j["status"] = "ok";
        sendJson(client, j);
        return;
    }

    // POST /api/road/condition
    if (req.method == "POST" && req.path == "/api/road/condition") {
        json body;
        try { body = json::parse(req.body); }
        catch (...) {
            sendResponse(client, 400, "application/json", "{\"error\":\"Invalid JSON body\"}");
            return;
        }

        string from = body.value("from", "");
        string to = body.value("to", "");
        string condition = body.value("condition", "normal");

        CityGraph* activeGraph = (currentMode == "satellite" && satWorld)
                                  ? &(satWorld->getGraph()) : &cityGraph;

        activeGraph->setRoadCondition(from, to, condition);
        cityGraph.saveToJson("city_graph.json");

        json j;
        j["status"] = "ok";
        sendJson(client, j);
        return;
    }

    // POST /api/mode
    if (req.method == "POST" && req.path == "/api/mode") {
        json body;
        try { body = json::parse(req.body); }
        catch (...) {
            sendResponse(client, 400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        string newMode = body.value("mode", "city");

        if (newMode == "satellite") {
            if (satWorld == nullptr) {
                satWorld = new SatelliteWorld(&blockchain, 18.0);
                satWorld->addBody("Sat-Alpha",   0, 0, 10, 0.0,  0.15, 0.2,  0.0);
                satWorld->addBody("Sat-Beta",    0, 0, 13, 1.0,  0.12, 0.3,  0.4);
                satWorld->addBody("Sat-Gamma",   2, 2,  8, 2.0,  0.20, 0.15, 0.8);
                satWorld->addBody("Sat-Delta",  -2, 0, 15, 0.5,  0.08, 0.25, 1.2);
                satWorld->addBody("Sat-Epsilon", 0,-2, 11, 3.0,  0.18, 0.1,  2.0);
                satWorld->orbitStep();
            }
            currentMode = "satellite";
        } else {
            currentMode = "city";
        }

        json modeJson;
        modeJson["mode"] = currentMode;
        ofstream modeFile("app_mode.json");
        if (modeFile.is_open()) {
            modeFile << modeJson.dump(2);
            modeFile.close();
        }

        json j;
        j["status"] = "ok";
        j["mode"] = currentMode;
        sendJson(client, j);
        return;
    }

    if (req.method == "OPTIONS") {
        sendResponse(client, 200, "text/plain", "");
        return;
    }

    sendResponse(client, 404, "application/json", "{\"error\":\"Not found\"}");
}

// ── Node Optimization ──
// Removed 4 nodes that were too close together (<0.6 km apart):
//   - Sardar Market  (126m from Ghanta Ghar)  → merged into Ghanta Ghar
//   - Sojati Gate    (149m from Jodhpur Jn)   → merged into Jodhpur Junction
//   - Jaswant Thada  (424m from Mehrangarh)   → merged into Mehrangarh Fort
//   - High Court     (609m from Ratanada)     → merged into Ratanada
// Edges were rewired to merge-target nodes; weights adjusted to preserve
// approximate real-world distances. Connectivity verified: the graph
// remains fully connected (every node is reachable from every other).
//
// Final graph: 18 nodes, well-spread across Jodhpur.
static void initJodhpur() {
    // ── 18 Jodhpur intersections (optimized — removed tightly-clustered nodes) ──
    cityGraph.addIntersection("Mandore",           26.3250, 73.0100);
    cityGraph.addIntersection("Kaylana Lake",      26.3000, 72.9800);
    cityGraph.addIntersection("Banar Road",        26.3100, 73.0400);
    cityGraph.addIntersection("Pal Village",       26.2350, 73.0000);
    cityGraph.addIntersection("AIIMS Jodhpur",     26.2520, 73.0450);
    cityGraph.addIntersection("Mahamandir",        26.3050, 73.0130);
    cityGraph.addIntersection("Loco Shed",         26.2950, 73.0340);
    // Jaswant Thada removed — merged into Mehrangarh Fort (424m apart)
    cityGraph.addIntersection("Mehrangarh Fort",   26.2984, 73.0183);
    cityGraph.addIntersection("Ghanta Ghar",       26.2920, 73.0169);
    // Sardar Market removed — merged into Ghanta Ghar (126m apart)
    // Sojati Gate   removed — merged into Jodhpur Junction (149m apart)
    cityGraph.addIntersection("Jodhpur Junction",  26.2880, 73.0210);
    cityGraph.addIntersection("Paota",             26.2810, 73.0100);
    cityGraph.addIntersection("Ratanada",          26.2700, 73.0050);
    // High Court removed — merged into Ratanada (609m apart)
    cityGraph.addIntersection("MBM Engineering",   26.2730, 73.0220);
    cityGraph.addIntersection("Chopasni Road",     26.2680, 73.0280);
    cityGraph.addIntersection("Kamla Nehru Nagar", 26.2750, 73.0350);
    cityGraph.addIntersection("Basni",             26.2550, 73.0300);
    cityGraph.addIntersection("Pratap Nagar",      26.2600, 73.0200);
    cityGraph.addIntersection("Pal Road",          26.2450, 73.0050);

    // ── Roads (rewired after node merges) ──
    // Northern cluster
    cityGraph.addRoad("Mandore", "Kaylana Lake", 4.0);
    cityGraph.addRoad("Mandore", "Mahamandir", 2.5);
    cityGraph.addRoad("Mahamandir", "Banar Road", 2.0);
    cityGraph.addRoad("Mahamandir", "Loco Shed", 1.8);
    cityGraph.addRoad("Banar Road", "Loco Shed", 1.5);

    // Fort area (Jaswant Thada merged into Mehrangarh Fort)
    // Kaylana Lake → Jaswant Thada (3.0) rewired to → Mehrangarh Fort (3.5)
    cityGraph.addRoad("Kaylana Lake", "Mehrangarh Fort", 3.5);
    cityGraph.addRoad("Mehrangarh Fort", "Mahamandir", 1.5);

    // Old city core (Sardar Market & Sojati Gate merged)
    // Ghanta Ghar → Mehrangarh Fort (existing 1.2)
    cityGraph.addRoad("Ghanta Ghar", "Mehrangarh Fort", 1.2);
    // Ghanta Ghar → Sardar Market → Sojati Gate → Jodhpur Jn  rewired to:
    cityGraph.addRoad("Ghanta Ghar", "Jodhpur Junction", 1.2);
    cityGraph.addRoad("Jodhpur Junction", "Paota", 1.2);

    // Southern corridor (High Court merged into Ratanada)
    cityGraph.addRoad("Paota", "Ratanada", 1.5);
    // Ratanada → High Court → MBM Engineering  rewired to:
    cityGraph.addRoad("Ratanada", "MBM Engineering", 1.2);
    // Ratanada → High Court → Pal Road  rewired to (keep existing Pal Road→Ratanada):
    cityGraph.addRoad("Pal Road", "Ratanada", 1.8);
    cityGraph.addRoad("Pal Road", "Pratap Nagar", 1.2);
    cityGraph.addRoad("Pal Road", "Pal Village", 1.5);

    // Eastern / AIIMS corridor
    cityGraph.addRoad("MBM Engineering", "Chopasni Road", 0.8);
    cityGraph.addRoad("Pratap Nagar", "Basni", 1.0);
    cityGraph.addRoad("Basni", "AIIMS Jodhpur", 2.0);
    cityGraph.addRoad("AIIMS Jodhpur", "Chopasni Road", 2.0);
    cityGraph.addRoad("Chopasni Road", "Kamla Nehru Nagar", 0.8);
    cityGraph.addRoad("Kamla Nehru Nagar", "AIIMS Jodhpur", 1.5);
    cityGraph.addRoad("Chopasni Road", "Basni", 1.5);
}

int main() {
    cout << "=== DSA Project Server ===" << endl;

    // version-check: if saved graph is stale (old Delhi data), discard it
    bool loadedFromFile = false;
    ifstream cityFile("city_graph.json");
    if (cityFile.is_open()) {
        try {
            json saved;
            cityFile >> saved;
            cityFile.close();
            // check if saved graph has Jodhpur nodes
            bool isJodhpur = false;
            if (saved.contains("nodes")) {
                for (auto& n : saved["nodes"]) {
                    if (n.value("name", "") == "Ghanta Ghar") { isJodhpur = true; break; }
                }
            }
            if (isJodhpur) {
                cityGraph.loadFromJson("city_graph.json");
                loadedFromFile = true;
                cout << "Loaded Jodhpur graph from file." << endl;
            } else {
                cout << "Old graph detected (not Jodhpur), starting fresh." << endl;
            }
        } catch (...) {
            cout << "Failed to parse city_graph.json, starting fresh." << endl;
        }
    }

    if (!loadedFromFile) {
        initJodhpur();
        cityGraph.saveToJson("city_graph.json");
        cout << "Created Jodhpur city graph (" << cityGraph.getNodeCount() << " nodes)." << endl;
    }

    ifstream chainFile("blockchain.json");
    if (chainFile.is_open()) {
        chainFile.close();
        blockchain.loadFromJson("blockchain.json");
        cout << "Loaded blockchain from file." << endl;
    }

    ifstream modeFile("app_mode.json");
    if (modeFile.is_open()) {
        json modeJson;
        modeFile >> modeJson;
        modeFile.close();
        if (modeJson.contains("mode")) {
            currentMode = modeJson["mode"];
        }
    }

    if (currentMode == "satellite") {
        satWorld = new SatelliteWorld(&blockchain, 15.0);
        ifstream orbitFile("satellite_orbit.json");
        if (orbitFile.is_open()) {
            orbitFile.close();
            satWorld->loadFromFiles("satellite_orbit.json");
            cout << "Loaded satellite orbit state." << endl;
        }
    }

    trafficSim = new TrafficSimulator(&cityGraph, &blockchain);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed!" << endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cout << "Socket creation failed!" << endl;
        WSACleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(static_cast<u_short>(PORT));

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Bind failed! Port " << PORT << " might already be in use." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        cout << "Listen failed!" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "\nServer running on http://localhost:" << PORT << endl;
    cout << "Mode: " << currentMode << " | Nodes: " << cityGraph.getNodeCount() << endl;
    cout << "Press Ctrl+C to stop.\n" << endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) continue;

        char buffer[8192];
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived > 0) {
            string rawRequest(buffer, static_cast<size_t>(bytesReceived));
            HttpRequest req = parseRequest(rawRequest);

            if (req.path.substr(0, 4) == "/api") {
                handleApiRequest(clientSocket, req);
            } else {
                serveStaticFile(clientSocket, req.path);
            }
        }

        closesocket(clientSocket);
    }

    delete trafficSim;
    delete satWorld;
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
