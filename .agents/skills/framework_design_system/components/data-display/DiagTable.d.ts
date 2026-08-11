import * as React from 'react';

/** Compact diagnostics table (label / mono value). Used inside network + system drawers. */
export interface DiagTableProps {
  /** [label, value] pairs, one per row. */
  rows: [React.ReactNode, React.ReactNode][];
  style?: React.CSSProperties;
}
export declare function DiagTable(props: DiagTableProps): React.JSX.Element;
