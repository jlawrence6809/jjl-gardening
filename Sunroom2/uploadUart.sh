#!/bin/bash

# Usage: ./monitor.sh [env]
# Example: ./monitor.sh nodemcu-32s

if [[ "$1" == "nodemcu-32s" || "$1" == "esp32-s3-devkit" ]]; then
  ENV="$1"
elif [[ -z "$1" ]]; then
  ENV="esp32-s3-devkit"
else
  echo "Error: ENV must be 'nodemcu-32s' or 'esp32-s3-devkit'"
  exit 1
fi

pio run -t upload -e "$ENV"