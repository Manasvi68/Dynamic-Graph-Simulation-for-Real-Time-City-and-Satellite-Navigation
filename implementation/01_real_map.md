# Phase 1: Real Jodhpur Map

## What changed

Replaced the fictional Delhi city graph (10 nodes, straight-line edges) with a real Jodhpur map featuring 35 actual intersections with GPS coordinates, and edges rendered as road-snapped polylines from the OSRM routing API.

## Files modified

### Backend (C++)
- **`graph.h`** — Added `NodeInfo` struct with `lat`/`lng`. Added `nodeLat`/`nodeLng` maps to `CityGraph`. New methods: `getNodeLat()`, `getNodeLng()`, updated `addIntersection()` to accept coordinates.
- **`graph.cpp`** — Implemented lat/lng storage, updated `saveToJson`/`loadFromJson` to persist coordinates.
- **`astar.cpp`** — Added Haversine distance function. The no-argument `runAstar()` overload now detects if nodes have real coordinates and uses Haversine heuristic automatically.
- **`server.cpp`** — Replaced `initDefaultCity()` with `initJodhpur()` containing 35 nodes and 56 bidirectional edges with km-based weights. Added stale graph detection on startup (checks for "Ghanta Ghar" node).

### Frontend
- **`frontend/src/data/jodhpurNodes.js`** — New file. Exports lat/lng coordinates for all 35 Jodhpur nodes and a `getNodePosition()` helper.
- **`frontend/src/data/jodhpurRoutes.json`** — New file (~200KB). Pre-baked OSRM road-snapped polylines for all 56 edges.
- **`frontend/scripts/fetchOSRM.cjs`** — New Node.js script that fetches OSRM polylines and writes the JSON file. Run once manually.
- **`frontend/src/components/GraphMap.jsx`** — Updated to import OSRM routes, render multi-point polylines instead of straight lines, center on Jodhpur (26.285, 73.020) at zoom 13.
- **`frontend/src/data/cityCoords.js`** — Replaced by `jodhpurNodes.js`.

### New files
- `build.bat` — Simple g++ build script for Windows.

## Decisions made

- **Pre-baked OSRM data**: Routes are fetched once and committed to git as a static JSON file. No runtime dependency on OSRM servers.
- **Haversine heuristic**: A* uses great-circle distance in km, which is admissible and consistent for road networks, making A* optimal.
- **35 nodes**: Covers major Jodhpur landmarks — Ghanta Ghar, Sardar Market, Mehrangarh Fort, AIIMS Jodhpur, Jodhpur Junction, Mandore, Kaylana Lake, etc.
- **Stale graph detection**: On startup, the server checks if the saved `city_graph.json` contains Jodhpur data. If it's old Delhi data, it initializes fresh.

## How to test

1. Build and run the server
2. Open `http://localhost:8080` or run `npm run dev`
3. The map should show Jodhpur with road-snapped polylines following actual streets
4. Select two nodes and compute a path — it should follow real roads

## Known limitations

- OSRM public demo server has rate limits. The fetch script includes 250ms delay between requests.
- If OSRM fails for an edge, it falls back to a straight line.
- Node coordinates are approximate (placed at major intersections, not exact addresses).
