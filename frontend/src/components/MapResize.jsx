import { useEffect } from 'react';
import { useMap } from 'react-leaflet';

function MapResize() {
  const map = useMap();

  useEffect(() => {
    const handleResize = () => {
      setTimeout(() => map.invalidateSize(), 100);
    };
    window.addEventListener('resize', handleResize);
    // also invalidate on mount (fixes initial render issues)
    setTimeout(() => map.invalidateSize(), 200);
    return () => window.removeEventListener('resize', handleResize);
  }, [map]);

  return null; // this component renders nothing, it just runs the effect
}

export default MapResize;
