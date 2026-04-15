import React from 'react';

const LABELS = { dijkstra: 'Dijkstra', astar: 'A*' };
const COLORS = { dijkstra: '#22d3ee', astar: '#f59e0b' };

function RouteCompare({ currentRoute, algo }) {
  if (!currentRoute?.found) return null;

  return (
    <div className="mt-2.5 space-y-1.5">
      <div className={`rounded-md border border-white/15 bg-white/5 px-2 py-1.5`}>
        <div className="flex items-center gap-1.5 mb-0.5">
          <span className="h-2 w-2 rounded-full" style={{ background: COLORS[algo] }} />
          <span className="text-[9px] font-bold uppercase tracking-widest text-zinc-500">{LABELS[algo]} Performance</span>
        </div>
        <p className="font-mono text-sm font-bold" style={{ color: COLORS[algo] }}>
          {currentRoute.totalCost.toFixed(2)}<span className="text-[9px] text-zinc-500"> km</span>
        </p>
        <p className="text-[9px] text-zinc-500">{currentRoute.nodesExplored} nodes explored · {currentRoute.path.length} hops</p>
      </div>

      {/* Path preview */}
      <p className="truncate rounded-md bg-white/[0.03] px-2 py-1 text-[9px] text-zinc-500" title={currentRoute.path.map(s => s.name).join(' → ')}>
        <span className="text-zinc-400">{currentRoute.path.map(s => s.name).join(' → ')}</span>
      </p>

      {/* Second-best hint */}
      {currentRoute.secondBest?.found && (
        <div className="flex items-center gap-1.5 px-1 mt-1">
          <span className="h-1.5 w-1.5 rounded-full bg-purple-400" />
          <span className="text-[9px] text-zinc-500">
            Alternate Route Available: <span className="font-mono font-semibold text-purple-300">{currentRoute.secondBest.totalCost.toFixed(2)} km</span>
          </span>
        </div>
      )}
    </div>
  );
}

export default RouteCompare;
