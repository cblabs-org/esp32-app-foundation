# ADR: Captive Portal Detection

## Context
DNS hijack alone (`dnsServer->start(53, "*", apIP)`) is not sufficient for modern phones to trigger the "Sign in to network" popup. Phones probe specific URLs and expect specific HTTP responses.

## Decision
Added explicit captive portal detection endpoints that redirect to `/`:

| Endpoint | Platform | Why redirect works |
|---|---|---|
| `/generate_204` | Android | Expects 204; getting 302 redirect triggers popup |
| `/hotspot-detect.html` | iOS / macOS | Apple CNA checks this; redirect opens portal |
| `/connecttest.txt` | Windows | NCSI connectivity check |
| `/ncsi.txt` | Windows | Secondary NCSI endpoint |
| `/success.txt` | Firefox | Firefox connectivity check |

Plus a catch-all `onNotFound` handler that redirects ANY unmatched URL to `/`. This catches:
- DNS-hijacked requests to arbitrary domains (e.g., `http://clients3.google.com/generate_204`)
- Any other connectivity check we haven't explicitly handled

## Consequences
- Phones connecting to the AP should get an automatic "Sign in to network" popup
- The popup opens the portal's root page directly
- All routes work in both AP and STA mode (in STA mode the captive portal endpoints are harmless — they just redirect to the portal)
