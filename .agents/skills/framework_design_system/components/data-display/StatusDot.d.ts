import * as React from 'react';

/** Glowing status dot (green/red/yellow/grey) with optional pulse + label. */
export interface StatusDotProps {
  status?: 'green' | 'red' | 'yellow' | 'grey';
  size?: number;
  pulse?: boolean;
  label?: React.ReactNode;
  style?: React.CSSProperties;
}
export declare function StatusDot(props: StatusDotProps): React.JSX.Element;
