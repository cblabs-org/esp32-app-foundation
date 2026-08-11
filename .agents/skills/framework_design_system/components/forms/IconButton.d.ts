import * as React from 'react';

/** Square icon-only button (copy, refresh, close…). Pass an 18px inline SVG as children. */
export interface IconButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  /** Accessible label / tooltip (required — the button has no text). */
  label: string;
}
export declare function IconButton(props: IconButtonProps): React.JSX.Element;
