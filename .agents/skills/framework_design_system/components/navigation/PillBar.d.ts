import * as React from 'react';

/**
 * Scrolling sub-pill navigation (the row under a top-level tab). Active pill
 * is solid blue.
 */
export interface PillItem { id: string; label: string; }
export interface PillBarProps {
  items: (string | PillItem)[];
  active?: string;
  onSelect?: (id: string) => void;
  style?: React.CSSProperties;
}
export declare function PillBar(props: PillBarProps): React.JSX.Element;
