# Tesla Model 3 BMS Interface

This project provides comprehensive interfaces for the Tesla Model 3 Battery Management System (BMS), offering both Arduino/PlatformIO firmware and a modern ESPHome touchscreen interface.

**Batman BMS code** originally created by Damien and Tom: https://github.com/damienmaguire/Tesla-M3-Bms-Software

## Project Overview

This repository contains two main implementations:

1. **Arduino/PlatformIO Firmware** - Core BMS interface with dual serial API
2. **ESPHome Interface** - Modern touchscreen display with Home Assistant integration

## Features

### Core BMS Interface (Arduino/PlatformIO)
- **Parameter API**: Read/write all 108+ BMS parameters via serial commands
- **Dual Serial Interface**: USB and hardware UART (pins 12/13) for flexible access
- **Real-time Monitoring**: Cell voltages, temperatures, balance status
- **Balance Control**: Enable/disable cell balancing remotely
- **AS8510 Support**: Integration with AS8510 analog front-end chips

### ESPHome Display Interface
- **Touchscreen Display**: 480x320 QSPI LCD with LVGL interface
- **Real-time Visualization**: Live BMS data with modern UI
- **Home Assistant Integration**: All parameters as HA entities
- **Touch Controls**: Balance on/off buttons directly on display
- **OTA Updates**: Over-the-air firmware updates

## Example Output
![image](https://github.com/user-attachments/assets/57a223b0-6f92-4a42-a34e-47e2fbd58b88)

```cpp
=== Cell Voltage Information ===
Total Cells Present: 23
Max Cell Voltage: 3.792V (Cell 13)
Min Cell Voltage: 3.770V (Cell 4)
Voltage Delta: 0.022V
Cells Balancing: 0

Individual Cell Voltages:
Cell 1: 3.789V    Cell 9: 3.790V     Cell 17: 3.791V
Cell 2: 3.789V    Cell 10: 3.790V    Cell 18: 3.790V
Cell 3: 3.790V    Cell 11: 3.790V    Cell 19: 3.792V
Cell 4: 3.770V    Cell 12: 3.791V    Cell 20: 3.791V
Cell 5: 3.790V    Cell 13: 3.792V    Cell 21: 3.791V
Cell 6: 3.789V    Cell 14: 3.792V    Cell 22: 3.787V
Cell 7: 3.790V    Cell 15: 3.791V    Cell 23: 3.790V
Cell 8: 3.789V    Cell 16: 3.791V
==============================

=== System Information ===
Total Pack Voltage: 87.10V
Average Cell Voltage: 3.787V
Temperature Range: 25.0°C - 29.0°C
Balance Status: OFF
==============================
```

## Hardware Requirements

### For Arduino/PlatformIO Implementation
- **ESP32 Development Board** (e.g., ESP32-DevKitC)
- **AS8510 Analog Front-End Chips** (for Tesla BMS interface)
- **Tesla Model 3 BMS Hardware**
- **Serial connections** for dual UART interface

### For ESPHome Display Interface
- **ESP32-S3 Development Board** with PSRAM (16MB flash)
- **JC4832W535 QSPI Display** (480x320 with touch)
- **Tesla BMS Connection** via UART

## Project Structure

```
tesla-m3-bms-mod-chip/
├── src/                           # Arduino/PlatformIO source code
│   ├── main.cpp                   # Main program entry point
│   ├── BatMan.cpp                 # Battery Management implementation
│   └── Param.cpp                  # Parameter management system
├── include/                       # Header files
│   ├── BatMan.h                   # BMS interface definitions
│   └── Param.h                    # Parameter system definitions
├── esphome-interface/             # ESPHome touchscreen interface
│   ├── tesla_bms_display.yaml    # Main ESPHome configuration
│   ├── cell_voltage_sensors.yaml        # Individual cell sensors
│   ├── external_components/              # Custom components
│   │   └── tesla_bms_uart/              # BMS UART parser component
│   └── README.md                        # ESPHome setup guide
├── AS8510-library/                # AS8510 chip support library
├── context/                       # Logic analyzer captures and data
├── PARAMETER_API.md               # Complete parameter API documentation
├── DUAL_SERIAL_API.md            # Dual serial interface guide
└── platformio.ini                # PlatformIO configuration
```

## Getting Started

### Arduino/PlatformIO Setup

1. **Install PlatformIO**:
   ```bash
   # Via VS Code extension or CLI
   pip install platformio
   ```

2. **Clone and build**:
   ```bash
   git clone <repository-url>
   cd tesla-m3-bms-mod-chip
   pio run
   ```

3. **Upload to ESP32**:
   ```bash
   pio run -t upload
   ```

4. **Monitor output**:
   ```bash
   pio device monitor
   ```

### ESPHome Display Setup

1. **Install ESPHome**:
   ```bash
   pip install esphome
   ```

2. **Configure and flash**:
   ```bash
   cd esphome-interface
   # Edit secrets.yaml with your WiFi credentials
   esphome run tesla_bms_display.yaml
   ```

See `esphome-interface/README.md` for detailed setup instructions.

## API Usage

### Parameter API Commands

```bash
# List all parameters
param list

# Get specific parameter
param get u1              # Cell 1 voltage
param get balance         # Balance status
param get CellsPresent    # Number of cells

# Set parameter (where applicable)
param set balance 1       # Enable balancing
param set balance 0       # Disable balancing

# Help
param help
```

### Dual Serial Interface

- **USB Serial**: Full system logs and API access
- **Hardware Serial** (pins 12/13): Clean API access without logs

### Available Parameters

#### System Parameters
- `numbmbs`, `LoopCnt`, `LoopState`, `CellsPresent`, `CellsBalancing`
- `BalanceCellList` - Comma-separated list of exact cell numbers being balanced

#### Cell Voltages
- `u1` through `u108` - Individual cell voltages (mV)
- `umax`, `umin`, `deltaV`, `uavg` - Voltage statistics
- `CellMax`, `CellMin` - Cell numbers with max/min voltages

#### Temperature & Control
- `Chipt0`, `TempMax`, `TempMin` - Temperature readings
- `balance` - Balance control (0=off, 1=on)

See `PARAMETER_API.md` for complete parameter documentation.

## Integration Options

### 1. Direct Serial Connection
Connect via USB or hardware UART for direct parameter access.

### 2. ESPHome + Home Assistant
Full home automation integration with:
- Real-time dashboards
- Alerting and automation
- Historical data logging
- Remote control capabilities

### 3. Custom Applications
Use the parameter API to build custom monitoring solutions.

## Documentation

- **[Parameter API Guide](PARAMETER_API.md)** - Complete API reference
- **[Dual Serial Interface](DUAL_SERIAL_API.md)** - Serial configuration guide
- **[ESPHome Setup](esphome-interface/README.md)** - Display interface guide

## Development

### Building
```bash
# Arduino/PlatformIO
pio run

# ESPHome
cd esphome-interface
esphome compile tesla_bms_display.yaml
```

### Testing
- Use logic analyzer captures in `context/` directory for debugging
- Monitor both serial interfaces for comprehensive system analysis
- Test with actual Tesla BMS hardware for validation

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test with actual hardware where possible
5. Submit a pull request

## Hardware Safety

⚠️ **Warning**: This project interfaces with high-voltage battery systems. Proper safety precautions and electrical knowledge are required. Always follow proper ESD procedures and battery safety protocols.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

## Changelog

### Version 1.3.1 (January 2025)
**New Feature: Exact Balance Cell Tracking**
- **NEW**: Added `BalanceCellList` parameter that provides a comma-separated list of exact cell numbers being balanced
- **Enhanced**: Cell voltage graph now shows precise balance indicators using exact BMS data instead of estimation
- **Improved**: Balance indication accuracy now at 100% - no more false positives or missed balancing cells
- **Technical**: Added string parameter support to Param system for text-based data
- **Display**: Extended red bars below graph baseline show exactly which cells are balancing
- **API**: New parameter accessible via serial command `param get BalanceCellList`

### Version 1.3.0 (January 2025)
**Added comprehensive ESPHome display interface enhancements:**

#### **New Third Display Page - Cell Voltage Graph**
- **Visual Bar Graph**: All 108 individual cell voltages displayed as vertical bars
- **Real-time Updates**: Bars dynamically adjust height based on actual voltages (3.0V-4.2V range)
- **Color-Coded Status**: 
  - 🔴 **Red**: >4.1V (overcharged cells)
  - 🟡 **Yellow**: 3.9-4.1V (high voltage)
  - 🟢 **Green**: 3.2-3.9V (normal operating range)  
  - 🔵 **Blue**: <3.2V (low voltage cells)
- **Compact Layout**: 3px wide bars with 4px spacing to fit all 108 cells on 480px display
- **Visual Scale**: Voltage reference lines at 3.0V, 3.4V, 3.8V, and 4.2V

#### **Enhanced Navigation System**
- **Three-Page Interface**: Main → Details → Cell Graph
- **Touch Navigation**: 
  - Main page: "Details >" button
  - Details page: "< Back" and "Cells >" buttons  
  - Cell graph: "< Details" button
- **Seamless Flow**: Intuitive page transitions with consistent UI

#### **Comprehensive Data Integration**
- **Parameter Expansion**: Added 15+ new BMS parameters
  - System: `numbmbs`, `LoopCnt`, `LoopState`, `CellsPresent`, `CellsBalancing`
  - Voltages: `umax`, `umin`, `deltaV`, `uavg`, `udc`, `CellMax`, `CellMin`
  - Temperatures: `Chipt0`, `Cellt0_0`, `Cellt0_1`, `TempMax`, `TempMin`
  - Control: `balance` status
- **Unit Conversion**: All voltage displays now show V instead of mV (3 decimal precision)
- **Individual Cell Monitoring**: All 108 cell voltages (u1-u108) with intelligent batching

#### **Performance Optimizations**
- **Intelligent Request Batching**: 6 batches of cell voltage requests with staggered timing
- **Memory Management**: Efficient bar creation with single initialization flag
- **Update Frequency**: 5s for main parameters, 10s for cell voltages
- **Visual Feedback**: Real-time balance status with color indicators

### Version 1.2.0 - Dual Page Display Interface
**Enhanced ESPHome touchscreen interface:**
- **Two-page Display**: Main overview + detailed battery information
- **Touch Controls**: Balance on/off buttons with immediate feedback
- **Real-time Updates**: Live parameter monitoring every 5-10 seconds
- **Professional UI**: Grid-based layout optimized for 480x320 display

### Version 1.1.0 - ESPHome Integration
**Added comprehensive ESPHome display interface:**
- **QSPI Display Support**: JC4832W535 480x320 touchscreen
- **LVGL Interface**: Modern touch-based UI
- **Home Assistant Integration**: All BMS parameters as HA entities
- **OTA Updates**: Over-the-air firmware updates
- **WiFi Connectivity**: Remote monitoring capabilities

### Version 1.0.0 - Core BMS Interface
**Initial release with Arduino/PlatformIO implementation:**
- **Parameter API**: 108+ BMS parameters via serial commands
- **Dual Serial Interface**: USB + hardware UART (pins 12/13)
- **AS8510 Integration**: Analog front-end chip support
- **Balance Control**: Remote cell balancing enable/disable
- **Real-time Monitoring**: Cell voltages, temperatures, system status

## Acknowledgments

- **Damien Maguire & Tom de Bree** - Original BATMan BMS software
- **ESPHome Community** - Framework and component support
