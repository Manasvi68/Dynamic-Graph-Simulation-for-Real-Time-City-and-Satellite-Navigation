const jodhpurNodes = {
  "Ghanta Ghar":       [26.2920, 73.0169],
  // Sardar Market removed — merged into Ghanta Ghar (126m apart)
  // Sojati Gate removed — merged into Jodhpur Junction (149m apart)
  "Jalori Gate":       [26.2850, 73.0230],
  "Nai Sadak":         [26.2935, 73.0210],
  "Mehrangarh Fort":   [26.2984, 73.0183],
  // Jaswant Thada removed — merged into Mehrangarh Fort (424m apart)
  "Jodhpur Junction":  [26.2880, 73.0210],
  "Paota":             [26.2810, 73.0100],
  "Ratanada":          [26.2700, 73.0050],
  // High Court removed — merged into Ratanada (609m apart)
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
  "Masuria Hill":      [26.2940, 73.0280],
  "MBM Engineering":   [26.2730, 73.0220],
  "Rai Ka Bagh":       [26.2860, 73.0120],
  "Sursagar":          [26.2830, 73.0300],
  "Loco Shed":         [26.2950, 73.0340],
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
