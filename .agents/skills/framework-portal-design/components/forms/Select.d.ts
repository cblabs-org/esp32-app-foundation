import * as React from 'react';

/** Native dropdown styled like the portal inputs (used for OTA channel, check hour). */
export interface SelectOption { value: string; label: string; }
export interface SelectProps extends Omit<React.SelectHTMLAttributes<HTMLSelectElement>, 'style'> {
  label?: string;
  /** Options as strings or {value,label}. Omit to pass <option> children. */
  options?: (string | SelectOption)[];
  style?: React.CSSProperties;
}
export declare function Select(props: SelectProps): React.JSX.Element;
