import { useEffect, useRef } from 'react';
import { useMap } from 'react-leaflet';

function MapResize() {
  const map = useMap();
  const containerRef = useRef(map.getContainer());

  useEffect(() => {
    const el = containerRef.current;
    if (!el) return;

    const ro = new ResizeObserver(() => {
      map.invalidateSize({ animate: false });
    });
    ro.observe(el);

    setTimeout(() => map.invalidateSize(), 200);

    return () => ro.disconnect();
  }, [map]);

  return null;
}

export default MapResize;
