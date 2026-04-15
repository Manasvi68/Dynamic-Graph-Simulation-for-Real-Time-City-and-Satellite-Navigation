# Phase 3: Route Transparency

## What changed

Added algorithm exploration visualization, a detailed route breakdown panel, and edge tooltips so users can understand exactly how and why a route was chosen.

## Files modified

### Backend (C++)
- **`dijkstra.h`** — Extended `PathResult` with `explorationOrder` (vector of node IDs in visit order) and `distAtExploration` (distance when each node was explored).
- **`dijkstra.cpp`** — Records exploration order and distances during Dijkstra's algorithm execution.
- **`astar.cpp`** — Same exploration recording for both A* overloads (Euclidean and Haversine).
- **`server.cpp`** — `/api/path` response now includes `explorationOrder` array with id, name, and distance for each explored node.

### Frontend
- **`frontend/src/components/RoutePanel.jsx`** — New component. Shows:
  - Each segment of the path with from/to names, distance, congestion multiplier, and condition
  - Total distance
  - Algorithm used and nodes explored count
- **`frontend/src/components/AlgoVisualizer.jsx`** — New component. Provides:
  - Play/Pause/Step/Reset controls for step-by-step algorithm replay
  - Speed slider
  - Step counter showing progress through exploration
  - Highlights nodes on the map: green = explored, yellow = current, grey = unexplored
- **`frontend/src/components/GraphMap.jsx`** — Accepts `algoHighlight` prop. When active, overrides node colors based on exploration state. Selected route rendered with animated marching dashes.
- **`frontend/src/App.jsx`** — Wired RoutePanel and AlgoVisualizer into the route section. Added `algoHighlight` state passed to GraphMap.
- **`frontend/src/index.css`** — Added `.algo-btn` styles and `dash-march` keyframe animation.

## How to test

1. Select two nodes and compute a path
2. The route breakdown appears below the path result, showing each segment
3. Click Play on the algorithm visualizer to watch Dijkstra/A* explore nodes step by step
4. Green nodes = already explored, yellow = currently being explored, grey = not yet reached
5. Use the speed slider to go faster or slower, or step through one node at a time

## Known limitations

- The visualizer replays the exploration from the last computed path — it doesn't re-run the algorithm in real-time
- No "second best" route comparison yet (planned as future enhancement)
- Exploration order is only available for the primary path, not for comparison routes
