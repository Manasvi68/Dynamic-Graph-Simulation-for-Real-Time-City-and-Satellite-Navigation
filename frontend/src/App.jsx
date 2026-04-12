import React, { useState, useEffect, useRef, useCallback } from 'react';
import { Map, Route, Zap, Shield, Sparkles } from 'lucide-react';
import GraphMap from './components/GraphMap';
import BlockchainPanel from './components/BlockchainPanel';
import * as api from './api';

const MIN_SIDEBAR = 260;
const MAX_SIDEBAR = 560;
const DEFAULT_SIDEBAR = 360;

function readStoredWidth() {
  try {
    const v = parseInt(localStorage.getItem('sidebarW'), 10);
    return Number.isFinite(v) ? v : DEFAULT_SIDEBAR;
  } catch {
    return DEFAULT_SIDEBAR;
  }
}

function clamp(w) {
  const vwMax = Math.min(MAX_SIDEBAR, window.innerWidth - 340);
  return Math.max(MIN_SIDEBAR, Math.min(vwMax, w));
}

function App() {
  const [graphData, setGraphData] = useState(null);
  const [pathData, setPathData] = useState(null);
  const [blocks, setBlocks] = useState([]);
  const [mode, setMode] = useState('city');
  const [loading, setLoading] = useState(true);
  const [busy, setBusy] = useState(false);

  const [fromNode, setFromNode] = useState('');
  const [toNode, setToNode] = useState('');
  const [algo, setAlgo] = useState('dijkstra');

  const [editFrom, setEditFrom] = useState('');
  const [editTo, setEditTo] = useState('');
  const [editWeight, setEditWeight] = useState('');
  const [autoSim, setAutoSim] = useState(true);

  const [sidebarWidth, setSidebarWidth] = useState(readStoredWidth);
  const dragging = useRef(null);

  const persist = useCallback((w) => {
    try { localStorage.setItem('sidebarW', String(w)); } catch {}
  }, []);

  useEffect(() => {
    const onResize = () => setSidebarWidth((w) => clamp(w));
    window.addEventListener('resize', onResize);
    return () => window.removeEventListener('resize', onResize);
  }, []);

  useEffect(() => {
    const onMove = (e) => {
      if (!dragging.current) return;
      const { x0, w0 } = dragging.current;
      setSidebarWidth(clamp(w0 + e.clientX - x0));
    };
    const onUp = () => {
      if (!dragging.current) return;
      dragging.current = null;
      document.body.style.cursor = '';
      document.body.style.userSelect = '';
      setSidebarWidth((w) => { persist(w); return w; });
    };
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
    return () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
  }, [persist]);

  const startDrag = (e) => {
    e.preventDefault();
    dragging.current = { x0: e.clientX, w0: sidebarWidth };
    document.body.style.cursor = 'col-resize';
    document.body.style.userSelect = 'none';
  };

  useEffect(() => {
    async function loadAll() {
      try {
        const [chainRes, modeRes] = await Promise.all([
          api.getBlockchain(),
          api.getMode(),
        ]);
        const m = modeRes.mode || 'city';
        const graphRes = await api.getGraph(m === 'city' ? { scope: 'city' } : {});
        setGraphData(graphRes);
        setBlocks(chainRes);
        setMode(m);
      } catch (err) {
        console.error('Failed to load data:', err);
      }
      setLoading(false);
    }
    loadAll();
  }, []);

  const refreshData = useCallback(async () => {
    const graphOpts = mode === 'city' ? { scope: 'city' } : {};
    const [graphRes, chainRes] = await Promise.all([api.getGraph(graphOpts), api.getBlockchain()]);
    setGraphData(graphRes);
    setBlocks(chainRes);
  }, [mode]);

  useEffect(() => {
    if (!autoSim || mode !== 'city' || loading) return undefined;

    let cancelled = false;
    let timerId = null;

    const scheduleNext = () => {
      const delay = 1000 + Math.random() * 1000;
      timerId = window.setTimeout(runTick, delay);
    };

    const runTick = async () => {
      if (cancelled) return;
      try {
        await api.simStep();
        if (!cancelled) await refreshData();
      } catch (err) {
        console.error('Auto simulation tick:', err);
      }
      if (!cancelled) scheduleNext();
    };

    scheduleNext();

    return () => {
      cancelled = true;
      if (timerId !== null) window.clearTimeout(timerId);
    };
  }, [autoSim, mode, loading, refreshData]);

  const handleFindPath = async () => {
    if (!fromNode || !toNode) return;
    setBusy(true);
    try {
      const result = await api.findPath(fromNode, toNode, algo);
      setPathData(result);
    } catch (err) {
      console.error('Path error:', err);
    } finally {
      setBusy(false);
    }
  };

  const handleSimStep = async () => {
    setBusy(true);
    try {
      await api.simStep();
      await refreshData();
      setPathData(null);
    } catch (err) {
      console.error('Sim step error:', err);
    } finally {
      setBusy(false);
    }
  };

  const handleUpdateRoad = async () => {
    if (!editFrom || !editTo || !editWeight) return;
    setBusy(true);
    try {
      await api.updateRoad(editFrom, editTo, parseFloat(editWeight));
      await refreshData();
      setPathData(null);
    } catch (err) {
      console.error('Update road error:', err);
    } finally {
      setBusy(false);
    }
  };

  const handleCloseRoad = async () => {
    if (!editFrom || !editTo) return;
    setBusy(true);
    try {
      await api.closeRoad(editFrom, editTo);
      await refreshData();
      setPathData(null);
    } catch (err) {
      console.error('Close road error:', err);
    } finally {
      setBusy(false);
    }
  };

  const handleModeSwitch = async () => {
    setBusy(true);
    try {
      const newMode = mode === 'city' ? 'satellite' : 'city';
      await api.setMode(newMode);
      setMode(newMode);
      setPathData(null);
      const graphRes = await api.getGraph(newMode === 'city' ? { scope: 'city' } : {});
      setGraphData(graphRes);
      const chainRes = await api.getBlockchain();
      setBlocks(chainRes);
    } catch (err) {
      console.error('Mode switch error:', err);
    } finally {
      setBusy(false);
    }
  };

  if (loading) {
    return (
      <div className="flex h-full items-center justify-center bg-[#050508]">
        <div className="panel-dark max-w-sm px-8 py-7 text-center">
          <div className="mx-auto mb-4 h-9 w-9 animate-spin rounded-full border-2 border-sky-400 border-t-transparent" />
          <p className="font-semibold text-zinc-100">Loading graph &amp; log…</p>
          <p className="mt-2 text-sm leading-relaxed text-zinc-200">
            Start the C++ server on <span className="font-mono font-semibold text-white">8080</span>, then refresh.
          </p>
        </div>
      </div>
    );
  }

  const nodeNames = graphData?.nodes?.map((n) => n.name) || [];

  return (
    <div className="flex h-full min-h-0 flex-col bg-[#050508] text-zinc-100">
      {/* ── Header ── */}
      <header className="flex shrink-0 items-center justify-between gap-3 border-b border-white/15 bg-[#0c0c12] px-4 py-3">
        <div className="flex min-w-0 items-center gap-3">
          <div className="flex h-10 w-10 shrink-0 items-center justify-center rounded-lg bg-gradient-to-br from-sky-500 to-cyan-600 shadow-lg shadow-sky-900/40">
            <Map size={20} className="text-white" strokeWidth={2.2} />
          </div>
          <div className="min-w-0">
            <h1 className="truncate text-base font-bold tracking-tight text-white">
              Dynamic routing
            </h1>
            <p className="truncate text-xs font-medium leading-snug text-zinc-200 sm:text-sm">
              Pathfind · simulate · log — map on the right updates live
            </p>
          </div>
        </div>

        <div className="flex shrink-0 items-center gap-3">
          <div
            className="flex items-center gap-2 rounded-full border border-white/20 bg-zinc-900 px-2.5 py-1.5"
            title="City: Delhi map. Satellite: orbital links."
          >
            <span className={`text-sm font-bold ${mode === 'city' ? 'text-sky-300' : 'text-zinc-300'}`}>
              City
            </span>
            <button
              type="button"
              onClick={handleModeSwitch}
              disabled={busy}
              className="relative h-6 w-11 rounded-full bg-zinc-700 transition-colors hover:bg-zinc-600 disabled:opacity-50"
              aria-label={`Switch to ${mode === 'city' ? 'satellite' : 'city'}`}
            >
              <span
                className={`absolute top-0.5 h-5 w-5 rounded-full bg-zinc-200 shadow transition-all ${
                  mode === 'satellite' ? 'left-5' : 'left-0.5'
                }`}
              />
            </button>
            <span className={`text-sm font-bold ${mode === 'satellite' ? 'text-sky-300' : 'text-zinc-300'}`}>
              Satellite
            </span>
          </div>
          {busy && (
            <span className="flex items-center gap-1.5 text-sm font-semibold text-amber-200">
              <Sparkles size={14} className="animate-pulse" />
              Busy
            </span>
          )}
        </div>
      </header>

      {/* ── Body: sidebar | handle | map ── */}
      <div className="flex h-full min-h-0 flex-1 flex-row overflow-hidden p-3">
        {/* Sidebar */}
        <aside
          className="event-log-sidebar grid h-full min-h-0 max-h-full shrink-0 grid-rows-[auto_auto_minmax(0,1fr)] gap-2.5 overflow-hidden pr-1"
          style={{ width: sidebarWidth }}
        >
          {/* ── Route panel ── */}
          <section className="panel-dark shrink-0 p-3.5">
            <h2 className="mb-1 flex items-center gap-2 text-sm font-bold uppercase tracking-wide text-white">
              <Route size={16} className="text-sky-400" />
              Route
            </h2>
            <p className="mb-3 text-sm font-medium leading-snug text-zinc-200">
              Shortest path shows in <span className="font-bold text-cyan-300">bright cyan</span> on the map →
            </p>
            <div className="grid grid-cols-2 gap-2">
              <div>
                <label className="mb-1 block text-xs font-bold uppercase tracking-wide text-zinc-200">From</label>
                <select value={fromNode} onChange={(e) => setFromNode(e.target.value)} className="input-dark">
                  <option value="">Select…</option>
                  {nodeNames.map((name) => (
                    <option key={name} value={name}>{name}</option>
                  ))}
                </select>
              </div>
              <div>
                <label className="mb-1 block text-xs font-bold uppercase tracking-wide text-zinc-200">To</label>
                <select value={toNode} onChange={(e) => setToNode(e.target.value)} className="input-dark">
                  <option value="">Select…</option>
                  {nodeNames.map((name) => (
                    <option key={name} value={name}>{name}</option>
                  ))}
                </select>
              </div>
            </div>
            <div className="mt-2 flex gap-1.5">
              <button
                type="button"
                onClick={() => setAlgo('dijkstra')}
                className={`btn-pill flex-1 ${algo === 'dijkstra' ? 'btn-pill-active' : ''}`}
              >
                Dijkstra
              </button>
              <button
                type="button"
                onClick={() => setAlgo('astar')}
                className={`btn-pill flex-1 ${algo === 'astar' ? 'btn-pill-active' : ''}`}
              >
                A*
              </button>
            </div>
            <button
              type="button"
              onClick={handleFindPath}
              disabled={!fromNode || !toNode || busy}
              className="btn-accent mt-2 w-full"
            >
              Compute path
            </button>
            {pathData?.found && (
              <div className="panel-dark-tight mt-3 space-y-1.5 p-3 text-sm">
                <p className="font-bold text-emerald-300">Path found</p>
                <p className="font-medium text-zinc-100">
                  Cost <span className="font-mono font-bold text-cyan-200">{pathData.totalCost.toFixed(2)}</span>
                  <span className="text-zinc-400"> · </span>
                  Nodes <span className="font-bold text-white">{pathData.nodesExplored}</span>
                </p>
                <p className="leading-snug text-zinc-200">{pathData.path.map((s) => s.name).join(' → ')}</p>
              </div>
            )}
            {pathData && !pathData.found && (
              <p className="mt-3 rounded-md border border-red-400/50 bg-red-950/70 px-3 py-2 text-sm font-semibold text-red-100">
                No path — try other places or check for a closed road.
              </p>
            )}
          </section>

          {/* ── Simulation panel ── */}
          <section className="panel-dark shrink-0 p-3.5">
            <h2 className="mb-1 flex items-center gap-2 text-sm font-bold uppercase tracking-wide text-white">
              <Zap size={16} className="text-amber-400" />
              Simulation
            </h2>
            <p className="mb-3 text-sm font-medium leading-snug text-zinc-200">
              {mode === 'city'
                ? 'Auto mode picks a random road every 1–2s and applies Normal, Traffic, Construction, Accident, or Blocked (infinite cost). You can still run a step manually or edit weights.'
                : 'Each step: satellites move; links can appear or disappear on the map.'}
            </p>
            {mode === 'city' && (
              <label className="mb-2 flex cursor-pointer items-center gap-2 text-sm font-semibold text-zinc-200">
                <input
                  type="checkbox"
                  className="h-4 w-4 rounded border-zinc-500 bg-zinc-800 text-amber-500 focus:ring-amber-500"
                  checked={autoSim}
                  onChange={(e) => setAutoSim(e.target.checked)}
                />
                Auto traffic simulation (1–2s)
              </label>
            )}
            <button
              type="button"
              onClick={handleSimStep}
              disabled={busy || mode !== 'city'}
              className="w-full rounded-lg border border-amber-300/40 bg-gradient-to-b from-amber-500 to-amber-700 py-2.5 text-sm font-bold text-white shadow-lg shadow-amber-950/60 hover:brightness-110 disabled:opacity-50"
            >
              Run simulation step
            </button>
            <p className="mb-2 mt-4 text-xs font-bold uppercase tracking-wide text-zinc-200">Edit road</p>
            <div className="grid grid-cols-2 gap-1.5">
              <select value={editFrom} onChange={(e) => setEditFrom(e.target.value)} className="input-dark">
                <option value="">From</option>
                {nodeNames.map((name) => (
                  <option key={name} value={name}>{name}</option>
                ))}
              </select>
              <select value={editTo} onChange={(e) => setEditTo(e.target.value)} className="input-dark">
                <option value="">To</option>
                {nodeNames.map((name) => (
                  <option key={name} value={name}>{name}</option>
                ))}
              </select>
            </div>
            <div className="mt-1.5 flex gap-1.5">
              <input
                type="number"
                placeholder="Weight"
                value={editWeight}
                onChange={(e) => setEditWeight(e.target.value)}
                className="input-dark min-w-0 flex-1"
                min="0.1"
                step="0.1"
              />
              <button
                type="button"
                onClick={handleUpdateRoad}
                disabled={!editFrom || !editTo || !editWeight || busy}
                className="shrink-0 rounded-lg border border-violet-300/40 bg-violet-600 px-3 py-2 text-sm font-bold text-white hover:bg-violet-500 disabled:opacity-45"
              >
                Set
              </button>
            </div>
            <button
              type="button"
              onClick={handleCloseRoad}
              disabled={!editFrom || !editTo || busy}
              className="mt-2 w-full rounded-lg border border-red-400/45 bg-red-950/80 py-2 text-sm font-bold text-red-100 hover:bg-red-900/70 disabled:opacity-45"
            >
              Remove road
            </button>
          </section>

          {/* ── Event log panel (scrolls independently; rest of sidebar stays fixed) ── */}
          <section className="panel-dark flex h-full min-h-0 min-w-0 flex-col overflow-hidden p-3.5">
            <h2 className="mb-1 flex shrink-0 items-center gap-2 text-sm font-bold uppercase tracking-wide text-white">
              <Shield size={16} className="text-emerald-400" />
              Event log
            </h2>
            <p className="mb-2 shrink-0 text-sm font-medium leading-snug text-zinc-200">
              Tap a row to expand. Scroll this list to see older entries.
            </p>
            <div
              className="event-log-scroll h-0 min-h-0 flex-1 overflow-y-scroll overflow-x-hidden rounded-lg border border-white/10 bg-black/25 py-1 pl-1 pr-1"
              role="region"
              aria-label="Event log entries"
            >
              <BlockchainPanel blocks={blocks} />
            </div>
          </section>
        </aside>

        {/* ── Drag handle ── */}
        <div
          className="drag-handle"
          onMouseDown={startDrag}
          role="separator"
          aria-orientation="vertical"
          aria-label="Drag to resize left panel"
          tabIndex={0}
          onKeyDown={(e) => {
            if (e.key !== 'ArrowLeft' && e.key !== 'ArrowRight') return;
            e.preventDefault();
            const step = e.shiftKey ? 30 : 12;
            const dir = e.key === 'ArrowRight' ? 1 : -1;
            setSidebarWidth((w) => {
              const next = clamp(w + dir * step);
              persist(next);
              return next;
            });
          }}
        />

        {/* ── Map ── */}
        <main className="flex h-full min-h-0 min-w-0 flex-1 flex-col">
          <GraphMap graphData={graphData} pathData={pathData} mode={mode} />
        </main>
      </div>
    </div>
  );
}

export default App;
