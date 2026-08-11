import * as React from 'react';

/** Transient toast notification. Colour-coded by type. Managed by window.showToast in the portal. */
export interface ToastProps {
  type?: 'success' | 'error' | 'warning' | 'info';
  /** Slide/fade in when true. @default true */
  visible?: boolean;
  children?: React.ReactNode;
  style?: React.CSSProperties;
}
export declare function Toast(props: ToastProps): React.JSX.Element;
