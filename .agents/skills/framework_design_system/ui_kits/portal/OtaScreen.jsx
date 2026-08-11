// OtaScreen.jsx — System › OTA: firmware status, update settings, GitHub check, advanced upload.
(function () {
  const NS = window.ESP32AppFoundationDesignSystem_aaa6bc;
  const { Card, Button, Select, ToggleSwitch, ProgressBar, InfoRow, Drawer, Alert } = NS;

  function SettingRow({ title, desc, children }) {
    return (
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', gap: 12, padding: '0.6rem 0', borderBottom: '1px solid var(--border)' }}>
        <div>
          <span style={{ fontSize: 'var(--text-body)', fontWeight: 'var(--weight-semibold)' }}>{title}</span>
          <span style={{ display: 'block', fontSize: 'var(--text-sm)', color: 'var(--text-muted)', marginTop: 2 }}>{desc}</span>
        </div>
        {children}
      </div>
    );
  }

  function OtaScreen() {
    const [enabled, setEnabled] = React.useState(true);
    const [auto, setAuto] = React.useState(true);
    const [channel, setChannel] = React.useState('stable');
    const [phase, setPhase] = React.useState('idle'); // idle | checking | downloading | done
    const [pct, setPct] = React.useState(0);

    const check = () => {
      setPhase('checking'); setPct(0);
      setTimeout(() => {
        setPhase('downloading');
        let p = 0;
        const t = setInterval(() => {
          p += 12; setPct(Math.min(p, 100));
          if (p >= 100) { clearInterval(t); setPhase('done'); window.showToast('Update complete — rebooting.', 'success'); }
        }, 260);
      }, 1200);
    };

    return (
      <React.Fragment>
        <Card header="Firmware">
          <InfoRow label="Version" value="v2.4.1" />
          <InfoRow label="Partition" value="ota_0" />
          <InfoRow label="Board" value="esp32-s3-devkitc" last />
        </Card>

        <Card header="Settings">
          <SettingRow title="Enable Firmware Updates" desc="Master switch — allow firmware to be applied">
            <ToggleSwitch checked={enabled} onChange={setEnabled} />
          </SettingRow>
          <SettingRow title="Auto-Check for Updates" desc="Periodically poll GitHub for new releases">
            <ToggleSwitch checked={auto} onChange={setAuto} />
          </SettingRow>
          <SettingRow title="Daily Check Time" desc="Hour of day for automatic checks">
            <Select value="3" options={Array.from({ length: 24 }, (_, i) => ({ value: String(i), label: String(i).padStart(2, '0') + ':00' }))} style={{ width: 'auto', minWidth: 90, marginTop: 0 }} />
          </SettingRow>
          <div style={{ paddingTop: '0.6rem' }}>
            <label style={{ fontSize: 'var(--text-body)', fontWeight: 'var(--weight-semibold)', display: 'block', marginBottom: '0.4rem' }}>Update Channel</label>
            <Select value={channel} onChange={e => setChannel(e.target.value)} options={[{ value: 'stable', label: 'Stable' }, { value: 'beta', label: 'Beta' }, { value: 'dev', label: 'Dev' }]} style={{ marginTop: 0 }} />
          </div>
        </Card>

        <Card header="Updates">
          {phase === 'done' && <Alert variant="success">Firmware up to date — v2.4.1 running.</Alert>}
          <Button full onClick={check} busy={phase === 'checking' || phase === 'downloading'} disabled={!enabled}>
            {phase === 'idle' || phase === 'done' ? 'Check for Updates' : phase === 'checking' ? 'Checking GitHub…' : 'Downloading…'}
          </Button>
          {(phase === 'downloading' || phase === 'done') && (
            <div style={{ marginTop: '1rem' }}>
              <ProgressBar label={phase === 'done' ? 'Update complete — rebooting…' : 'Downloading firmware…'} value={pct} max={100} color="var(--primary)" />
            </div>
          )}
        </Card>

        <Drawer summary="Advanced" variant="advanced">
          <p style={{ fontSize: 'var(--text-body)', color: 'var(--text-muted)', marginTop: 0 }}>Upload a signed firmware binary directly. Both the compiled <code style={{ fontFamily: 'var(--font-mono)' }}>.bin</code> and its matching <code style={{ fontFamily: 'var(--font-mono)' }}>.sig</code> are required.</p>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem', marginBottom: '0.8rem' }}>
            {['Firmware (.bin)', 'Signature (.sig)'].map(l => (
              <div key={l} style={{ display: 'flex', gap: 8, alignItems: 'center' }}>
                <span style={{ padding: '0.4rem 0.8rem', background: 'var(--card-bg)', border: '1px solid var(--border)', borderRadius: 'var(--radius-lg)', fontSize: 'var(--text-base-sm)', minWidth: 110, textAlign: 'center' }}>{l}</span>
                <span style={{ fontSize: 'var(--text-base-sm)', color: 'var(--text-muted)' }}>No file selected</span>
              </div>
            ))}
          </div>
          <Button variant="secondary" full disabled style={{ marginBottom: '0.8rem' }}>Upload Firmware</Button>
          <div style={{ borderTop: '1px solid var(--warning)', paddingTop: '0.8rem' }}>
            <Button variant="secondary" full onClick={() => window.showToast('Rolling back firmware…', 'warning')} style={{ color: 'var(--warning)', borderColor: 'var(--warning)' }}>Rollback to Previous</Button>
          </div>
        </Drawer>
      </React.Fragment>
    );
  }

  window.OtaScreen = OtaScreen;
})();
