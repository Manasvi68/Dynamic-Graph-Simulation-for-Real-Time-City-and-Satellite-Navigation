# Phase 5: Satellite Mode Redesign

## What changed

Completely redesigned the satellite mode from a Leaflet-based simple view to a NASA ground control-style SVG visualization with elliptical orbits, a CSS 3D globe, orbit trail lines, line-of-sight occlusion, and signal delay display.

## Files modified

### Backend (C++)
- **`satellite_world.h`** — Replaced `radius` in `Body` struct with `semiMajor`, `eccentricity`, and `inclination`. Added `planetRadius` for LOS computation. New method: `hasLineOfSight()`, `getBodies()`.
- **`satellite_world.cpp`** — Full rewrite of orbital mechanics:
  - Elliptical orbit computation: `x = a·cos(θ)·cos(i) - b·sin(θ)·sin(i)`, `y = a·cos(θ)·sin(i) + b·sin(θ)·cos(i)` where `b = a·√(1-e²)`
  - Line-of-sight check: tests if the line segment between two satellites passes through the planet sphere (origin, radius 3.0)
  - Links only form if distance ≤ threshold AND line-of-sight is clear
- **`server.cpp`** — Updated satellite initialization with eccentricity and inclination parameters. `/api/graph` in satellite mode now includes `orbitParams` per node (semiMajor, eccentricity, inclination, angle, speed, center).

### Frontend
- **`frontend/src/components/SatelliteView.jsx`** — New component replacing GraphMap for satellite mode:
  - Deep space SVG background with 200 twinkling stars (animated via `sin(tick)`)
  - CSS radial gradient planet with continent hint ellipses
  - Satellite orbit trails as dashed SVG ellipses
  - Active links as animated dashed lines with glow effect on path
  - Satellite nodes as pulsing glowing dots
  - Info panel overlay showing satellite distances, active path, and signal delay
  - All animation driven by single `requestAnimationFrame` loop
- **`frontend/src/App.jsx`** — Conditional render: `SatelliteView` for satellite mode, `GraphMap` for city mode.
- **`frontend/src/index.css`** — Added `.sat-view`, `.sat-canvas`, `.sat-svg`, `.sat-info-panel`, and related styles with NASA ground control aesthetic.

## Orbital mechanics

Each satellite has:
- **Semi-major axis** (a): determines orbit size
- **Eccentricity** (e): 0 = circle, 0.3 = noticeable ellipse
- **Inclination** (i): rotation of orbit plane, creating visual variety
- **Angular speed**: how fast it orbits

The elliptical parametric equation:
```
x = cx + a·cos(θ)·cos(i) - b·sin(θ)·sin(i)
y = cy + a·cos(θ)·sin(i) + b·sin(θ)·cos(i)
where b = a·√(1 - e²)
```

## Line-of-sight computation

Two satellites can link only if the straight line between them doesn't pass through the planet:
1. Compute the closest point on the line segment AB to the origin (planet center)
2. If that distance < planet radius, the link is blocked

This creates realistic behavior where satellites on opposite sides of the planet cannot communicate directly.

## Signal delay

Displayed in the info panel as: `distance / SPEED_OF_LIGHT_SCALED * 1000` milliseconds. The speed of light is scaled to 50 units/step for visual relevance.

## How to test

1. Switch to Satellite mode using the header toggle
2. Observe satellites orbiting on elliptical paths with faint trail lines
3. Click "Run simulation step" to advance orbits
4. Watch links appear/disappear as satellites move
5. Select two satellites and compute a path — it shows on the globe view
6. Check the info panel for distances and signal delay

## Known limitations

- The "globe" is a 2D SVG representation, not true 3D (planet is just a circle with gradient)
- Satellite count is fixed at 5 (configurable in server.cpp)
- No atmospheric effects or signal degradation
- Orbit visualization is projected onto 2D, so inclination appears as rotation rather than tilt
