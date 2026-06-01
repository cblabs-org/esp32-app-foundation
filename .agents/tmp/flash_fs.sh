#!/bin/bash
for i in $(seq 1 10); do
  echo "Attempt $i..."
  if ~/.platformio/penv/bin/python -m esptool --chip esp32s3 --port /dev/cu.usbmodem1101 --before usb-reset write-flash 0x3F0000 /Users/mcneillm/Documents/Projects/esp32-app-foundation/.pio/build/esp32s3nano/littlefs.bin 2>&1; then
    echo "SUCCESS on attempt $i"
    exit 0
  fi
  sleep 1
done
echo "FAILED after 10 attempts"
exit 1
