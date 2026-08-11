import React from 'react';

/** Password field with a show/hide eye toggle on the right edge. */
export function PasswordInput({ label, hasValue = false, placeholder, style = {}, id, ...rest }) {
  const [show, setShow] = React.useState(false);
  // When a password already exists we show a masked hint (never the real value).
  const ph = hasValue ? '\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022' : placeholder;
  const field = (
    <div style={{ position: 'relative', marginTop: label ? '0.4rem' : 0 }}>
      <input id={id} type={show ? 'text' : 'password'} placeholder={ph} style={{
        width: '100%', boxSizing: 'border-box', padding: '0.8rem 2.5rem 0.8rem 1rem',
        borderRadius: 'var(--radius-lg)', border: '1px solid var(--border)', background: 'var(--bg)',
        color: 'var(--text)', fontSize: 'var(--text-md)', fontFamily: 'var(--font-sans)',
        transition: 'border-color var(--transition-base)', ...style,
      }}
      onFocus={e => e.target.style.borderColor = 'var(--primary)'}
      onBlur={e => e.target.style.borderColor = 'var(--border)'} {...rest} />
      <button type="button" title="Toggle visibility" onClick={() => setShow(s => !s)}
        style={{ position: 'absolute', right: 8, top: '50%', transform: 'translateY(-50%)',
          background: 'none', border: 'none', cursor: 'pointer', padding: 4, color: 'var(--text-muted)',
          display: 'flex', alignItems: 'center' }}>
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
          {show
            ? <><path d="M2 2l20 20" /><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z" /></>
            : <><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z" /><circle cx="12" cy="12" r="3" /></>}
        </svg>
      </button>
    </div>
  );
  if (!label) return field;
  return (
    <label htmlFor={id} style={{ display: 'block', marginBottom: '1rem', fontWeight: 'var(--weight-semibold)', fontSize: 'var(--text-label)', color: 'var(--text)' }}>
      {label}{field}
    </label>
  );
}
