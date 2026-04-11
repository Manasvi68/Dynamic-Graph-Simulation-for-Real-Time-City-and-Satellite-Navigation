import React, { useMemo } from 'react';
import { MapContainer, TileLayer, CircleMarker, Polyline, Tooltip } from 'react-leaflet';
import { CRS } from 'leaflet';
import MapResize from './MapResize';
import { getNodePosition } from '../data/cityCoords';

function GraphMap({ graphData, pathData, mode }) {
  const nodePositions = useMemo(() => {
    const positions = {};
    if (!graphData?.nodes) return positions;

    graphData.nodes.forEach((node, index) => {
      if (mode === 'satellite' && node.posX !== undefined) {
        positions[node.id] = [node.posY, node.posX];
      } else {
        positions[node.id] = getNodePosition(node.name, index);
      }
    });
    return positions;
  }, [graphData, mode]);

  const pathNodeIds = useMemo(() => {
    if (!pathData?.found) return new Set();
    return new Set(pathData.path.map((step) => step.id));
  }, [pathData]);

  const pathEdgeKeys = useMemo(() => {
    if (!pathData?.found) return new Set();
    const keys = new Set();
    for (let i = 0; i < pathData.path.length - 1; i++) {
      const a = pathData.path[i].id;
      const b = pathData.path[i + 1].id;
      keys.add(`${a}-${b}`);
      keys.add(`${b}-${a}`);
    }
    return keys;
  }, [pathData]);

  const mapCenter = mode === 'satellite' ? [0, 0] : [28.61, 77.18];
  const mapZoom = mode === 'satellite' ? 3 : 12;
  const crs = mode === 'satellite' ? CRS.Simple : CRS.EPSG3857;

  if (!graphData?.nodes) {
    return (
      <div className="flex h-full min-h-0 w-full items-center justify-center rounded-xl border border-white/10 bg-zinc-950">
        <div className="px-4 text-center">
          <p className="text-base font-bold text-white">Loading map…</p>
          <p className="mt-2 text-sm font-medium text-zinc-300">Waiting for graph data</p>
        </div>
      </div>
    );
  }

  return (
    <div className="flex h-full min-h-0 flex-1 flex-col overflow-hidden rounded-xl border border-white/[0.08] bg-[#0a0a0e] shadow-[0_0_40px_rgba(0,0,0,0.5)]">
      <div className="flex shrink-0 items-center justify-between border-b border-white/15 bg-zinc-900 px-3 py-2.5">
        <span className="text-xs font-bold uppercase tracking-widest text-white">
          {mode === 'satellite' ? 'Satellite view' : 'City map'}
        </span>
        <span className="font-mono text-xs font-semibold text-zinc-200">
          {graphData.nodeCount} nodes · {graphData.edges?.length ?? 0} edges
        </span>
      </div>
      <div className="relative min-h-0 flex-1">
        <MapContainer
          key={mode}
          center={mapCenter}
          zoom={mapZoom}
          crs={crs}
          className="z-0 h-full w-full rounded-b-xl"
          style={{ background: mode === 'satellite' ? '#0a0a0e' : '#dde4ed' }}
        >
          {mode !== 'satellite' && (
            <TileLayer
              url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
              attribution='&copy; OpenStreetMap'
            />
          )}

          <MapResize />

          {graphData.edges?.map((edge, idx) => {
            const fromPos = nodePositions[edge.from];
            const toPos = nodePositions[edge.to];
            if (!fromPos || !toPos) return null;

            const onPath = pathEdgeKeys.has(`${edge.from}-${edge.to}`);

            return (
              <Polyline
                key={`e-${edge.from}-${edge.to}-${idx}`}
                positions={[fromPos, toPos]}
                pathOptions={{
                  color: onPath ? '#22d3ee' : '#52525b',
                  weight: onPath ? 5 : 2,
                  opacity: onPath ? 1 : 0.55,
                }}
              />
            );
          })}

          {graphData.nodes.map((node) => {
            const pos = nodePositions[node.id];
            if (!pos) return null;

            const onPath = pathNodeIds.has(node.id);

            return (
              <CircleMarker
                key={`n-${node.id}`}
                center={pos}
                radius={onPath ? 10 : 7}
                pathOptions={{
                  fillColor: onPath ? '#22d3ee' : '#e4e4e7',
                  color: onPath ? '#0891b2' : '#71717a',
                  weight: 2,
                  fillOpacity: 0.95,
                }}
              >
                <Tooltip direction="top" offset={[0, -8]} permanent={graphData.nodes.length <= 15}>
                  <span className="text-xs font-semibold text-zinc-900">{node.name}</span>
                </Tooltip>
              </CircleMarker>
            );
          })}
        </MapContainer>
      </div>
    </div>
  );
}

export default GraphMap;
