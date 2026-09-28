import * as React from 'react';

/** Centred modal over a blurred scrim, with a close button. Click the backdrop to dismiss. */
export interface DialogProps {
  open?: boolean;
  onClose?: () => void;
  children?: React.ReactNode;
  style?: React.CSSProperties;
}
export declare function Dialog(props: DialogProps): React.JSX.Element;
