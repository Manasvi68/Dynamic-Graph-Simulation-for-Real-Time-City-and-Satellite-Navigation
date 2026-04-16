const jodhpurNodes = {
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


export function getNodePosition(name, index) {
  if (jodhpurNodes[name]) {
    return jodhpurNodes[name];
  }
  const row = Math.floor(index / 6);
  const col = index % 6;
  return [26.28 + row * 0.01, 73.01 + col * 0.01];
}

export default jodhpurNodes;
