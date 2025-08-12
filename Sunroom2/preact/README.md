# Sunroom2 Web Interface

A modern, responsive web interface built with Preact for controlling the ESP32-based home automation system. This interface provides real-time device control, sensor monitoring, and automation rule configuration for managing environmental systems in sunrooms, barns, and other home automation applications.

## Overview

This web interface serves as the primary user interface for the Sunroom2 home automation platform. It's designed to be lightweight and efficient, running directly on the ESP32 microcontroller while providing a modern web experience.

## Features

- **Real-time Device Control**: Manual control of up to 13 relays with instant feedback
- **Sensor Monitoring**: Live display of temperature, humidity, light levels, and switch states
- **Automation Rules**: Visual rule editor with custom domain-specific language (DSL)
- **System Information**: Device status, uptime, and diagnostics
- **WiFi Configuration**: Easy network setup and management
- **Responsive Design**: Works on desktop and mobile devices
- **Efficient Asset Management**: Optimized for ESP32 memory constraints

## Technology Stack

- **Framework**: Preact (3KB React alternative)
- **Build Tool**: Preact CLI with custom Webpack configuration
- **Language**: TypeScript for type safety
- **Bundling**: Custom ESP32 asset optimization
- **Compression**: Gzip compression for minimal memory footprint

## Project Structure

```
preact/
├── src/
│   ├── index.tsx          # Main application component
│   ├── RuleParser.ts      # Automation rule validation engine
│   └── style.css         # Global styles
├── esp/
│   ├── esp-build-plugin.js    # Custom Webpack plugin for ESP32
│   └── static_files_h.ejs     # Template for C header generation
├── build/                 # Built assets (generated)
│   ├── bundle.*.js       # Compiled JavaScript
│   ├── bundle.*.css      # Compiled CSS
│   ├── index.html        # Main HTML file
│   └── static_files.h    # C header with embedded assets
├── HomeAutomation.g4     # ANTLR grammar (experimental)
├── package.json          # Dependencies and scripts
├── preact.config.js      # Build configuration
└── tsconfig.json         # TypeScript configuration
```

## Getting Started

### Prerequisites

- Node.js 14+ and npm
- ESP32 development environment (PlatformIO)

### Installation

1. **Install dependencies**:
   ```bash
   cd preact
   npm install
   ```

2. **Development mode**:
   ```bash
   npm run dev
   ```
   This starts a development server at `http://localhost:8080` with hot reloading.

3. **Build for production**:
   ```bash
   npm run build
   ```
   This creates optimized assets in the `build/` directory and generates `static_files.h` for the ESP32.

### Available Scripts

- `npm run build` - Full build (TypeScript + Preact + ESP32 assets)
- `npm run build:preact` - Build only the Preact application
- `npm run build:ts` - Compile TypeScript only
- `npm run dev` - Start development server
- `npm run serve` - Serve built files locally
- `npm run lint` - Run ESLint
- `npm run antlr` - Generate ANTLR parser (experimental)

## Configuration

### Device Configuration

The interface automatically detects whether it's running on a Sunroom or Barn configuration based on the `RELAY_COUNT` constant:

```typescript
// Sunroom configuration
export const RELAY_COUNT = 8 as const;

// Barn configuration  
export const RELAY_COUNT = 13 as const;
```

### Build Configuration

The build process is optimized for ESP32 deployment through `preact.config.js`:

- **No Service Worker**: Disabled for ESP32 compatibility
- **No ESM modules**: Uses traditional bundling
- **Custom plugin**: Generates C header files with embedded assets
- **Asset exclusion**: Removes unnecessary files from ESP32 build

## Components Overview

### Main Application (`index.tsx`)

The root component that orchestrates the entire interface:

- **App**: Main container with dynamic title based on device hostname
- **RelayControls**: Manual relay control with real-time status
- **GlobalInfo**: System information (chip ID, temperature, uptime)
- **SensorInfo**: Environmental sensor readings
- **WifiForm**: Network configuration
- **Restart**: System restart functionality

### Relay Control System

**Features**:
- Visual toggle switches with state indication
- Three-state control: Off (0), On (1), Don't Care (2)
- Automation button for rule configuration
- Real-time status updates

**State Management**:
```typescript
type RelayStateValue = {
  force: RelaySubState;  // Manual override (0=off, 1=on, 2=auto)
  auto: RelaySubState;   // Automation state
};
```

### Automation Dialog

**Features**:
- Inline label editing
- Rule syntax validation
- Real-time parsing feedback
- JSON-based rule storage

**Rule Format**:
```json
["IF", 
  ["GT", "temperature", 25], 
  ["SET", "relay_0", true], 
  ["SET", "relay_0", false]
]
```

## Automation Rule Engine

### Rule Syntax

The system uses a LISP-like syntax for automation rules:

#### Functions
- **Logic**: `IF`, `AND`, `OR`, `NOT`
- **Comparison**: `EQ`, `NE`, `GT`, `LT`, `GTE`, `LTE`
- **Action**: `SET`

#### Data Types
- **Sensors**: `temperature`, `humidity`, `photoSensor`, `lightSwitch`, `currentTime`
- **Actuators**: `relay_1`, `relay_2`, ..., `relay_13`
- **Values**: Numbers, booleans, time strings (`@HH:MM:SS`)

#### Example Rules

**Temperature Control**:
```json
["IF", ["GT", "temperature", 25], 
  ["SET", "relay_0", true], 
  ["SET", "relay_0", false]]
```

**Time-based Control**:
```json
["IF", ["EQ", "currentTime", "@18:00:00"], 
  ["SET", "relay_1", true], 
  ["NOP"]]
```

**Complex Logic**:
```json
["IF", ["AND", ["GT", "temperature", 20], ["LT", "humidity", 60]], 
  ["SET", "relay_2", true], 
  ["SET", "relay_2", false]]
```

### Rule Validation

The `RuleParser.ts` module provides comprehensive validation:

- **Syntax checking**: JSON structure validation
- **Type checking**: Ensures correct data types for operations
- **Function validation**: Verifies function names and argument counts
- **Size limits**: Enforces 256-byte limit for ESP32 storage

## ESP32 Integration

### Asset Pipeline

The custom `ESPBuildPlugin` handles asset optimization:

1. **Gzip Compression**: All assets are compressed
2. **C Header Generation**: Creates `static_files.h` with embedded files
3. **MIME Type Detection**: Proper content-type headers
4. **Size Optimization**: Excludes unnecessary files

### Generated Output

```c
// Example generated static_files.h
const unsigned char index_html_gz[] PROGMEM = {
  0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
  // ... compressed HTML content
};
const size_t index_html_gz_len = 1234;
```

### API Integration

The interface communicates with ESP32 endpoints:

- `GET /relays` - Relay states
- `POST /relays` - Set relay states  
- `GET /sensor-info` - Sensor readings
- `GET /global-info` - System information
- `POST /wifi-settings` - WiFi configuration
- `GET/POST /rule` - Automation rules
- `POST /reset` - System restart

## Development Guidelines

### Code Style

- Use TypeScript for type safety
- Follow React/Preact conventions
- Prefer functional components with hooks
- Use meaningful component and variable names

### Performance Considerations

- Minimize bundle size (ESP32 memory constraints)
- Use efficient state management
- Avoid unnecessary re-renders
- Optimize API calls

### Adding New Features

1. **New Components**: Add to `src/` directory
2. **API Integration**: Update fetch calls in components
3. **Rule Engine**: Extend `RuleParser.ts` for new functions
4. **Styling**: Update `style.css` with new styles

## Debugging

### Development Tools

- **Browser DevTools**: Standard React debugging
- **Console Logging**: Use `console.log` for debugging
- **Network Tab**: Monitor API calls to ESP32

### Common Issues

- **Build Failures**: Check TypeScript errors and dependencies
- **API Errors**: Verify ESP32 connectivity and endpoints
- **Rule Validation**: Use the validation button in automation dialog
- **Asset Size**: Monitor bundle size for ESP32 limits

## Deployment

### Production Build

```bash
npm run build
```

This generates:
- Optimized JavaScript/CSS bundles
- Compressed assets for ESP32
- `static_files.h` C header file

### ESP32 Integration

The built assets are automatically embedded into the ESP32 firmware during compilation. The ESP32 serves these files from internal flash memory.

## Browser Compatibility

- **Modern Browsers**: Chrome 60+, Firefox 55+, Safari 11+, Edge 79+
- **Mobile**: iOS Safari 11+, Chrome Mobile 60+
- **Features Used**: ES6+, Fetch API, CSS Grid/Flexbox

## Security Considerations

- **Local Network Only**: Interface typically runs on local WiFi
- **No Authentication**: Designed for trusted local environments
- **Input Validation**: Rules are validated before submission
- **XSS Protection**: Proper input sanitization

## Future Enhancements

- **Dark Mode**: Theme switching capability
- **Data Logging**: Historical sensor data visualization
- **Mobile App**: Native mobile application
- **Voice Control**: Integration with smart assistants
- **Graphical Rule Editor**: Visual programming interface

## Contributing

When contributing to the web interface:

1. Follow existing code patterns and TypeScript conventions
2. Test changes on actual ESP32 hardware when possible
3. Ensure bundle size remains optimized for ESP32
4. Update documentation for new features
5. Validate rule engine changes thoroughly

## License

Part of the Sunroom2 home automation project. For personal use.

---

This web interface provides a modern, efficient way to control and monitor your ESP32-based home automation system. Its lightweight design and comprehensive features make it ideal for embedded web applications while maintaining a professional user experience.