Portal action button — blue primary with outline, secondary (neutral) and danger (destructive) variants.

```jsx
<Button onClick={save}>Save & Connect</Button>
<Button variant="outline">Test Connection</Button>
<Button variant="danger" full>Forget Network</Button>
<Button busy>Scanning…</Button>
```

Buttons use imperative labels in Title Case ("Check for Updates"). Danger variant is for reversible-but-serious actions; pair it with the armed-confirmation pattern for irreversible ones.
