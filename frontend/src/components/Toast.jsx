import React from 'react';
import { X } from 'lucide-react';

const ICONS = {
  info: '\u2139\uFE0F',
  warning: '\u26A0\uFE0F',
  error: '\uD83D\uDEA8',
  success: '\u2705',
};

function ToastContainer({ toasts, onDismiss }) {
  if (!toasts.length) return null;

  return (
    <div className="toast-container">
      {toasts.map((t) => (
        <div
          key={t.id}
          className={`toast-item toast-${t.type} ${t.exiting ? 'toast-exit' : 'toast-enter'}`}
        >
          <span className="toast-icon">{ICONS[t.type] || ICONS.info}</span>
          <span className="toast-msg">{t.message}</span>
          <button type="button" className="toast-close" onClick={() => onDismiss(t.id)}>
            <X size={12} />
          </button>
        </div>
      ))}
    </div>
  );
}

export default ToastContainer;
