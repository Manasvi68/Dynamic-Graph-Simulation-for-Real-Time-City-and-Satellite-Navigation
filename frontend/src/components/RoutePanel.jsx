import React from 'react';

function RoutePanel({ pathData, graphData }) {
  if (!pathData?.found) return null;

  const edgeMap = {};
  if (graphData?.edges) {
    graphData.edges.forEach((e) => {
      edgeMap[`${e.from}-${e.to}`] = e;
      edgeMap[`${e.to}-${e.from}`] = e;
    });
  }

  const segments = [];
  for (let i = 0; i < pathData.path.length - 1; i++) {
    const a = pathData.path[i];
    const b = pathData.path[i + 1];
    const edge = edgeMap[`${a.id}-${b.id}`];
    segments.push({
      from: a.name,
      to: b.name,
      weight: edge?.weight ?? 0,
      baseWeight: edge?.baseWeight ?? edge?.weight ?? 0,
      condition: edge?.condition || 'normal',
    });
  }

  const totalWeight = segments.reduce((s, seg) => s + seg.weight, 0);

  return (
    <div className="panel-dark-tight mt-3 p-3 text-base">
      <p className="mb-2 text-sm font-bold uppercase tracking-wide text-zinc-400">Route breakdown</p>
      <div className="space-y-1">
        {segments.map((seg, i) => {
          const mult = seg.baseWeight > 0 ? (seg.weight / seg.baseWeight).toFixed(1) : '1.0';
          return (
            <div key={i} className="flex items-center gap-2 text-zinc-200">
              <span className="font-medium text-white">{seg.from}</span>
              <span className="text-zinc-500">&rarr;</span>
              <span className="font-medium text-white">{seg.to}</span>
              <span className="ml-auto font-mono text-sm text-cyan-300">{seg.weight.toFixed(2)} km</span>
              {mult !== '1.0' && (
                <span className="font-mono text-sm text-amber-300">x{mult}</span>
              )}
              <span className={`text-sm cond-${seg.condition}`}>{seg.condition}</span>
            </div>
          );
        })}
      </div>
      <div className="mt-2 flex items-center justify-between border-t border-white/10 pt-2">
        <span className="text-sm font-bold uppercase text-zinc-400">Total</span>
        <span className="font-mono text-base font-bold text-cyan-200">{totalWeight.toFixed(2)} km</span>
      </div>
      <div className="mt-1 text-sm text-zinc-400">
        Algorithm: <span className="font-semibold text-zinc-200">{pathData.algorithm}</span>
        {' · '}Nodes explored: <span className="font-semibold text-zinc-200">{pathData.nodesExplored}</span>
      </div>
    </div>
  );
}

export default RoutePanel;
