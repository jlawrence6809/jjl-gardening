#!/bin/bash

# Perform curl command with full path to the file
curl -F "image=@.pio/build/nodemcu-32s/firmware.bin" http://sunroom2.local/update