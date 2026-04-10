async function loadGraph() {
  try {
    const res = await fetch('/api/graph');
    const data = await res.json();
    document.getElementById('graph-output').textContent = JSON.stringify(data, null, 2);
  } catch (err) {
    document.getElementById('graph-output').textContent = 'Error: ' + err.message;
  }
}

async function findPath() {
  const from = document.getElementById('path-from').value;
  const to = document.getElementById('path-to').value;
  const algo = document.getElementById('path-algo').value;

  if (!from || !to) {
    document.getElementById('path-output').textContent = 'Enter both From and To.';
    return;
  }

  try {
    const params = new URLSearchParams({ from, to, algo });
    const res = await fetch('/api/path?' + params);
    const data = await res.json();

    if (data.found) {
      const pathStr = data.path.map(s => s.name).join(' → ');
      document.getElementById('path-output').textContent =
        `Cost: ${data.totalCost.toFixed(2)}\nExplored: ${data.nodesExplored} nodes\nPath: ${pathStr}`;
    } else {
      document.getElementById('path-output').textContent = 'No path found!';
    }
  } catch (err) {
    document.getElementById('path-output').textContent = 'Error: ' + err.message;
  }
}

async function simStep() {
  try {
    await fetch('/api/sim/step', { method: 'POST' });
    loadGraph();
    loadChain();
  } catch (err) {
    console.error(err);
  }
}

async function loadChain() {
  try {
    const res = await fetch('/api/blockchain');
    const data = await res.json();
    document.getElementById('chain-output').textContent = JSON.stringify(data, null, 2);
  } catch (err) {
    document.getElementById('chain-output').textContent = 'Error: ' + err.message;
  }
}

// load data on page open
loadGraph();
loadChain();