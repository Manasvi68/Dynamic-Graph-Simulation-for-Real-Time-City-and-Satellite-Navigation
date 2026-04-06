import React, { useMemo } from 'react';
import { MapContainer, TileLayer, CircleMarker, Polyline, Tooltip } from 'react-leaflet';
import { CRS } from 'leaflet';
import MapResize from './MapResize';
import { getNodePosition } from '../data/cityCoords';

function GraphMap({ graphData, pathData, mode }) {
  // build a lookup of nodeId -> position
  const nodePositions = useMemo(() => {
    const positions = {};
    if (!graphData || !graphData.nodes) return positions;

    graphData.nodes.forEach((node, index) => {
      if (mode === 'satellite' && node.posX !== undefined) {
        // satellite mode: use server-provided positions
        positions[node.id] = [node.posY, node.posX]; // leaflet wants [lat, lng] = [y, x]
      } else {
        // city mode: use our hardcoded coordinates
        positions[node.id] = getNodePosition(node.name, index);
      }
    });
    return positions;
  }, [graphData, mode]);

  // which node ids are on the shortest path?
  const pathNodeIds = useMemo(() => {
    if (!pathData || !pathData.found) return new Set();
    return new Set(pathData.path.map(step => step.id));
  }, [pathData]);

  // which edges are on the shortest path?
  const pathEdgeKeys = useMemo(() => {
    if (!pathData || !pathData.found) return new Set();
    const keys = new Set();
    for (let i = 0; i < pathData.path.length - 1; i++) {
      const a = pathData.path[i].id;
      const b = pathData.path[i + 1].id;
      keys.add(`${a}-${b}`);
      keys.add(`${b}-${a}`);
    }
    return keys;
  }, [pathData]);

  // map settings based on mode
  const mapCenter = mode === 'satellite' ? [0, 0] : [28.61, 77.18];
  const mapZoom = mode === 'satellite' ? 3 : 12;
  const crs = mode === 'satellite' ? CRS.Simple : CRS.EPSG3857;

  // don't render anything if no data yet
  if (!graphData || !graphData.nodes) {
    return <div className="w-full h-full flex items-center justify-center text-gray-500">Loading map...</div>;
  }

  return (
    <MapContainer
      key={mode}
      center={mapCenter}
      zoom={mapZoom}
      crs={crs}
      className="w-full h-full rounded-lg"
      style={{ background: '#1a1a2e' }}
    >
      {mode !== 'satellite' && (
        <TileLayer
          url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
          attribution="&copy; OpenStreetMap"
          className="dark-tiles"
        />
      )}

      <MapResize />

      {/* draw edges */}
      {graphData.edges && graphData.edges.map((edge, idx) => {
        const fromPos = nodePositions[edge.from];
        const toPos = nodePositions[edge.to];
        if (!fromPos || !toPos) return null;

        const isOnPath = pathEdgeKeys.has(`${edge.from}-${edge.to}`);

        return (
          <Polyline
            key={`edge-${idx}`}
            positions={[fromPos, toPos]}
            color={isOnPath ? '#22d3ee' : '#475569'}
            weight={isOnPath ? 4 : 2}
            opacity={isOnPath ? 1 : 0.5}
          />
        );
      })}

      {/* draw nodes */}
      {graphData.nodes.map((node) => {
        const pos = nodePositions[node.id];
        if (!pos) return null;

        const isOnPath = pathNodeIds.has(node.id);

        return (
          <CircleMarker
            key={`node-${node.id}`}
            center={pos}
            radius={isOnPath ? 9 : 6}
            fillColor={isOnPath ? '#22d3ee' : '#e2e8f0'}
            color={isOnPath ? '#06b6d4' : '#64748b'}
            weight={2}
            fillOpacity={0.9}
          >
            <Tooltip direction="top" offset={[0, -8]} permanent={graphData.nodes.length <= 15}>
              <span className="text-xs font-medium">{node.name}</span>
            </Tooltip>
          </CircleMarker>
        );
      })}
    </MapContainer>
  );
}

export default GraphMap;
