import * as React from 'react';

/** Collapsible section built on the portal's <details> pattern. advanced = yellow expert zone; info = neutral diagnostics. */
export interface DrawerProps {
  summary: React.ReactNode;
  variant?: 'advanced' | 'info';
  open?: boolean;
  children?: React.ReactNode;
  style?: React.CSSProperties;
}
export declare function Drawer(props: DrawerProps): React.JSX.Element;
