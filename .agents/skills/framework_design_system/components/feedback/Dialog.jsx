import React from 'react';

/** Modal card centred over a blurred scrim, with a close (×) button. */
export function Dialog({ open = true, onClose, children, style = {} }) {
  if (!open) return null;
  return (
    <div onClick={e => { if (e.target === e.currentTarget) onClose && onClose(); }}
      style={{ position: 'fixed', inset: 0, zIndex: 9998, display: 'flex', alignItems: 'center',
        justifyContent: 'center', background: 'var(--scrim)', backdropFilter: 'blur(var(--blur-backdrop))' }}>
      <div style={{ position: 'relative', borderRadius: 'var(--radius-xl)', background: 'var(--bg)',
        color: 'var(--text)', maxWidth: 500, width: '90%', maxHeight: '90vh', overflowY: 'auto',
        boxShadow: 'var(--shadow-dialog)', padding: '1.5rem', fontFamily: 'var(--font-sans)', ...style }}>
        <button onClick={() => onClose && onClose()} title="Close" style={{ position: 'absolute',
          top: '0.75rem', right: '0.75rem', background: 'none', border: 'none', cursor: 'pointer',
          padding: '0.25rem', minWidth: 36, minHeight: 36, color: 'var(--text-muted)',
          display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
          <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><line x1="18" y1="6" x2="6" y2="18" /><line x1="6" y1="6" x2="18" y2="18" /></svg>
        </button>
        {children}
      </div>
    </div>
  );
}
