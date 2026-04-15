import React, { useState, useEffect, useRef, useCallback } from 'react';
import { Play, Pause, SkipForward, RotateCcw } from 'lucide-react';

const EMPTY_EXPLORATION = [];

function AlgoVisualizer({ pathData, onHighlight }) {
  const [step, setStep] = useState(-1);
  const [playing, setPlaying] = useState(false);
  const [speed, setSpeed] = useState(300);
  const timerRef = useRef(null);

  const exploration = pathData?.explorationOrder || EMPTY_EXPLORATION;
  const maxStep = exploration.length - 1;

  const updateHighlight = useCallback((s) => {
    if (!onHighlight) return;
    if (s < 0) {
      onHighlight({ explored: new Set(), current: null });
      return;
    }
    const explored = new Set();
    for (let i = 0; i <= s && i < exploration.length; i++) {
      explored.add(exploration[i].id);
    }
    onHighlight({ explored, current: exploration[s]?.id ?? null });
  }, [exploration, onHighlight]);

  useEffect(() => {
    if (!pathData?.found) return;
    updateHighlight(step);
  }, [step, updateHighlight, pathData]);

  useEffect(() => {
    if (!playing) {
      if (timerRef.current) clearInterval(timerRef.current);
      return;
    }
    timerRef.current = setInterval(() => {
      setStep((s) => {
        if (s >= maxStep) {
          setPlaying(false);
          return s;
        }
        return s + 1;
      });
    }, speed);
    return () => clearInterval(timerRef.current);
  }, [playing, speed, maxStep]);

  useEffect(() => {
    return () => {
      if (onHighlight) onHighlight(null);
    };
  }, [onHighlight]);

  if (!pathData?.found || exploration.length === 0) return null;

  const reset = () => { setStep(-1); setPlaying(false); };
  const stepFwd = () => { if (step < maxStep) setStep(step + 1); };

  return (
    <div className="panel-dark-tight mt-3 p-3">
      <p className="mb-2 text-sm font-bold uppercase tracking-wide text-zinc-400">Algorithm visualization</p>
      <div className="flex items-center gap-2">
        <button type="button" onClick={() => setPlaying(!playing)} className="algo-btn" title={playing ? 'Pause' : 'Play'}>
          {playing ? <Pause size={14} /> : <Play size={14} />}
        </button>
        <button type="button" onClick={stepFwd} disabled={step >= maxStep} className="algo-btn" title="Step forward">
          <SkipForward size={14} />
        </button>
        <button type="button" onClick={reset} className="algo-btn" title="Reset">
          <RotateCcw size={14} />
        </button>
        <input
          type="range"
          min={50}
          max={800}
          value={800 - speed + 50}
          onChange={(e) => setSpeed(800 - Number(e.target.value) + 50)}
          className="ml-2 h-1.5 flex-1 cursor-pointer accent-sky-500"
          title="Speed"
        />
        <span className="font-mono text-sm text-zinc-400 w-14 text-right">
          {step < 0 ? '---' : `${step + 1}/${exploration.length}`}
        </span>
      </div>
      {step >= 0 && (
        <div className="mt-2 text-sm text-zinc-300">
          Exploring: <span className="font-bold text-emerald-300">{exploration[step]?.name}</span>
          {' '}(dist: <span className="font-mono text-cyan-300">{exploration[step]?.dist?.toFixed(2)}</span>)
        </div>
      )}
      <div className="mt-1 text-xs text-zinc-500">
        Green = explored &middot; Yellow = current &middot; Grey = unexplored
      </div>
    </div>
  );
}

export default AlgoVisualizer;
