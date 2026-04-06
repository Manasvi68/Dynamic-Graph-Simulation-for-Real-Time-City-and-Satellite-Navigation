// lat/lng positions for city nodes on the Leaflet map
// these are approximate locations in New Delhi for realism

const cityCoords = {
  "Connaught Place": [28.6315, 77.2167],
  "Chandni Chowk":   [28.6506, 77.2334],
  "Karol Bagh":      [28.6519, 77.1909],
  "Saket":           [28.5244, 77.2066],
  "Dwarka":          [28.5921, 77.0460],
  "Nehru Place":     [28.5491, 77.2533],
  "Lajpat Nagar":    [28.5700, 77.2400],
  "Hauz Khas":       [28.5494, 77.2001],
  "Rohini":          [28.7495, 77.0565],
  "Janakpuri":       [28.6292, 77.0826],
};

// fallback: grid layout for unknown nodes
export function getNodePosition(name, index) {
  if (cityCoords[name]) {
    return cityCoords[name];
  }
  // arrange in a grid if we don't know the position
  const row = Math.floor(index / 4);
  const col = index % 4;
  return [28.6 + row * 0.05, 77.1 + col * 0.05];
}

export default cityCoords;
