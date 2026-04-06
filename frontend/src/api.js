const BASE = '';  // empty string = same origin (works with proxy in dev, and direct in production)

// fetch the current graph (nodes + edges)
export async function getGraph() {
  const res = await fetch(`${BASE}/api/graph`);
  return res.json();
}

// find shortest path between two nodes
export async function findPath(from, to, algo = 'dijkstra') {
  const params = new URLSearchParams({ from, to, algo });
  const res = await fetch(`${BASE}/api/path?${params}`);
  return res.json();
}

// get the entire blockchain
export async function getBlockchain() {
  const res = await fetch(`${BASE}/api/blockchain`);
  return res.json();
}

// run one simulation step
export async function simStep() {
  const res = await fetch(`${BASE}/api/sim/step`, { method: 'POST' });
  return res.json();
}

// update a road's weight
export async function updateRoad(from, to, weight) {
  const res = await fetch(`${BASE}/api/road/update`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ from, to, weight }),
  });
  return res.json();
}

// close (remove) a road
export async function closeRoad(from, to) {
  const res = await fetch(`${BASE}/api/road/close`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ from, to }),
  });
  return res.json();
}

// get current mode
export async function getMode() {
  const res = await fetch(`${BASE}/api/mode`);
  return res.json();
}

// switch mode (city / satellite)
export async function setMode(mode) {
  const res = await fetch(`${BASE}/api/mode`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ mode }),
  });
  return res.json();
}