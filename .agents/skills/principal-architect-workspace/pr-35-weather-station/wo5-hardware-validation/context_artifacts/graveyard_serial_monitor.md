# Graveyard: Serial Monitor Flashing & Polling

## Issue: Empty Monitor Logs & Download Mode
While running WO5, the device failed to produce any serial logs over the MCP monitor after being externally interrupted. 

**Attempt 1:** A simple Python script was executed to manually pulse DTR/RTS (`dtr=False`, `rts=True` -> `dtr=True`, `rts=False`). 
**Failure:** This exact sequence sets `IO0=LOW` right before `EN=HIGH`, plunging the ESP32 into **Download Mode** instead of application space. The device became completely silent over UART.

**Attempt 2:** A PTY-wrapped `pio run -t monitor` execution.
**Failure:** Pseudo-terminals (PTYs) do not support the specific ioctls required by `pyserial` (`termios.error: (25, 'Inappropriate ioctl for device')`). The native command failed to execute.

## Resolution
The correct python script for a standard hardware reset on macOS is:
```python
import serial
import time
port = "/dev/cu.usbserial-5AA70033491"
ser = serial.Serial(port)
ser.setDTR(False) # IO0 HIGH
ser.setRTS(True)  # EN LOW
time.sleep(0.2)
ser.setDTR(False) # IO0 HIGH
ser.setRTS(False) # EN HIGH
```
Alternatively, native serial operations must be delegated directly to the `mcp_platformio_start_monitor` which has the privileges and TTY environment to assert pins correctly.

## Issue: MCP Policy Block
We attempted to autonomously run `upload_firmware` to resolve the empty logs (assuming corrupted firmware), but the MCP agent blocked it via the `flash_requires_approval` internal policy. The correct path when encountering silent serial logs on macOS is to verify the hardware wasn't accidentally dumped into Download Mode before assuming the firmware itself is missing.
