import React, { useState, useEffect } from 'react';
import { Map, Route, Zap, Link2, Shield, Satellite } from 'lucide-react';
import GraphMap from './components/GraphMap';
import BlockchainPanel from './components/BlockchainPanel';
import * as api from './api';

function App() {
  // ── state ──
  const [graphData, setGraphData] = useState(null);
  const [pathData, setPathData] = useState(null);
  const [blocks, setBlocks] = useState([]);
  const [mode, setMode] = useState('city');
  const [loading, setLoading] = useState(true);

  // pathfinding form state
  const [fromNode, setFromNode] = useState('');
  const [toNode, setToNode] = useState('');
  const [algo, setAlgo] = useState('dijkstra');

  // road edit form state
  const [editFrom, setEditFrom] = useState('');
  const [editTo, setEditTo] = useState('');
  const [editWeight, setEditWeight] = useState('');

  // ── load initial data ──
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
        setMode(modeRes.mode || 'city');
      } catch (err) {
        console.error('Failed to load data:', err);
      }
      setLoading(false);
    }
    loadAll();
  }, []);

  // ── refresh data helper ──
  async function refreshData() {
    const [graphRes, chainRes] = await Promise.all([
      api.getGraph(),
      api.getBlockchain(),
    ]);
    setGraphData(graphRes);
    setBlocks(chainRes);
  }

  // placeholder for event handlers (Day 13 will fill these in)
    const handleFindPath = async () => {
    if (!fromNode || !toNode) return;
    try {
      const result = await api.findPath(fromNode, toNode, algo);
      setPathData(result);
    } catch (err) {
      console.error('Path error:', err);
    }
  };
  
  const handleSimStep = async () => {
    try {
      await api.simStep();
      await refreshData();
      setPathData(null); // clear old path since graph changed
    } catch (err) {
      console.error('Sim step error:', err);
    }
  };

  const handleUpdateRoad = async () => {
    if (!editFrom || !editTo || !editWeight) return;
    try {
      await api.updateRoad(editFrom, editTo, parseFloat(editWeight));
      await refreshData();
      setPathData(null);
    } catch (err) {
      console.error('Update road error:', err);
    }
  };

  const handleCloseRoad = async () => {
    if (!editFrom || !editTo) return;
    try {
      await api.closeRoad(editFrom, editTo);
      await refreshData();
      setPathData(null);
    } catch (err) {
      console.error('Close road error:', err);
    }
  };
  
  const handleModeSwitch = async () => {
    try {
      const newMode = mode === 'city' ? 'satellite' : 'city';
      await api.setMode(newMode);
      setMode(newMode);
      setPathData(null);
      // reload graph for the new mode
      const graphRes = await api.getGraph();
      setGraphData(graphRes);
      const chainRes = await api.getBlockchain();
      setBlocks(chainRes);
    } catch (err) {
      console.error('Mode switch error:', err);
    }
  };

  if (loading) {
    return (
      <div className="min-h-screen bg-[#0f0f1a] flex items-center justify-center text-gray-400">
        Loading...
      </div>
    );
  }

  // get node names for dropdowns
  const nodeNames = graphData?.nodes?.map(n => n.name) || [];

  return (
    <div className="min-h-screen bg-[#0f0f1a] text-gray-100 flex flex-col">
      {/* header */}
      <header className="glass-panel px-6 py-3 flex items-center justify-between">
        <div className="flex items-center gap-3">
          <Map size={22} className="text-cyan-400" />
          <h1 className="text-lg font-semibold tracking-tight">
            Dynamic Graph Simulation
          </h1>
        </div>
        <div className="flex items-center gap-2 text-sm">
          <span className={mode === 'city' ? 'text-cyan-400' : 'text-gray-500'}>City</span>
          <button
            onClick={handleModeSwitch}
            className="w-12 h-6 rounded-full bg-gray-700 relative transition-colors"
          >
            <div className={`w-5 h-5 rounded-full bg-cyan-400 absolute top-0.5 transition-all ${
              mode === 'satellite' ? 'left-6' : 'left-0.5'
            }`} />
          </button>
          <span className={mode === 'satellite' ? 'text-cyan-400' : 'text-gray-500'}>Satellite</span>
        </div>
      </header>

      {/* main content */}
      <div className="flex-1 flex overflow-hidden">
        {/* sidebar */}
        <aside className="w-80 glass-panel m-2 rounded-lg p-4 overflow-y-auto space-y-6">
          {/* pathfinding section — Day 13 will add the form here */}
          <section>
            <h2 className="flex items-center gap-2 text-sm font-semibold text-gray-300 mb-3">
              <Route size={16} className="text-cyan-400" />
              Find Path
            </h2>
            <p className="text-xs text-gray-500">Controls coming Day 13...</p>
          </section>

                    {/* pathfinding section */}
          <section>
            <h2 className="flex items-center gap-2 text-sm font-semibold text-gray-300 mb-3">
              <Route size={16} className="text-cyan-400" />
              Find Path
            </h2>
            <div className="space-y-2">
              <select
                value={fromNode}
                onChange={(e) => setFromNode(e.target.value)}
                className="w-full bg-gray-800 border border-gray-700 rounded px-2 py-1.5 text-sm"
              >
                <option value="">From...</option>
                {nodeNames.map(name => (
                  <option key={name} value={name}>{name}</option>
                ))}
              </select>

              <select
                value={toNode}
                onChange={(e) => setToNode(e.target.value)}
                className="w-full bg-gray-800 border border-gray-700 rounded px-2 py-1.5 text-sm"
              >
                <option value="">To...</option>
                {nodeNames.map(name => (
                  <option key={name} value={name}>{name}</option>
                ))}
              </select>

              <div className="flex gap-2">
                <button
                  onClick={() => setAlgo('dijkstra')}
                  className={`flex-1 py-1 rounded text-xs font-medium transition-colors ${
                    algo === 'dijkstra'
                      ? 'bg-cyan-600 text-white'
                      : 'bg-gray-800 text-gray-400 hover:bg-gray-700'
                  }`}
                >
                  Dijkstra
                </button>
                <button
                  onClick={() => setAlgo('astar')}
                  className={`flex-1 py-1 rounded text-xs font-medium transition-colors ${
                    algo === 'astar'
                      ? 'bg-cyan-600 text-white'
                      : 'bg-gray-800 text-gray-400 hover:bg-gray-700'
                  }`}
                >
                  A*
                </button>
              </div>

              <button
                onClick={handleFindPath}
                className="w-full py-2 bg-cyan-600 hover:bg-cyan-500 rounded text-sm font-medium transition-colors"
              >
                Find Shortest Path
              </button>

              {/* path result display */}
              {pathData && pathData.found && (
                <div className="glass-panel rounded p-2 mt-2 text-xs space-y-1">
                  <div className="text-cyan-300 font-medium">
                    Cost: {pathData.totalCost.toFixed(2)} | Explored: {pathData.nodesExplored} nodes
                  </div>
                  <div className="text-gray-400">
                    {pathData.path.map(s => s.name).join(' → ')}
                  </div>
                </div>
              )}
              {pathData && !pathData.found && (
                <div className="text-red-400 text-xs mt-1">No path found!</div>
              )}
            </div>
          </section>

          {/* blockchain section */}
          <section>
            <h2 className="flex items-center gap-2 text-sm font-semibold text-gray-300 mb-3">
              <Shield size={16} className="text-emerald-400" />
              Blockchain Log
            </h2>
            <BlockchainPanel blocks={blocks} />
          </section>
        </aside>

                  {/* simulation section */}
          <section>
            <h2 className="flex items-center gap-2 text-sm font-semibold text-gray-300 mb-3">
              <Zap size={16} className="text-amber-400" />
              Simulation
            </h2>
            <div className="space-y-3">
              <button
                onClick={handleSimStep}
                className="w-full py-2 bg-amber-600 hover:bg-amber-500 rounded text-sm font-medium transition-colors"
              >
                Run Simulation Step
              </button>

              {/* road update form */}
              <div className="space-y-1">
                <label className="text-xs text-gray-500">Update Road Weight</label>
                <div className="flex gap-1">
                  <select
                    value={editFrom}
                    onChange={(e) => setEditFrom(e.target.value)}
                    className="flex-1 bg-gray-800 border border-gray-700 rounded px-1.5 py-1 text-xs"
                  >
                    <option value="">From</option>
                    {nodeNames.map(name => (
                      <option key={name} value={name}>{name}</option>
                    ))}
                  </select>
                  <select
                    value={editTo}
                    onChange={(e) => setEditTo(e.target.value)}
                    className="flex-1 bg-gray-800 border border-gray-700 rounded px-1.5 py-1 text-xs"
                  >
                    <option value="">To</option>
                    {nodeNames.map(name => (
                      <option key={name} value={name}>{name}</option>
                    ))}
                  </select>
                </div>
                <div className="flex gap-1">
                  <input
                    type="number"
                    placeholder="Weight"
                    value={editWeight}
                    onChange={(e) => setEditWeight(e.target.value)}
                    className="flex-1 bg-gray-800 border border-gray-700 rounded px-2 py-1 text-xs"
                    min="0.1"
                    step="0.1"
                  />
                  <button
                    onClick={handleUpdateRoad}
                    className="px-3 py-1 bg-indigo-600 hover:bg-indigo-500 rounded text-xs font-medium transition-colors"
                  >
                    Update
                  </button>
                </div>
              </div>

              {/* road close */}
              <button
                onClick={handleCloseRoad}
                disabled={!editFrom || !editTo}
                className="w-full py-1.5 bg-red-700 hover:bg-red-600 disabled:bg-gray-700 disabled:text-gray-500 rounded text-xs font-medium transition-colors"
              >
                Close Road ({editFrom || '?'} → {editTo || '?'})
              </button>
            </div>
          </section>
        
        {/* map area */}
        <main className="flex-1 m-2 ml-0 rounded-lg overflow-hidden">
          <GraphMap graphData={graphData} pathData={pathData} mode={mode} />
        </main>
      </div>
    </div>
  );
}

export default App;
