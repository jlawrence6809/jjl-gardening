# Home automation platform

This controls various devices in my home like the barn and the sunroom(s). Checkout the definitions.h file to see #defines to change around devices.

Checkout the extra_scripts.py file to see how it is built/compiled. Basically:

- cd preact && npm run build
- pio run

Make sure to check the board you're using in platformio.ini. There is nodemcu and the esp32-s3 board (external library install) currently.
