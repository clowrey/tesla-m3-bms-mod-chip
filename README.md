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

## Acknowledgments

- **Damien Maguire & Tom de Bree** - Original BATMan BMS software
- **ESPHome Community** - Framework and component support
