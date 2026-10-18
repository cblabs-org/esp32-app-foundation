# Graveyard: Upload Debugging Dead Ends

## Red Herring 1: TLS Handshake Failures
Early investigation focused on TLS issues (`-0x7780` errors) as the cause of upload failures. This was a distraction — the TLS errors occurred when accessing via HTTPS with a self-signed cert, but the upload bug existed identically over HTTP.

## Red Herring 2: Browser-Specific Issues
Tested in multiple browsers (Chrome, different Chrome profiles). The upload hung in all of them. This suggested a server-side issue, not a browser quirk — but consumed debugging time.

## Red Herring 3: ApiQueue Serialisation
Suspected the `ApiQueue` in `app.js` was blocking the upload. While the queue IS a constraint (any stalled request blocks the pipeline), the upload was correctly being enqueued and dispatched. The fix did bypass the queue (using XHR directly), but this was for progress tracking, not because the queue was the root cause.

## Red Herring 4: Socket Exhaustion / MAX_UPLOAD_SIZE
Investigated whether ESP32 httpd was running out of sockets or rejecting the 1.3 MB binary. Confirmed `MAX_UPLOAD_SIZE` was 2 MB (sufficient) and curl could complete the upload (proving the socket layer worked).

## Key Insight: curl Worked But Got No Response
The breakthrough was that `curl -v -X POST -F "firmware=@..." -F "signature=@..." http://192.168.20.215/api/ota/upload` uploaded all 1251 KB successfully but **timed out waiting for a response** (exit code 28). This proved:
- The TCP connection was fine
- Data transfer was fine
- The server was silently dropping the data (not processing it)
- No serial output appeared from the upload handler

This pointed directly at the handler's field-matching logic silently ignoring both fields.

## Dead End: First Fix Attempt (filename matching only)
The first fix corrected the filename matching but didn't address field ordering. Testing with the browser showed `SECURITY ABORT — no signature provided` because firmware was still being verified before the signature arrived. Required the second fix (onRequest lifecycle split).
