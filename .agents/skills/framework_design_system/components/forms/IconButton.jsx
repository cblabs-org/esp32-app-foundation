import React from 'react';

/** 34x34 bordered square button holding a single 18px icon. Hover = primary wash. */
export function IconButton({ label, children, style = {}, ...rest }) {
  return (
    <button type="button" title={label} aria-label={label} style={{
      background: 'none', border: '1px solid var(--border)', borderRadius: 'var(--radius-sm)',
      width: 34, height: 34, padding: 0, cursor: 'pointer', color: 'var(--text)',
      transition: 'background var(--transition-fast)', display: 'inline-flex',
      alignItems: 'center', justifyContent: 'center', ...style }}
      onMouseEnter={e => e.currentTarget.style.background = 'var(--primary-alpha)'}
      onMouseLeave={e => e.currentTarget.style.background = 'none'} {...rest}>
      {children}
    </button>
  );
}
