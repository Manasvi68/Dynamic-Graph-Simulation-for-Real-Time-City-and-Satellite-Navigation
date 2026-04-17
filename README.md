# Dynamic Graph Simulation for Real-Time City and Satellite Navigation

A full-stack DSA project that simulates shortest-path navigation on **dynamic graphs** in two domains:
- **City road network** (traffic-aware routing)
- **Satellite communication network** (time-varying links with line-of-sight constraints)

The platform combines graph algorithms, simulation, and visualization in a single interactive system.

---

## Features

- Dual mode simulation: **City** and **Satellite**
- Shortest path with **Dijkstra** and **A\***
- Dynamic edge updates: traffic, congestion, accidents, construction, closures, recovery
- Algorithm transparency: exploration order + step-by-step visual replay
- Animated traffic indicators on map routes
- Blockchain-like event ledger for simulation history
- Interactive frontend with responsive UI, keyboard shortcuts, and toast alerts

---

## Tech Stack

### Backend
- C++17
- Winsock2 HTTP server
- STL data structures and algorithms
- `nlohmann/json` for JSON parsing/serialization

### Frontend
- React + Vite
- Leaflet + React-Leaflet (city map)
- SVG/CSS animations (satellite view)
- Tailwind/PostCSS + custom CSS

### Data
- JSON persistence files (`city_graph.json`, `satellite_orbit.json`, `blockchain.json`, `app_mode.json`)
- OpenStreetMap tiles
- OSRM (used once to generate road polylines)

---

## Project Architecture

```text
React Frontend (Vite)  <----HTTP/JSON---->  C++ Backend (Winsock)
     |                                              |
     |-- City Map (Leaflet)                         |-- Graph model
     |-- Satellite View (SVG)                       |-- Dijkstra / A*
     |-- Route + Algo Panels                        |-- Traffic simulation
     |-- Blockchain Panel                           |-- Satellite world
                                                    |-- Blockchain-like ledger
```

---

## Repository Structure

```text
.
├── server.cpp                 # HTTP API + mode switching + persistence + static serving
├── graph.h/.cpp               # Graph representation and operations
├── dijkstra.h/.cpp            # Dijkstra shortest path
├── astar.h/.cpp               # A* shortest path (Euclidean/Haversine)
├── traffic_sim.h/.cpp         # Dynamic road condition simulation
├── satellite_world.h/.cpp     # Orbital updates + line-of-sight links
├── blockchain.h/.cpp          # Event chain logging and validation
├── frontend/
│   ├── src/App.jsx            # Main frontend orchestration
│   ├── src/components/        # GraphMap, SatelliteView, panels, UI widgets
│   ├── src/api.js             # Backend API wrappers
│   └── scripts/fetchOSRM.cjs  # One-time route polyline generation
├── implementation/            # Phase-wise implementation notes
├── PROJECT_REPORT.md          # Detailed 4–5 page project report
└── build.bat                  # Windows backend build script
```

---

## Setup and Run

## 1) Backend Build (Windows)

Use either script:

```powershell
.\build.bat
```

Or manual compile:

```powershell
g++ -std=c++17 -O2 -o server.exe graph.cpp dijkstra.cpp astar.cpp blockchain.cpp traffic_sim.cpp satellite_world.cpp server.cpp -lws2_32
```

Run server:

```powershell
.\server.exe
```

Backend serves APIs and static frontend on `http://localhost:8080`.

---

## 2) Frontend (Development Mode)

```powershell
cd frontend
npm install
npm run dev
```

---

## 3) Optional: Generate OSRM Polylines (One-Time)

```powershell
node frontend/scripts/fetchOSRM.cjs
```

---

## API Overview

### Graph and Mode
- `GET /api/graph` - current graph snapshot
- `GET /api/mode` - active mode
- `POST /api/mode` - switch mode (`city` / `satellite`)

### Pathfinding
- `GET /api/path?from=<id>&to=<id>&algo=dijkstra|astar`
  - Returns route, total cost, explored nodes, and exploration order

### Simulation and Road Control
- `POST /api/sim/step` - run one simulation step
- `POST /api/road/update` - update road weight
- `POST /api/road/close` - close road
- `POST /api/road/condition` - set road condition

### Event Ledger
- `GET /api/blockchain` - returns logged event blocks

---

## Road Conditions (City Mode)

- `normal`
- `light_traffic`
- `heavy_traffic`
- `congestion`
- `accident`
- `construction`
- `closed`

These conditions change edge cost and visual style, and influence chosen routes.

---

## Satellite Mode Highlights

- Elliptical orbital motion per satellite
- Dynamic link creation/removal by:
  - distance threshold
  - line-of-sight (planet occlusion) test
- Live path rendering and estimated signal delay display

---

## Algorithmic Concepts Demonstrated

- Weighted graph modeling with adjacency lists
- Dijkstra shortest path
- A* shortest path with geographic heuristic (Haversine)
- Priority queue based exploration
- Dynamic edge state transitions
- Hash-linked append-only event chain

---

## Validation Approach

Current validation is scenario-based:
- Manual path checks across source/destination pairs
- Simulation step verification for condition transitions
- Dijkstra vs A* behavior comparison
- Blockchain event inspection
- Satellite link/path behavior verification

---
## Future Improvements

- Automated test suite (backend + frontend)
- Live traffic ingestion and predictive updates
- Better alternate-route ranking (k-shortest paths)
- Larger graph benchmarking and performance dashboards
- Stronger cryptographic hashing for ledger integrity

---
