// Portal.jsx — the ESP32 App Foundation portal shell: top bar, sub-pill bar,
// content router, bottom navigation, theme + credits dialog + toast host.
const NS = window.ESP32AppFoundationDesignSystem_aaa6bc;
const { ThemeToggle, PillBar, Dialog, Toast, InfoRow } = NS;

const NAV = [
  { id: 'wifi', label: 'WIFI', icon: <><path d="M5 12.55a11 11 0 0 1 14.08 0"/><path d="M1.42 9a16 16 0 0 1 21.16 0"/><path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><line x1="12" y1="20" x2="12.01" y2="20"/></> },
  { id: 'security', label: 'SECURITY', icon: <><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></> },
  { id: 'system', label: 'SYSTEM', icon: <><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06A1.65 1.65 0 0 0 4.68 15a1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06A1.65 1.65 0 0 0 9 4.68a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"/></> },
];
const PILLS = { system: ['Status', 'Time', 'OTA', 'Keys', 'MCP', 'Backup'], security: ['Password', 'TLS'] };

function Portal() {
  const [tab, setTab] = React.useState('wifi');
  const [pill, setPill] = React.useState('Status');
  const [theme, setTheme] = React.useState('light');
  const [credits, setCredits] = React.useState(false);
  const [toast, setToast] = React.useState(null);

  // Expose a portal-wide toast so screens can call window.showToast(msg, type)
  React.useEffect(() => {
    window.showToast = (message, type = 'info') => {
      setToast({ message, type });
      clearTimeout(window.__tT);
      window.__tT = setTimeout(() => setToast(null), 2600);
    };
  }, []);

  const changeTab = (id) => { setTab(id); if (PILLS[id]) setPill(PILLS[id][0]); };
  const dataTheme = theme === 'auto' ? undefined : theme;

  let body;
  if (tab === 'wifi') body = <window.WifiScreen />;
  else if (tab === 'security') body = <window.SecurityScreen pill={pill} />;
  else if (tab === 'system') body = pill === 'OTA' ? <window.OtaScreen /> : pill === 'Status' ? <window.SystemScreen /> : <Placeholder name={pill} />;

  return (
    <div style={{ display: 'flex', justifyContent: 'center', padding: '24px 0', minHeight: '100vh' }}>
      <div data-theme={dataTheme} style={{
        position: 'relative', width: 480, maxWidth: '100vw', height: 820, background: 'var(--bg)',
        color: 'var(--text)', borderRadius: 28, overflow: 'hidden', boxShadow: '0 24px 60px rgba(0,0,0,0.28)',
        fontFamily: 'var(--font-sans)', display: 'flex', flexDirection: 'column',
        transition: 'background var(--transition-slow), color var(--transition-slow)',
      }}>
        {/* scrollable content */}
        <div style={{ flex: 1, overflowY: 'auto', padding: '1rem 1rem 90px' }}>
          <header style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '1.5rem' }}>
            <h2 style={{ margin: 0, fontSize: 'var(--text-h2)', fontWeight: 'var(--weight-heavy)', letterSpacing: 'var(--tracking-title)', color: 'var(--primary)' }}>
              ESP32 App Foundation
            </h2>
            <div style={{ display: 'flex', gap: 8, alignItems: 'center' }}>
              <button onClick={() => setCredits(true)} title="Credits & Info" style={iconBtn}>
                <svg viewBox="0 0 24 24" width="20" height="20" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><circle cx="12" cy="12" r="10"/><line x1="12" y1="16" x2="12" y2="12"/><line x1="12" y1="8" x2="12.01" y2="8"/></svg>
              </button>
              <ThemeToggle value={theme} onChange={setTheme} />
            </div>
          </header>

          {PILLS[tab] && <PillBar items={PILLS[tab]} active={pill} onSelect={setPill} />}
          <div key={tab + pill} style={{ animation: 'af-slide-up 0.3s ease-out' }}>{body}</div>
        </div>

        {/* bottom nav */}
        <nav style={{
          position: 'absolute', bottom: 0, left: 0, right: 0, height: 'var(--nav-height)',
          background: 'var(--nav-bg)', borderTop: '1px solid var(--border)', display: 'flex',
          alignItems: 'center', padding: '0 5px', zIndex: 10,
        }}>
          {NAV.map(n => {
            const on = n.id === tab;
            return (
              <a key={n.id} href="#" onClick={e => { e.preventDefault(); changeTab(n.id); }} style={{
                flex: 1, display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 4,
                textDecoration: 'none', fontSize: 'var(--text-2xs)', fontWeight: 'var(--weight-bold)',
                textTransform: 'uppercase', letterSpacing: '0.3px',
                color: on ? 'var(--primary)' : 'var(--text-muted)',
              }}>
                <span style={{ width: 24, height: 24, transform: on ? 'translateY(-2px)' : 'none', transition: 'var(--transition-base)' }}>
                  <svg viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">{n.icon}</svg>
                </span>
                {n.label}
              </a>
            );
          })}
        </nav>

        {/* toast host */}
        {toast && (
          <div style={{ position: 'absolute', bottom: 80, left: '50%', transform: 'translateX(-50%)', width: '90%', maxWidth: 420, zIndex: 40 }}>
            <Toast type={toast.type}>{toast.message}</Toast>
          </div>
        )}

        {/* credits modal */}
        <Dialog open={credits} onClose={() => setCredits(false)}>
          <h2 style={{ margin: '0 0 4px', color: 'var(--primary)', fontSize: '1.4rem' }}>Credits &amp; Info</h2>
          <p style={{ fontSize: 'var(--text-body)', color: 'var(--text-muted)', marginTop: 0 }}>A decoupled, contract-based firmware framework for ESP32 devices.</p>
          <div style={{ marginTop: 12 }}>
            <InfoRow label="Firmware" value="v2.4.1" />
            <InfoRow label="Framework" value="App Foundation" />
            <InfoRow label="Board" value="ESP32-S3-DevKitC" />
            <InfoRow label="License" value="MIT" last />
          </div>
        </Dialog>
      </div>
    </div>
  );
}

const iconBtn = {
  background: 'transparent', border: 'none', color: 'var(--text-muted)', width: 32, height: 32,
  borderRadius: '50%', display: 'flex', alignItems: 'center', justifyContent: 'center', cursor: 'pointer',
};

function Placeholder({ name }) {
  const { Card } = NS;
  return <Card header={name}><p style={{ color: 'var(--text-muted)', fontSize: 'var(--text-body)', margin: 0 }}>The <strong>{name}</strong> module contributes its UI here at runtime. This demo focuses on WiFi, Status, OTA and Security.</p></Card>;
}

window.Portal = Portal;
