import React, { useState, useEffect, useRef, useMemo } from 'react';
import { Marker } from 'react-leaflet';
import { divIcon } from 'leaflet';

const CONDITION_CONFIG = {
  normal:        { count: 0, speed: 0 },
  light_traffic: { count: 2, speed: 0.0004 },
  heavy_traffic: { count: 5, speed: 0.00025 },
  congestion:    { count: 8, speed: 0.00008 },
  accident:      { count: 0, speed: 0 },
  construction:  { count: 1, speed: 0.0001 },
  closed:        { count: 0, speed: 0 },
};

function polylineLength(pts) {
  let len = 0;
  for (let i = 1; i < pts.length; i++) {
    const dx = pts[i][0] - pts[i-1][0];
    const dy = pts[i][1] - pts[i-1][1];
    len += Math.sqrt(dx*dx + dy*dy);
  }
  return len;
}

function interpolatePolyline(pts, t) {
  if (pts.length < 2) return pts[0] || [0,0];
  const total = polylineLength(pts);
  const target = t * total;
  let acc = 0;
  for (let i = 1; i < pts.length; i++) {
    const dx = pts[i][0] - pts[i-1][0];
    const dy = pts[i][1] - pts[i-1][1];
    const segLen = Math.sqrt(dx*dx + dy*dy);
    if (acc + segLen >= target && segLen > 0) {
      const frac = (target - acc) / segLen;
      return [pts[i-1][0] + dx * frac, pts[i-1][1] + dy * frac];
    }
    acc += segLen;
  }
  return pts[pts.length - 1];
}

const carIcon = divIcon({
  html: '<div class="traffic-car"></div>',
  className: 'traffic-car-wrapper',
  iconSize: [8, 8],
  iconAnchor: [4, 4],
});

function TrafficAnimations({ edges, getEdgePolyline }) {
  const [tick, setTick] = useState(0);
  const rafRef = useRef(null);
  const startRef = useRef(performance.now());

  useEffect(() => {
    const animate = () => {
      const elapsed = performance.now() - startRef.current;
      setTick(elapsed);
      rafRef.current = requestAnimationFrame(animate);
    };
    rafRef.current = requestAnimationFrame(animate);
    return () => { if (rafRef.current) cancelAnimationFrame(rafRef.current); };
  }, []);

  const cars = useMemo(() => {
    if (!edges) return [];
    const result = [];
    edges.forEach((edge, edgeIdx) => {
      const condition = edge.condition || 'normal';
      const cfg = CONDITION_CONFIG[condition] || CONDITION_CONFIG.normal;
      if (cfg.count === 0) return;

      const polyline = getEdgePolyline(edge);
      if (!polyline || polyline.length < 2) return;

      for (let c = 0; c < cfg.count; c++) {
        const offset = c / cfg.count;
        result.push({ edgeIdx, carIdx: c, offset, speed: cfg.speed, polyline });
      }
    });
    return result;
  }, [edges, getEdgePolyline]);

  const carPositions = cars.map((car) => {
    const t = ((tick * car.speed + car.offset) % 1 + 1) % 1;
    return interpolatePolyline(car.polyline, t);
  });

  return (
    <>
      {carPositions.map((pos, i) => (
        <Marker
          key={`car-${cars[i].edgeIdx}-${cars[i].carIdx}`}
          position={pos}
          icon={carIcon}
          interactive={false}
        />
      ))}
    </>
  );
}

export default TrafficAnimations;
