import * as React from 'react';

/** Full-screen restart overlay (spinner + message + sub-line) over a strong blurred scrim. */
export interface RebootOverlayProps {
  visible?: boolean;
  message?: string;
  sub?: string;
  style?: React.CSSProperties;
}
export declare function RebootOverlay(props: RebootOverlayProps): React.JSX.Element;
