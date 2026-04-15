import React, { useMemo, useCallback, useState } from 'react';
import { MapContainer, TileLayer, CircleMarker, Polyline, Tooltip, Marker } from 'react-leaflet';
import { CRS, divIcon } from 'leaflet';
import MapResize from './MapResize';
import MapLegend from './MapLegend';
import { getNodePosition } from '../data/jodhpurNodes';
import osrmRoutes from '../data/jodhpurRoutes.json';

/* ── Enhanced edge visibility ──
 * Weights & opacity increased for clear road rendering against map tiles.
 * Normal roads use a brighter slate for better contrast on OSM layers. */
const CONDITION_STYLES = {
  normal:        { color: '#8899aa', weight: 3.5, dashArray: null,    opacity: 0.7  },
  light_traffic: { color: '#eab308', weight: 4,   dashArray: null,    opacity: 0.8  },
  heavy_traffic: { color: '#f97316', weight: 4.5, dashArray: null,    opacity: 0.85 },
  congestion:    { color: '#dc2626', weight: 5,   dashArray: null,    opacity: 0.9  },
  accident:      { color: '#ef4444', weight: 5.5, dashArray: null,    opacity: 0.95 },
  construction:  { color: '#d97706', weight: 4.5, dashArray: '8,6',   opacity: 0.85 },
  closed:        { color: '#71717a', weight: 3,   dashArray: '6,8',   opacity: 0.65 },
};

// Multi-route styles — bolder for clear visibility
const PRIMARY_STYLE   = { color: '#22d3ee', weight: 7,   dashArray: '10,8', opacity: 0.95, className: 'path-animated' };
const PRIMARY_GLOW    = { color: '#22d3ee', weight: 14,  dashArray: null,   opacity: 0.18, className: 'path-glow' };
const ALT_STYLE       = { color: '#f59e0b', weight: 5,   dashArray: '8,6',  opacity: 0.85 };
const SECOND_BEST_STYLE = { color: '#a855f7', weight: 4, dashArray: '4,6',  opacity: 0.70 };

function makeIconMarker(text, className) {
  return divIcon({
    html: `<span class="${className}">${text}</span>`,
    className: 'condition-icon-wrapper',
    iconSize: [20, 20],
    iconAnchor: [10, 10],
  });
}

function midpoint(pts) {
  if (!pts || pts.length === 0) return [0, 0];
  const mid = Math.floor(pts.length / 2);
  return pts[mid];
}

/* Build a set of edge keys from a path result */
function buildEdgeKeys(pathResult) {
  if (!pathResult?.found) return new Set();
  const keys = new Set();
  for (let i = 0; i < pathResult.path.length - 1; i++) {
    const a = pathResult.path[i].id;
    const b = pathResult.path[i + 1].id;
    keys.add(`${a}-${b}`);
    keys.add(`${b}-${a}`);
  }
  return keys;
}

function buildNodeIds(pathResult) {
  if (!pathResult?.found) return new Set();
  return new Set(pathResult.path.map((step) => step.id));
}

function GraphMap({ graphData, pathData, altPathData, mode }) {
  const [showAllRoads, setShowAllRoads] = useState(false);
  const nodePositions = useMemo(() => {
    const positions = {};
    if (!graphData?.nodes) return positions;
    graphData.nodes.forEach((node, index) => {
      if (mode === 'satellite' && node.posX !== undefined) {
        positions[node.id] = [node.posY, node.posX];
      } else if (node.lat && node.lng) {
        positions[node.id] = [node.lat, node.lng];
      } else {
        positions[node.id] = getNodePosition(node.name, index);
      }
    });
    return positions;
  }, [graphData, mode]);

  // Primary path
  const primaryEdgeKeys = useMemo(() => buildEdgeKeys(pathData), [pathData]);
  const primaryNodeIds = useMemo(() => buildNodeIds(pathData), [pathData]);

  // Second-best path (from primary algo's secondBest)
  const secondBestEdgeKeys = useMemo(() => {
    if (!pathData?.secondBest?.found) return new Set();
    const keys = new Set();
    for (let i = 0; i < pathData.secondBest.path.length - 1; i++) {
      const a = pathData.secondBest.path[i].id;
      const b = pathData.secondBest.path[i + 1].id;
      keys.add(`${a}-${b}`);
      keys.add(`${b}-${a}`);
    }
    return keys;
  }, [pathData]);

  const edgeNameIndex = useMemo(() => {
    if (!graphData?.edges) return {};
    const idx = {};
    graphData.edges.forEach((edge) => {
      idx[`${edge.from}-${edge.to}`] = { fromName: edge.fromName, toName: edge.toName };
    });
    return idx;
  }, [graphData]);

  const renderedEdges = useMemo(() => {
    if (!graphData?.edges) return [];
    const unique = [];
    const seen = new Set();
    graphData.edges.forEach((edge) => {
      const a = Math.min(edge.from, edge.to);
      const b = Math.max(edge.from, edge.to);
      const key = `${a}-${b}`;
      if (seen.has(key)) return;
      seen.add(key);
      unique.push(edge);
    });
    return unique;
  }, [graphData]);

  const mapCenter = mode === 'satellite' ? [0, 0] : [26.285, 73.020];
  const mapZoom = mode === 'satellite' ? 3 : 13;
  const crs = mode === 'satellite' ? CRS.Simple : CRS.EPSG3857;

  const getEdgePolyline = useCallback((edge) => {
    const info = edgeNameIndex[`${edge.from}-${edge.to}`];
    if (!info) return null;
    const key1 = `${info.fromName}-${info.toName}`;
    const key2 = `${info.toName}-${info.fromName}`;
    let pts = osrmRoutes[key1] || osrmRoutes[key2];
    if (pts) {
      if (osrmRoutes[key2] && !osrmRoutes[key1]) pts = [...pts].reverse();
      return pts;
    }
    const fromPos = nodePositions[edge.from];
    const toPos = nodePositions[edge.to];
    if (fromPos && toPos) return [fromPos, toPos];
    return null;
  }, [edgeNameIndex, nodePositions]);

  if (!graphData?.nodes) {
    return (
      <div className="flex h-full min-h-0 w-full items-center justify-center rounded-xl border border-white/10 bg-zinc-950">
        <div className="px-4 text-center">
          <p className="text-base font-bold text-white">Loading map...</p>
          <p className="mt-2 text-base font-medium text-zinc-300">Waiting for graph data</p>
        </div>
      </div>
    );
  }

  function getEdgeStyle(edge, edgeKey) {
    // Priority: primary > secondBest > condition
    if (primaryEdgeKeys.has(edgeKey)) return PRIMARY_STYLE;
    if (secondBestEdgeKeys.has(edgeKey)) return SECOND_BEST_STYLE;
    const condition = edge.condition || 'normal';
    return CONDITION_STYLES[condition] || CONDITION_STYLES.normal;
  }

  const conditionIcons = [];
  renderedEdges.forEach((edge, idx) => {
    const condition = edge.condition || 'normal';
    if (condition !== 'accident' && condition !== 'closed' && condition !== 'construction') return;
    const polyline = getEdgePolyline(edge);
    if (!polyline) return;
    const pos = midpoint(polyline);
    const text = condition === 'accident' ? '\u26A0\uFE0F' : condition === 'closed' ? '\uD83D\uDEA7' : '\uD83D\uDD36';
    conditionIcons.push({ pos, text, key: `icon-${edge.from}-${edge.to}-${idx}`, condition });
  });

  // Determine if an edge should be visible
  function shouldShowEdge(edge) {
    const edgeKey = `${edge.from}-${edge.to}`;
    if (primaryEdgeKeys.has(edgeKey)) return true;
    if (secondBestEdgeKeys.has(edgeKey)) return true;
    if (showAllRoads) return true;
    const hasIssue = (edge.condition || 'normal') !== 'normal';
    if (hasIssue) return true;
    return false;
  }

  return (
    <div className="rounded-xl border border-white/[0.08] bg-[#0a0a0e] shadow-[0_0_40px_rgba(0,0,0,0.5)]" style={{display:'flex',flexDirection:'column',height:'100%',minHeight:0,flex:'1 1 0%',overflow:'hidden'}}>
      <div className="flex shrink-0 items-center justify-between border-b border-white/15 bg-zinc-900 px-3 py-2.5">
        <span className="text-sm font-bold uppercase tracking-widest text-white">
          {mode === 'satellite' ? 'Satellite view' : 'Jodhpur city map'}
        </span>
        <div className="flex items-center gap-2">
          {mode !== 'satellite' && (
            <button
              type="button"
              onClick={() => setShowAllRoads((v) => !v)}
              className="rounded border border-white/20 bg-black/30 px-2 py-1 text-xs font-semibold uppercase tracking-wide text-zinc-200 hover:bg-black/50"
            >
              {showAllRoads ? 'Focus path' : 'Show all roads'}
            </button>
          )}
          <span className="font-mono text-sm font-semibold text-zinc-200">
            {graphData.nodeCount} nodes · {renderedEdges.length} roads
          </span>
        </div>
      </div>
      <div style={{position:'relative',flex:'1 1 0%',minHeight:0,overflow:'hidden'}}>
        <MapContainer
          key={mode}
          center={mapCenter}
          zoom={mapZoom}
          crs={crs}
          className="z-0 rounded-b-xl"
          style={{
            position: 'absolute',
            top: 0,
            left: 0,
            right: 0,
            bottom: 0,
            background: mode === 'satellite' ? '#0a0a0e' : '#dde4ed',
          }}
        >
          {mode !== 'satellite' && (
            <TileLayer
              url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
              attribution='&copy; OpenStreetMap'
            />
          )}

          <MapResize />

          {/* Glow layer — rendered FIRST (beneath everything) for primary path edges */}
          {renderedEdges.map((edge, idx) => {
            const edgeKey = `${edge.from}-${edge.to}`;
            if (!primaryEdgeKeys.has(edgeKey)) return null;
            const polyline = getEdgePolyline(edge);
            if (!polyline) return null;
            return (
              <Polyline
                key={`glow-${edge.from}-${edge.to}-${idx}`}
                positions={polyline}
                pathOptions={{
                  color: PRIMARY_GLOW.color,
                  weight: PRIMARY_GLOW.weight,
                  opacity: PRIMARY_GLOW.opacity,
                  className: PRIMARY_GLOW.className || undefined,
                }}
              />
            );
          })}

          {/* Render edges — bottom layer: secondBest, then alt, then primary on top */}
          {renderedEdges.map((edge, idx) => {
            if (!shouldShowEdge(edge)) return null;
            const polyline = getEdgePolyline(edge);
            if (!polyline) return null;
            const edgeKey = `${edge.from}-${edge.to}`;
            const style = getEdgeStyle(edge, edgeKey);

            return (
              <Polyline
                key={`e-${edge.from}-${edge.to}-${idx}`}
                positions={polyline}
                pathOptions={{
                  color: style.color,
                  weight: style.weight,
                  opacity: style.opacity,
                  dashArray: style.dashArray || undefined,
                  className: style.className || undefined,
                }}
              >
                <Tooltip sticky>
                  <div className="edge-tooltip">
                    <strong>{edge.fromName} → {edge.toName}</strong>
                    <br />
                    Weight: {edge.weight?.toFixed(2)} km
                    {edge.baseWeight && edge.baseWeight !== edge.weight && (
                      <> (base: {edge.baseWeight?.toFixed(2)})</>
                    )}
                    <br />
                    Condition: <span className={`cond-${edge.condition || 'normal'}`}>{edge.condition || 'normal'}</span>
                  </div>
                </Tooltip>
              </Polyline>
            );
          })}

          {conditionIcons.map((ic) => (
            <Marker
              key={ic.key}
              position={ic.pos}
              icon={makeIconMarker(ic.text, `condition-icon condition-icon-${ic.condition}`)}
            />
          ))}

          {graphData.nodes.map((node) => {
            const pos = nodePositions[node.id];
            if (!pos) return null;
            const onPrimary = primaryNodeIds.has(node.id);
            let fillColor = '#e4e4e7';
            let strokeColor = '#71717a';
            let radius = 7;

            if (onPrimary) {
              fillColor = '#22d3ee';
              strokeColor = '#0891b2';
              radius = 10;
            }

            return (
              <CircleMarker
                key={`n-${node.id}`}
                center={pos}
                radius={radius}
                pathOptions={{
                  fillColor,
                  color: strokeColor,
                  weight: 2,
                  fillOpacity: 0.95,
                }}
              >
                <Tooltip direction="top" offset={[0, -8]} permanent={graphData.nodes.length <= 20}>
                  <span className="text-sm font-semibold text-zinc-900">{node.name}</span>
                </Tooltip>
              </CircleMarker>
            );
          })}
        </MapContainer>

        {mode !== 'satellite' && <MapLegend />}
      </div>
    </div>
  );
}

export default GraphMap;
