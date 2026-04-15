# Dynamic City Navigation System — Overview

## What is this project?

A full-stack Dynamic City Navigation System that combines real-time pathfinding, traffic simulation, blockchain-based event logging, and satellite network visualization. Built as a DSA (Data Structures & Algorithms) project demonstrating graph algorithms, priority queues, hash chains, and orbital mechanics.

## Architecture

```
┌──────────────────────┐     HTTP/JSON      ┌──────────────────────────────┐
│   React Frontend     │ ◄───────────────► │   C++ Backend (Winsock)      │
│   (Vite + Leaflet)   │    port 8080       │   Single-process server      │
└──────────────────────┘                    └──────────────────────────────┘
         │                                           │
         ├── GraphMap.jsx (Leaflet + OSRM)          ├── graph.h/cpp (CityGraph)
         ├── SatelliteView.jsx (SVG/CSS 3D)         ├── dijkstra.h/cpp
         ├── TrafficAnimations.jsx                   ├── astar.h/cpp (Haversine)
         ├── MapLegend.jsx                          ├── blockchain.h/cpp
         ├── RoutePanel.jsx                         ├── traffic_sim.h/cpp
         ├── AlgoVisualizer.jsx                     ├── satellite_world.h/cpp
         └── Toast.jsx + useToast.js                └── server.cpp (HTTP handler)
```

## How to build and run

### Backend (C++)
```bash
# From project root:
g++ -std=c++17 -O2 -o server.exe graph.cpp dijkstra.cpp astar.cpp blockchain.cpp traffic_sim.cpp satellite_world.cpp server.cpp -lws2_32

# Or use the batch file:
.\build.bat

# Run:
.\server.exe
```

### Frontend (React)
```bash
cd frontend
npm install
npm run dev      # Development (proxied to localhost:8080)
npm run build    # Production (output in frontend/dist/, served by C++ server)
```

### OSRM Route Data (run once)
```bash
node frontend/scripts/fetchOSRM.cjs
```

## Upgrade phases completed

1. **Real Map** — 35 Jodhpur intersections with GPS coordinates, OSRM road-snapped polylines
2. **Road Conditions** — 7 condition types with distinct visuals, condition-aware traffic simulation
3. **Route Transparency** — Algorithm visualization, route breakdown panel, edge tooltips
4. **Traffic Animations** — Animated car markers along polylines, condition-based counts
5. **Satellite Redesign** — Elliptical orbits, CSS 3D globe, line-of-sight, signal delay
6. **UI Polish** — Collapsible sidebar, keyboard shortcuts, toast notifications, mobile responsive

## Key libraries

- **Backend**: nlohmann/json (header-only JSON), Winsock2 (networking), C++17 STL
- **Frontend**: React 18, Vite, Leaflet + react-leaflet, Tailwind CSS, Lucide icons
- **Data**: OSRM public API (build-time only), OpenStreetMap tiles (runtime)

## Team

Student DSA project team.
