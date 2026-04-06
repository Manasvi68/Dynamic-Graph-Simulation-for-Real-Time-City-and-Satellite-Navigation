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
  const handleFindPath = async () => { /* Day 13 */ };
  const handleSimStep = async () => { /* Day 13 */ };
  const handleUpdateRoad = async () => { /* Day 13 */ };
  const handleCloseRoad = async () => { /* Day 13 */ };
  const handleModeSwitch = async () => { /* Day 13 */ };

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

          {/* simulation section — Day 13 */}
          <section>
            <h2 className="flex items-center gap-2 text-sm font-semibold text-gray-300 mb-3">
              <Zap size={16} className="text-amber-400" />
              Simulation
            </h2>
            <p className="text-xs text-gray-500">Controls coming Day 13...</p>
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

        {/* map area */}
        <main className="flex-1 m-2 ml-0 rounded-lg overflow-hidden">
          <GraphMap graphData={graphData} pathData={pathData} mode={mode} />
        </main>
      </div>
    </div>
  );
}

export default App;
