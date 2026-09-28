// SecurityScreen.jsx — Security › Password (portal login) + TLS (certificate) sub-pills.
(function () {
  const NS = window.ESP32AppFoundationDesignSystem_aaa6bc;
  const { Card, Button, PasswordInput, InfoRow, Badge, Alert } = NS;

  function SecurityScreen({ pill = 'Password' }) {
    if (pill === 'TLS') return <TlsPanel />;
    return <PasswordPanel />;
  }

  function PasswordPanel() {
    const [cur, setCur] = React.useState('');
    const [next, setNext] = React.useState('');
    return (
      <Card header="Portal Password">
        <Alert variant="info">Set a password to protect the configuration portal. Leave blank to keep the portal open on the local network.</Alert>
        <PasswordInput label="Current Password" placeholder="Enter current password" value={cur} onChange={e => setCur(e.target.value)} />
        <PasswordInput label="New Password" placeholder="Choose a new password" value={next} onChange={e => setNext(e.target.value)} autoComplete="new-password" />
        <Button full onClick={() => window.showToast('Password updated.', 'success')} disabled={!next}>Update Password</Button>
      </Card>
    );
  }

  function TlsPanel() {
    return (
      <Card header="TLS &amp; Certificates">
        <div style={{ display: 'flex', alignItems: 'center', gap: 10, marginBottom: '0.75rem' }}>
          <span style={{ fontSize: 'var(--text-body)', fontWeight: 'var(--weight-semibold)' }}>Certificate Bundle</span>
          <Badge variant="ok">Loaded</Badge>
        </div>
        <InfoRow label="Bundle" value="DEFAULT_CMN" />
        <InfoRow label="Root CAs" value="41" />
        <InfoRow label="TLS Memory" value="~26 KB" />
        <InfoRow label="Verification" value={<span style={{ color: 'var(--success)' }}>Full chain</span>} last />
        <div style={{ marginTop: '1rem' }}>
          <Button variant="outline" full onClick={() => window.showToast('Certificate bundle refreshed.', 'success')}>Refresh Bundle</Button>
        </div>
        <Button variant="danger" full onClick={() => window.showToast('TLS verification disabled — insecure.', 'warning')} style={{ marginTop: '0.75rem' }}>Disable TLS Verification</Button>
      </Card>
    );
  }

  window.SecurityScreen = SecurityScreen;
})();
