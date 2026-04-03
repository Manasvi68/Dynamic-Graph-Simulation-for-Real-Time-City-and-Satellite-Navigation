#include "graph.h"
#include "dijkstra.h"
#include "astar.h"
#include "blockchain.h"
#include "traffic_sim.h"
#include "satellite_world.h"
#include "json.hpp"

// Winsock headers for Windows networking
#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <ctime>

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
