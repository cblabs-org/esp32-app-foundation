// WifiScreen.jsx — WiFi settings: live status, network scan, credentials, diagnostics.
(function () {
  const NS = window.ESP32AppFoundationDesignSystem_aaa6bc;
  const { Card, Button, Input, PasswordInput, SignalBars, Badge, Drawer, InfoRow } = NS;

  const FAKE = [
    { ssid: 'workshop-2.4G', rssi: -47, secure: true },
    { ssid: 'FRITZ!Box 7590', rssi: -58, secure: true },
    { ssid: 'ESP-Guest', rssi: -66, secure: false },
    { ssid: 'BTHub6-QM9K', rssi: -74, secure: true },
    { ssid: 'Sonos_Living', rssi: -81, secure: true },
  ];

  function WifiScreen() {
    const [scanning, setScanning] = React.useState(false);
    const [nets, setNets] = React.useState([]);
    const [ssid, setSsid] = React.useState('workshop-2.4G');
    const [pass, setPass] = React.useState('');
    const [result, setResult] = React.useState(null);
    const [forgetArmed, setForgetArmed] = React.useState(false);

    const scan = () => {
      setScanning(true); setNets([]);
      setTimeout(() => { setNets(FAKE); setScanning(false); }, 1200);
    };
    React.useEffect(scan, []);

    const test = () => {
      setResult({ kind: 'pending', text: 'Testing connection…' });
      setTimeout(() => setResult({ kind: 'ok', text: '✓ Connected successfully. IP: 192.168.1.42' }), 1400);
    };
    const save = () => window.showToast('Credentials saved — device reconnecting.', 'success');
    const forget = () => {
      if (!forgetArmed) { setForgetArmed(true); setTimeout(() => setForgetArmed(false), 3000); return; }
      window.showToast('WiFi credentials erased.', 'warning'); setForgetArmed(false);
    };

    return (
      <Card header="WiFi Settings">
        {/* status header */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '0.75rem', padding: '0.75rem 1rem', background: 'var(--card-bg)', border: '1px solid var(--border)', borderRadius: 'var(--radius-xl)', marginBottom: '1rem', flexWrap: 'wrap' }}>
          <Badge variant="mode" style={{ background: 'var(--success)', color: '#fff' }}>STA</Badge>
          <span style={{ fontWeight: 'var(--weight-semibold)', fontSize: 'var(--text-body)' }}>workshop-2.4G</span>
          <span style={{ color: 'var(--text-muted)', fontSize: 'var(--text-base-sm)' }}>192.168.1.42</span>
          <span style={{ marginLeft: 'auto', display: 'inline-flex', gap: 6, alignItems: 'center', fontSize: 'var(--text-sm)', color: 'var(--text-muted)' }}>
            <SignalBars rssi={-47} />-47 dBm
          </span>
        </div>

        {/* networks */}
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '0.5rem' }}>
          <label style={{ margin: 0, fontWeight: 'var(--weight-bold)', fontSize: 'var(--text-body)' }}>Available Networks</label>
          <Button variant="primary" onClick={scan} busy={scanning} style={{ padding: '0.4rem 0.8rem', fontSize: 'var(--text-base-sm)', minWidth: 70 }}>{scanning ? 'Scanning' : 'Scan'}</Button>
        </div>
        <div style={{ maxHeight: 200, overflowY: 'auto', border: '1px solid var(--border)', borderRadius: 'var(--radius-lg)', marginBottom: '0.75rem' }}>
          {nets.length === 0
            ? <div style={{ padding: '1rem', textAlign: 'center', color: 'var(--text-muted)', fontSize: 'var(--text-body)' }}>{scanning ? 'Scanning…' : 'Press Scan to discover networks'}</div>
            : nets.map((n, i) => {
              const on = n.ssid === ssid;
              return (
                <div key={n.ssid} onClick={() => { setSsid(n.ssid); setPass(''); setResult(null); }} style={{
                  display: 'flex', alignItems: 'center', gap: '0.5rem', padding: '0.6rem 0.75rem', cursor: 'pointer',
                  borderBottom: i < nets.length - 1 ? '1px solid var(--border)' : 'none',
                  background: on ? 'rgba(33,150,243,0.08)' : 'transparent',
                }}>
                  <span style={{ flex: 1, fontSize: 'var(--text-body)', fontWeight: on ? 'var(--weight-bold)' : 'var(--weight-medium)' }}>{n.ssid}</span>
                  <SignalBars rssi={n.rssi} />
                  {n.secure && <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" style={{ opacity: 0.6 }}><rect x="3" y="11" width="18" height="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>}
                  <span style={{ fontSize: 'var(--text-xs)', color: 'var(--text-muted)', minWidth: 48, textAlign: 'right' }}>{n.rssi} dB</span>
                </div>
              );
            })}
        </div>

        <Input label="SSID" value={ssid} onChange={e => setSsid(e.target.value)} />
        <PasswordInput label="Password" placeholder="Network password" value={pass} onChange={e => setPass(e.target.value)} autoComplete="new-password" />

        <div style={{ display: 'flex', gap: '0.5rem', marginBottom: '0.75rem' }}>
          <Button variant="outline" full onClick={test} busy={result && result.kind === 'pending'}>Test Connection</Button>
          <Button full onClick={save}>Save &amp; Connect</Button>
        </div>
        {result && result.kind !== 'pending' && (
          <div style={{ fontSize: 'var(--text-base-sm)', marginBottom: '0.75rem', color: result.kind === 'ok' ? 'var(--success)' : 'var(--error)' }}>{result.text}</div>
        )}

        <Drawer summary={<span style={{ display: 'inline-flex', alignItems: 'center', gap: 8 }}><span style={{ width: 8, height: 8, borderRadius: '50%', background: 'var(--success)' }} />Network Diagnostics</span>} variant="info">
          <InfoRow label="Mode" value="STA" />
          <InfoRow label="MAC Address" value="A4:CF:12:98:2B:0E" />
          <InfoRow label="Channel" value="6" />
          <InfoRow label="Gateway" value="192.168.1.1" />
          <InfoRow label="RSSI" value="-47 dBm (Excellent)" last />
        </Drawer>

        <Button variant="danger" full onClick={forget} style={{ marginTop: '0.5rem', padding: '0.7rem', fontSize: 'var(--text-body)', ...(forgetArmed ? { background: 'var(--error)', color: '#fff' } : {}) }}>
          {forgetArmed ? '⚠ Confirm Forget? (3s)' : 'Forget Network'}
        </Button>
      </Card>
    );
  }

  window.WifiScreen = WifiScreen;
})();
