import React from 'react';

const ICONS = {
  auto: <><rect x="2" y="3" width="20" height="14" rx="2" ry="2" /><line x1="8" y1="21" x2="16" y2="21" /><line x1="12" y1="17" x2="12" y2="21" /></>,
  light: <><circle cx="12" cy="12" r="5" /><line x1="12" y1="1" x2="12" y2="3" /><line x1="12" y1="21" x2="12" y2="23" /><line x1="4.22" y1="4.22" x2="5.64" y2="5.64" /><line x1="18.36" y1="18.36" x2="19.78" y2="19.78" /><line x1="1" y1="12" x2="3" y2="12" /><line x1="21" y1="12" x2="23" y2="12" /><line x1="4.22" y1="19.78" x2="5.64" y2="18.36" /><line x1="18.36" y1="5.64" x2="19.78" y2="4.22" /></>,
  dark: <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z" />,
};

/** Segmented auto/light/dark theme picker. */
export function ThemeToggle({ value = 'auto', onChange, style = {} }) {
  return (
    <div style={{ display: 'flex', background: 'var(--card-bg)', border: '1px solid var(--border)',
      borderRadius: 'var(--radius-round)', padding: 4, gap: 4, ...style }}>
      {['auto', 'light', 'dark'].map(m => {
        const on = m === value;
        return (
          <button key={m} title={m} onClick={() => onChange && onChange(m)} style={{
            width: 32, height: 32, borderRadius: '50%', border: 'none', cursor: 'pointer',
            display: 'flex', alignItems: 'center', justifyContent: 'center',
            background: on ? 'var(--bg)' : 'transparent', color: on ? 'var(--primary)' : 'var(--text-muted)',
            boxShadow: on ? 'var(--shadow-toggle-active)' : 'none', transition: 'all var(--transition-base)' }}>
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">{ICONS[m]}</svg>
          </button>
        );
      })}
    </div>
  );
}
