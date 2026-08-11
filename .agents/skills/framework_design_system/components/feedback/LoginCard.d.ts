import * as React from 'react';

/** Portal auth gate shown on 401. Lock icon, password field, submit; render centred over a scrim. */
export interface LoginCardProps {
  title?: string;
  subtitle?: string;
  /** Error message shown in a red alert above the field. */
  error?: string;
  onSubmit?: (password: string) => void;
  style?: React.CSSProperties;
}
export declare function LoginCard(props: LoginCardProps): React.JSX.Element;
