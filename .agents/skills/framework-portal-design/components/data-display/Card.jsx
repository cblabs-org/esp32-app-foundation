import React from 'react';

/** Rounded surface container. Header + body. Variants tint the border/background. */
export function Card({ header, variant = 'default', interactive = false, disabled = false, children, style = {}, ...rest }) {
  const [hover, setHover] = React.useState(false);
  const variants = {
    default:  { borderColor: 'var(--border)', background: 'var(--card-bg)' },
    danger:   { borderColor: 'var(--error)', background: 'rgba(198,40,40,0.04)' },
    advanced: { borderColor: 'var(--warning)', background: 'rgba(249,168,37,0.04)' },
  };
  const v = variants[variant] || variants.default;
  const s = {
    borderRadius: 'var(--radius-2xl)', padding: '1.25rem', marginBottom: '1rem',
    border: '1px solid ' + v.borderColor, background: v.background, boxSizing: 'border-box',
    color: 'var(--text)', fontFamily: 'var(--font-sans)',
    opacity: disabled ? 0.5 : 1, pointerEvents: disabled ? 'none' : undefined,
    filter: disabled ? 'grayscale(30%)' : undefined,
    cursor: interactive ? 'pointer' : undefined,
    transition: interactive ? 'transform var(--transition-fast), box-shadow var(--transition-fast)' : undefined,
    transform: interactive && hover ? 'translateY(-2px)' : undefined,
    boxShadow: interactive && hover ? 'var(--shadow-card-hover)' : undefined,
    ...style,
  };
  const hd = header && (
    <div style={{ fontWeight: 'var(--weight-bold)', fontSize: 'var(--text-card-title)', marginBottom: '1rem',
      color: variant === 'danger' ? 'var(--error)' : variant === 'advanced' ? 'var(--warning)' : 'var(--text)' }}>
      {header}
    </div>
  );
  return (
    <article style={s}
      onMouseEnter={() => interactive && setHover(true)}
      onMouseLeave={() => interactive && setHover(false)} {...rest}>
      {hd}{children}
    </article>
  );
}
