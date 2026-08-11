import React from 'react';

/** 401 auth gate: a centred card (lock icon, title, password field, submit). */
export function LoginCard({ title = 'Authentication Required', subtitle = 'Enter the portal password to continue.', error, onSubmit, style = {} }) {
  const [pw, setPw] = React.useState('');
  return (
    <div style={{ background: 'var(--card-bg)', border: '1px solid var(--border)', borderRadius: 'var(--radius-2xl)',
      padding: 32, width: 340, maxWidth: '90vw', textAlign: 'center', boxShadow: 'var(--shadow-login)',
      fontFamily: 'var(--font-sans)', color: 'var(--text)', ...style }}>
      <div style={{ marginBottom: 16, color: 'var(--primary)', display: 'flex', justifyContent: 'center' }}>
        <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2" /><path d="M7 11V7a5 5 0 0 1 10 0v4" /></svg>
      </div>
      <h2 style={{ margin: '0 0 4px', fontSize: '1.4em' }}>{title}</h2>
      <p style={{ color: 'var(--text-muted)', margin: '0 0 20px', fontSize: '0.9em' }}>{subtitle}</p>
      {error && <div style={{ padding: '0.75rem 1rem', borderRadius: 'var(--radius-md)', border: '1px solid var(--error)',
        background: 'color-mix(in srgb, var(--error) 10%, transparent)', color: 'var(--error)', fontSize: 'var(--text-body)', marginBottom: '0.75rem' }}>{error}</div>}
      <form onSubmit={e => { e.preventDefault(); onSubmit && onSubmit(pw); }}>
        <div style={{ marginBottom: 12 }}>
          <input type="password" value={pw} onChange={e => setPw(e.target.value)} placeholder="Password"
            style={{ width: '100%', padding: '10px 14px', border: '1px solid var(--border)', borderRadius: 'var(--radius-md)',
              background: 'var(--bg)', color: 'var(--text)', fontSize: '1em', boxSizing: 'border-box' }}
            onFocus={e => { e.target.style.borderColor = 'var(--primary)'; e.target.style.boxShadow = '0 0 0 3px var(--primary-alpha)'; }}
            onBlur={e => { e.target.style.borderColor = 'var(--border)'; e.target.style.boxShadow = 'none'; }} />
        </div>
        <button type="submit" style={{ width: '100%', padding: '0.8rem 1.25rem', border: 'none',
          borderRadius: 'var(--radius-lg)', background: 'var(--primary)', color: '#fff',
          fontWeight: 'var(--weight-bold)', fontSize: 'var(--text-md)', cursor: 'pointer' }}>Unlock</button>
      </form>
    </div>
  );
}
