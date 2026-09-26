import React from 'react';

/** Multi-line input matching the portal's field styling. */
export function Textarea({ label, rows = 4, style = {}, id, ...rest }) {
  const ta = (
    <textarea id={id} rows={rows} style={{
      width: '100%', boxSizing: 'border-box', padding: '0.8rem 1rem', borderRadius: 'var(--radius-lg)',
      border: '1px solid var(--border)', background: 'var(--bg)', color: 'var(--text)',
      fontSize: 'var(--text-md)', fontFamily: 'var(--font-sans)', marginTop: label ? '0.4rem' : 0,
      accentColor: 'var(--primary)', resize: 'vertical', lineHeight: 'var(--leading-body)',
      transition: 'border-color var(--transition-base)', ...style }}
      onFocus={e => e.target.style.borderColor = 'var(--primary)'}
      onBlur={e => e.target.style.borderColor = 'var(--border)'} {...rest} />
  );
  if (!label) return ta;
  return (
    <label htmlFor={id} style={{ display: 'block', marginBottom: '1rem', fontWeight: 'var(--weight-semibold)', fontSize: 'var(--text-label)', color: 'var(--text)' }}>
      {label}{ta}
    </label>
  );
}
