import React, { useState, useEffect, useRef, useCallback } from 'react';
import { Map, Route, Zap, Shield, Sparkles, PanelLeftClose, PanelLeftOpen, ChevronDown, ChevronUp } from 'lucide-react';
import GraphMap from './components/GraphMap';
import SatelliteView from './components/SatelliteView';
import BlockchainPanel from './components/BlockchainPanel';
import RouteCompare from './components/RouteCompare';
import ToastContainer from './components/Toast';
import useToast from './hooks/useToast';
import * as api from './api';

const MIN_SIDEBAR = 280;
const MAX_SIDEBAR = 460;
const DEFAULT_SIDEBAR = 340;

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

/* ─── Collapsible section wrapper ─── */
function Section({ icon, title, defaultOpen = true, accent = 'sky', children }) {
  const [open, setOpen] = useState(defaultOpen);
  const accentMap = { sky: 'text-sky-400', amber: 'text-amber-400', emerald: 'text-emerald-400' };
  return (
    <section className="panel-dark overflow-hidden shrink-0 flex flex-col min-h-0">
      <button
        type="button"
        onClick={() => setOpen((v) => !v)}
        className="flex w-full items-center gap-2 px-3.5 py-2.5 text-left transition-colors hover:bg-white/5"
      >
        {React.cloneElement(icon, { size: 15, className: accentMap[accent] || 'text-sky-400' })}
        <span className="flex-1 text-xs font-bold uppercase tracking-widest text-white">{title}</span>
        {open ? <ChevronUp size={14} className="text-zinc-500" /> : <ChevronDown size={14} className="text-zinc-500" />}
      </button>
      {open && <div className="border-t border-white/8 px-3.5 pb-3.5 pt-3 overflow-y-auto min-h-0">{children}</div>}
    </section>
  );
}

function App() {
  const [graphData, setGraphData] = useState(null);
  const [currentRoute, setCurrentRoute] = useState(null);
  const [blocks, setBlocks] = useState([]);
  const [mode, setMode] = useState('city');
  const [loading, setLoading] = useState(true);
  const [busy, setBusy] = useState(false);
  const [autoSim, setAutoSim] = useState(false);

  const [fromNode, setFromNode] = useState('');
  const [toNode, setToNode] = useState('');

  const [editFrom, setEditFrom] = useState('');
  const [editTo, setEditTo] = useState('');
  const [editWeight, setEditWeight] = useState('');

  const [sidebarWidth, setSidebarWidth] = useState(readStoredWidth);
  const [sidebarOpen, setSidebarOpen] = useState(true);
  const [isMobile, setIsMobile] = useState(() => window.innerWidth < 640);
  const dragging = useRef(null);
  const prevBlockCount = useRef(0);

  const { toasts, addToast, dismissToast } = useToast(4000);

  const persist = useCallback((w) => {
    try { localStorage.setItem('sidebarW', String(w)); } catch {}
  }, []);

  useEffect(() => {
    setSidebarWidth((w) => clamp(w));
    const onResize = () => {
      setSidebarWidth((w) => clamp(w));
      setIsMobile(window.innerWidth < 640);
    };
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
        const [graphRes, chainRes, modeRes] = await Promise.all([
          api.getGraph(),
          api.getBlockchain(),
          api.getMode(),
        ]);
        setGraphData(graphRes);
        setBlocks(chainRes);
        prevBlockCount.current = chainRes.length;
        setMode(modeRes.mode || 'city');
      } catch (err) {
        console.error('Failed to load data:', err);
      }
      setLoading(false);
    }
    loadAll();
  }, []);

  // toast on new blockchain entries
  useEffect(() => {
    if (blocks.length > prevBlockCount.current) {
      const newBlocks = blocks.slice(prevBlockCount.current);
      newBlocks.forEach((block) => {
        const data = typeof block.data === 'object' ? block.data : {};
        const type = data.type || '';
        const from = data.fromNode || '';
        const to = data.toNode || '';
        let msg = `${type}: ${from} → ${to}`;
        let toastType = 'info';
        if (type === 'accident') { msg = `Accident on ${from} → ${to}`; toastType = 'error'; }
        else if (type === 'ROAD_CLOSED') { msg = `Road closed: ${from} → ${to}`; toastType = 'warning'; }
        else if (type === 'congestion') { msg = `Congestion on ${from} → ${to}`; toastType = 'warning'; }
        else if (type === 'RECOVERY') { msg = `Road recovered: ${from} → ${to}`; toastType = 'success'; }
        else if (type === 'SAT_LINK_UP') { msg = `Satellite link: ${from} ↔ ${to}`; toastType = 'success'; }
        else if (type === 'SAT_LINK_DOWN') { msg = `Link lost: ${from} ↔ ${to}`; toastType = 'warning'; }
        addToast(msg, toastType);
      });
    }
    prevBlockCount.current = blocks.length;
  }, [blocks, addToast]);

  async function refreshData() {
    const [graphRes, chainRes] = await Promise.all([api.getGraph(), api.getBlockchain()]);
    setGraphData(graphRes);
    setBlocks(chainRes);
  }

  /* ── Compute algorithm based on mode ── */
  const computeRoute = useCallback(async (from, to) => {
    if (!from || !to) return;
    try {
      const algo = mode === 'city' ? 'astar' : 'dijkstra';
      const pathData = await api.findPath(from, to, algo);
      setCurrentRoute(pathData);
    } catch (err) {
      console.error('Route compute error:', err);
    }
  }, [mode]);

  const handleFindPath = async () => {
    if (!fromNode || !toNode) return;
    setBusy(true);
    try {
      await computeRoute(fromNode, toNode);
    } finally {
      setBusy(false);
    }
  };

  const handleSimStep = async () => {
    setBusy(true);
    try {
      await api.simStep();
      await refreshData();
      // Auto-recompute routes if a route was previously set
      if (fromNode && toNode) {
        await computeRoute(fromNode, toNode);
        addToast('Routes auto-updated after simulation', 'info');
      }
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
      if (fromNode && toNode) {
        await computeRoute(fromNode, toNode);
        addToast('Routes auto-updated after road edit', 'info');
      }
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
      if (fromNode && toNode) {
        await computeRoute(fromNode, toNode);
        addToast('Routes auto-updated after road closure', 'info');
      }
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
      setCurrentRoute(null);
      const graphRes = await api.getGraph();
      setGraphData(graphRes);
      const chainRes = await api.getBlockchain();
      setBlocks(chainRes);
    } catch (err) {
      console.error('Mode switch error:', err);
    } finally {
      setBusy(false);
    }
  };

  // Auto-simulation effect
  const handleSimStepRef = useRef(null);
  const busyRef = useRef(false);

  useEffect(() => { handleSimStepRef.current = handleSimStep; });
  useEffect(() => { busyRef.current = busy; }, [busy]);

  useEffect(() => {
    let interval;
    if (autoSim) {
      interval = setInterval(() => {
        if (!busyRef.current && handleSimStepRef.current) {
          handleSimStepRef.current();
        }
      }, 3000);
    }
    return () => clearInterval(interval);
  }, [autoSim]);

  // Keyboard shortcuts
  useEffect(() => {
    const handler = (e) => {
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT' || e.target.tagName === 'TEXTAREA') return;
      switch (e.key.toLowerCase()) {
        case 'r':
          e.preventDefault();
          setCurrentRoute(null);
          addToast('Routes reset', 'info');
          break;
        case 'f':
          e.preventDefault();
          document.querySelector('.route-from-select')?.focus();
          break;
        case 's':
          e.preventDefault();
          handleSimStep();
          break;
        case 'escape':
          setSidebarOpen(false);
          break;
      }
    };
    window.addEventListener('keydown', handler);
    return () => window.removeEventListener('keydown', handler);
  }, [handleSimStep, addToast]);

  if (loading) {
    return (
      <div className="flex h-full items-center justify-center bg-[#050508]">
        <div className="panel-dark max-w-sm px-8 py-7 text-center">
          <div className="mx-auto mb-4 h-9 w-9 animate-spin rounded-full border-2 border-sky-400 border-t-transparent" />
          <p className="font-semibold text-zinc-100">Loading graph &amp; log...</p>
          <p className="mt-2 text-sm leading-relaxed text-zinc-200">
            Start the C++ server on <span className="font-mono font-semibold text-white">8080</span>, then refresh.
          </p>
        </div>
      </div>
    );
  }

  const nodeNames = graphData?.nodes?.map((n) => n.name).sort() || [];

  return (
    <div className="flex h-full min-h-0 flex-col bg-[#050508] text-zinc-100">
      {/* Header */}
      <header className="flex shrink-0 items-center justify-between gap-3 border-b border-white/15 bg-[#0c0c12] px-4 py-2.5">
        <div className="flex min-w-0 items-center gap-3">
          <div className="flex h-9 w-9 shrink-0 items-center justify-center rounded-lg bg-gradient-to-br from-sky-500 to-cyan-600 shadow-lg shadow-sky-900/40">
            <Map size={18} className="text-white" strokeWidth={2.2} />
          </div>
          <div className="min-w-0">
            <h1 className="truncate text-sm font-bold tracking-tight text-white sm:text-base">
              Dynamic City Navigation
            </h1>
            <p className="hidden truncate text-xs font-medium leading-snug text-zinc-400 sm:block">
              Jodhpur · Pathfind · Simulate · Blockchain log
            </p>
          </div>
        </div>

        <div className="flex shrink-0 items-center gap-2.5">
          <div
            className="flex items-center gap-2 rounded-full border border-white/20 bg-zinc-900 px-2.5 py-1.5"
            title="City: Jodhpur map. Satellite: orbital links."
          >
            <span className={`whitespace-nowrap text-xs font-bold sm:text-sm ${mode === 'city' ? 'text-sky-300' : 'text-zinc-400'}`}>
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
            <span className={`whitespace-nowrap text-xs font-bold sm:text-sm ${mode === 'satellite' ? 'text-sky-300' : 'text-zinc-400'}`}>
              Sat
            </span>
          </div>

          {busy && (
            <span className="flex items-center gap-1.5 text-xs font-semibold text-amber-200 sm:text-sm">
              <Sparkles size={14} className="animate-pulse" />
              Busy
            </span>
          )}

          <button
            type="button"
            className="sidebar-toggle-btn hidden sm:flex"
            onClick={() => setSidebarOpen((v) => !v)}
            title={sidebarOpen ? 'Collapse panel' : 'Expand panel'}
          >
            {sidebarOpen ? <PanelLeftClose size={16} /> : <PanelLeftOpen size={16} />}
          </button>
        </div>
      </header>

      {/* Body */}
      <div style={{display:'flex',flexDirection:'row',flex:'1 1 0%',minHeight:0,padding:'0.75rem',gap:0,overflow:'hidden'}}>
        {/* Sidebar */}
        {sidebarOpen && (
          <aside
            className="sidebar-aside flex h-full min-h-0 shrink-0 flex-col gap-2 overflow-y-auto overflow-x-hidden pr-1"
            style={{ width: isMobile ? '100%' : sidebarWidth }}
          >
            {/* ── Route Section ── */}
            <Section icon={<Route />} title="Route" accent="sky">
              <div className="grid grid-cols-2 gap-2">
                <div>
                  <label className="mb-1 block text-[10px] font-bold uppercase tracking-widest text-zinc-400">From</label>
                  <select value={fromNode} onChange={(e) => setFromNode(e.target.value)} className="input-dark route-from-select">
                    <option value="">Select...</option>
                    {nodeNames.map((name) => (
                      <option key={name} value={name}>{name}</option>
                    ))}
                  </select>
                </div>
                <div>
                  <label className="mb-1 block text-[10px] font-bold uppercase tracking-widest text-zinc-400">To</label>
                  <select value={toNode} onChange={(e) => setToNode(e.target.value)} className="input-dark">
                    <option value="">Select...</option>
                    {nodeNames.map((name) => (
                      <option key={name} value={name}>{name}</option>
                    ))}
                  </select>
                </div>
              </div>

              <div className="mt-2 text-[10px] font-bold uppercase tracking-widest text-zinc-500">
                Active Algorithm: <span className="text-emerald-400">{mode === 'city' ? 'A*' : 'Dijkstra'}</span>
              </div>

              <button
                type="button"
                onClick={handleFindPath}
                disabled={!fromNode || !toNode || busy}
                className="btn-accent mt-2.5 w-full"
              >
                Compute paths
              </button>

              {/* Route comparison */}
              {currentRoute && (
                <RouteCompare
                  currentRoute={currentRoute}
                  algo={mode === 'city' ? 'astar' : 'dijkstra'}
                />
              )}
              {currentRoute && !currentRoute.found && (
                <p className="mt-3 rounded-md border border-red-400/50 bg-red-950/70 px-3 py-2 text-xs font-semibold text-red-100">
                  No path found — try other places or check for closed roads.
                </p>
              )}
            </Section>

            {/* ── Simulation Section ── */}
            <Section icon={<Zap />} title="Simulation" accent="amber">
              <p className="mb-3 text-xs font-medium leading-snug text-zinc-400">
                {mode === 'city'
                  ? 'Each step: random traffic or road closure. Paths auto-update.'
                  : 'Each step: satellites move; links can appear or disappear.'}
              </p>
              <div className="flex gap-2">
                <button
                  type="button"
                  onClick={() => setAutoSim((v) => !v)}
                  className={`flex-1 rounded-lg border py-2 text-xs font-bold transition-colors ${
                    autoSim
                      ? 'border-emerald-500/50 bg-emerald-950/50 text-emerald-400 hover:bg-emerald-900/50 shadow-[0_0_15px_rgba(16,185,129,0.2)]'
                      : 'border-zinc-700 bg-zinc-800 text-zinc-300 hover:bg-zinc-700'
                  }`}
                >
                  {autoSim ? '🛑 Stop Auto-Sim' : '⏳ Start Auto-Sim'}
                </button>
                <button
                  type="button"
                  onClick={handleSimStep}
                  disabled={busy || autoSim}
                  title="Manual step is disabled while auto-sim is running"
                  className="flex-1 rounded-lg border border-amber-300/40 bg-gradient-to-b from-amber-500 to-amber-700 py-2 text-xs font-bold text-white shadow-lg shadow-amber-950/60 hover:brightness-110 disabled:opacity-50"
                >
                  ⚡ Step Once
                </button>
              </div>

              {/* Edit road */}
              <div className="mt-3 border-t border-white/8 pt-3">
                <p className="mb-2 text-[10px] font-bold uppercase tracking-widest text-zinc-500">Edit Road</p>
                <div className="grid grid-cols-2 gap-1.5">
                  <select value={editFrom} onChange={(e) => setEditFrom(e.target.value)} className="input-dark text-xs">
                    <option value="">From</option>
                    {nodeNames.map((name) => (
                      <option key={name} value={name}>{name}</option>
                    ))}
                  </select>
                  <select value={editTo} onChange={(e) => setEditTo(e.target.value)} className="input-dark text-xs">
                    <option value="">To</option>
                    {nodeNames.map((name) => (
                      <option key={name} value={name}>{name}</option>
                    ))}
                  </select>
                </div>
                <div className="mt-1.5 flex gap-1.5">
                  <input
                    type="number"
                    placeholder="Weight (km)"
                    value={editWeight}
                    onChange={(e) => setEditWeight(e.target.value)}
                    className="input-dark min-w-0 flex-1 text-xs"
                    min="0.1"
                    step="0.1"
                  />
                  <button
                    type="button"
                    onClick={handleUpdateRoad}
                    disabled={!editFrom || !editTo || !editWeight || busy}
                    className="shrink-0 rounded-lg border border-violet-300/40 bg-violet-600 px-3 py-1.5 text-xs font-bold text-white hover:bg-violet-500 disabled:opacity-45"
                  >
                    Set
                  </button>
                </div>
                <button
                  type="button"
                  onClick={handleCloseRoad}
                  disabled={!editFrom || !editTo || busy}
                  className="mt-1.5 w-full rounded-lg border border-red-400/45 bg-red-950/80 py-1.5 text-xs font-bold text-red-100 hover:bg-red-900/70 disabled:opacity-45"
                >
                  Remove road
                </button>
              </div>
            </Section>

            {/* ── Event Log Section ── */}
            <Section icon={<Shield />} title="Event Log" accent="emerald" defaultOpen={true}>
              <div className="min-h-[100px] max-h-[300px] overflow-hidden">
                <BlockchainPanel blocks={blocks} />
              </div>
            </Section>
          </aside>
        )}

        {/* Drag handle */}
        {sidebarOpen && (
          <div
            className="drag-handle hidden sm:block"
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
        )}

        {/* Map */}
        <main style={{display:'flex',flexDirection:'column',flex:'1 1 0%',minHeight:0,minWidth:0,height:'100%'}}>
          {mode === 'satellite'
            ? <SatelliteView graphData={graphData} pathData={currentRoute} />
            : <GraphMap
                graphData={graphData}
                pathData={currentRoute}
                mode={mode}
              />
          }
        </main>
      </div>

      {/* Toasts */}
      <ToastContainer toasts={toasts} onDismiss={dismissToast} />
    </div>
  );
}

export default App;
