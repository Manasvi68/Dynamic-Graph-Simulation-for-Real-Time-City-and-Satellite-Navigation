# Phase 4: Traffic Animations

## What changed

Added animated car markers that move along road polylines, with the number of cars and their speed determined by each road's condition.

## Files modified

### Frontend
- **`frontend/src/components/TrafficAnimations.jsx`** — New component. Core mechanics:
  - Uses `requestAnimationFrame` for smooth 60fps animation
  - Cars are positioned by interpolating along OSRM polyline points
  - Each car has a base offset (evenly distributed along the edge) and speed
  - Position recalculated every frame based on elapsed time
- **`frontend/src/components/GraphMap.jsx`** — Integrated `TrafficAnimations` as a layer inside the MapContainer. Wrapped `getEdgePolyline` in `useCallback` for stable reference.
- **`frontend/src/index.css`** — Added `.traffic-car` and `.traffic-car-wrapper` styles.

### New assets
- **`frontend/src/assets/traffic/car.svg`** — SVG car icon (yellow)
- **`frontend/src/assets/traffic/barrier.svg`** — SVG barrier for closed roads
- **`frontend/src/assets/traffic/warning.svg`** — SVG warning triangle for accidents

## Car count and speed by condition

| Condition | Cars | Speed | Visual |
|-----------|------|-------|--------|
| normal | 0 | — | No cars |
| light_traffic | 2 | moderate | Yellow dots, flowing |
| heavy_traffic | 5 | slow | Slow-moving cluster |
| congestion | 8 | near-still | Dense, barely moving |
| accident | 0 | — | Warning icon at midpoint |
| construction | 1 | very slow | Single slow worker marker |
| closed | 0 | — | Barrier icon at midpoint |

## How it works

1. `TrafficAnimations` receives the edge list and a `getEdgePolyline` function
2. On mount, it starts a `requestAnimationFrame` loop that increments a tick counter
3. For each edge with cars > 0, it creates N car entries with evenly spaced offsets
4. Each frame, the car's position along the polyline is computed: `t = (tick * speed + offset) % 1`
5. The `interpolatePolyline()` function maps `t ∈ [0,1]` to a lat/lng point along the multi-segment polyline
6. Cars are rendered as Leaflet `Marker` components with a custom `divIcon`

## Performance considerations

- Cars are lightweight Leaflet markers with minimal DOM (8x8 px circles)
- Only edges with non-zero car counts generate car markers
- The animation uses a single `requestAnimationFrame` loop, not per-car timers
- With 56 edges and worst-case all congested (8 cars each = 448 markers), performance stays smooth

## How to test

1. Run several simulation steps to create traffic conditions
2. Observe yellow dots moving along roads with `light_traffic`, `heavy_traffic`, or `congestion`
3. Roads with `congestion` should have many slow-moving dots
4. Roads with `accident` show a flashing warning icon at the midpoint

## Known limitations

- Cars don't change lanes or avoid each other
- Car icons are simple circles, not directional vehicle shapes
- Animation resets when the graph data updates (cars jump to new positions)
