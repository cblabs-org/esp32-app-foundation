import serial, time
s = serial.Serial('/dev/cu.usbserial-5AA70033491', 115200, timeout=2)
print("Waiting for boot...")
start = time.time()
out = ""
while time.time() - start < 10:
    chunk = s.read(1000)
    if chunk:
        text = chunk.decode('utf-8', 'replace')
        out += text
        if "Boot complete" in text:
            break
print(out)
