import React from 'react';

/** Text/number/password field with an optional label. Full-width by default. */
export function Input({ label, type = 'text', invalid = false, feedback, style = {}, id, ...rest }) {
  const inputEl = (
    <input id={id} type={type} style={{
      width: '100%', boxSizing: 'border-box', padding: '0.8rem 1rem',
      borderRadius: 'var(--radius-lg)', border: '1px solid ' + (invalid ? 'var(--error)' : 'var(--border)'),
      background: 'var(--bg)', color: 'var(--text)', fontSize: 'var(--text-md)',
      fontFamily: 'var(--font-sans)', marginTop: label ? '0.4rem' : 0,
      accentColor: 'var(--primary)', transition: 'border-color var(--transition-base)', ...style,
    }}
    onFocus={e => { if (!invalid) e.target.style.borderColor = 'var(--primary)'; }}
    onBlur={e => { if (!invalid) e.target.style.borderColor = 'var(--border)'; }} {...rest} />
  );
  if (!label && !feedback) return inputEl;
  return (
    <label htmlFor={id} style={{ display: 'block', marginBottom: '1rem', fontWeight: 'var(--weight-semibold)', fontSize: 'var(--text-label)', color: 'var(--text)' }}>
      {label}
      {inputEl}
      {feedback && <span style={{ fontSize: 'var(--text-sm)', marginTop: '0.25rem', display: 'block',
        color: invalid ? 'var(--error)' : 'var(--success)' }}>{feedback}</span>}
    </label>
  );
}
