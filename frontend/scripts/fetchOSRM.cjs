/**
 * OSRM Route Fetcher
 * Run once: node frontend/scripts/fetchOSRM.cjs
 * Fetches road-snapped polylines from the public OSRM demo server
 * and writes them to frontend/src/data/jodhpurRoutes.json
 * 
 * Preserves existing routes and only fetches missing ones.
 */

const fs = require('fs');
const path = require('path');
const https = require('https');

const nodes = {
  "Ghanta Ghar":       [26.2920, 73.0169],
  "Mehrangarh Fort":   [26.2984, 73.0183],
  "Jodhpur Junction":  [26.2880, 73.0210],
  "Paota":             [26.2810, 73.0100],
  "Ratanada":          [26.2700, 73.0050],
  "Basni":             [26.2550, 73.0300],
  "Mandore":           [26.3250, 73.0100],
  "Mahamandir":        [26.3050, 73.0130],
  "Pal Road":          [26.2450, 73.0050],
  "AIIMS Jodhpur":     [26.2520, 73.0450],
  "Chopasni Road":     [26.2680, 73.0280],
  "Banar Road":        [26.3100, 73.0400],
  "Pratap Nagar":      [26.2600, 73.0200],
  "Kaylana Lake":      [26.3000, 72.9800],
  "MBM Engineering":   [26.2730, 73.0220],
};

// All edges matching the 15-node graph in server.cpp initJodhpur()
const edges = [
  // Northern cluster
  ["Mandore", "Kaylana Lake"],
  ["Mandore", "Mahamandir"],
  ["Mandore", "Banar Road"],
  ["Mahamandir", "Banar Road"],
  ["Kaylana Lake", "Mahamandir"],
  ["Kaylana Lake", "Mehrangarh Fort"],

  // Fort & core city connections
  ["Mehrangarh Fort", "Mahamandir"],
  ["Mehrangarh Fort", "Ghanta Ghar"],
  ["Mehrangarh Fort", "Paota"],
  ["Ghanta Ghar", "Jodhpur Junction"],
  ["Ghanta Ghar", "Paota"],
  ["Ghanta Ghar", "Mahamandir"],
  ["Ghanta Ghar", "Banar Road"],
  ["Jodhpur Junction", "Paota"],
  ["Jodhpur Junction", "MBM Engineering"],
  ["Jodhpur Junction", "Chopasni Road"],
  ["Banar Road", "Jodhpur Junction"],
  ["Banar Road", "Mehrangarh Fort"],

  // Southern corridor
  ["Paota", "Ratanada"],
  ["Ratanada", "MBM Engineering"],
  ["Ratanada", "Pratap Nagar"],
  ["Pal Road", "Ratanada"],
  ["Pal Road", "Pratap Nagar"],
  ["Kaylana Lake", "Paota"],
  ["Mahamandir", "Paota"],

  // Eastern / AIIMS corridor
  ["MBM Engineering", "Chopasni Road"],
  ["MBM Engineering", "Basni"],
  ["Chopasni Road", "AIIMS Jodhpur"],
  ["Chopasni Road", "Basni"],
  ["Basni", "AIIMS Jodhpur"],
  ["Basni", "Pal Road"],
  ["Pratap Nagar", "Basni"],
  ["Pratap Nagar", "MBM Engineering"],
  ["Pratap Nagar", "Chopasni Road"],
  ["AIIMS Jodhpur", "Banar Road"],
];

function fetchUrl(url) {
  return new Promise((resolve, reject) => {
    https.get(url, { headers: { 'User-Agent': 'DSA-Project-Route-Fetcher/1.0' } }, (res) => {
      let data = '';
      res.on('data', (chunk) => data += chunk);
      res.on('end', () => {
        try { resolve(JSON.parse(data)); }
        catch (e) { reject(new Error(`Parse error: ${data.slice(0, 200)}`)); }
      });
    }).on('error', reject);
  });
}

function decodePolyline(encoded) {
  const points = [];
  let index = 0, lat = 0, lng = 0;
  while (index < encoded.length) {
    let b, shift = 0, result = 0;
    do { b = encoded.charCodeAt(index++) - 63; result |= (b & 0x1f) << shift; shift += 5; } while (b >= 0x20);
    lat += (result & 1) ? ~(result >> 1) : (result >> 1);
    shift = 0; result = 0;
    do { b = encoded.charCodeAt(index++) - 63; result |= (b & 0x1f) << shift; shift += 5; } while (b >= 0x20);
    lng += (result & 1) ? ~(result >> 1) : (result >> 1);
    points.push([lat / 1e5, lng / 1e5]);
  }
  return points;
}

function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

async function main() {
  const outPath = path.join(__dirname, '..', 'src', 'data', 'jodhpurRoutes.json');
  
  // Load existing routes to preserve them
  let routes = {};
  if (fs.existsSync(outPath)) {
    try {
      routes = JSON.parse(fs.readFileSync(outPath, 'utf-8'));
      console.log(`Loaded ${Object.keys(routes).length} existing routes`);
    } catch (e) {
      console.log('Could not parse existing routes, starting fresh');
    }
  }

  let fetched = 0, skipped = 0, failed = 0;

  for (const [fromName, toName] of edges) {
    const key = `${fromName}-${toName}`;
    const reverseKey = `${toName}-${fromName}`;
    
    // Skip if we already have this route (forward or reverse)
    if (routes[key] || routes[reverseKey]) {
      skipped++;
      continue;
    }

    const from = nodes[fromName];
    const to = nodes[toName];
    if (!from || !to) { console.log(`Missing coords: ${fromName} or ${toName}`); failed++; continue; }

    const url = `https://router.project-osrm.org/route/v1/driving/${from[1]},${from[0]};${to[1]},${to[0]}?overview=full&geometries=polyline`;

    try {
      const data = await fetchUrl(url);
      if (data.code === 'Ok' && data.routes && data.routes[0]) {
        const polyline = decodePolyline(data.routes[0].geometry);
        routes[key] = polyline;
        console.log(`[${++fetched}] ${key}: ${polyline.length} points`);
      } else {
        routes[key] = [from, to];
        console.log(`[${++fetched}] ${key}: OSRM returned no route, using straight line`);
      }
    } catch (err) {
      routes[key] = [from, to];
      console.log(`[${++fetched}] ${key}: FAILED (${err.message}), using straight line`);
      failed++;
    }

    await sleep(300);
  }

  fs.writeFileSync(outPath, JSON.stringify(routes, null, 2));
  console.log(`\nDone! ${fetched} new routes fetched, ${skipped} already existed, ${failed} failures.`);
  console.log(`Total routes in file: ${Object.keys(routes).length}`);
  console.log(`Saved to ${outPath}`);
}

main().catch(console.error);
