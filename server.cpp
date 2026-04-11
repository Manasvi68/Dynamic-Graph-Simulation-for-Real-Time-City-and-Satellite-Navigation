#include "graph.h"
#include "dijkstra.h"
#include "astar.h"
#include "blockchain.h"
#include "traffic_sim.h"
#include "satellite_world.h"
#include "json.hpp"

// Winsock2 for Windows networking (must come before other Windows headers)
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

// ── globals ──
// (in a real project these would be in a class, but for a student project globals are fine)
CityGraph cityGraph;
Blockchain blockchain;
TrafficSimulator* trafficSim = nullptr;
SatelliteWorld* satWorld = nullptr;
string currentMode = "city";  // "city" or "satellite"

const int PORT = 8080;
const string STATIC_DIR_REACT = "frontend/dist";
const string STATIC_DIR_FALLBACK = "web";

// forward declarations (implemented further down)
struct HttpRequest {
    string method;   // GET, POST, etc.
    string path;     // /api/graph, /index.html, etc.
    string query;    // everything after ? in the URL
    string body;     // POST body
};

HttpRequest parseRequest(const string& raw);
void sendResponse(SOCKET client, int statusCode, const string& contentType, const string& body);
void sendJson(SOCKET client, const json& j);
void serveStaticFile(SOCKET client, const string& urlPath);
void handleApiRequest(SOCKET client, const HttpRequest& req);

// ── parse raw HTTP request into our struct ──
HttpRequest parseRequest(const string& raw) {
    HttpRequest req;
    req.method = "";
    req.path = "";
    req.query = "";
    req.body = "";

    // first line looks like: "GET /api/graph?from=Home HTTP/1.1\r\n"
    size_t firstSpace = raw.find(' ');
    if (firstSpace == string::npos) return req;

    req.method = raw.substr(0, firstSpace);

    size_t secondSpace = raw.find(' ', firstSpace + 1);
    if (secondSpace == string::npos) return req;

    string fullPath = raw.substr(firstSpace + 1, secondSpace - firstSpace - 1);

    // split path and query string at '?'
    size_t questionMark = fullPath.find('?');
    if (questionMark != string::npos) {
        req.path = fullPath.substr(0, questionMark);
        req.query = fullPath.substr(questionMark + 1);
    } else {
        req.path = fullPath;
    }

    // extract body (everything after the blank line \r\n\r\n)
    size_t bodyStart = raw.find("\r\n\r\n");
    if (bodyStart != string::npos) {
        req.body = raw.substr(bodyStart + 4);
    }

    return req;
}

// helper to extract a query parameter value
// e.g., getQueryParam("from=Home&to=Market&algo=dijkstra", "from") returns "Home"
static string getQueryParam(const string& query, const string& key) {
    string search = key + "=";
    size_t start = query.find(search);
    if (start == string::npos) return "";

    start += search.length();
    size_t end = query.find('&', start);
    if (end == string::npos) {
        return query.substr(start);
    }
    return query.substr(start, end - start);
}

// URL decode (replace %20 with space, + with space, etc.)
static string urlDecode(const string& str) {
    string result;
    for (int i = 0; i < str.length(); i++) {
        if (str[i] == '%' && i + 2 < str.length()) {
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

// ── send an HTTP response ──
void sendResponse(SOCKET client, int statusCode, const string& contentType, const string& body) {
    string statusText = "OK";
    if (statusCode == 404) statusText = "Not Found";
    if (statusCode == 400) statusText = "Bad Request";
    if (statusCode == 500) statusText = "Internal Server Error";

    ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";            // CORS — let frontend on different port access us
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    string responseStr = response.str();
    ::send(client, responseStr.c_str(), static_cast<int>(responseStr.size()), 0);
}

// shortcut to send a JSON response
void sendJson(SOCKET client, const json& j) {
    sendResponse(client, 200, "application/json", j.dump(2));
}

// ── guess content type from file extension ──
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

// ── serve a static file from disk ──
void serveStaticFile(SOCKET client, const string& urlPath) {
    // figure out which directory to serve from
    string staticDir = STATIC_DIR_REACT;

    // check if React build exists
    ifstream testFile(staticDir + "/index.html");
    if (!testFile.is_open()) {
        staticDir = STATIC_DIR_FALLBACK;  // fall back to web/ folder
    } else {
        testFile.close();
    }

    // map URL path to file path
    string filePath = staticDir;
    if (urlPath == "/" || urlPath.empty()) {
        filePath += "/index.html";
    } else {
        filePath += urlPath;
    }

    // try to open the file
    ifstream file(filePath, ios::binary);
    if (!file.is_open()) {
        // for SPA routing: if file not found, serve index.html
        // (React Router handles the routing client-side)
        filePath = staticDir + "/index.html";
        file.open(filePath, ios::binary);
        if (!file.is_open()) {
            sendResponse(client, 404, "text/plain", "File not found");
            return;
        }
    }

    // read entire file into string
    ostringstream contents;
    contents << file.rdbuf();
    file.close();

    string contentType = guessContentType(filePath);
    sendResponse(client, 200, contentType, contents.str());
}

// ── handle all /api/ requests ──
void handleApiRequest(SOCKET client, const HttpRequest& req) {

    // ── GET /api/graph ──
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

        // build nodes array
        json nodesArr = json::array();
        const auto& nameMap = activeGraph->getNameMap();
        for (auto it = nameMap.begin(); it != nameMap.end(); it++) {
            json nodeObj;
            nodeObj["id"] = it->first;
            nodeObj["name"] = it->second;

            // include positions if in satellite mode
            if (currentMode == "satellite" && satWorld != nullptr) {
                auto px = satWorld->getPositionsX();
                auto py = satWorld->getPositionsY();
                if (it->first < px.size()) {
                    nodeObj["posX"] = px[it->first];
                    nodeObj["posY"] = py[it->first];
                }
            }

            nodesArr.push_back(nodeObj);
        }
        j["nodes"] = nodesArr;

        // build edges array
        json edgesArr = json::array();
        const auto& adjList = activeGraph->getAdjacencyList();
        for (auto it = adjList.begin(); it != adjList.end(); it++) {
            int fromId = it->first;
            const auto& edges = it->second;
            for (int i = 0; i < edges.size(); i++) {
                json edgeObj;
                edgeObj["from"] = fromId;
                edgeObj["fromName"] = activeGraph->getNodeName(fromId);
                edgeObj["to"] = edges[i].to;
                edgeObj["toName"] = activeGraph->getNodeName(edges[i].to);
                edgeObj["weight"] = edges[i].weight;
                edgesArr.push_back(edgeObj);
            }
        }
        j["edges"] = edgesArr;

        sendJson(client, j);
        return;
    }

    // ── GET /api/blockchain ──
    if (req.method == "GET" && req.path == "/api/blockchain") {
        json j = json::array();
        const auto& chain = blockchain.getChain();
        for (int i = 0; i < chain.size(); i++) {
            json blockObj;
            blockObj["index"] = chain[i].index;
            blockObj["hash"] = chain[i].hash;
            blockObj["previousHash"] = chain[i].previousHash;
            blockObj["timestamp"] = chain[i].timestamp;

            // try to parse the data field as JSON for cleaner output
            try {
                blockObj["data"] = json::parse(chain[i].data);
            } catch (...) {
                blockObj["data"] = chain[i].data;  // genesis block has plain string
            }

            j.push_back(blockObj);
        }
        sendJson(client, j);
        return;
    }

    // ── GET /api/mode ──
    if (req.method == "GET" && req.path == "/api/mode") {
        json j;
        j["mode"] = currentMode;
        sendJson(client, j);
        return;
    }

        // ── GET /api/path?from=X&to=Y&algo=dijkstra|astar ──
    if (req.method == "GET" && req.path == "/api/path") {
        string fromName = urlDecode(getQueryParam(req.query, "from"));
        string toName = urlDecode(getQueryParam(req.query, "to"));
        string algo = getQueryParam(req.query, "algo");

        if (fromName.empty() || toName.empty()) {
            sendResponse(client, 400, "application/json", "{\"error\":\"Missing from or to parameter\"}");
            return;
        }

        if (algo.empty()) algo = "dijkstra";  // default

        CityGraph* activeGraph;
        if (currentMode == "satellite" && satWorld != nullptr) {
            activeGraph = &(satWorld->getGraph());
        } else {
            activeGraph = &cityGraph;
        }

        PathResult result;
        if (algo == "astar") {
            if (currentMode == "satellite" && satWorld != nullptr) {
                // use Euclidean heuristic with real positions
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
        for (int i = 0; i < result.path.size(); i++) {
            json step;
            step["id"] = result.path[i];
            step["name"] = activeGraph->getNodeName(result.path[i]);
            pathArr.push_back(step);
        }
        j["path"] = pathArr;

        sendJson(client, j);
        return;
    }

        // ── POST /api/sim/step ──
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

    // ── POST /api/road/update ──
    if (req.method == "POST" && req.path == "/api/road/update") {
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
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

        // find old weight for the event
        double oldWeight = 0;
        int fromId = activeGraph->getNodeId(from);
        if (fromId != -1) {
            vector<Edge> neighbors = activeGraph->getNeighbors(fromId);
            for (int i = 0; i < neighbors.size(); i++) {
                if (neighbors[i].to == activeGraph->getNodeId(to)) {
                    oldWeight = neighbors[i].weight;
                    break;
                }
            }
        }

        activeGraph->updateRoadWeight(from, to, weight);

        // log to blockchain
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

    // ── POST /api/road/close ──
    if (req.method == "POST" && req.path == "/api/road/close") {
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            sendResponse(client, 400, "application/json", "{\"error\":\"Invalid JSON body\"}");
            return;
        }

        string from = body.value("from", "");
        string to = body.value("to", "");

        CityGraph* activeGraph = (currentMode == "satellite" && satWorld)
                                  ? &(satWorld->getGraph()) : &cityGraph;

        // find old weight
        double oldWeight = 0;
        int fromId = activeGraph->getNodeId(from);
        if (fromId != -1) {
            vector<Edge> neighbors = activeGraph->getNeighbors(fromId);
            for (int i = 0; i < neighbors.size(); i++) {
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

    // ── POST /api/mode ──
    if (req.method == "POST" && req.path == "/api/mode") {
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            sendResponse(client, 400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        string newMode = body.value("mode", "city");

        if (newMode == "satellite") {
            if (satWorld == nullptr) {
                satWorld = new SatelliteWorld(&blockchain, 15.0);
                satWorld->addBody("Sat-Alpha",   0,  0, 10, 0.0, 0.2);
                satWorld->addBody("Sat-Beta",    0,  0, 12, 1.0, 0.15);
                satWorld->addBody("Sat-Gamma",   5,  5,  8, 2.0, 0.25);
                satWorld->addBody("Sat-Delta",  -5,  0, 15, 0.5, 0.1);
                satWorld->addBody("Sat-Epsilon", 0, -5,  9, 3.0, 0.3);
                satWorld->orbitStep();  // initial topology build
            }
            currentMode = "satellite";
        } else {
            currentMode = "city";
        }

        // save mode preference
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

    // ── OPTIONS (CORS preflight) ──
    if (req.method == "OPTIONS") {
        sendResponse(client, 200, "text/plain", "");
        return;
    }

    // ── unknown API route ──
    sendResponse(client, 404, "application/json", "{\"error\":\"Not found\"}");
}

// ── initialize the default city graph ──
static void initDefaultCity() {
    cityGraph.addIntersection("Connaught Place");
    cityGraph.addIntersection("Chandni Chowk");
    cityGraph.addIntersection("Karol Bagh");
    cityGraph.addIntersection("Saket");
    cityGraph.addIntersection("Dwarka");
    cityGraph.addIntersection("Nehru Place");
    cityGraph.addIntersection("Lajpat Nagar");
    cityGraph.addIntersection("Hauz Khas");
    cityGraph.addIntersection("Rohini");
    cityGraph.addIntersection("Janakpuri");

    cityGraph.addRoad("Connaught Place", "Chandni Chowk", 4.0);
    cityGraph.addRoad("Connaught Place", "Karol Bagh", 3.5);
    cityGraph.addRoad("Connaught Place", "Nehru Place", 6.0);
    cityGraph.addRoad("Chandni Chowk", "Rohini", 12.0);
    cityGraph.addRoad("Karol Bagh", "Janakpuri", 8.0);
    cityGraph.addRoad("Karol Bagh", "Dwarka", 14.0);
    cityGraph.addRoad("Saket", "Hauz Khas", 3.0);
    cityGraph.addRoad("Saket", "Nehru Place", 5.0);
    cityGraph.addRoad("Nehru Place", "Lajpat Nagar", 3.0);
    cityGraph.addRoad("Lajpat Nagar", "Hauz Khas", 4.0);
    cityGraph.addRoad("Hauz Khas", "Dwarka", 10.0);
    cityGraph.addRoad("Dwarka", "Janakpuri", 5.0);
    cityGraph.addRoad("Rohini", "Janakpuri", 9.0);
    cityGraph.addRoad("Connaught Place", "Lajpat Nagar", 7.0);
}

int main() {
    cout << "=== DSA Project Server ===" << endl;

    // try to load saved state, or create default city
    ifstream cityFile("city_graph.json");
    if (cityFile.is_open()) {
        cityFile.close();
        cityGraph.loadFromJson("city_graph.json");
        cout << "Loaded city graph from file." << endl;
    } else {
        initDefaultCity();
        cout << "Created default city graph." << endl;
    }

    // load blockchain if exists
    ifstream chainFile("blockchain.json");
    if (chainFile.is_open()) {
        chainFile.close();
        blockchain.loadFromJson("blockchain.json");
        cout << "Loaded blockchain from file." << endl;
    }

    // load mode preference
    ifstream modeFile("app_mode.json");
    if (modeFile.is_open()) {
        json modeJson;
        modeFile >> modeJson;
        modeFile.close();
        if (modeJson.contains("mode")) {
            currentMode = modeJson["mode"];
        }
    }

    // if in satellite mode, restore satellite state
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

    // ── start Winsock ──
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed!" << endl;
        return 1;
    }

    // ── create socket ──
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cout << "Socket creation failed!" << endl;
        WSACleanup();
        return 1;
    }

    // allow port reuse (helpful when restarting quickly)
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    // bind to port
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(static_cast<u_short>(PORT));

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Bind failed! Port " << PORT << " might already be in use." << endl;
        cout << "Try: netstat -ano | findstr :" << PORT << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // start listening
    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        cout << "Listen failed!" << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "\nServer running on http://localhost:" << PORT << endl;
    cout << "Press Ctrl+C to stop.\n" << endl;

    // ── main accept loop ──
    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) continue;

        // read the request (up to 8KB)
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

    // cleanup (never actually reached, but good practice)
    delete trafficSim;
    delete satWorld;
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
