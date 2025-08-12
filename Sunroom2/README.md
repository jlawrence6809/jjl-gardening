# Sunroom2 - Home Automation Platform

A comprehensive ESP32-based home automation system for controlling various devices in home environments like sunrooms and barns. This project combines hardware control with a web-based interface for easy management of environmental conditions and automated systems.

## Features

- **Multi-Device Support**: Controls up to 13 relays for various devices (fans, heaters, lights, etc.)
- **Environmental Monitoring**: Temperature and humidity sensing with DS18B20 and AHTX0 sensors
- **Web Interface**: Built with Preact for a modern, responsive control panel
- **Automation Rules**: Custom rule engine for automated device control based on environmental conditions
- **WiFi Configuration**: Easy WiFi setup through web interface
- **OTA Updates**: Over-the-air firmware updates via web interface
- **Multiple Board Support**: Compatible with NodeMCU-32S and ESP32-S3 DevKit
- **Real-time Monitoring**: Live sensor data and device status updates

## Hardware Requirements

### Supported Boards
- **NodeMCU-32S** (currently used for sunroom and barn)
- **ESP32-S3 DevKit** (with external library installation)

### Sensors & Components
- DS18B20 temperature sensor
- AHTX0 temperature/humidity sensor
- Photo sensor for light detection
- Light switch input
- Multiple relay modules (up to 13 relays)
- Various controlled devices (fans, heaters, lights, etc.)

## Project Structure

```
Sunroom2/
├── src/                    # Arduino source code
│   ├── main.cpp           # Main application entry point
│   ├── definitions.h      # Hardware configuration and constants
│   ├── servers.cpp        # Web server and API endpoints
│   ├── rule_helpers.cpp   # Automation rule engine
│   ├── temperatureMoisture.cpp  # Sensor management
│   └── ...                # Additional helper modules
├── preact/                # Web interface
│   ├── src/               # Preact application source
│   ├── build/             # Built web assets
│   └── package.json       # Node.js dependencies
├── platformio.ini         # PlatformIO configuration
├── extra_script.py        # Build automation script
└── uploadViaEndpoint.sh   # OTA upload script
```

## Setup Instructions

### Prerequisites
- PlatformIO IDE or CLI
- Node.js and npm (for web interface)
- Arduino IDE (optional, for debugging)

### Installation

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd jjl-gardening/Sunroom2
   ```

2. **Configure hardware settings**
   Edit `src/definitions.h` to match your hardware setup:
   ```cpp
   // Choose your board
   #define ESP32_NODE_MCU  // or #define ESP32_S3
   
   // Choose your environment
   #define SUNROOM         // or #define BARN
   ```

3. **Install web interface dependencies**
   ```bash
   cd preact
   npm install
   ```

4. **Build and upload**
   ```bash
   # Build web interface and firmware
   cd preact && npm run build
   cd .. && pio run
   
   # Or use the custom build target
   pio run -t preact_build
   ```

### Configuration

#### Board Selection
In `src/definitions.h`, uncomment the appropriate board:
```cpp
// #define ESP32_S3
#define ESP32_NODE_MCU
```

#### Environment Configuration
Choose your deployment environment:
```cpp
// #define SUNROOM
#define BARN
```

#### Relay Configuration
Configure relay pins and behavior for your setup:
```cpp
#ifdef BARN
constexpr int RELAY_COUNT = 13;
constexpr int RELAY_PINS[RELAY_COUNT] = {15, 2, 4, 16, 17, 5, 18, 19, 32, 33, 25, 26, 27};
constexpr bool RELAY_IS_INVERTED[RELAY_COUNT] = {true, true, true, true, true, true, true, true, false, false, false, false, false};
#endif
```

## Web Interface

The system includes a modern web interface built with Preact that provides:
- Real-time device status monitoring
- Manual relay control
- Automation rule configuration
- WiFi settings management
- System status and diagnostics

### Accessing the Interface
1. Connect to the device's WiFi network
2. Navigate to the device's IP address in your browser
3. Configure WiFi settings if needed
4. Access the control panel

## API Endpoints

The system provides REST API endpoints for integration:

### Device Control
- `GET /api/relays` - Get current relay states
- `POST /api/relays` - Set relay states
- `GET /api/sensors` - Get sensor readings
- `GET /api/status` - Get system status

### Configuration
- `POST /api/wifi` - Update WiFi credentials
- `POST /api/rules` - Update automation rules
- `GET /api/config` - Get current configuration

### System
- `POST /update` - OTA firmware update
- `GET /api/restart` - Restart the device

## Automation Rules

The system supports custom automation rules for controlling devices based on environmental conditions. Rules can be configured through the web interface and support:

- Temperature-based triggers
- Humidity-based triggers
- Time-based schedules
- Light level conditions
- Manual override capabilities

### Rule Examples
```
// Turn on fan when temperature > 25°C
if temperature > 25 then relay_0 = ON

// Turn on heater when temperature < 18°C
if temperature < 18 then relay_1 = ON

// Turn on lights at 6 AM
if time = 06:00 then relay_2 = ON
```

## OTA Updates

The system supports over-the-air firmware updates:

1. Build the firmware: `pio run`
2. Use the upload script: `./uploadViaEndpoint.sh <device-ip>`
3. Or upload manually through the web interface

## Development

### Adding New Features
1. Add new functionality in the appropriate source file
2. Update `definitions.h` if new constants are needed
3. Add API endpoints in `servers.cpp` if needed
4. Update the web interface in the `preact/` directory

### Debugging
- Serial output is available at 115200 baud
- Use PlatformIO's serial monitor: `pio device monitor`
- Check the web interface's system status page

## Troubleshooting

### Common Issues
- **WiFi Connection**: Ensure WiFi credentials are correctly configured
- **Sensor Readings**: Check sensor connections and pin assignments
- **Relay Control**: Verify relay pin configurations and inversion settings
- **OTA Updates**: Ensure sufficient flash memory and stable connection

### Debug Information
The system provides debug information through:
- Serial console output
- Web interface status page
- API endpoints for system diagnostics

## License

This project is for personal use. Please respect the original author's intentions.

## Contributing

This is a personal project, but suggestions and improvements are welcome through appropriate channels.

---

For more detailed information about specific components, see the individual source files and the `preact/` directory for web interface documentation.
