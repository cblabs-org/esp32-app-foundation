import * as React from 'react';

/** Inline status/meta pill. ok/warn = tinted rounded; subtle = uppercase tag; mode = STA/AP square chip. */
export interface BadgeProps {
  variant?: 'ok' | 'warn' | 'subtle' | 'mode';
  children?: React.ReactNode;
  style?: React.CSSProperties;
}
export declare function Badge(props: BadgeProps): React.JSX.Element;
