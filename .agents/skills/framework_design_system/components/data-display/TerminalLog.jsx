import React from 'react';

/** Dark monospace log viewer (green on near-black). Pass lines[] or children. */
export function TerminalLog({ lines, children, style = {} }) {
  return (
    <div style={{ background: 'var(--terminal-bg)', color: 'var(--terminal-fg)',
      fontFamily: 'var(--font-mono)', fontSize: 'var(--text-sm)', padding: '0.75rem',
      borderRadius: 'var(--radius-md)', maxHeight: 200, overflowY: 'auto',
      whiteSpace: 'pre-wrap', wordBreak: 'break-all', ...style }}>
      {lines
        ? lines.map((l, i) => <div key={i} style={{ lineHeight: 1.4, marginBottom: 2 }}>{l}</div>)
        : children}
    </div>
  );
}
