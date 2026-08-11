import * as React from 'react';

/**
 * The portal's fundamental surface — 16px radius, hairline border, tinted
 * background. `danger` and `advanced` variants recolour the border + header
 * for destructive and expert zones. `interactive` adds a hover lift.

 */
export interface CardProps extends React.HTMLAttributes<HTMLElement> {
  header?: React.ReactNode;
  variant?: 'default' | 'danger' | 'advanced';
  interactive?: boolean;
  disabled?: boolean;
}
export declare function Card(props: CardProps): React.JSX.Element;
