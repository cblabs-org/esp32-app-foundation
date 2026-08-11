import React from 'react';

/** Native select styled to match the portal inputs. Pass {value,label} options or children. */
export function Select({ label, options, style = {}, id, children, ...rest }) {
  const chevron = "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='8' viewBox='0 0 12 8' fill='none' stroke='%236c757d' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3E%3Cpolyline points='1 1.5 6 6.5 11 1.5'/%3E%3C/svg%3E";
  const sel = (
    <select id={id} style={{
      width: '100%', boxSizing: 'border-box', padding: '0.8rem 2.5rem 0.8rem 1rem',
      borderRadius: 'var(--radius-lg)', border: '1px solid var(--border)',
      background: 'var(--bg)', color: 'var(--text)', fontSize: 'var(--text-md)',
      fontFamily: 'var(--font-sans)', marginTop: label ? '0.4rem' : 0,
      accentColor: 'var(--primary)',
      appearance: 'none', WebkitAppearance: 'none', MozAppearance: 'none',
      backgroundImage: `url("${chevron}")`, backgroundRepeat: 'no-repeat',
      backgroundPosition: 'right 1rem center', cursor: 'pointer', ...style,
    }} {...rest}>
      {options ? options.map(o => {
        const v = typeof o === 'string' ? o : o.value;
        const l = typeof o === 'string' ? o : o.label;
        return <option key={v} value={v}>{l}</option>;
      }) : children}
    </select>
  );
  if (!label) return sel;
  return (
    <label htmlFor={id} style={{ display: 'block', marginBottom: '1rem', fontWeight: 'var(--weight-semibold)', fontSize: 'var(--text-label)', color: 'var(--text)' }}>
      {label}{sel}
    </label>
  );
}
