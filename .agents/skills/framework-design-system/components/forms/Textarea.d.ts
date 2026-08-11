import * as React from 'react';

/** Multi-line text field (e.g. pasting a certificate or config blob). */
export interface TextareaProps extends Omit<React.TextareaHTMLAttributes<HTMLTextAreaElement>, 'style'> {
  label?: string;
  style?: React.CSSProperties;
}
export declare function Textarea(props: TextareaProps): React.JSX.Element;
