import React from 'react';

/** RAG-coloured progress row: label, value/max caption, and a filled track. */
export function ProgressBar({ label, value = 0, max = 100, valueText, color, style = {} }) {
  const pct = max > 0 ? Math.round((value / max) * 100) : 0;
  const rag = pct >= 50 ? 'var(--success)' : pct >= 25 ? 'var(--warning)' : 'var(--error)';
  const fill = color || rag;
  return (
    <div style={{ marginBottom: '0.75rem', fontFamily: 'var(--font-sans)', ...style }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: 'var(--text-base-sm)', marginBottom: 4, color: 'var(--text)' }}>
        <span>{label}</span>
        <span style={{ color: 'var(--text-muted)' }}>{valueText != null ? valueText : pct + '%'}</span>
      </div>
      <div style={{ background: 'var(--bg)', borderRadius: 'var(--radius-sm)', height: 10, overflow: 'hidden', border: '1px solid var(--border)' }}>
        <div style={{ width: pct + '%', height: '100%', background: fill, borderRadius: 'var(--radius-sm)', transition: 'var(--transition-progress)' }} />
      </div>
    </div>
  );
}
