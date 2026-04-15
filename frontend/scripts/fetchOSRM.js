import { createRequire } from 'module';

// ESM wrapper so the plan's script name remains `fetchOSRM.js`.
// The implementation lives in fetchOSRM.cjs for compatibility.
const require = createRequire(import.meta.url);
require('./fetchOSRM.cjs');
/**
 * OSRM Route Fetcher
 * Run once: node frontend/scripts/fetchOSRM.js
 * Fetches road-snapped polylines from the public OSRM demo server
 * and writes them to frontend/src/data/jodhpurRoutes.json
 */

const fs = require('fs');
const path = require('path');
const https = require('https');

const nodes = {
  "Ghanta Ghar":       [26.2920, 73.0169],
  "Sardar Market":     [26.2910, 73.0175],
  "Sojati Gate":       [26.2870, 73.0200],
  "Jalori Gate":       [26.2850, 73.0230],
  "Nai Sadak":         [26.2935, 73.0210],
  "Mehrangarh Fort":   [26.2984, 73.0183],
  "Jodhpur Junction":  [26.2880, 73.0210],
  "Paota":             [26.2810, 73.0100],
  "Ratanada":          [26.2700, 73.0050],
  "Shastri Nagar":     [26.2650, 73.0150],
  "Sardarpura":        [26.2800, 73.0250],
  "Basni":             [26.2550, 73.0300],
  "Mandore":           [26.3250, 73.0100],
  "Mahamandir":        [26.3050, 73.0130],
  "Kamla Nehru Nagar": [26.2750, 73.0350],
  "Pal Road":          [26.2450, 73.0050],
  "Residency Road":    [26.2820, 73.0170],
  "Circuit House":     [26.2760, 73.0100],
  "AIIMS Jodhpur":     [26.2520, 73.0450],
  "Chopasni Road":     [26.2680, 73.0280],
  "Banar Road":        [26.3100, 73.0400],
  "Pratap Nagar":      [26.2600, 73.0200],
  "Bhagat Ki Kothi":   [26.2780, 73.0080],
  "Baldev Nagar":      [26.2850, 73.0330],
  "Pal Village":       [26.2350, 73.0000],
  "Sangariya":         [26.3350, 73.0250],
  "Kaylana Lake":      [26.3000, 72.9800],
  "Umaid Bhawan":      [26.2770, 73.0310],
  "Jaswant Thada":     [26.2960, 73.0150],
  "Masuria Hill":      [26.2940, 73.0280],
  "MBM Engineering":   [26.2730, 73.0220],
  "High Court":        [26.2690, 73.0110],
  "Rai Ka Bagh":       [26.2860, 73.0120],
  "Sursagar":          [26.2830, 73.0300],
  "Loco Shed":         [26.2950, 73.0340],
};

const edges = [
  ["Ghanta Ghar", "Sardar Market"],
  ["Ghanta Ghar", "Nai Sadak"],
  ["Ghanta Ghar", "Mehrangarh Fort"],
  ["Ghanta Ghar", "Jaswant Thada"],
  ["Sardar Market", "Sojati Gate"],
  ["Sojati Gate", "Jalori Gate"],
  ["Sojati Gate", "Jodhpur Junction"],
  ["Jalori Gate", "Sardarpura"],
  ["Nai Sadak", "Masuria Hill"],
  ["Nai Sadak", "Baldev Nagar"],
  ["Mehrangarh Fort", "Jaswant Thada"],
  ["Mehrangarh Fort", "Mahamandir"],
  ["Jodhpur Junction", "Paota"],
  ["Jodhpur Junction", "Residency Road"],
  ["Paota", "Ratanada"],
  ["Paota", "Bhagat Ki Kothi"],
  ["Paota", "Rai Ka Bagh"],
  ["Ratanada", "Shastri Nagar"],
  ["Ratanada", "High Court"],
  ["Ratanada", "Circuit House"],
  ["Shastri Nagar", "Pratap Nagar"],
  ["Shastri Nagar", "MBM Engineering"],
  ["Sardarpura", "Kamla Nehru Nagar"],
  ["Sardarpura", "Sursagar"],
  ["Basni", "Pratap Nagar"],
  ["Basni", "AIIMS Jodhpur"],
  ["Mandore", "Mahamandir"],
  ["Mandore", "Sangariya"],
  ["Mandore", "Kaylana Lake"],
  ["Mahamandir", "Banar Road"],
  ["Mahamandir", "Loco Shed"],
  ["Kamla Nehru Nagar", "Chopasni Road"],
  ["Kamla Nehru Nagar", "AIIMS Jodhpur"],
  ["Pal Road", "Pal Village"],
  ["Pal Road", "Ratanada"],
  ["Residency Road", "Rai Ka Bagh"],
  ["Residency Road", "Circuit House"],
  ["Circuit House", "Bhagat Ki Kothi"],
  ["Circuit House", "High Court"],
  ["AIIMS Jodhpur", "Chopasni Road"],
  ["Chopasni Road", "Basni"],
  ["Chopasni Road", "MBM Engineering"],
  ["Banar Road", "Loco Shed"],
  ["Banar Road", "Sangariya"],
  ["Pratap Nagar", "Pal Road"],
  ["Bhagat Ki Kothi", "Rai Ka Bagh"],
  ["Baldev Nagar", "Sursagar"],
  ["Baldev Nagar", "Masuria Hill"],
  ["Kaylana Lake", "Jaswant Thada"],
  ["Umaid Bhawan", "MBM Engineering"],
  ["Umaid Bhawan", "Chopasni Road"],
  ["Masuria Hill", "Loco Shed"],
  ["MBM Engineering", "High Court"],
  ["High Court", "Pal Road"],
  ["Sursagar", "Kamla Nehru Nagar"],
  ["Loco Shed", "Sangariya"],
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
  const routes = {};
  let fetched = 0, failed = 0;

  for (const [fromName, toName] of edges) {
    const from = nodes[fromName];
    const to = nodes[toName];
    if (!from || !to) { console.log(`Missing coords: ${fromName} or ${toName}`); failed++; continue; }

    const key = `${fromName}-${toName}`;
    const url = `https://router.project-osrm.org/route/v1/driving/${from[1]},${from[0]};${to[1]},${to[0]}?overview=full&geometries=polyline`;

    try {
      const data = await fetchUrl(url);
      if (data.code === 'Ok' && data.routes && data.routes[0]) {
        const polyline = decodePolyline(data.routes[0].geometry);
        routes[key] = polyline;
        console.log(`[${++fetched}/${edges.length}] ${key}: ${polyline.length} points`);
      } else {
        routes[key] = [from, to];
        console.log(`[${++fetched}/${edges.length}] ${key}: OSRM returned no route, using straight line`);
      }
    } catch (err) {
      routes[key] = [from, to];
      console.log(`[${++fetched}/${edges.length}] ${key}: FAILED (${err.message}), using straight line`);
      failed++;
    }

    await sleep(250);
  }

  const outPath = path.join(__dirname, '..', 'src', 'data', 'jodhpurRoutes.json');
  fs.writeFileSync(outPath, JSON.stringify(routes, null, 2));
  console.log(`\nDone! ${fetched} edges processed (${failed} failures). Saved to ${outPath}`);
}

main().catch(console.error);
