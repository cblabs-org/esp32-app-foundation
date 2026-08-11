import * as React from 'react';

/** Inline feedback box, tinted by variant. Persistent (unlike the transient Toast). */
export interface AlertProps extends React.HTMLAttributes<HTMLDivElement> {
  variant?: 'success' | 'error' | 'warning' | 'info';
}
export declare function Alert(props: AlertProps): React.JSX.Element;
