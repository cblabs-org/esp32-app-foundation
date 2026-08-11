import * as React from 'react';

/** Controlled on/off switch. Track goes blue when on; used for OTA auto-check, etc. */
export interface ToggleSwitchProps {
  checked?: boolean;
  onChange?: (checked: boolean) => void;
  disabled?: boolean;
  id?: string;
}
export declare function ToggleSwitch(props: ToggleSwitchProps): React.JSX.Element;
