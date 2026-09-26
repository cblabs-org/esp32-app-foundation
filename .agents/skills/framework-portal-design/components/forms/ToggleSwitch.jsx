import React from 'react';

/** iOS-style on/off switch. Track turns blue when checked; 48x26 knob slides 22px. */
export function ToggleSwitch({ checked = false, onChange, disabled = false, id }) {
  return (
    <label htmlFor={id} style={{ position: 'relative', display: 'inline-block', width: 48, height: 26, flexShrink: 0, opacity: disabled ? 0.5 : 1, filter: disabled ? 'grayscale(40%)' : undefined }}>
      <input id={id} type="checkbox" checked={checked} disabled={disabled}
        onChange={e => onChange && onChange(e.target.checked)}
        style={{ opacity: 0, width: 0, height: 0, position: 'absolute' }} />
      <span style={{ position: 'absolute', inset: 0, cursor: disabled ? 'not-allowed' : 'pointer',
        background: checked ? 'var(--primary)' : 'var(--border)', borderRadius: 'var(--radius-full)',
        transition: 'background var(--transition-slow)' }}>
        <span style={{ position: 'absolute', height: 20, width: 20, left: 3, bottom: 3, background: '#fff',
          borderRadius: '50%', boxShadow: 'var(--shadow-toggle-knob)',
          transition: 'transform var(--transition-slow)', transform: checked ? 'translateX(22px)' : 'none' }} />
      </span>
    </label>
  );
}
