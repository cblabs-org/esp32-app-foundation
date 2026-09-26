import * as React from 'react';

/**
 * Labelled text input. Rounded 10px, hairline border that turns blue on focus
 * and red when `invalid`. Renders a label + validation feedback line when given.
 */
export interface InputProps extends Omit<React.InputHTMLAttributes<HTMLInputElement>, 'style'> {
  /** Optional label rendered above the field. */
  label?: string;
  /** Error styling on the border + feedback text. @default false */
  invalid?: boolean;
  /** Small helper/validation line under the field. */
  feedback?: string;
  style?: React.CSSProperties;
}
export declare function Input(props: InputProps): React.JSX.Element;
