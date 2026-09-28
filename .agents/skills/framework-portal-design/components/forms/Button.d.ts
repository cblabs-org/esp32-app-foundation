import * as React from 'react';

/**
 * Primary action control for the ESP32 portal. Blue primary by default,
 * with outline / neutral-secondary / destructive-danger variants. Shrinks
 * slightly on press and shows an inline spinner while `busy`.

 */
export interface ButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  /** Visual style. @default "primary" */
  variant?: 'primary' | 'outline' | 'secondary' | 'danger';
  /** Show a spinner and block interaction. @default false */
  busy?: boolean;
  /** Stretch to fill the container width. @default false */
  full?: boolean;
}
export declare function Button(props: ButtonProps): React.JSX.Element;
