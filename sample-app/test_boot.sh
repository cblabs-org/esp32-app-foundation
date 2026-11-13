#!/bin/bash
DEVICE_IP="${DEVICE_IP:?set DEVICE_IP to the device's address}"
curl -s -X POST "http://${DEVICE_IP}/api/system/restart" -H "Content-Type: application/json" -d '{"confirm":true}' > /dev/null &
sleep 0.5
curl -s "http://${DEVICE_IP}/api/config"
