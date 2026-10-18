# ADR: Risk-Floor Channel Scanning Logic

## Decision

Implement risk-floor scanning in the deploy script (Python) rather than on-device. The device simply reads its configured channel — the deploy script ensures each channel always has the best available version.

## Channel Stability Ordering

```
dev(0) < beta(1) < rc(2) < stable(3)
```

## Rule

A version with stability level S is a valid candidate for any channel with floor F where S >= F.

| Deploying | Valid for channels |
|-----------|-------------------|
| `v1.0.0` (stable=3) | dev, beta, rc, stable |
| `v1.0.0-rc.1` (rc=2) | dev, beta, rc |
| `v1.0.0-beta.1` (beta=1) | dev, beta |
| `v1.0.0-dev.5` (dev=0) | dev only |

## Implementation

`FirmwareVersion` class provides `__lt__`, `__gt__`, etc. via a sort key tuple:
```python
(major, minor, patch, channel_numeric, pre_num)
```

This matches the firmware's `SemVer.hpp` comparison logic exactly. Channel numeric values: dev=0, beta=1, rc=2, stable=3.

`apply_risk_floor()` iterates all channels in the manifest. For each channel, if the new version's stability >= the channel floor AND the new version > the existing version, it upgrades that channel entry (version, assets, securityVersion).

## Why deploy-side, not device-side

The device should never need to scan multiple channels or compare versions across channels. This is a deployment concern. The deploy script has full visibility of all channels and can make the optimal decision once, at deploy time, rather than having every device do it on every check.
