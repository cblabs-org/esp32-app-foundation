import serial
import time
import sys

port = '/dev/cu.usbmodem1101'
try:
    ser = serial.Serial(port, 115200, timeout=1)
    
    # Toggle DTR/RTS to reset the ESP32
    ser.setDTR(False)
    ser.setRTS(True)
    time.sleep(0.1)
    ser.setDTR(True)
    ser.setRTS(False)
    time.sleep(0.1)
    
    start_time = time.time()
    while time.time() - start_time < 15:
        line = ser.readline()
        if line:
            print(line.decode('utf-8', errors='replace'), end='')
            
    ser.close()
except Exception as e:
    print(f"Error: {e}")
