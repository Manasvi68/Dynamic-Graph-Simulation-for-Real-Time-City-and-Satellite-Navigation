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
    send(client, responseStr.c_str(), responseStr.size(), 0);
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