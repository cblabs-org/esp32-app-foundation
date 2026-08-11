import React from 'react';

/** Full-screen dark spinner overlay shown during device restarts. Position:fixed. */
export function RebootOverlay({ visible = true, message = 'Restarting device…', sub = 'The device will be back online in a few seconds.', style = {} }) {
  return (
    <div style={{ position: 'fixed', inset: 0, zIndex: 10000, background: 'var(--scrim-strong)',
      backdropFilter: 'blur(var(--blur-login))', WebkitBackdropFilter: 'blur(var(--blur-login))',
      display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center',
      color: '#fff', gap: '1.25rem', fontFamily: 'var(--font-sans)',
      opacity: visible ? 1 : 0, visibility: visible ? 'visible' : 'hidden',
      transition: 'opacity var(--transition-slow), visibility var(--transition-slow)', ...style }}>
      <span style={{ width: 44, height: 44, borderRadius: '50%', border: '3px solid rgba(255,255,255,0.25)',
        borderTopColor: '#fff', animation: 'af-spin 0.8s linear infinite' }} />
      <div style={{ fontSize: '1.15rem', fontWeight: 'var(--weight-semibold)', textAlign: 'center' }}>{message}</div>
      <div style={{ fontSize: 'var(--text-body)', opacity: 0.7, textAlign: 'center', maxWidth: 320, lineHeight: 'var(--leading-body)' }}>{sub}</div>
    </div>
  );
}
