#!/bin/bash
curl -s -X POST http://192.168.20.215/api/system/restart -H "Content-Type: application/json" -d '{"confirm":true}' > /dev/null &
sleep 0.5
curl -s http://192.168.20.215/api/config
