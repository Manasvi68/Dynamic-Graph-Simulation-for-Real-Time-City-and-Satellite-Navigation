import React, { useState, useEffect, useRef, useMemo } from 'react';

const SCALE = 18;
const CX = 400;
const CY = 300;

function toScreen(x, y) {
  return { x: CX + x * SCALE, y: CY - y * SCALE };
}

function SatelliteView({ graphData, pathData }) {
  const svgRef = useRef(null);
  const [tick, setTick] = useState(0);
  const rafRef = useRef(null);

  useEffect(() => {
    let start = performance.now();
    const animate = () => {
      setTick(performance.now() - start);
      rafRef.current = requestAnimationFrame(animate);
    };
    rafRef.current = requestAnimationFrame(animate);
    return () => cancelAnimationFrame(rafRef.current);
  }, []);

  const nodes = graphData?.nodes || [];
  const edges = graphData?.edges || [];

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

  const pathNodeIds = useMemo(() => {
    if (!pathData?.found) return new Set();
    return new Set(pathData.path.map((s) => s.id));
  }, [pathData]);

  const stars = useMemo(() => {
    const s = [];
    for (let i = 0; i < 200; i++) {
      s.push({
        x: Math.random() * 800,
        y: Math.random() * 600,
        r: Math.random() * 1.2 + 0.3,
        delay: Math.random() * 3,
      });
    }
    return s;
  }, []);

  const nodeScreenPos = {};
  nodes.forEach((n) => {
    if (n.posX !== undefined && n.posY !== undefined) {
      const p = toScreen(n.posX, n.posY);
      nodeScreenPos[n.id] = p;
    }
  });

  const orbitEllipses = nodes.map((n) => {
    if (!n.orbitParams) return null;
    const op = n.orbitParams;
    const a = op.semiMajor * SCALE;
    const e = op.eccentricity || 0;
    const b = a * Math.sqrt(1 - e * e);
    const inc = (op.inclination || 0) * (180 / Math.PI);
    const center = toScreen(op.centerX || 0, op.centerY || 0);
    return { a, b, cx: center.x, cy: center.y, rotation: -inc, name: n.name, id: n.id };
  }).filter(Boolean);

  return (
    <div className="sat-view">
      <div className="sat-header">
        <span className="text-sm font-bold uppercase tracking-widest text-cyan-300">
          Satellite command
        </span>
        <span className="font-mono text-sm font-semibold text-zinc-400">
          {nodes.length} satellites · {edges.length} links
        </span>
      </div>
      <div className="sat-canvas">
        <svg ref={svgRef} viewBox="0 0 800 600" className="sat-svg">
          {/* Stars */}
          {stars.map((s, i) => (
            <circle key={`star-${i}`} cx={s.x} cy={s.y} r={s.r}
              fill="#fff" opacity={0.3 + 0.7 * Math.abs(Math.sin((tick / 1000 + s.delay) * 0.8))} />
          ))}

          {/* Planet */}
          <defs>
            <radialGradient id="planet-grad" cx="35%" cy="35%">
              <stop offset="0%" stopColor="#1e3a5f" />
              <stop offset="50%" stopColor="#0f2440" />
              <stop offset="100%" stopColor="#060d1a" />
            </radialGradient>
            <radialGradient id="glow" cx="50%" cy="50%">
              <stop offset="0%" stopColor="rgba(34,211,238,0.08)" />
              <stop offset="100%" stopColor="transparent" />
            </radialGradient>
          </defs>
          <circle cx={CX} cy={CY} r={80} fill="url(#glow)" />
          <circle cx={CX} cy={CY} r={3 * SCALE} fill="url(#planet-grad)" stroke="#1e3a5f" strokeWidth="1" />

          {/* Continent hints */}
          <ellipse cx={CX - 10} cy={CY - 10} rx={12} ry={8} fill="#1a4d2e" opacity="0.3" />
          <ellipse cx={CX + 15} cy={CY + 5} rx={8} ry={14} fill="#1a4d2e" opacity="0.25" />
          <ellipse cx={CX - 20} cy={CY + 15} rx={6} ry={4} fill="#1a4d2e" opacity="0.2" />

          {/* Orbit trails */}
          {orbitEllipses.map((oe) => (
            <ellipse
              key={`orbit-${oe.id}`}
              cx={oe.cx} cy={oe.cy}
              rx={oe.a} ry={oe.b}
              fill="none" stroke="rgba(100,200,255,0.08)" strokeWidth="1"
              strokeDasharray="4,6"
              transform={`rotate(${oe.rotation} ${oe.cx} ${oe.cy})`}
            />
          ))}

          {/* Links */}
          {edges.map((edge, idx) => {
            const from = nodeScreenPos[edge.from];
            const to = nodeScreenPos[edge.to];
            if (!from || !to) return null;
            const onPath = pathEdgeKeys.has(`${edge.from}-${edge.to}`);
            const pulseOffset = (tick / 15) % 40;

            return (
              <g key={`link-${idx}`}>
                <line
                  x1={from.x} y1={from.y} x2={to.x} y2={to.y}
                  stroke={onPath ? '#22d3ee' : 'rgba(34,211,238,0.25)'}
                  strokeWidth={onPath ? 2.5 : 1}
                  strokeDasharray={onPath ? '8,4' : '3,5'}
                  strokeDashoffset={pulseOffset}
                />
                {onPath && (
                  <line
                    x1={from.x} y1={from.y} x2={to.x} y2={to.y}
                    stroke="rgba(34,211,238,0.15)" strokeWidth="6"
                  />
                )}
              </g>
            );
          })}

          {/* Satellite nodes */}
          {nodes.map((n) => {
            const pos = nodeScreenPos[n.id];
            if (!pos) return null;
            const onPath = pathNodeIds.has(n.id);
            const pulse = 3 + Math.sin(tick / 400 + n.id) * 1.5;

            return (
              <g key={`sat-${n.id}`}>
                {onPath && <circle cx={pos.x} cy={pos.y} r={pulse + 8} fill="none" stroke="rgba(34,211,238,0.2)" strokeWidth="1" />}
                <circle cx={pos.x} cy={pos.y} r={pulse + 3} fill="none" stroke="rgba(34,211,238,0.15)" strokeWidth="1" />
                <circle cx={pos.x} cy={pos.y} r={onPath ? 6 : 4.5}
                  fill={onPath ? '#22d3ee' : '#67e8f9'}
                  stroke={onPath ? '#fff' : '#164e63'}
                  strokeWidth={onPath ? 1.5 : 0.8}
                  filter="url(#glow)"
                />
                <text x={pos.x} y={pos.y - 12} textAnchor="middle"
                  className="sat-label" fill="#94a3b8" fontSize="9" fontWeight="600">
                  {n.name}
                </text>
              </g>
            );
          })}
        </svg>

        {/* Info panel overlay */}
        <div className="sat-info-panel">
          <div className="sat-info-title">SATELLITE STATUS</div>
          {nodes.map((n) => {
            const dist = n.posX !== undefined
              ? Math.sqrt(n.posX * n.posX + (n.posY || 0) * (n.posY || 0)).toFixed(1)
              : '?';
            return (
              <div key={n.id} className="sat-info-row">
                <span className="sat-dot" />
                <span className="sat-info-name">{n.name}</span>
                <span className="sat-info-val">{dist} u</span>
              </div>
            );
          })}
          {pathData?.found && (
            <div className="mt-2 border-t border-cyan-900/50 pt-2">
              <div className="text-xs font-bold text-cyan-400">PATH</div>
              <div className="text-xs text-zinc-400 leading-relaxed">
                {pathData.path.map((s) => s.name).join(' → ')}
              </div>
              <div className="text-xs text-zinc-500 mt-0.5">
                Signal delay: {(pathData.totalCost / 50 * 1000).toFixed(0)} ms
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

export default SatelliteView;
