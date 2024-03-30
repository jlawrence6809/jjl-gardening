#!/bin/bash

# Check if domain name is passed as an argument
if [ -z "$1" ]
then
  echo "No domain name supplied. Usage: ./uploadViaEndpoint.sh <domain_name>"
  exit 1
fi

# Perform curl command with full path to the file
curl -F "image=@.pio/build/nodemcu-32s/firmware.bin" http://$1.local/update