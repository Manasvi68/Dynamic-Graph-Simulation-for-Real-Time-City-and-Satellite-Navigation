import React, { useState } from 'react';

const CONDITIONS = [
  { key: 'normal',        label: 'Normal',       color: '#3b82f6', dash: false, icon: null },
  { key: 'light_traffic', label: 'Light Traffic', color: '#eab308', dash: false, icon: null },
  { key: 'heavy_traffic', label: 'Heavy Traffic', color: '#f97316', dash: false, icon: null },
  { key: 'congestion',    label: 'Congestion',    color: '#dc2626', dash: false, icon: null },
  { key: 'accident',      label: 'Accident',      color: '#ef4444', dash: false, icon: '\u26A0' },
  { key: 'construction',  label: 'Construction',  color: '#d97706', dash: true,  icon: '\uD83D\uDD36' },
  { key: 'closed',        label: 'Closed',        color: '#71717a', dash: true,  icon: '\uD83D\uDEA7' },
];

function MapLegend() {
  const [open, setOpen] = useState(true);

  return (
    <div className="map-legend">
      <button
        type="button"
        className="legend-toggle"
        onClick={() => setOpen(!open)}
      >
        {open ? 'Legend \u25BC' : 'Legend \u25B2'}
      </button>
      {open && (
        <div className="legend-body">
          {CONDITIONS.map((c) => (
            <div key={c.key} className="legend-row">
              <svg width="28" height="10" viewBox="0 0 28 10">
                <line
                  x1="0" y1="5" x2="28" y2="5"
                  stroke={c.color}
                  strokeWidth={3}
                  strokeDasharray={c.dash ? '6,4' : 'none'}
                />
              </svg>
              <span className="legend-label">
                {c.icon && <span className="legend-icon">{c.icon}</span>}
                {c.label}
              </span>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}

export default MapLegend;
