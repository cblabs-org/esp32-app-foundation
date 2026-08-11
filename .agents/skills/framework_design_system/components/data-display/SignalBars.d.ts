import * as React from 'react';

/** WiFi signal strength as 4 ascending bars. Pass rssi (dBm) or an explicit bars count. */
export interface SignalBarsProps {
  /** RSSI in dBm; mapped to 0–4 bars. */
  rssi?: number;
  /** Explicit bar count 0–4 (overrides rssi). */
  bars?: number;
  style?: React.CSSProperties;
}
export declare function SignalBars(props: SignalBarsProps): React.JSX.Element;
