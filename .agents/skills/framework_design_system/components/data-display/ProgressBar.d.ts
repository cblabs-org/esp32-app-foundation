import * as React from 'react';

/** RAG progress bar (green ≥50%, amber ≥25%, red <25%). Used for heap, storage, OTA. */
export interface ProgressBarProps {
  label: React.ReactNode;
  value: number;
  max: number;
  /** Override the right-hand caption (default is the computed percentage). */
  valueText?: React.ReactNode;
  /** Force a fill colour instead of the RAG rule. */
  color?: string;
  style?: React.CSSProperties;
}
export declare function ProgressBar(props: ProgressBarProps): React.JSX.Element;
