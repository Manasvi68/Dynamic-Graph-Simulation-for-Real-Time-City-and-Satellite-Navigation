# Phase 2: Road Conditions

## What changed

Added a road condition system where each edge has a condition (normal, light_traffic, heavy_traffic, congestion, accident, construction, closed) with distinct visual styles, a map legend, and condition-aware traffic simulation.

## Files modified

### Backend (C++)
- **`graph.h`** — Added `condition` and `baseWeight` fields to `Edge` struct. New methods: `setRoadCondition()`, `getRoadCondition()`.
- **`graph.cpp`** — Implemented condition get/set, updated `saveToJson`/`loadFromJson` to persist `baseWeight` and `condition`.
- **`traffic_sim.h`** / **`traffic_sim.cpp`** — Complete overhaul. Instead of simple weight changes:
  - `light_traffic`: base weight × 1.2–1.4
  - `heavy_traffic`: base weight × 1.5–2.0
  - `congestion`: base weight × 2.5–4.0
  - `accident`: base weight × 5.0
  - `construction`: base weight × 3.0
  - `closed`: removes the road entirely
  - 20% chance of recovery back to normal
- **`server.cpp`** — New endpoint `POST /api/road/condition`. `/api/graph` response includes `condition` and `baseWeight` per edge.

### Frontend
- **`frontend/src/components/GraphMap.jsx`** — Edge rendering now uses a `CONDITION_STYLES` map for colors, weights, dash patterns, and opacity. Added edge tooltips showing weight, base weight, multiplier, and condition.
- **`frontend/src/components/MapLegend.jsx`** — New component. Floating, toggleable legend showing all 7 condition types with colored line samples.
- **`frontend/src/index.css`** — Added CSS for condition icon markers, legend, animated path (marching dashes), condition icon flashing.
- **`frontend/src/components/BlockchainPanel.jsx`** — Updated type labels to include new condition event types.
- **`frontend/src/api.js`** — Added `setCondition()` API function.

## Visual mapping

| Condition | Color | Weight | Style | Icon |
|-----------|-------|--------|-------|------|
| normal | #3b82f6 (blue) | 3 | solid | — |
| light_traffic | #eab308 (yellow) | 3 | solid | — |
| heavy_traffic | #f97316 (orange) | 4 | solid | — |
| congestion | #dc2626 (red) | 5 | solid | — |
| accident | #ef4444 (red) | 5 | solid | ⚠️ (flashing) |
| construction | #d97706 (amber) | 4 | dashed 8,6 | 🔶 |
| closed | #71717a (grey) | 2 | dashed 6,8 | 🚧 |

## How to test

1. Run several simulation steps — edges will change color based on their condition
2. Hover over any edge to see its tooltip with weight, base weight, and condition
3. Toggle the legend to see the condition key
4. Conditions escalate and recover naturally through simulation

## Known limitations

- The condition applies symmetrically to both directions of a bidirectional road
- No time-based recovery — conditions only change on simulation steps
