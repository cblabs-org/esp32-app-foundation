// SystemScreen.jsx — System › Status: device info, memory/storage meters, network, danger zone.
(function () {
  const NS = window.ESP32AppFoundationDesignSystem_aaa6bc;
  const { Card, Button, InfoRow, ProgressBar, SignalBars } = NS;

  function SystemScreen() {
    const [restartArmed, setRestartArmed] = React.useState(false);
    const [showReset, setShowReset] = React.useState(false);
    const [resetText, setResetText] = React.useState('');

    const restart = () => {
      if (!restartArmed) { setRestartArmed(true); setTimeout(() => setRestartArmed(false), 3000); return; }
      window.showToast('Restarting device…', 'info'); setRestartArmed(false);
    };

    return (
      <React.Fragment>
        <Card header="Device Information">
          <InfoRow label="Firmware" value="v2.4.1" />
          <InfoRow label="Build" value="2026-08-02·a1f9c" />
          <InfoRow label="Chip" value="ESP32-S3 rev 0" />
          <InfoRow label="Cores" value="2" />
          <InfoRow label="Flash" value="8.0 MB" />
          <InfoRow label="Uptime" value="2d 5h 13m" />
          <InfoRow label="Temperature" value={<span style={{ color: 'var(--success)' }}>48.2°C</span>} last />
        </Card>

        <Card header="Memory & Storage">
          <div style={sectionLabel}>Internal DRAM</div>
          <ProgressBar label="Free Heap" value={142} max={320} valueText="142 KB / 320 KB (44%)" />
          <ProgressBar label="Min Free (Watermark)" value={98} max={320} valueText="98 KB / 320 KB (31%)" />
          <div style={{ ...sectionLabel, marginTop: '0.75rem' }}>PSRAM</div>
          <ProgressBar label="PSRAM Free" value={6800} max={8192} valueText="6.6 MB / 8.0 MB (83%)" />
          <div style={{ ...sectionLabel, marginTop: '0.75rem' }}>Storage</div>
          <ProgressBar label="Storage Free" value={1150} max={1536} valueText="1.1 MB / 1.5 MB (75%)" />
        </Card>

        <Card header="Network">
          <InfoRow label="Mode" value="Station" />
          <InfoRow label="SSID" value="workshop-2.4G" />
          <InfoRow label="IP Address" value="192.168.1.42" />
          <InfoRow label="MAC" value="A4:CF:12:98:2B:0E" />
          <InfoRow label="Signal" value={<span style={{ display: 'inline-flex', gap: 6, alignItems: 'center' }}>-47 dBm Excellent <SignalBars rssi={-47} /></span>} last />
        </Card>

        <Card header="Danger Zone" variant="danger">
          <div style={{ display: 'flex', gap: 10 }}>
            <Button variant="secondary" full onClick={restart} style={restartArmed ? { background: 'var(--error)', color: '#fff', borderColor: 'var(--error)' } : {}}>
              {restartArmed ? 'Confirm Restart?' : 'Restart Device'}
            </Button>
            <Button variant="secondary" full onClick={() => setShowReset(s => !s)} style={{ color: 'var(--error)', borderColor: 'var(--error)' }}>Factory Reset</Button>
          </div>
          {showReset && (
            <div style={{ marginTop: '1rem' }}>
              <p style={{ fontSize: 'var(--text-base-sm)', color: 'var(--error)', marginBottom: '0.5rem' }}>Type <strong>RESET</strong> to confirm. This erases all configuration.</p>
              <input value={resetText} onChange={e => setResetText(e.target.value)} placeholder="Type RESET…" style={{ width: '100%', boxSizing: 'border-box', padding: '0.8rem 1rem', borderRadius: 'var(--radius-lg)', border: '1px solid var(--border)', background: 'var(--bg)', color: 'var(--text)', marginBottom: '0.5rem' }} />
              <Button full disabled={resetText !== 'RESET'} onClick={() => { window.showToast('Factory reset in progress…', 'error'); setShowReset(false); setResetText(''); }} style={{ background: 'var(--error)' }}>Confirm Factory Reset</Button>
            </div>
          )}
        </Card>

        <div style={{ textAlign: 'center', padding: '0.5rem', fontSize: 'var(--text-xs)', color: 'var(--text-muted)' }}>Auto-refresh: active</div>
      </React.Fragment>
    );
  }

  const sectionLabel = { fontSize: 'var(--text-sm)', color: 'var(--text-muted)', textTransform: 'uppercase', letterSpacing: '0.05em', marginBottom: '0.4rem' };

  window.SystemScreen = SystemScreen;
})();
