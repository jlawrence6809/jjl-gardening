# Propagation Box Controller

Controls a fan, lights, and heat mat using data gathered from a temperature/humidity sensor as well as a temperature probe you place within some soil in any of the pots within the propagation box.

## Hardware
* ESP32
* AHT20 Temperature/Humidity Sensor
* DS18B20 Temperature Probe
* Mosfets
* 12V Power Supply
* 5V Buck Converter
* 12V Fan
* 12V Heat Mat
* 12V LED Strip


## Software
* PlatformIO
* Arduino

## Setup
1. Clone this repo
2. Install PlatformIO
3. Open the project in PlatformIO
4. Upload the code to your ESP32


## Usage
1. After first upload of code, the ESP32 will start in AP mode.
2. Search for a wifi signal named that starts with "esp_XXXX", the Xs being random hex characters which comprise a part of the esp32's built in unique id.
3. Connect to the wifi signal and navigate to http://esp_XXXX.local/ in your browser (replace XXXX with the wifi ap name).
4. At the bottom of the page there is a form to enter your wifi credentials. Enter them and submit the form.
5. The ESP32 will reboot and connect to your wifi network.
6. Navigate to http://esp_XXXX.local/ again and you should see the current temperature and humidity readings and you can set the desired temperature and humidity, which will be saved to the ESP32's flash memory. These values will be used to control the fan, heat mat, and lights.
7. Note: You do not have to provide your wifi credentials, but you will have to connect to the esp's AP each time you wish to monitor the box or change the desired temperature/humidity values.