import { useEffect, useRef } from 'react';
import { useMap } from 'react-leaflet';

function MapResize() {
  const map = useMap();
  const containerRef = useRef(map.getContainer());

  useEffect(() => {
    const el = containerRef.current;
    if (!el) return;

    let debounceTimer;
    const runInvalidate = () => {
      // pan: false keeps the view fixed when the panel resizes (e.g. after graph / path updates)
      map.invalidateSize({ animate: false, pan: false });
    };

    const ro = new ResizeObserver(() => {
      clearTimeout(debounceTimer);
      debounceTimer = setTimeout(runInvalidate, 80);
    });
    ro.observe(el);

    const initialTimer = setTimeout(runInvalidate, 200);

    return () => {
      ro.disconnect();
      clearTimeout(debounceTimer);
      clearTimeout(initialTimer);
    };
  }, [map]);

  return null;
}

export default MapResize;
