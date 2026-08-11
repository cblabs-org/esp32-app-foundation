import * as React from 'react';

/** Key/value row (muted label left, bold value right) with a hairline divider. */
export interface InfoRowProps {
  label: React.ReactNode;
  value: React.ReactNode;
  /** Drop the bottom border on the last row. @default false */
  last?: boolean;
  style?: React.CSSProperties;
}
export declare function InfoRow(props: InfoRowProps): React.JSX.Element;
