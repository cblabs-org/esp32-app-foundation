import * as React from 'react';

/** Segmented auto/light/dark theme switch (top bar). Sets [data-theme] in the live portal. */
export interface ThemeToggleProps {
  value?: 'auto' | 'light' | 'dark';
  onChange?: (mode: 'auto' | 'light' | 'dark') => void;
  style?: React.CSSProperties;
}
export declare function ThemeToggle(props: ThemeToggleProps): React.JSX.Element;
