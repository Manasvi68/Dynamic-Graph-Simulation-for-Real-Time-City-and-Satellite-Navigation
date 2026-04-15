import { useState, useCallback, useRef } from 'react';

let nextId = 0;

export default function useToast(autoDismiss = 4000) {
  const [toasts, setToasts] = useState([]);
  const timersRef = useRef({});

  const addToast = useCallback((message, type = 'info') => {
    const id = ++nextId;
    setToasts((prev) => [...prev.slice(-4), { id, message, type, exiting: false }]);

    timersRef.current[id] = setTimeout(() => {
      setToasts((prev) => prev.map((t) => (t.id === id ? { ...t, exiting: true } : t)));
      setTimeout(() => {
        setToasts((prev) => prev.filter((t) => t.id !== id));
        delete timersRef.current[id];
      }, 300);
    }, autoDismiss);

    return id;
  }, [autoDismiss]);

  const dismissToast = useCallback((id) => {
    if (timersRef.current[id]) clearTimeout(timersRef.current[id]);
    setToasts((prev) => prev.filter((t) => t.id !== id));
  }, []);

  return { toasts, addToast, dismissToast };
}
