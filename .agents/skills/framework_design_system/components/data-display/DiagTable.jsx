import React from 'react';

/** Two-column diagnostics table: bold label (40%) left, mono value right-aligned. */
export function DiagTable({ rows = [], style = {} }) {
  return (
    <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: 'var(--text-sm)',
      marginTop: '1rem', color: 'var(--text-muted)', fontFamily: 'var(--font-sans)', ...style }}>
      <tbody>
        {rows.map(([label, value], i) => (
          <tr key={i} style={{ borderBottom: i < rows.length - 1 ? '1px solid var(--border)' : 'none' }}>
            <td style={{ padding: '6px 0', fontWeight: 'var(--weight-bold)', color: 'var(--text)', width: '40%' }}>{label}</td>
            <td style={{ padding: '6px 0', fontFamily: 'var(--font-mono)', textAlign: 'right' }}>{value}</td>
          </tr>
        ))}
      </tbody>
    </table>
  );
}
