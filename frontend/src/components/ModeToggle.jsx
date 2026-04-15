import React from 'react';
import { Building2, Satellite } from 'lucide-react';

/**
 * ModeToggle — Premium animated pill-style toggle for City / Satellite mode.
 *
 * Props:
 *   mode      — 'city' | 'satellite'
 *   onSwitch  — callback to toggle mode
 *   disabled  — whether toggle is disabled (e.g. while busy)
 */
function ModeToggle({ mode, onSwitch, disabled = false }) {
  const isCity = mode === 'city';

  return (
    <div className="mode-toggle-wrapper" title="Switch between City and Satellite mode">
      {/* Sliding background pill */}
      <div
        className={`mode-toggle-pill ${isCity ? 'mode-toggle-pill--city' : 'mode-toggle-pill--sat'}`}
      />

      {/* City option */}
      <button
        type="button"
        className={`mode-toggle-option ${isCity ? 'mode-toggle-option--active' : ''}`}
        onClick={!isCity && !disabled ? onSwitch : undefined}
        disabled={disabled}
        aria-label="Switch to city mode"
        aria-pressed={isCity}
      >
        <Building2 size={14} strokeWidth={2.2} />
        <span>City</span>
      </button>

      {/* Satellite option */}
      <button
        type="button"
        className={`mode-toggle-option ${!isCity ? 'mode-toggle-option--active' : ''}`}
        onClick={isCity && !disabled ? onSwitch : undefined}
        disabled={disabled}
        aria-label="Switch to satellite mode"
        aria-pressed={!isCity}
      >
        <Satellite size={14} strokeWidth={2.2} />
        <span>Satellite</span>
      </button>
    </div>
  );
}

export default ModeToggle;
