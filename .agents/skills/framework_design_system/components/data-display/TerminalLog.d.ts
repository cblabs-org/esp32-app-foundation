import * as React from 'react';

/** Serial/OTA log viewer — green monospace on #1a1a1a, scrolls, wraps long tokens. */
export interface TerminalLogProps {
  /** One entry per line. Omit to pass pre-formatted children. */
  lines?: React.ReactNode[];
  children?: React.ReactNode;
  style?: React.CSSProperties;
}
export declare function TerminalLog(props: TerminalLogProps): React.JSX.Element;
