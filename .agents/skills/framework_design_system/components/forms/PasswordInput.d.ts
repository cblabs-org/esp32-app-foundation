import * as React from 'react';

/** Password field with an inline show/hide eye button (WiFi & auth screens). */
export interface PasswordInputProps extends Omit<React.InputHTMLAttributes<HTMLInputElement>, 'type' | 'style'> {
  label?: string;
  style?: React.CSSProperties;
  /** A password is already set: shows a masked •••• hint without holding the real value. @default false */
  hasValue?: boolean;
}
export declare function PasswordInput(props: PasswordInputProps): React.JSX.Element;
