/* @ds-bundle: {"format":4,"namespace":"ESP32AppFoundationDesignSystem_aaa6bc","components":[{"name":"Badge","sourcePath":"components/data-display/Badge.jsx"},{"name":"Card","sourcePath":"components/data-display/Card.jsx"},{"name":"DiagTable","sourcePath":"components/data-display/DiagTable.jsx"},{"name":"InfoRow","sourcePath":"components/data-display/InfoRow.jsx"},{"name":"ProgressBar","sourcePath":"components/data-display/ProgressBar.jsx"},{"name":"SignalBars","sourcePath":"components/data-display/SignalBars.jsx"},{"name":"StatusDot","sourcePath":"components/data-display/StatusDot.jsx"},{"name":"TerminalLog","sourcePath":"components/data-display/TerminalLog.jsx"},{"name":"Alert","sourcePath":"components/feedback/Alert.jsx"},{"name":"Callout","sourcePath":"components/feedback/Callout.jsx"},{"name":"Dialog","sourcePath":"components/feedback/Dialog.jsx"},{"name":"LoginCard","sourcePath":"components/feedback/LoginCard.jsx"},{"name":"RebootOverlay","sourcePath":"components/feedback/RebootOverlay.jsx"},{"name":"Toast","sourcePath":"components/feedback/Toast.jsx"},{"name":"Button","sourcePath":"components/forms/Button.jsx"},{"name":"IconButton","sourcePath":"components/forms/IconButton.jsx"},{"name":"Input","sourcePath":"components/forms/Input.jsx"},{"name":"PasswordInput","sourcePath":"components/forms/PasswordInput.jsx"},{"name":"Select","sourcePath":"components/forms/Select.jsx"},{"name":"Textarea","sourcePath":"components/forms/Textarea.jsx"},{"name":"ToggleSwitch","sourcePath":"components/forms/ToggleSwitch.jsx"},{"name":"Drawer","sourcePath":"components/navigation/Drawer.jsx"},{"name":"PillBar","sourcePath":"components/navigation/PillBar.jsx"},{"name":"ThemeToggle","sourcePath":"components/navigation/ThemeToggle.jsx"}],"sourceHashes":{"components/data-display/Badge.jsx":"a4f1b0b4d26a","components/data-display/Card.jsx":"f897720a9153","components/data-display/DiagTable.jsx":"ccd232fd2574","components/data-display/InfoRow.jsx":"06eb8aeba1f3","components/data-display/ProgressBar.jsx":"d0c448fcd47c","components/data-display/SignalBars.jsx":"2ac0543adb54","components/data-display/StatusDot.jsx":"b4840cf6cfbf","components/data-display/TerminalLog.jsx":"23b1b4b0f799","components/feedback/Alert.jsx":"720d23730db6","components/feedback/Callout.jsx":"fe62b126582b","components/feedback/Dialog.jsx":"6eb4014ba1fd","components/feedback/LoginCard.jsx":"c2dd6a4d2736","components/feedback/RebootOverlay.jsx":"2a468626c9bb","components/feedback/Toast.jsx":"e07aa7c43fb2","components/forms/Button.jsx":"32382f0296ac","components/forms/IconButton.jsx":"23dbde4343a1","components/forms/Input.jsx":"79aee75ed787","components/forms/PasswordInput.jsx":"e1d22be5c2d4","components/forms/Select.jsx":"25a87f7844b9","components/forms/Textarea.jsx":"587885ef4453","components/forms/ToggleSwitch.jsx":"4451436adad3","components/navigation/Drawer.jsx":"b7b72871198c","components/navigation/PillBar.jsx":"37abb4b9f226","components/navigation/ThemeToggle.jsx":"b16acfbfebf0","ui_kits/portal/OtaScreen.jsx":"811d759647e5","ui_kits/portal/Portal.jsx":"5f80d21c7c36","ui_kits/portal/SecurityScreen.jsx":"f66c48ee5eee","ui_kits/portal/SystemScreen.jsx":"af4fb3d54acc","ui_kits/portal/WifiScreen.jsx":"6b42fb82bec0"},"inlinedExternals":[],"unexposedExports":[]} */

(() => {

const __ds_ns = (window.ESP32AppFoundationDesignSystem_aaa6bc = window.ESP32AppFoundationDesignSystem_aaa6bc || {});

const __ds_scope = {};

(__ds_ns.__errors = __ds_ns.__errors || []);

// components/data-display/Badge.jsx
try { (() => {
/** Small pill label for status/meta. Variants: ok | warn | subtle | mode. */
function Badge({
  variant = 'subtle',
  children,
  style = {}
}) {
  const variants = {
    ok: {
      background: 'rgba(46,125,50,0.12)',
      color: 'var(--success)',
      padding: '4px 12px',
      fontSize: '0.85em',
      fontWeight: 'var(--weight-medium)',
      borderRadius: 'var(--radius-xl)',
      textTransform: 'none',
      letterSpacing: 0
    },
    warn: {
      background: 'rgba(249,168,37,0.12)',
      color: 'var(--warning)',
      padding: '4px 12px',
      fontSize: '0.85em',
      fontWeight: 'var(--weight-medium)',
      borderRadius: 'var(--radius-xl)',
      textTransform: 'none',
      letterSpacing: 0
    },
    subtle: {
      background: 'var(--primary-alpha)',
      color: 'var(--primary)',
      padding: '0.15rem 0.5rem',
      fontSize: 'var(--text-2xs)',
      fontWeight: 'var(--weight-bold)',
      borderRadius: 'var(--radius-xl)',
      textTransform: 'uppercase',
      letterSpacing: '0.05em'
    },
    mode: {
      background: 'var(--border)',
      color: 'var(--text-muted)',
      padding: '2px 8px',
      fontSize: 'var(--text-xs)',
      fontWeight: 'var(--weight-bold)',
      borderRadius: 'var(--radius-sm)',
      textTransform: 'uppercase',
      letterSpacing: 'var(--tracking-caps)'
    }
  };
  return /*#__PURE__*/React.createElement("span", {
    style: {
      display: 'inline-block',
      fontFamily: 'var(--font-sans)',
      ...(variants[variant] || variants.subtle),
      ...style
    }
  }, children);
}
Object.assign(__ds_scope, { Badge });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/data-display/Badge.jsx", error: String((e && e.message) || e) }); }

// components/data-display/Card.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Rounded surface container. Header + body. Variants tint the border/background. */
function Card({
  header,
  variant = 'default',
  interactive = false,
  disabled = false,
  children,
  style = {},
  ...rest
}) {
  const [hover, setHover] = React.useState(false);
  const variants = {
    default: {
      borderColor: 'var(--border)',
      background: 'var(--card-bg)'
    },
    danger: {
      borderColor: 'var(--error)',
      background: 'rgba(198,40,40,0.04)'
    },
    advanced: {
      borderColor: 'var(--warning)',
      background: 'rgba(249,168,37,0.04)'
    }
  };
  const v = variants[variant] || variants.default;
  const s = {
    borderRadius: 'var(--radius-2xl)',
    padding: '1.25rem',
    marginBottom: '1rem',
    border: '1px solid ' + v.borderColor,
    background: v.background,
    boxSizing: 'border-box',
    color: 'var(--text)',
    fontFamily: 'var(--font-sans)',
    opacity: disabled ? 0.5 : 1,
    pointerEvents: disabled ? 'none' : undefined,
    filter: disabled ? 'grayscale(30%)' : undefined,
    cursor: interactive ? 'pointer' : undefined,
    transition: interactive ? 'transform var(--transition-fast), box-shadow var(--transition-fast)' : undefined,
    transform: interactive && hover ? 'translateY(-2px)' : undefined,
    boxShadow: interactive && hover ? 'var(--shadow-card-hover)' : undefined,
    ...style
  };
  const hd = header && /*#__PURE__*/React.createElement("div", {
    style: {
      fontWeight: 'var(--weight-bold)',
      fontSize: 'var(--text-card-title)',
      marginBottom: '1rem',
      color: variant === 'danger' ? 'var(--error)' : variant === 'advanced' ? 'var(--warning)' : 'var(--text)'
    }
  }, header);
  return /*#__PURE__*/React.createElement("article", _extends({
    style: s,
    onMouseEnter: () => interactive && setHover(true),
    onMouseLeave: () => interactive && setHover(false)
  }, rest), hd, children);
}
Object.assign(__ds_scope, { Card });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/data-display/Card.jsx", error: String((e && e.message) || e) }); }

// components/data-display/DiagTable.jsx
try { (() => {
/** Two-column diagnostics table: bold label (40%) left, mono value right-aligned. */
function DiagTable({
  rows = [],
  style = {}
}) {
  return /*#__PURE__*/React.createElement("table", {
    style: {
      width: '100%',
      borderCollapse: 'collapse',
      fontSize: 'var(--text-sm)',
      marginTop: '1rem',
      color: 'var(--text-muted)',
      fontFamily: 'var(--font-sans)',
      ...style
    }
  }, /*#__PURE__*/React.createElement("tbody", null, rows.map(([label, value], i) => /*#__PURE__*/React.createElement("tr", {
    key: i,
    style: {
      borderBottom: i < rows.length - 1 ? '1px solid var(--border)' : 'none'
    }
  }, /*#__PURE__*/React.createElement("td", {
    style: {
      padding: '6px 0',
      fontWeight: 'var(--weight-bold)',
      color: 'var(--text)',
      width: '40%'
    }
  }, label), /*#__PURE__*/React.createElement("td", {
    style: {
      padding: '6px 0',
      fontFamily: 'var(--font-mono)',
      textAlign: 'right'
    }
  }, value)))));
}
Object.assign(__ds_scope, { DiagTable });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/data-display/DiagTable.jsx", error: String((e && e.message) || e) }); }

// components/data-display/InfoRow.jsx
try { (() => {
/** Label/value row with a hairline underline. Used throughout device info panels. */
function InfoRow({
  label,
  value,
  last = false,
  style = {}
}) {
  return /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      justifyContent: 'space-between',
      padding: '0.4rem 0',
      borderBottom: last ? 'none' : '1px solid var(--border)',
      fontSize: 'var(--text-body)',
      fontFamily: 'var(--font-sans)',
      ...style
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      color: 'var(--text-muted)'
    }
  }, label), /*#__PURE__*/React.createElement("strong", {
    style: {
      color: 'var(--text)'
    }
  }, value));
}
Object.assign(__ds_scope, { InfoRow });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/data-display/InfoRow.jsx", error: String((e && e.message) || e) }); }

// components/data-display/ProgressBar.jsx
try { (() => {
/** RAG-coloured progress row: label, value/max caption, and a filled track. */
function ProgressBar({
  label,
  value = 0,
  max = 100,
  valueText,
  color,
  style = {}
}) {
  const pct = max > 0 ? Math.round(value / max * 100) : 0;
  const rag = pct >= 50 ? 'var(--success)' : pct >= 25 ? 'var(--warning)' : 'var(--error)';
  const fill = color || rag;
  return /*#__PURE__*/React.createElement("div", {
    style: {
      marginBottom: '0.75rem',
      fontFamily: 'var(--font-sans)',
      ...style
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      justifyContent: 'space-between',
      fontSize: 'var(--text-base-sm)',
      marginBottom: 4,
      color: 'var(--text)'
    }
  }, /*#__PURE__*/React.createElement("span", null, label), /*#__PURE__*/React.createElement("span", {
    style: {
      color: 'var(--text-muted)'
    }
  }, valueText != null ? valueText : pct + '%')), /*#__PURE__*/React.createElement("div", {
    style: {
      background: 'var(--bg)',
      borderRadius: 'var(--radius-sm)',
      height: 10,
      overflow: 'hidden',
      border: '1px solid var(--border)'
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      width: pct + '%',
      height: '100%',
      background: fill,
      borderRadius: 'var(--radius-sm)',
      transition: 'var(--transition-progress)'
    }
  })));
}
Object.assign(__ds_scope, { ProgressBar });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/data-display/ProgressBar.jsx", error: String((e && e.message) || e) }); }

// components/data-display/SignalBars.jsx
try { (() => {
function rssiToBars(rssi) {
  if (rssi === 0) return 0;
  if (rssi >= -50) return 4;
  if (rssi >= -60) return 3;
  if (rssi >= -70) return 2;
  if (rssi >= -80) return 1;
  return 0;
}

/** 4-bar WiFi signal strength from an RSSI (dBm) value, or an explicit bars count. */
function SignalBars({
  rssi,
  bars,
  style = {}
}) {
  const n = bars != null ? bars : rssiToBars(rssi ?? 0);
  return /*#__PURE__*/React.createElement("span", {
    "aria-label": 'Signal: ' + n + ' of 4 bars',
    style: {
      display: 'inline-flex',
      alignItems: 'flex-end',
      gap: 2,
      height: 16,
      verticalAlign: 'middle',
      ...style
    }
  }, [0, 1, 2, 3].map(i => /*#__PURE__*/React.createElement("span", {
    key: i,
    style: {
      display: 'inline-block',
      width: 3,
      height: 4 + i * 3,
      borderRadius: 1,
      background: i < n ? 'var(--primary)' : 'var(--border)'
    }
  })));
}
Object.assign(__ds_scope, { SignalBars });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/data-display/SignalBars.jsx", error: String((e && e.message) || e) }); }

// components/data-display/StatusDot.jsx
try { (() => {
const COLOR = {
  green: 'var(--success)',
  red: 'var(--error)',
  yellow: 'var(--warning)',
  grey: 'var(--text-muted)'
};

/** Glowing status indicator dot. Optional pulse animation and trailing label. */
function StatusDot({
  status = 'grey',
  size = 10,
  pulse = false,
  label,
  style = {}
}) {
  const glow = status === 'grey' ? 0.4 : 0.8;
  const dot = /*#__PURE__*/React.createElement("span", {
    style: {
      width: size,
      height: size,
      borderRadius: '50%',
      display: 'inline-block',
      background: COLOR[status],
      boxShadow: '0 0 ' + size * glow + 'px ' + COLOR[status],
      animation: pulse ? 'af-pulse 1s infinite' : undefined,
      flexShrink: 0,
      ...style
    }
  });
  if (!label) return dot;
  return /*#__PURE__*/React.createElement("span", {
    style: {
      display: 'inline-flex',
      alignItems: 'center',
      gap: 8,
      fontSize: 'var(--text-body)',
      fontWeight: 'var(--weight-semibold)',
      color: 'var(--text)'
    }
  }, dot, label);
}
Object.assign(__ds_scope, { StatusDot });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/data-display/StatusDot.jsx", error: String((e && e.message) || e) }); }

// components/data-display/TerminalLog.jsx
try { (() => {
/** Dark monospace log viewer (green on near-black). Pass lines[] or children. */
function TerminalLog({
  lines,
  children,
  style = {}
}) {
  return /*#__PURE__*/React.createElement("div", {
    style: {
      background: 'var(--terminal-bg)',
      color: 'var(--terminal-fg)',
      fontFamily: 'var(--font-mono)',
      fontSize: 'var(--text-sm)',
      padding: '0.75rem',
      borderRadius: 'var(--radius-md)',
      maxHeight: 200,
      overflowY: 'auto',
      whiteSpace: 'pre-wrap',
      wordBreak: 'break-all',
      ...style
    }
  }, lines ? lines.map((l, i) => /*#__PURE__*/React.createElement("div", {
    key: i,
    style: {
      lineHeight: 1.4,
      marginBottom: 2
    }
  }, l)) : children);
}
Object.assign(__ds_scope, { TerminalLog });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/data-display/TerminalLog.jsx", error: String((e && e.message) || e) }); }

// components/feedback/Alert.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
const V = {
  success: {
    color: 'var(--success)'
  },
  error: {
    color: 'var(--error)'
  },
  warning: {
    color: 'var(--warning)'
  },
  info: {
    color: 'var(--primary)'
  }
};

/** Inline feedback box with a tinted background + coloured border. */
function Alert({
  variant = 'info',
  children,
  style = {},
  ...rest
}) {
  const c = (V[variant] || V.info).color;
  const bg = variant === 'info' ? 'var(--primary-alpha)' : 'color-mix(in srgb, ' + c + ' 10%, transparent)';
  return /*#__PURE__*/React.createElement("div", _extends({
    role: variant === 'error' ? 'alert' : 'status',
    style: {
      padding: '0.75rem 1rem',
      borderRadius: 'var(--radius-md)',
      border: '1px solid ' + c,
      background: bg,
      color: c,
      fontSize: 'var(--text-body)',
      marginBottom: '0.75rem',
      fontFamily: 'var(--font-sans)',
      lineHeight: 'var(--leading-body)',
      ...style
    }
  }, rest), children);
}
Object.assign(__ds_scope, { Alert });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/feedback/Alert.jsx", error: String((e && e.message) || e) }); }

// components/feedback/Callout.jsx
try { (() => {
/** Left-accented instruction block. Blue by default. */
function Callout({
  children,
  style = {}
}) {
  return /*#__PURE__*/React.createElement("div", {
    style: {
      borderLeft: '4px solid var(--primary)',
      background: 'var(--primary-alpha)',
      padding: '0.75rem 1rem',
      borderRadius: '0 var(--radius-md) var(--radius-md) 0',
      marginBottom: '1rem',
      fontSize: 'var(--text-body)',
      lineHeight: 'var(--leading-body)',
      color: 'var(--text)',
      fontFamily: 'var(--font-sans)',
      ...style
    }
  }, children);
}
Object.assign(__ds_scope, { Callout });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/feedback/Callout.jsx", error: String((e && e.message) || e) }); }

// components/feedback/Dialog.jsx
try { (() => {
/** Modal card centred over a blurred scrim, with a close (×) button. */
function Dialog({
  open = true,
  onClose,
  children,
  style = {}
}) {
  if (!open) return null;
  return /*#__PURE__*/React.createElement("div", {
    onClick: e => {
      if (e.target === e.currentTarget) onClose && onClose();
    },
    style: {
      position: 'fixed',
      inset: 0,
      zIndex: 9998,
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'center',
      background: 'var(--scrim)',
      backdropFilter: 'blur(var(--blur-backdrop))'
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'relative',
      borderRadius: 'var(--radius-xl)',
      background: 'var(--bg)',
      color: 'var(--text)',
      maxWidth: 500,
      width: '90%',
      maxHeight: '90vh',
      overflowY: 'auto',
      boxShadow: 'var(--shadow-dialog)',
      padding: '1.5rem',
      fontFamily: 'var(--font-sans)',
      ...style
    }
  }, /*#__PURE__*/React.createElement("button", {
    onClick: () => onClose && onClose(),
    title: "Close",
    style: {
      position: 'absolute',
      top: '0.75rem',
      right: '0.75rem',
      background: 'none',
      border: 'none',
      cursor: 'pointer',
      padding: '0.25rem',
      minWidth: 36,
      minHeight: 36,
      color: 'var(--text-muted)',
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'center'
    }
  }, /*#__PURE__*/React.createElement("svg", {
    width: "24",
    height: "24",
    viewBox: "0 0 24 24",
    fill: "none",
    stroke: "currentColor",
    strokeWidth: "2",
    strokeLinecap: "round",
    strokeLinejoin: "round"
  }, /*#__PURE__*/React.createElement("line", {
    x1: "18",
    y1: "6",
    x2: "6",
    y2: "18"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "6",
    y1: "6",
    x2: "18",
    y2: "18"
  }))), children));
}
Object.assign(__ds_scope, { Dialog });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/feedback/Dialog.jsx", error: String((e && e.message) || e) }); }

// components/feedback/LoginCard.jsx
try { (() => {
/** 401 auth gate: a centred card (lock icon, title, password field, submit). */
function LoginCard({
  title = 'Authentication Required',
  subtitle = 'Enter the portal password to continue.',
  error,
  onSubmit,
  style = {}
}) {
  const [pw, setPw] = React.useState('');
  return /*#__PURE__*/React.createElement("div", {
    style: {
      background: 'var(--card-bg)',
      border: '1px solid var(--border)',
      borderRadius: 'var(--radius-2xl)',
      padding: 32,
      width: 340,
      maxWidth: '90vw',
      textAlign: 'center',
      boxShadow: 'var(--shadow-login)',
      fontFamily: 'var(--font-sans)',
      color: 'var(--text)',
      ...style
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      marginBottom: 16,
      color: 'var(--primary)',
      display: 'flex',
      justifyContent: 'center'
    }
  }, /*#__PURE__*/React.createElement("svg", {
    width: "48",
    height: "48",
    viewBox: "0 0 24 24",
    fill: "none",
    stroke: "currentColor",
    strokeWidth: "2",
    strokeLinecap: "round",
    strokeLinejoin: "round"
  }, /*#__PURE__*/React.createElement("rect", {
    x: "3",
    y: "11",
    width: "18",
    height: "11",
    rx: "2",
    ry: "2"
  }), /*#__PURE__*/React.createElement("path", {
    d: "M7 11V7a5 5 0 0 1 10 0v4"
  }))), /*#__PURE__*/React.createElement("h2", {
    style: {
      margin: '0 0 4px',
      fontSize: '1.4em'
    }
  }, title), /*#__PURE__*/React.createElement("p", {
    style: {
      color: 'var(--text-muted)',
      margin: '0 0 20px',
      fontSize: '0.9em'
    }
  }, subtitle), error && /*#__PURE__*/React.createElement("div", {
    style: {
      padding: '0.75rem 1rem',
      borderRadius: 'var(--radius-md)',
      border: '1px solid var(--error)',
      background: 'color-mix(in srgb, var(--error) 10%, transparent)',
      color: 'var(--error)',
      fontSize: 'var(--text-body)',
      marginBottom: '0.75rem'
    }
  }, error), /*#__PURE__*/React.createElement("form", {
    onSubmit: e => {
      e.preventDefault();
      onSubmit && onSubmit(pw);
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      marginBottom: 12
    }
  }, /*#__PURE__*/React.createElement("input", {
    type: "password",
    value: pw,
    onChange: e => setPw(e.target.value),
    placeholder: "Password",
    style: {
      width: '100%',
      padding: '10px 14px',
      border: '1px solid var(--border)',
      borderRadius: 'var(--radius-md)',
      background: 'var(--bg)',
      color: 'var(--text)',
      fontSize: '1em',
      boxSizing: 'border-box'
    },
    onFocus: e => {
      e.target.style.borderColor = 'var(--primary)';
      e.target.style.boxShadow = '0 0 0 3px var(--primary-alpha)';
    },
    onBlur: e => {
      e.target.style.borderColor = 'var(--border)';
      e.target.style.boxShadow = 'none';
    }
  })), /*#__PURE__*/React.createElement("button", {
    type: "submit",
    style: {
      width: '100%',
      padding: '0.8rem 1.25rem',
      border: 'none',
      borderRadius: 'var(--radius-lg)',
      background: 'var(--primary)',
      color: '#fff',
      fontWeight: 'var(--weight-bold)',
      fontSize: 'var(--text-md)',
      cursor: 'pointer'
    }
  }, "Unlock")));
}
Object.assign(__ds_scope, { LoginCard });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/feedback/LoginCard.jsx", error: String((e && e.message) || e) }); }

// components/feedback/RebootOverlay.jsx
try { (() => {
/** Full-screen dark spinner overlay shown during device restarts. Position:fixed. */
function RebootOverlay({
  visible = true,
  message = 'Restarting device…',
  sub = 'The device will be back online in a few seconds.',
  style = {}
}) {
  return /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'fixed',
      inset: 0,
      zIndex: 10000,
      background: 'var(--scrim-strong)',
      backdropFilter: 'blur(var(--blur-login))',
      WebkitBackdropFilter: 'blur(var(--blur-login))',
      display: 'flex',
      flexDirection: 'column',
      alignItems: 'center',
      justifyContent: 'center',
      color: '#fff',
      gap: '1.25rem',
      fontFamily: 'var(--font-sans)',
      opacity: visible ? 1 : 0,
      visibility: visible ? 'visible' : 'hidden',
      transition: 'opacity var(--transition-slow), visibility var(--transition-slow)',
      ...style
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      width: 44,
      height: 44,
      borderRadius: '50%',
      border: '3px solid rgba(255,255,255,0.25)',
      borderTopColor: '#fff',
      animation: 'af-spin 0.8s linear infinite'
    }
  }), /*#__PURE__*/React.createElement("div", {
    style: {
      fontSize: '1.15rem',
      fontWeight: 'var(--weight-semibold)',
      textAlign: 'center'
    }
  }, message), /*#__PURE__*/React.createElement("div", {
    style: {
      fontSize: 'var(--text-body)',
      opacity: 0.7,
      textAlign: 'center',
      maxWidth: 320,
      lineHeight: 'var(--leading-body)'
    }
  }, sub));
}
Object.assign(__ds_scope, { RebootOverlay });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/feedback/RebootOverlay.jsx", error: String((e && e.message) || e) }); }

// components/feedback/Toast.jsx
try { (() => {
const BG = {
  success: 'var(--success)',
  error: 'var(--error)',
  warning: 'var(--warning)',
  info: 'var(--primary)'
};

/** Transient notification pill. Render inside a bottom-centred stack. */
function Toast({
  type = 'info',
  visible = true,
  children,
  style = {}
}) {
  return /*#__PURE__*/React.createElement("div", {
    style: {
      padding: '0.75rem 1rem',
      borderRadius: 'var(--radius-lg)',
      fontSize: 'var(--text-body)',
      fontWeight: 'var(--weight-semibold)',
      color: '#fff',
      background: BG[type] || '#333',
      fontFamily: 'var(--font-sans)',
      transition: 'all var(--transition-slow)',
      transform: visible ? 'translateY(0)' : 'translateY(20px)',
      opacity: visible ? 1 : 0,
      boxShadow: 'var(--shadow-card-hover)',
      ...style
    }
  }, children);
}
Object.assign(__ds_scope, { Toast });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/feedback/Toast.jsx", error: String((e && e.message) || e) }); }

// components/forms/Button.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
const VARIANTS = {
  primary: {
    background: 'var(--primary)',
    color: '#fff',
    border: 'none'
  },
  outline: {
    background: 'transparent',
    color: 'var(--primary)',
    border: '2px solid var(--primary)'
  },
  secondary: {
    background: 'var(--card-bg)',
    color: 'var(--text)',
    border: '1px solid var(--border)'
  },
  danger: {
    background: 'transparent',
    color: 'var(--error)',
    border: '2px solid var(--error)'
  }
};

/** Primary action control. Variants: primary | outline | secondary | danger. */
function Button({
  variant = 'primary',
  busy = false,
  disabled = false,
  full = false,
  type = 'button',
  children,
  style = {},
  ...rest
}) {
  const off = disabled || busy;
  const s = {
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
    gap: '0.5rem',
    fontSize: 'var(--text-md)',
    fontWeight: 'var(--weight-bold)',
    fontFamily: 'var(--font-sans)',
    padding: '0.8rem 1.25rem',
    borderRadius: 'var(--radius-lg)',
    lineHeight: 1,
    cursor: off ? 'not-allowed' : 'pointer',
    transition: 'var(--transition-base)',
    width: full ? '100%' : 'auto',
    boxSizing: 'border-box',
    ...(VARIANTS[variant] || VARIANTS.primary),
    ...style,
    // disabled beats busy beats resting; applied last so it wins over style/variant
    opacity: disabled ? 0.5 : busy ? 0.7 : 1,
    filter: disabled ? 'grayscale(40%)' : undefined
  };
  return /*#__PURE__*/React.createElement("button", _extends({
    type: type,
    disabled: off,
    style: s,
    onMouseDown: e => {
      if (!off) e.currentTarget.style.transform = 'scale(0.98)';
    },
    onMouseUp: e => {
      e.currentTarget.style.transform = 'scale(1)';
    },
    onMouseLeave: e => {
      e.currentTarget.style.transform = 'scale(1)';
    }
  }, rest), busy && /*#__PURE__*/React.createElement(Spinner, {
    variant: variant
  }), children);
}
function Spinner({
  variant
}) {
  const dark = variant === 'primary';
  return /*#__PURE__*/React.createElement("span", {
    style: {
      width: 14,
      height: 14,
      borderRadius: '50%',
      border: '2px solid ' + (dark ? 'rgba(255,255,255,0.4)' : 'rgba(0,0,0,0.2)'),
      borderTopColor: dark ? '#fff' : 'currentColor',
      display: 'inline-block',
      animation: 'af-spin 0.8s linear infinite'
    }
  });
}
Object.assign(__ds_scope, { Button });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/forms/Button.jsx", error: String((e && e.message) || e) }); }

// components/forms/IconButton.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** 34x34 bordered square button holding a single 18px icon. Hover = primary wash. */
function IconButton({
  label,
  children,
  style = {},
  ...rest
}) {
  return /*#__PURE__*/React.createElement("button", _extends({
    type: "button",
    title: label,
    "aria-label": label,
    style: {
      background: 'none',
      border: '1px solid var(--border)',
      borderRadius: 'var(--radius-sm)',
      width: 34,
      height: 34,
      padding: 0,
      cursor: 'pointer',
      color: 'var(--text)',
      transition: 'background var(--transition-fast)',
      display: 'inline-flex',
      alignItems: 'center',
      justifyContent: 'center',
      ...style
    },
    onMouseEnter: e => e.currentTarget.style.background = 'var(--primary-alpha)',
    onMouseLeave: e => e.currentTarget.style.background = 'none'
  }, rest), children);
}
Object.assign(__ds_scope, { IconButton });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/forms/IconButton.jsx", error: String((e && e.message) || e) }); }

// components/forms/Input.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Text/number/password field with an optional label. Full-width by default. */
function Input({
  label,
  type = 'text',
  invalid = false,
  feedback,
  style = {},
  id,
  ...rest
}) {
  const inputEl = /*#__PURE__*/React.createElement("input", _extends({
    id: id,
    type: type,
    style: {
      width: '100%',
      boxSizing: 'border-box',
      padding: '0.8rem 1rem',
      borderRadius: 'var(--radius-lg)',
      border: '1px solid ' + (invalid ? 'var(--error)' : 'var(--border)'),
      background: 'var(--bg)',
      color: 'var(--text)',
      fontSize: 'var(--text-md)',
      fontFamily: 'var(--font-sans)',
      marginTop: label ? '0.4rem' : 0,
      accentColor: 'var(--primary)',
      transition: 'border-color var(--transition-base)',
      ...style
    },
    onFocus: e => {
      if (!invalid) e.target.style.borderColor = 'var(--primary)';
    },
    onBlur: e => {
      if (!invalid) e.target.style.borderColor = 'var(--border)';
    }
  }, rest));
  if (!label && !feedback) return inputEl;
  return /*#__PURE__*/React.createElement("label", {
    htmlFor: id,
    style: {
      display: 'block',
      marginBottom: '1rem',
      fontWeight: 'var(--weight-semibold)',
      fontSize: 'var(--text-label)',
      color: 'var(--text)'
    }
  }, label, inputEl, feedback && /*#__PURE__*/React.createElement("span", {
    style: {
      fontSize: 'var(--text-sm)',
      marginTop: '0.25rem',
      display: 'block',
      color: invalid ? 'var(--error)' : 'var(--success)'
    }
  }, feedback));
}
Object.assign(__ds_scope, { Input });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/forms/Input.jsx", error: String((e && e.message) || e) }); }

// components/forms/PasswordInput.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Password field with a show/hide eye toggle on the right edge. */
function PasswordInput({
  label,
  hasValue = false,
  placeholder,
  style = {},
  id,
  ...rest
}) {
  const [show, setShow] = React.useState(false);
  // When a password already exists we show a masked hint (never the real value).
  const ph = hasValue ? '\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022' : placeholder;
  const field = /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'relative',
      marginTop: label ? '0.4rem' : 0
    }
  }, /*#__PURE__*/React.createElement("input", _extends({
    id: id,
    type: show ? 'text' : 'password',
    placeholder: ph,
    style: {
      width: '100%',
      boxSizing: 'border-box',
      padding: '0.8rem 2.5rem 0.8rem 1rem',
      borderRadius: 'var(--radius-lg)',
      border: '1px solid var(--border)',
      background: 'var(--bg)',
      color: 'var(--text)',
      fontSize: 'var(--text-md)',
      fontFamily: 'var(--font-sans)',
      transition: 'border-color var(--transition-base)',
      ...style
    },
    onFocus: e => e.target.style.borderColor = 'var(--primary)',
    onBlur: e => e.target.style.borderColor = 'var(--border)'
  }, rest)), /*#__PURE__*/React.createElement("button", {
    type: "button",
    title: "Toggle visibility",
    onClick: () => setShow(s => !s),
    style: {
      position: 'absolute',
      right: 8,
      top: '50%',
      transform: 'translateY(-50%)',
      background: 'none',
      border: 'none',
      cursor: 'pointer',
      padding: 4,
      color: 'var(--text-muted)',
      display: 'flex',
      alignItems: 'center'
    }
  }, /*#__PURE__*/React.createElement("svg", {
    width: "20",
    height: "20",
    viewBox: "0 0 24 24",
    fill: "none",
    stroke: "currentColor",
    strokeWidth: "2",
    strokeLinecap: "round",
    strokeLinejoin: "round"
  }, show ? /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement("path", {
    d: "M2 2l20 20"
  }), /*#__PURE__*/React.createElement("path", {
    d: "M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"
  })) : /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement("path", {
    d: "M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"
  }), /*#__PURE__*/React.createElement("circle", {
    cx: "12",
    cy: "12",
    r: "3"
  })))));
  if (!label) return field;
  return /*#__PURE__*/React.createElement("label", {
    htmlFor: id,
    style: {
      display: 'block',
      marginBottom: '1rem',
      fontWeight: 'var(--weight-semibold)',
      fontSize: 'var(--text-label)',
      color: 'var(--text)'
    }
  }, label, field);
}
Object.assign(__ds_scope, { PasswordInput });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/forms/PasswordInput.jsx", error: String((e && e.message) || e) }); }

// components/forms/Select.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Native select styled to match the portal inputs. Pass {value,label} options or children. */
function Select({
  label,
  options,
  style = {},
  id,
  children,
  ...rest
}) {
  const chevron = "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='8' viewBox='0 0 12 8' fill='none' stroke='%236c757d' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3E%3Cpolyline points='1 1.5 6 6.5 11 1.5'/%3E%3C/svg%3E";
  const sel = /*#__PURE__*/React.createElement("select", _extends({
    id: id,
    style: {
      width: '100%',
      boxSizing: 'border-box',
      padding: '0.8rem 2.5rem 0.8rem 1rem',
      borderRadius: 'var(--radius-lg)',
      border: '1px solid var(--border)',
      background: 'var(--bg)',
      color: 'var(--text)',
      fontSize: 'var(--text-md)',
      fontFamily: 'var(--font-sans)',
      marginTop: label ? '0.4rem' : 0,
      accentColor: 'var(--primary)',
      appearance: 'none',
      WebkitAppearance: 'none',
      MozAppearance: 'none',
      backgroundImage: `url("${chevron}")`,
      backgroundRepeat: 'no-repeat',
      backgroundPosition: 'right 1rem center',
      cursor: 'pointer',
      ...style
    }
  }, rest), options ? options.map(o => {
    const v = typeof o === 'string' ? o : o.value;
    const l = typeof o === 'string' ? o : o.label;
    return /*#__PURE__*/React.createElement("option", {
      key: v,
      value: v
    }, l);
  }) : children);
  if (!label) return sel;
  return /*#__PURE__*/React.createElement("label", {
    htmlFor: id,
    style: {
      display: 'block',
      marginBottom: '1rem',
      fontWeight: 'var(--weight-semibold)',
      fontSize: 'var(--text-label)',
      color: 'var(--text)'
    }
  }, label, sel);
}
Object.assign(__ds_scope, { Select });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/forms/Select.jsx", error: String((e && e.message) || e) }); }

// components/forms/Textarea.jsx
try { (() => {
function _extends() { return _extends = Object.assign ? Object.assign.bind() : function (n) { for (var e = 1; e < arguments.length; e++) { var t = arguments[e]; for (var r in t) ({}).hasOwnProperty.call(t, r) && (n[r] = t[r]); } return n; }, _extends.apply(null, arguments); }
/** Multi-line input matching the portal's field styling. */
function Textarea({
  label,
  rows = 4,
  style = {},
  id,
  ...rest
}) {
  const ta = /*#__PURE__*/React.createElement("textarea", _extends({
    id: id,
    rows: rows,
    style: {
      width: '100%',
      boxSizing: 'border-box',
      padding: '0.8rem 1rem',
      borderRadius: 'var(--radius-lg)',
      border: '1px solid var(--border)',
      background: 'var(--bg)',
      color: 'var(--text)',
      fontSize: 'var(--text-md)',
      fontFamily: 'var(--font-sans)',
      marginTop: label ? '0.4rem' : 0,
      accentColor: 'var(--primary)',
      resize: 'vertical',
      lineHeight: 'var(--leading-body)',
      transition: 'border-color var(--transition-base)',
      ...style
    },
    onFocus: e => e.target.style.borderColor = 'var(--primary)',
    onBlur: e => e.target.style.borderColor = 'var(--border)'
  }, rest));
  if (!label) return ta;
  return /*#__PURE__*/React.createElement("label", {
    htmlFor: id,
    style: {
      display: 'block',
      marginBottom: '1rem',
      fontWeight: 'var(--weight-semibold)',
      fontSize: 'var(--text-label)',
      color: 'var(--text)'
    }
  }, label, ta);
}
Object.assign(__ds_scope, { Textarea });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/forms/Textarea.jsx", error: String((e && e.message) || e) }); }

// components/forms/ToggleSwitch.jsx
try { (() => {
/** iOS-style on/off switch. Track turns blue when checked; 48x26 knob slides 22px. */
function ToggleSwitch({
  checked = false,
  onChange,
  disabled = false,
  id
}) {
  return /*#__PURE__*/React.createElement("label", {
    htmlFor: id,
    style: {
      position: 'relative',
      display: 'inline-block',
      width: 48,
      height: 26,
      flexShrink: 0,
      opacity: disabled ? 0.5 : 1,
      filter: disabled ? 'grayscale(40%)' : undefined
    }
  }, /*#__PURE__*/React.createElement("input", {
    id: id,
    type: "checkbox",
    checked: checked,
    disabled: disabled,
    onChange: e => onChange && onChange(e.target.checked),
    style: {
      opacity: 0,
      width: 0,
      height: 0,
      position: 'absolute'
    }
  }), /*#__PURE__*/React.createElement("span", {
    style: {
      position: 'absolute',
      inset: 0,
      cursor: disabled ? 'not-allowed' : 'pointer',
      background: checked ? 'var(--primary)' : 'var(--border)',
      borderRadius: 'var(--radius-full)',
      transition: 'background var(--transition-slow)'
    }
  }, /*#__PURE__*/React.createElement("span", {
    style: {
      position: 'absolute',
      height: 20,
      width: 20,
      left: 3,
      bottom: 3,
      background: '#fff',
      borderRadius: '50%',
      boxShadow: 'var(--shadow-toggle-knob)',
      transition: 'transform var(--transition-slow)',
      transform: checked ? 'translateX(22px)' : 'none'
    }
  })));
}
Object.assign(__ds_scope, { ToggleSwitch });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/forms/ToggleSwitch.jsx", error: String((e && e.message) || e) }); }

// components/navigation/Drawer.jsx
try { (() => {
/** Collapsible <details> section. variant "advanced" = yellow expert zone; "info" = neutral. */
function Drawer({
  summary,
  variant = 'info',
  open = false,
  children,
  style = {}
}) {
  const [isOpen, setOpen] = React.useState(open);
  const adv = variant === 'advanced';
  const color = adv ? 'var(--warning)' : 'var(--text-muted)';
  return /*#__PURE__*/React.createElement("article", {
    style: {
      border: '1px solid ' + (adv ? 'var(--warning)' : 'var(--border)'),
      background: adv ? 'rgba(249,168,37,0.04)' : 'transparent',
      borderRadius: adv ? 'var(--radius-2xl)' : 'var(--radius-lg)',
      overflow: 'hidden',
      marginBottom: '0.75rem',
      padding: adv ? '1.25rem' : 0,
      fontFamily: 'var(--font-sans)',
      ...style
    }
  }, /*#__PURE__*/React.createElement("div", {
    onClick: () => setOpen(o => !o),
    style: {
      display: 'flex',
      justifyContent: 'space-between',
      alignItems: 'center',
      cursor: 'pointer',
      userSelect: 'none',
      color,
      fontWeight: adv ? 'var(--weight-bold)' : 'var(--weight-semibold)',
      fontSize: adv ? 'var(--text-card-title)' : 'var(--text-body)',
      padding: adv ? '0.25rem 0 0.75rem' : '0.6rem 0.75rem'
    }
  }, /*#__PURE__*/React.createElement("span", null, summary), /*#__PURE__*/React.createElement("span", {
    style: {
      width: 5,
      height: 5,
      borderRight: '2px solid currentColor',
      borderBottom: '2px solid currentColor',
      transform: 'rotate(' + (isOpen ? 45 : -45) + 'deg)',
      transition: 'transform var(--transition-base)',
      flexShrink: 0
    }
  })), isOpen && /*#__PURE__*/React.createElement("div", {
    style: {
      padding: adv ? 0 : '0 0.75rem 0.5rem',
      fontSize: adv ? undefined : 'var(--text-base-sm)'
    }
  }, children));
}
Object.assign(__ds_scope, { Drawer });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/navigation/Drawer.jsx", error: String((e && e.message) || e) }); }

// components/navigation/PillBar.jsx
try { (() => {
/** Horizontal scrolling sub-navigation of rounded pill buttons. */
function PillBar({
  items = [],
  active,
  onSelect,
  style = {}
}) {
  return /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      gap: 8,
      padding: '0.5rem 0',
      overflowX: 'auto',
      marginBottom: '0.5rem',
      fontFamily: 'var(--font-sans)',
      ...style
    }
  }, items.map(it => {
    const id = typeof it === 'string' ? it : it.id;
    const label = typeof it === 'string' ? it : it.label;
    const on = id === active;
    return /*#__PURE__*/React.createElement("button", {
      key: id,
      onClick: () => onSelect && onSelect(id),
      style: {
        padding: '0.4rem 1rem',
        borderRadius: 'var(--radius-pill)',
        whiteSpace: 'nowrap',
        flexShrink: 0,
        border: '1px solid ' + (on ? 'var(--primary)' : 'var(--border)'),
        background: on ? 'var(--primary)' : 'transparent',
        color: on ? '#fff' : 'var(--text-muted)',
        fontSize: 'var(--text-base-sm)',
        fontWeight: 'var(--weight-semibold)',
        cursor: 'pointer',
        transition: 'all var(--transition-base)'
      }
    }, label);
  }));
}
Object.assign(__ds_scope, { PillBar });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/navigation/PillBar.jsx", error: String((e && e.message) || e) }); }

// components/navigation/ThemeToggle.jsx
try { (() => {
const ICONS = {
  auto: /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement("rect", {
    x: "2",
    y: "3",
    width: "20",
    height: "14",
    rx: "2",
    ry: "2"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "8",
    y1: "21",
    x2: "16",
    y2: "21"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "12",
    y1: "17",
    x2: "12",
    y2: "21"
  })),
  light: /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement("circle", {
    cx: "12",
    cy: "12",
    r: "5"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "12",
    y1: "1",
    x2: "12",
    y2: "3"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "12",
    y1: "21",
    x2: "12",
    y2: "23"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "4.22",
    y1: "4.22",
    x2: "5.64",
    y2: "5.64"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "18.36",
    y1: "18.36",
    x2: "19.78",
    y2: "19.78"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "1",
    y1: "12",
    x2: "3",
    y2: "12"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "21",
    y1: "12",
    x2: "23",
    y2: "12"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "4.22",
    y1: "19.78",
    x2: "5.64",
    y2: "18.36"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "18.36",
    y1: "5.64",
    x2: "19.78",
    y2: "4.22"
  })),
  dark: /*#__PURE__*/React.createElement("path", {
    d: "M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"
  })
};

/** Segmented auto/light/dark theme picker. */
function ThemeToggle({
  value = 'auto',
  onChange,
  style = {}
}) {
  return /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      background: 'var(--card-bg)',
      border: '1px solid var(--border)',
      borderRadius: 'var(--radius-round)',
      padding: 4,
      gap: 4,
      ...style
    }
  }, ['auto', 'light', 'dark'].map(m => {
    const on = m === value;
    return /*#__PURE__*/React.createElement("button", {
      key: m,
      title: m,
      onClick: () => onChange && onChange(m),
      style: {
        width: 32,
        height: 32,
        borderRadius: '50%',
        border: 'none',
        cursor: 'pointer',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        background: on ? 'var(--bg)' : 'transparent',
        color: on ? 'var(--primary)' : 'var(--text-muted)',
        boxShadow: on ? 'var(--shadow-toggle-active)' : 'none',
        transition: 'all var(--transition-base)'
      }
    }, /*#__PURE__*/React.createElement("svg", {
      width: "16",
      height: "16",
      viewBox: "0 0 24 24",
      fill: "none",
      stroke: "currentColor",
      strokeWidth: "2.5",
      strokeLinecap: "round",
      strokeLinejoin: "round"
    }, ICONS[m]));
  }));
}
Object.assign(__ds_scope, { ThemeToggle });
})(); } catch (e) { __ds_ns.__errors.push({ path: "components/navigation/ThemeToggle.jsx", error: String((e && e.message) || e) }); }

// ui_kits/portal/OtaScreen.jsx
try { (() => {
// OtaScreen.jsx — System › OTA: firmware status, update settings, GitHub check, advanced upload.
(function () {
  const NS = window.ESP32AppFoundationDesignSystem_aaa6bc;
  const {
    Card,
    Button,
    Select,
    ToggleSwitch,
    ProgressBar,
    InfoRow,
    Drawer,
    Alert
  } = NS;
  function SettingRow({
    title,
    desc,
    children
  }) {
    return /*#__PURE__*/React.createElement("div", {
      style: {
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        gap: 12,
        padding: '0.6rem 0',
        borderBottom: '1px solid var(--border)'
      }
    }, /*#__PURE__*/React.createElement("div", null, /*#__PURE__*/React.createElement("span", {
      style: {
        fontSize: 'var(--text-body)',
        fontWeight: 'var(--weight-semibold)'
      }
    }, title), /*#__PURE__*/React.createElement("span", {
      style: {
        display: 'block',
        fontSize: 'var(--text-sm)',
        color: 'var(--text-muted)',
        marginTop: 2
      }
    }, desc)), children);
  }
  function OtaScreen() {
    const [enabled, setEnabled] = React.useState(true);
    const [auto, setAuto] = React.useState(true);
    const [channel, setChannel] = React.useState('stable');
    const [phase, setPhase] = React.useState('idle'); // idle | checking | downloading | done
    const [pct, setPct] = React.useState(0);
    const check = () => {
      setPhase('checking');
      setPct(0);
      setTimeout(() => {
        setPhase('downloading');
        let p = 0;
        const t = setInterval(() => {
          p += 12;
          setPct(Math.min(p, 100));
          if (p >= 100) {
            clearInterval(t);
            setPhase('done');
            window.showToast('Update complete — rebooting.', 'success');
          }
        }, 260);
      }, 1200);
    };
    return /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement(Card, {
      header: "Firmware"
    }, /*#__PURE__*/React.createElement(InfoRow, {
      label: "Version",
      value: "v2.4.1"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Partition",
      value: "ota_0"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Board",
      value: "esp32-s3-devkitc",
      last: true
    })), /*#__PURE__*/React.createElement(Card, {
      header: "Settings"
    }, /*#__PURE__*/React.createElement(SettingRow, {
      title: "Enable Firmware Updates",
      desc: "Master switch \u2014 allow firmware to be applied"
    }, /*#__PURE__*/React.createElement(ToggleSwitch, {
      checked: enabled,
      onChange: setEnabled
    })), /*#__PURE__*/React.createElement(SettingRow, {
      title: "Auto-Check for Updates",
      desc: "Periodically poll GitHub for new releases"
    }, /*#__PURE__*/React.createElement(ToggleSwitch, {
      checked: auto,
      onChange: setAuto
    })), /*#__PURE__*/React.createElement(SettingRow, {
      title: "Daily Check Time",
      desc: "Hour of day for automatic checks"
    }, /*#__PURE__*/React.createElement(Select, {
      value: "3",
      options: Array.from({
        length: 24
      }, (_, i) => ({
        value: String(i),
        label: String(i).padStart(2, '0') + ':00'
      })),
      style: {
        width: 'auto',
        minWidth: 90,
        marginTop: 0
      }
    })), /*#__PURE__*/React.createElement("div", {
      style: {
        paddingTop: '0.6rem'
      }
    }, /*#__PURE__*/React.createElement("label", {
      style: {
        fontSize: 'var(--text-body)',
        fontWeight: 'var(--weight-semibold)',
        display: 'block',
        marginBottom: '0.4rem'
      }
    }, "Update Channel"), /*#__PURE__*/React.createElement(Select, {
      value: channel,
      onChange: e => setChannel(e.target.value),
      options: [{
        value: 'stable',
        label: 'Stable'
      }, {
        value: 'beta',
        label: 'Beta'
      }, {
        value: 'dev',
        label: 'Dev'
      }],
      style: {
        marginTop: 0
      }
    }))), /*#__PURE__*/React.createElement(Card, {
      header: "Updates"
    }, phase === 'done' && /*#__PURE__*/React.createElement(Alert, {
      variant: "success"
    }, "Firmware up to date \u2014 v2.4.1 running."), /*#__PURE__*/React.createElement(Button, {
      full: true,
      onClick: check,
      busy: phase === 'checking' || phase === 'downloading',
      disabled: !enabled
    }, phase === 'idle' || phase === 'done' ? 'Check for Updates' : phase === 'checking' ? 'Checking GitHub…' : 'Downloading…'), (phase === 'downloading' || phase === 'done') && /*#__PURE__*/React.createElement("div", {
      style: {
        marginTop: '1rem'
      }
    }, /*#__PURE__*/React.createElement(ProgressBar, {
      label: phase === 'done' ? 'Update complete — rebooting…' : 'Downloading firmware…',
      value: pct,
      max: 100,
      color: "var(--primary)"
    }))), /*#__PURE__*/React.createElement(Drawer, {
      summary: "Advanced",
      variant: "advanced"
    }, /*#__PURE__*/React.createElement("p", {
      style: {
        fontSize: 'var(--text-body)',
        color: 'var(--text-muted)',
        marginTop: 0
      }
    }, "Upload a signed firmware binary directly. Both the compiled ", /*#__PURE__*/React.createElement("code", {
      style: {
        fontFamily: 'var(--font-mono)'
      }
    }, ".bin"), " and its matching ", /*#__PURE__*/React.createElement("code", {
      style: {
        fontFamily: 'var(--font-mono)'
      }
    }, ".sig"), " are required."), /*#__PURE__*/React.createElement("div", {
      style: {
        display: 'flex',
        flexDirection: 'column',
        gap: '0.5rem',
        marginBottom: '0.8rem'
      }
    }, ['Firmware (.bin)', 'Signature (.sig)'].map(l => /*#__PURE__*/React.createElement("div", {
      key: l,
      style: {
        display: 'flex',
        gap: 8,
        alignItems: 'center'
      }
    }, /*#__PURE__*/React.createElement("span", {
      style: {
        padding: '0.4rem 0.8rem',
        background: 'var(--card-bg)',
        border: '1px solid var(--border)',
        borderRadius: 'var(--radius-lg)',
        fontSize: 'var(--text-base-sm)',
        minWidth: 110,
        textAlign: 'center'
      }
    }, l), /*#__PURE__*/React.createElement("span", {
      style: {
        fontSize: 'var(--text-base-sm)',
        color: 'var(--text-muted)'
      }
    }, "No file selected")))), /*#__PURE__*/React.createElement(Button, {
      variant: "secondary",
      full: true,
      disabled: true,
      style: {
        marginBottom: '0.8rem'
      }
    }, "Upload Firmware"), /*#__PURE__*/React.createElement("div", {
      style: {
        borderTop: '1px solid var(--warning)',
        paddingTop: '0.8rem'
      }
    }, /*#__PURE__*/React.createElement(Button, {
      variant: "secondary",
      full: true,
      onClick: () => window.showToast('Rolling back firmware…', 'warning'),
      style: {
        color: 'var(--warning)',
        borderColor: 'var(--warning)'
      }
    }, "Rollback to Previous"))));
  }
  window.OtaScreen = OtaScreen;
})();
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/portal/OtaScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/portal/Portal.jsx
try { (() => {
// Portal.jsx — the ESP32 App Foundation portal shell: top bar, sub-pill bar,
// content router, bottom navigation, theme + credits dialog + toast host.
const NS = window.ESP32AppFoundationDesignSystem_aaa6bc;
const {
  ThemeToggle,
  PillBar,
  Dialog,
  Toast,
  InfoRow
} = NS;
const NAV = [{
  id: 'wifi',
  label: 'WIFI',
  icon: /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement("path", {
    d: "M5 12.55a11 11 0 0 1 14.08 0"
  }), /*#__PURE__*/React.createElement("path", {
    d: "M1.42 9a16 16 0 0 1 21.16 0"
  }), /*#__PURE__*/React.createElement("path", {
    d: "M8.53 16.11a6 6 0 0 1 6.95 0"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "12",
    y1: "20",
    x2: "12.01",
    y2: "20"
  }))
}, {
  id: 'security',
  label: 'SECURITY',
  icon: /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement("rect", {
    x: "3",
    y: "11",
    width: "18",
    height: "11",
    rx: "2",
    ry: "2"
  }), /*#__PURE__*/React.createElement("path", {
    d: "M7 11V7a5 5 0 0 1 10 0v4"
  }))
}, {
  id: 'system',
  label: 'SYSTEM',
  icon: /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement("circle", {
    cx: "12",
    cy: "12",
    r: "3"
  }), /*#__PURE__*/React.createElement("path", {
    d: "M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"
  }))
}];
const PILLS = {
  system: ['Status', 'Time', 'OTA', 'Keys', 'MCP', 'Backup'],
  security: ['Password', 'TLS']
};
function Portal() {
  const [tab, setTab] = React.useState('wifi');
  const [pill, setPill] = React.useState('Status');
  const [theme, setTheme] = React.useState('light');
  const [credits, setCredits] = React.useState(false);
  const [toast, setToast] = React.useState(null);

  // Expose a portal-wide toast so screens can call window.showToast(msg, type)
  React.useEffect(() => {
    window.showToast = (message, type = 'info') => {
      setToast({
        message,
        type
      });
      clearTimeout(window.__tT);
      window.__tT = setTimeout(() => setToast(null), 2600);
    };
  }, []);
  const changeTab = id => {
    setTab(id);
    if (PILLS[id]) setPill(PILLS[id][0]);
  };
  const dataTheme = theme === 'auto' ? undefined : theme;
  let body;
  if (tab === 'wifi') body = /*#__PURE__*/React.createElement(window.WifiScreen, null);else if (tab === 'security') body = /*#__PURE__*/React.createElement(window.SecurityScreen, {
    pill: pill
  });else if (tab === 'system') body = pill === 'OTA' ? /*#__PURE__*/React.createElement(window.OtaScreen, null) : pill === 'Status' ? /*#__PURE__*/React.createElement(window.SystemScreen, null) : /*#__PURE__*/React.createElement(Placeholder, {
    name: pill
  });
  return /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      justifyContent: 'center',
      padding: '24px 0',
      minHeight: '100vh'
    }
  }, /*#__PURE__*/React.createElement("div", {
    "data-theme": dataTheme,
    style: {
      position: 'relative',
      width: 480,
      maxWidth: '100vw',
      height: 820,
      background: 'var(--bg)',
      color: 'var(--text)',
      borderRadius: 28,
      overflow: 'hidden',
      boxShadow: '0 24px 60px rgba(0,0,0,0.28)',
      fontFamily: 'var(--font-sans)',
      display: 'flex',
      flexDirection: 'column',
      transition: 'background var(--transition-slow), color var(--transition-slow)'
    }
  }, /*#__PURE__*/React.createElement("div", {
    style: {
      flex: 1,
      overflowY: 'auto',
      padding: '1rem 1rem 90px'
    }
  }, /*#__PURE__*/React.createElement("header", {
    style: {
      display: 'flex',
      justifyContent: 'space-between',
      alignItems: 'center',
      marginBottom: '1.5rem'
    }
  }, /*#__PURE__*/React.createElement("h2", {
    style: {
      margin: 0,
      fontSize: 'var(--text-h2)',
      fontWeight: 'var(--weight-heavy)',
      letterSpacing: 'var(--tracking-title)',
      color: 'var(--primary)'
    }
  }, "ESP32 App Foundation"), /*#__PURE__*/React.createElement("div", {
    style: {
      display: 'flex',
      gap: 8,
      alignItems: 'center'
    }
  }, /*#__PURE__*/React.createElement("button", {
    onClick: () => setCredits(true),
    title: "Credits & Info",
    style: iconBtn
  }, /*#__PURE__*/React.createElement("svg", {
    viewBox: "0 0 24 24",
    width: "20",
    height: "20",
    fill: "none",
    stroke: "currentColor",
    strokeWidth: "2",
    strokeLinecap: "round",
    strokeLinejoin: "round"
  }, /*#__PURE__*/React.createElement("circle", {
    cx: "12",
    cy: "12",
    r: "10"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "12",
    y1: "16",
    x2: "12",
    y2: "12"
  }), /*#__PURE__*/React.createElement("line", {
    x1: "12",
    y1: "8",
    x2: "12.01",
    y2: "8"
  }))), /*#__PURE__*/React.createElement(ThemeToggle, {
    value: theme,
    onChange: setTheme
  }))), PILLS[tab] && /*#__PURE__*/React.createElement(PillBar, {
    items: PILLS[tab],
    active: pill,
    onSelect: setPill
  }), /*#__PURE__*/React.createElement("div", {
    key: tab + pill,
    style: {
      animation: 'af-slide-up 0.3s ease-out'
    }
  }, body)), /*#__PURE__*/React.createElement("nav", {
    style: {
      position: 'absolute',
      bottom: 0,
      left: 0,
      right: 0,
      height: 'var(--nav-height)',
      background: 'var(--nav-bg)',
      borderTop: '1px solid var(--border)',
      display: 'flex',
      alignItems: 'center',
      padding: '0 5px',
      zIndex: 10
    }
  }, NAV.map(n => {
    const on = n.id === tab;
    return /*#__PURE__*/React.createElement("a", {
      key: n.id,
      href: "#",
      onClick: e => {
        e.preventDefault();
        changeTab(n.id);
      },
      style: {
        flex: 1,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        gap: 4,
        textDecoration: 'none',
        fontSize: 'var(--text-2xs)',
        fontWeight: 'var(--weight-bold)',
        textTransform: 'uppercase',
        letterSpacing: '0.3px',
        color: on ? 'var(--primary)' : 'var(--text-muted)'
      }
    }, /*#__PURE__*/React.createElement("span", {
      style: {
        width: 24,
        height: 24,
        transform: on ? 'translateY(-2px)' : 'none',
        transition: 'var(--transition-base)'
      }
    }, /*#__PURE__*/React.createElement("svg", {
      viewBox: "0 0 24 24",
      width: "24",
      height: "24",
      fill: "none",
      stroke: "currentColor",
      strokeWidth: "2",
      strokeLinecap: "round",
      strokeLinejoin: "round"
    }, n.icon)), n.label);
  })), toast && /*#__PURE__*/React.createElement("div", {
    style: {
      position: 'absolute',
      bottom: 80,
      left: '50%',
      transform: 'translateX(-50%)',
      width: '90%',
      maxWidth: 420,
      zIndex: 40
    }
  }, /*#__PURE__*/React.createElement(Toast, {
    type: toast.type
  }, toast.message)), /*#__PURE__*/React.createElement(Dialog, {
    open: credits,
    onClose: () => setCredits(false)
  }, /*#__PURE__*/React.createElement("h2", {
    style: {
      margin: '0 0 4px',
      color: 'var(--primary)',
      fontSize: '1.4rem'
    }
  }, "Credits & Info"), /*#__PURE__*/React.createElement("p", {
    style: {
      fontSize: 'var(--text-body)',
      color: 'var(--text-muted)',
      marginTop: 0
    }
  }, "A decoupled, contract-based firmware framework for ESP32 devices."), /*#__PURE__*/React.createElement("div", {
    style: {
      marginTop: 12
    }
  }, /*#__PURE__*/React.createElement(InfoRow, {
    label: "Firmware",
    value: "v2.4.1"
  }), /*#__PURE__*/React.createElement(InfoRow, {
    label: "Framework",
    value: "App Foundation"
  }), /*#__PURE__*/React.createElement(InfoRow, {
    label: "Board",
    value: "ESP32-S3-DevKitC"
  }), /*#__PURE__*/React.createElement(InfoRow, {
    label: "License",
    value: "MIT",
    last: true
  })))));
}
const iconBtn = {
  background: 'transparent',
  border: 'none',
  color: 'var(--text-muted)',
  width: 32,
  height: 32,
  borderRadius: '50%',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  cursor: 'pointer'
};
function Placeholder({
  name
}) {
  const {
    Card
  } = NS;
  return /*#__PURE__*/React.createElement(Card, {
    header: name
  }, /*#__PURE__*/React.createElement("p", {
    style: {
      color: 'var(--text-muted)',
      fontSize: 'var(--text-body)',
      margin: 0
    }
  }, "The ", /*#__PURE__*/React.createElement("strong", null, name), " module contributes its UI here at runtime. This demo focuses on WiFi, Status, OTA and Security."));
}
window.Portal = Portal;
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/portal/Portal.jsx", error: String((e && e.message) || e) }); }

// ui_kits/portal/SecurityScreen.jsx
try { (() => {
// SecurityScreen.jsx — Security › Password (portal login) + TLS (certificate) sub-pills.
(function () {
  const NS = window.ESP32AppFoundationDesignSystem_aaa6bc;
  const {
    Card,
    Button,
    PasswordInput,
    InfoRow,
    Badge,
    Alert
  } = NS;
  function SecurityScreen({
    pill = 'Password'
  }) {
    if (pill === 'TLS') return /*#__PURE__*/React.createElement(TlsPanel, null);
    return /*#__PURE__*/React.createElement(PasswordPanel, null);
  }
  function PasswordPanel() {
    const [cur, setCur] = React.useState('');
    const [next, setNext] = React.useState('');
    return /*#__PURE__*/React.createElement(Card, {
      header: "Portal Password"
    }, /*#__PURE__*/React.createElement(Alert, {
      variant: "info"
    }, "Set a password to protect the configuration portal. Leave blank to keep the portal open on the local network."), /*#__PURE__*/React.createElement(PasswordInput, {
      label: "Current Password",
      placeholder: "Enter current password",
      value: cur,
      onChange: e => setCur(e.target.value)
    }), /*#__PURE__*/React.createElement(PasswordInput, {
      label: "New Password",
      placeholder: "Choose a new password",
      value: next,
      onChange: e => setNext(e.target.value),
      autoComplete: "new-password"
    }), /*#__PURE__*/React.createElement(Button, {
      full: true,
      onClick: () => window.showToast('Password updated.', 'success'),
      disabled: !next
    }, "Update Password"));
  }
  function TlsPanel() {
    return /*#__PURE__*/React.createElement(Card, {
      header: "TLS & Certificates"
    }, /*#__PURE__*/React.createElement("div", {
      style: {
        display: 'flex',
        alignItems: 'center',
        gap: 10,
        marginBottom: '0.75rem'
      }
    }, /*#__PURE__*/React.createElement("span", {
      style: {
        fontSize: 'var(--text-body)',
        fontWeight: 'var(--weight-semibold)'
      }
    }, "Certificate Bundle"), /*#__PURE__*/React.createElement(Badge, {
      variant: "ok"
    }, "Loaded")), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Bundle",
      value: "DEFAULT_CMN"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Root CAs",
      value: "41"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "TLS Memory",
      value: "~26 KB"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Verification",
      value: /*#__PURE__*/React.createElement("span", {
        style: {
          color: 'var(--success)'
        }
      }, "Full chain"),
      last: true
    }), /*#__PURE__*/React.createElement("div", {
      style: {
        marginTop: '1rem'
      }
    }, /*#__PURE__*/React.createElement(Button, {
      variant: "outline",
      full: true,
      onClick: () => window.showToast('Certificate bundle refreshed.', 'success')
    }, "Refresh Bundle")), /*#__PURE__*/React.createElement(Button, {
      variant: "danger",
      full: true,
      onClick: () => window.showToast('TLS verification disabled — insecure.', 'warning'),
      style: {
        marginTop: '0.75rem'
      }
    }, "Disable TLS Verification"));
  }
  window.SecurityScreen = SecurityScreen;
})();
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/portal/SecurityScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/portal/SystemScreen.jsx
try { (() => {
// SystemScreen.jsx — System › Status: device info, memory/storage meters, network, danger zone.
(function () {
  const NS = window.ESP32AppFoundationDesignSystem_aaa6bc;
  const {
    Card,
    Button,
    InfoRow,
    ProgressBar,
    SignalBars
  } = NS;
  function SystemScreen() {
    const [restartArmed, setRestartArmed] = React.useState(false);
    const [showReset, setShowReset] = React.useState(false);
    const [resetText, setResetText] = React.useState('');
    const restart = () => {
      if (!restartArmed) {
        setRestartArmed(true);
        setTimeout(() => setRestartArmed(false), 3000);
        return;
      }
      window.showToast('Restarting device…', 'info');
      setRestartArmed(false);
    };
    return /*#__PURE__*/React.createElement(React.Fragment, null, /*#__PURE__*/React.createElement(Card, {
      header: "Device Information"
    }, /*#__PURE__*/React.createElement(InfoRow, {
      label: "Firmware",
      value: "v2.4.1"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Build",
      value: "2026-08-02\xB7a1f9c"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Chip",
      value: "ESP32-S3 rev 0"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Cores",
      value: "2"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Flash",
      value: "8.0 MB"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Uptime",
      value: "2d 5h 13m"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Temperature",
      value: /*#__PURE__*/React.createElement("span", {
        style: {
          color: 'var(--success)'
        }
      }, "48.2\xB0C"),
      last: true
    })), /*#__PURE__*/React.createElement(Card, {
      header: "Memory & Storage"
    }, /*#__PURE__*/React.createElement("div", {
      style: sectionLabel
    }, "Internal DRAM"), /*#__PURE__*/React.createElement(ProgressBar, {
      label: "Free Heap",
      value: 142,
      max: 320,
      valueText: "142 KB / 320 KB (44%)"
    }), /*#__PURE__*/React.createElement(ProgressBar, {
      label: "Min Free (Watermark)",
      value: 98,
      max: 320,
      valueText: "98 KB / 320 KB (31%)"
    }), /*#__PURE__*/React.createElement("div", {
      style: {
        ...sectionLabel,
        marginTop: '0.75rem'
      }
    }, "PSRAM"), /*#__PURE__*/React.createElement(ProgressBar, {
      label: "PSRAM Free",
      value: 6800,
      max: 8192,
      valueText: "6.6 MB / 8.0 MB (83%)"
    }), /*#__PURE__*/React.createElement("div", {
      style: {
        ...sectionLabel,
        marginTop: '0.75rem'
      }
    }, "Storage"), /*#__PURE__*/React.createElement(ProgressBar, {
      label: "Storage Free",
      value: 1150,
      max: 1536,
      valueText: "1.1 MB / 1.5 MB (75%)"
    })), /*#__PURE__*/React.createElement(Card, {
      header: "Network"
    }, /*#__PURE__*/React.createElement(InfoRow, {
      label: "Mode",
      value: "Station"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "SSID",
      value: "workshop-2.4G"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "IP Address",
      value: "192.168.1.42"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "MAC",
      value: "A4:CF:12:98:2B:0E"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Signal",
      value: /*#__PURE__*/React.createElement("span", {
        style: {
          display: 'inline-flex',
          gap: 6,
          alignItems: 'center'
        }
      }, "-47 dBm Excellent ", /*#__PURE__*/React.createElement(SignalBars, {
        rssi: -47
      })),
      last: true
    })), /*#__PURE__*/React.createElement(Card, {
      header: "Danger Zone",
      variant: "danger"
    }, /*#__PURE__*/React.createElement("div", {
      style: {
        display: 'flex',
        gap: 10
      }
    }, /*#__PURE__*/React.createElement(Button, {
      variant: "secondary",
      full: true,
      onClick: restart,
      style: restartArmed ? {
        background: 'var(--error)',
        color: '#fff',
        borderColor: 'var(--error)'
      } : {}
    }, restartArmed ? 'Confirm Restart?' : 'Restart Device'), /*#__PURE__*/React.createElement(Button, {
      variant: "secondary",
      full: true,
      onClick: () => setShowReset(s => !s),
      style: {
        color: 'var(--error)',
        borderColor: 'var(--error)'
      }
    }, "Factory Reset")), showReset && /*#__PURE__*/React.createElement("div", {
      style: {
        marginTop: '1rem'
      }
    }, /*#__PURE__*/React.createElement("p", {
      style: {
        fontSize: 'var(--text-base-sm)',
        color: 'var(--error)',
        marginBottom: '0.5rem'
      }
    }, "Type ", /*#__PURE__*/React.createElement("strong", null, "RESET"), " to confirm. This erases all configuration."), /*#__PURE__*/React.createElement("input", {
      value: resetText,
      onChange: e => setResetText(e.target.value),
      placeholder: "Type RESET\u2026",
      style: {
        width: '100%',
        boxSizing: 'border-box',
        padding: '0.8rem 1rem',
        borderRadius: 'var(--radius-lg)',
        border: '1px solid var(--border)',
        background: 'var(--bg)',
        color: 'var(--text)',
        marginBottom: '0.5rem'
      }
    }), /*#__PURE__*/React.createElement(Button, {
      full: true,
      disabled: resetText !== 'RESET',
      onClick: () => {
        window.showToast('Factory reset in progress…', 'error');
        setShowReset(false);
        setResetText('');
      },
      style: {
        background: 'var(--error)'
      }
    }, "Confirm Factory Reset"))), /*#__PURE__*/React.createElement("div", {
      style: {
        textAlign: 'center',
        padding: '0.5rem',
        fontSize: 'var(--text-xs)',
        color: 'var(--text-muted)'
      }
    }, "Auto-refresh: active"));
  }
  const sectionLabel = {
    fontSize: 'var(--text-sm)',
    color: 'var(--text-muted)',
    textTransform: 'uppercase',
    letterSpacing: '0.05em',
    marginBottom: '0.4rem'
  };
  window.SystemScreen = SystemScreen;
})();
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/portal/SystemScreen.jsx", error: String((e && e.message) || e) }); }

// ui_kits/portal/WifiScreen.jsx
try { (() => {
// WifiScreen.jsx — WiFi settings: live status, network scan, credentials, diagnostics.
(function () {
  const NS = window.ESP32AppFoundationDesignSystem_aaa6bc;
  const {
    Card,
    Button,
    Input,
    PasswordInput,
    SignalBars,
    Badge,
    Drawer,
    InfoRow
  } = NS;
  const FAKE = [{
    ssid: 'workshop-2.4G',
    rssi: -47,
    secure: true
  }, {
    ssid: 'FRITZ!Box 7590',
    rssi: -58,
    secure: true
  }, {
    ssid: 'ESP-Guest',
    rssi: -66,
    secure: false
  }, {
    ssid: 'BTHub6-QM9K',
    rssi: -74,
    secure: true
  }, {
    ssid: 'Sonos_Living',
    rssi: -81,
    secure: true
  }];
  function WifiScreen() {
    const [scanning, setScanning] = React.useState(false);
    const [nets, setNets] = React.useState([]);
    const [ssid, setSsid] = React.useState('workshop-2.4G');
    const [pass, setPass] = React.useState('');
    const [result, setResult] = React.useState(null);
    const [forgetArmed, setForgetArmed] = React.useState(false);
    const scan = () => {
      setScanning(true);
      setNets([]);
      setTimeout(() => {
        setNets(FAKE);
        setScanning(false);
      }, 1200);
    };
    React.useEffect(scan, []);
    const test = () => {
      setResult({
        kind: 'pending',
        text: 'Testing connection…'
      });
      setTimeout(() => setResult({
        kind: 'ok',
        text: '✓ Connected successfully. IP: 192.168.1.42'
      }), 1400);
    };
    const save = () => window.showToast('Credentials saved — device reconnecting.', 'success');
    const forget = () => {
      if (!forgetArmed) {
        setForgetArmed(true);
        setTimeout(() => setForgetArmed(false), 3000);
        return;
      }
      window.showToast('WiFi credentials erased.', 'warning');
      setForgetArmed(false);
    };
    return /*#__PURE__*/React.createElement(Card, {
      header: "WiFi Settings"
    }, /*#__PURE__*/React.createElement("div", {
      style: {
        display: 'flex',
        alignItems: 'center',
        gap: '0.75rem',
        padding: '0.75rem 1rem',
        background: 'var(--card-bg)',
        border: '1px solid var(--border)',
        borderRadius: 'var(--radius-xl)',
        marginBottom: '1rem',
        flexWrap: 'wrap'
      }
    }, /*#__PURE__*/React.createElement(Badge, {
      variant: "mode",
      style: {
        background: 'var(--success)',
        color: '#fff'
      }
    }, "STA"), /*#__PURE__*/React.createElement("span", {
      style: {
        fontWeight: 'var(--weight-semibold)',
        fontSize: 'var(--text-body)'
      }
    }, "workshop-2.4G"), /*#__PURE__*/React.createElement("span", {
      style: {
        color: 'var(--text-muted)',
        fontSize: 'var(--text-base-sm)'
      }
    }, "192.168.1.42"), /*#__PURE__*/React.createElement("span", {
      style: {
        marginLeft: 'auto',
        display: 'inline-flex',
        gap: 6,
        alignItems: 'center',
        fontSize: 'var(--text-sm)',
        color: 'var(--text-muted)'
      }
    }, /*#__PURE__*/React.createElement(SignalBars, {
      rssi: -47
    }), "-47 dBm")), /*#__PURE__*/React.createElement("div", {
      style: {
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        marginBottom: '0.5rem'
      }
    }, /*#__PURE__*/React.createElement("label", {
      style: {
        margin: 0,
        fontWeight: 'var(--weight-bold)',
        fontSize: 'var(--text-body)'
      }
    }, "Available Networks"), /*#__PURE__*/React.createElement(Button, {
      variant: "primary",
      onClick: scan,
      busy: scanning,
      style: {
        padding: '0.4rem 0.8rem',
        fontSize: 'var(--text-base-sm)',
        minWidth: 70
      }
    }, scanning ? 'Scanning' : 'Scan')), /*#__PURE__*/React.createElement("div", {
      style: {
        maxHeight: 200,
        overflowY: 'auto',
        border: '1px solid var(--border)',
        borderRadius: 'var(--radius-lg)',
        marginBottom: '0.75rem'
      }
    }, nets.length === 0 ? /*#__PURE__*/React.createElement("div", {
      style: {
        padding: '1rem',
        textAlign: 'center',
        color: 'var(--text-muted)',
        fontSize: 'var(--text-body)'
      }
    }, scanning ? 'Scanning…' : 'Press Scan to discover networks') : nets.map((n, i) => {
      const on = n.ssid === ssid;
      return /*#__PURE__*/React.createElement("div", {
        key: n.ssid,
        onClick: () => {
          setSsid(n.ssid);
          setPass('');
          setResult(null);
        },
        style: {
          display: 'flex',
          alignItems: 'center',
          gap: '0.5rem',
          padding: '0.6rem 0.75rem',
          cursor: 'pointer',
          borderBottom: i < nets.length - 1 ? '1px solid var(--border)' : 'none',
          background: on ? 'rgba(33,150,243,0.08)' : 'transparent'
        }
      }, /*#__PURE__*/React.createElement("span", {
        style: {
          flex: 1,
          fontSize: 'var(--text-body)',
          fontWeight: on ? 'var(--weight-bold)' : 'var(--weight-medium)'
        }
      }, n.ssid), /*#__PURE__*/React.createElement(SignalBars, {
        rssi: n.rssi
      }), n.secure && /*#__PURE__*/React.createElement("svg", {
        width: "14",
        height: "14",
        viewBox: "0 0 24 24",
        fill: "none",
        stroke: "currentColor",
        strokeWidth: "2",
        style: {
          opacity: 0.6
        }
      }, /*#__PURE__*/React.createElement("rect", {
        x: "3",
        y: "11",
        width: "18",
        height: "11",
        rx: "2",
        ry: "2"
      }), /*#__PURE__*/React.createElement("path", {
        d: "M7 11V7a5 5 0 0 1 10 0v4"
      })), /*#__PURE__*/React.createElement("span", {
        style: {
          fontSize: 'var(--text-xs)',
          color: 'var(--text-muted)',
          minWidth: 48,
          textAlign: 'right'
        }
      }, n.rssi, " dB"));
    })), /*#__PURE__*/React.createElement(Input, {
      label: "SSID",
      value: ssid,
      onChange: e => setSsid(e.target.value)
    }), /*#__PURE__*/React.createElement(PasswordInput, {
      label: "Password",
      placeholder: "Network password",
      value: pass,
      onChange: e => setPass(e.target.value),
      autoComplete: "new-password"
    }), /*#__PURE__*/React.createElement("div", {
      style: {
        display: 'flex',
        gap: '0.5rem',
        marginBottom: '0.75rem'
      }
    }, /*#__PURE__*/React.createElement(Button, {
      variant: "outline",
      full: true,
      onClick: test,
      busy: result && result.kind === 'pending'
    }, "Test Connection"), /*#__PURE__*/React.createElement(Button, {
      full: true,
      onClick: save
    }, "Save & Connect")), result && result.kind !== 'pending' && /*#__PURE__*/React.createElement("div", {
      style: {
        fontSize: 'var(--text-base-sm)',
        marginBottom: '0.75rem',
        color: result.kind === 'ok' ? 'var(--success)' : 'var(--error)'
      }
    }, result.text), /*#__PURE__*/React.createElement(Drawer, {
      summary: /*#__PURE__*/React.createElement("span", {
        style: {
          display: 'inline-flex',
          alignItems: 'center',
          gap: 8
        }
      }, /*#__PURE__*/React.createElement("span", {
        style: {
          width: 8,
          height: 8,
          borderRadius: '50%',
          background: 'var(--success)'
        }
      }), "Network Diagnostics"),
      variant: "info"
    }, /*#__PURE__*/React.createElement(InfoRow, {
      label: "Mode",
      value: "STA"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "MAC Address",
      value: "A4:CF:12:98:2B:0E"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Channel",
      value: "6"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "Gateway",
      value: "192.168.1.1"
    }), /*#__PURE__*/React.createElement(InfoRow, {
      label: "RSSI",
      value: "-47 dBm (Excellent)",
      last: true
    })), /*#__PURE__*/React.createElement(Button, {
      variant: "danger",
      full: true,
      onClick: forget,
      style: {
        marginTop: '0.5rem',
        padding: '0.7rem',
        fontSize: 'var(--text-body)',
        ...(forgetArmed ? {
          background: 'var(--error)',
          color: '#fff'
        } : {})
      }
    }, forgetArmed ? '⚠ Confirm Forget? (3s)' : 'Forget Network'));
  }
  window.WifiScreen = WifiScreen;
})();
})(); } catch (e) { __ds_ns.__errors.push({ path: "ui_kits/portal/WifiScreen.jsx", error: String((e && e.message) || e) }); }

__ds_ns.Badge = __ds_scope.Badge;

__ds_ns.Card = __ds_scope.Card;

__ds_ns.DiagTable = __ds_scope.DiagTable;

__ds_ns.InfoRow = __ds_scope.InfoRow;

__ds_ns.ProgressBar = __ds_scope.ProgressBar;

__ds_ns.SignalBars = __ds_scope.SignalBars;

__ds_ns.StatusDot = __ds_scope.StatusDot;

__ds_ns.TerminalLog = __ds_scope.TerminalLog;

__ds_ns.Alert = __ds_scope.Alert;

__ds_ns.Callout = __ds_scope.Callout;

__ds_ns.Dialog = __ds_scope.Dialog;

__ds_ns.LoginCard = __ds_scope.LoginCard;

__ds_ns.RebootOverlay = __ds_scope.RebootOverlay;

__ds_ns.Toast = __ds_scope.Toast;

__ds_ns.Button = __ds_scope.Button;

__ds_ns.IconButton = __ds_scope.IconButton;

__ds_ns.Input = __ds_scope.Input;

__ds_ns.PasswordInput = __ds_scope.PasswordInput;

__ds_ns.Select = __ds_scope.Select;

__ds_ns.Textarea = __ds_scope.Textarea;

__ds_ns.ToggleSwitch = __ds_scope.ToggleSwitch;

__ds_ns.Drawer = __ds_scope.Drawer;

__ds_ns.PillBar = __ds_scope.PillBar;

__ds_ns.ThemeToggle = __ds_scope.ThemeToggle;

})();
