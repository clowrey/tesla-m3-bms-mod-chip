# Tesla Model 3 BMS Display Interface - Project Documentation

## Project Overview

The Tesla Model 3 BMS Display Interface is a comprehensive battery management system monitoring solution built with ESPHome. It provides real-time visualization of battery parameters through a three-page touchscreen interface, enabling detailed monitoring of all 108 individual cell voltages along with system-wide battery statistics.

## System Architecture

### Hardware Components
- **ESP32-S3 DevKit-C-1** - Main controller with WiFi connectivity
- **480x320 Touchscreen Display** - QSPI interface with LVGL UI framework
- **UART Communication** - Direct connection to Tesla BMS via GPIO pins (TX: 43, RX: 44)
- **Backlight Control** - PWM-controlled display backlight with LEDC

### Software Stack
- **ESPHome** - Core firmware platform
- **LVGL** - Advanced UI framework for touchscreen interface
- **Home Assistant Integration** - API connectivity for remote monitoring
- **Custom BMS UART Component** - Specialized communication handler

## Feature Evolution

### Version 1.0.0 - Core BMS Interface
- Basic UART communication with Tesla BMS
- Parameter request/response parsing
- Simple display output
- Balance control commands

### Version 1.1.0 - ESPHome Integration
- Full ESPHome configuration
- Home Assistant API integration
- WiFi connectivity and OTA updates
- Web server interface

### Version 1.2.0 - Dual Page Display Interface
- **Main Page**: Overview with key parameters and balance controls
- **Details Page**: Comprehensive battery statistics and system information
- Touchscreen navigation between pages
- Real-time parameter updates

### Version 1.3.0 - Cell Voltage Visualization (Current)
- **Third Page**: Visual bar chart of all 108 individual cell voltages
- Color-coded voltage status indicators
- Real-time balancing status visualization
- Complete three-page navigation system

## Technical Specifications

### Display Interface
- **Resolution**: 480x320 pixels in landscape mode
- **Color Depth**: Full color LVGL interface
- **Update Rate**: 100ms for data parsing, 10s for parameter requests
- **Pages**: 3-page touchscreen interface with navigation buttons

### Data Monitoring
- **System Parameters**: 15+ core BMS parameters
- **Individual Cells**: All 108 cell voltages monitored
- **Temperature Sensors**: Multiple temperature monitoring points
- **Balance Control**: Real-time balance status and control
- **Voltage Statistics**: Min/Max/Average/Delta calculations

### Communication Protocol
- **UART Settings**: 115200 baud, 8N1
- **Request Format**: `param get <parameter_name>\n`
- **Response Format**: `<parameter>: <value>\n`
- **Request Batching**: Intelligent batching to prevent system overload

## Page-by-Page Interface Guide

### Main Page - System Overview
**Purpose**: Quick status overview and balance controls

**Key Information**:
- BMS connection status
- BMB count and balance status
- Max/Min cell voltages with cell numbers
- Voltage delta and average
- Pack voltage and chip voltage range
- Loop counter and system state
- Chip temperature
- Current time

**Controls**:
- Balance ON/OFF buttons
- "Details >" navigation button

### Details Page - Comprehensive Statistics
**Purpose**: Detailed battery system information

**Key Information**:
- Average and DC voltage readings
- Cell count (present and balancing)
- Multi-sensor temperature monitoring
- Temperature range calculations
- BMB count and loop details
- Balance status and system state

**Controls**:
- "< Back" navigation to main page
- "Cells >" navigation to voltage graph

### Cell Graph Page - Voltage Visualization
**Purpose**: Visual representation of all 108 cell voltages

**Features**:
- Vertical bar chart with 108 individual cell bars
- Color-coded voltage status:
  - **Red**: >4.1V (over-voltage)
  - **Yellow**: 3.9-4.1V (high voltage)
  - **Green**: 3.2-3.9V (normal range)
  - **Blue**: <3.2V (low voltage)
  - **Orange**: Currently balancing cells
- Real-time voltage scale (2.6V - 4.2V)
- Grid lines for voltage reference
- Balance status indicators

**Controls**:
- "< Details" navigation button
- Real-time clock display

## Configuration Details

### Parameter Request Schedule
- **Main Parameters**: Every 5 seconds with 100ms delays
- **Cell Voltages**: Every 10 seconds in 6 batches
- **Batch Timing**: Staggered (0s, 2s, 4s, 6s, 8s, 9s offsets)
- **Individual Delays**: 50ms between cell requests

### Voltage Unit Conversion
- **BMS Output**: Millivolts (mV)
- **Display Format**: Volts (V) with 3 decimal precision
- **Conversion**: `voltage_V = voltage_mV / 1000.0`

### Color Coding Standards
```yaml
High Voltage (>4.1V): Red (255, 0, 0)
Normal High (3.9-4.1V): Yellow (255, 255, 0)
Normal Range (3.2-3.9V): Green (0, 255, 0)
Low Voltage (<3.2V): Blue (0, 100, 255)
Balancing: Orange (255, 140, 0)
No Data: Gray (100, 100, 100)
```

## Parameter Monitoring

### System Parameters
- `numbmbs` - Number of BMB boards
- `LoopCnt` - Loop counter
- `LoopState` - System state
- `CellsPresent` - Total cells detected
- `CellsBalancing` - Cells currently balancing

### Voltage Parameters
- `umax` - Maximum cell voltage
- `umin` - Minimum cell voltage
- `deltaV` - Voltage difference (max-min)
- `uavg` - Average cell voltage
- `udc` - DC pack voltage
- `CellMax` - Cell number with maximum voltage
- `CellMin` - Cell number with minimum voltage

### Temperature Parameters
- `Chipt0` - Chip temperature
- `Cellt0_0` - Cell temperature sensor 0
- `Cellt0_1` - Cell temperature sensor 1
- `TempMax` - Maximum temperature
- `TempMin` - Minimum temperature

### Individual Cell Voltages
- `u1` through `u108` - Individual cell voltages (mV)

### Balance Control
- `balance` - Balance status (on/off)
- `BalanceCellList` - Comma-separated list of balancing cells

## Development Notes

### Data Validation
- **5V Initialization Check**: Skips 5000mV values to prevent misleading data
- **Valid Range Checking**: Ensures voltage values are within reasonable ranges
- **NaN Handling**: Properly handles uninitialized sensor values

### Performance Optimization
- **Batch Processing**: Cell voltage requests spread across time to prevent overload
- **Memory Management**: Efficient LVGL object creation and updating
- **Buffer Management**: UART buffer size limits to prevent overflow

### Error Handling
- **Connection Status**: Visual indicators for BMS connection status
- **Invalid Data**: Graceful handling of malformed responses
- **Timeout Protection**: Prevents system lockup on communication failures

## Usage Instructions

### Initial Setup
1. Flash ESPHome configuration to ESP32-S3
2. Connect to WiFi network using provided credentials
3. Verify UART connection to Tesla BMS
4. Access web interface for remote monitoring

### Normal Operation
1. **Main Page**: Monitor overall system status
2. **Balance Control**: Use ON/OFF buttons as needed
3. **Detailed Analysis**: Navigate to Details page for comprehensive data
4. **Cell Analysis**: Use Cell Graph page for voltage distribution analysis

### Troubleshooting
- **No Data**: Check UART connections and BMS power
- **Communication Errors**: Verify baud rate and pin assignments
- **Display Issues**: Check backlight settings and LVGL configuration

## Integration with Home Assistant

### Sensor Entities
- All 108 individual cell voltages available as sensors
- System parameters exposed as individual entities
- Balance status and control integration
- Temperature monitoring entities

### Automation Possibilities
- Alert on voltage imbalances
- Automatic balance control based on conditions
- Temperature-based safety shutdowns
- Historical data logging and analysis

## Future Enhancement Opportunities

### Potential Improvements
- **Historical Graphing**: Add trend analysis capabilities
- **Alert System**: Configurable voltage/temperature thresholds
- **Data Logging**: Local storage of historical data
- **Remote Control**: Enhanced balance control features
- **Multi-Pack Support**: Expansion for multiple battery packs

### Technical Enhancements
- **Faster Updates**: Optimize communication timing
- **Additional Sensors**: Support for more temperature sensors
- **Calibration**: Cell voltage calibration capabilities
- **Diagnostics**: Enhanced error reporting and diagnostics

## File Structure

```
esphome-interface/
├── tesla_bms_display.yaml          # Main configuration file
├── cell_voltage_sensors.yaml       # Individual cell sensor definitions
├── external_components/
│   └── tesla_bms_uart/             # Custom UART component
│       ├── __init__.py
│       ├── component.yaml
│       ├── tesla_bms_uart.cpp
│       └── tesla_bms_uart.h
└── README.md                       # Project documentation
```

## Changelog Summary

### Version 1.3.0 (Current)
- Added third page with cell voltage visualization
- Implemented 108 individual cell voltage bars
- Added color-coded voltage status indicators
- Enhanced navigation with three-page system
- Improved balance status visualization

### Version 1.2.0
- Added second page with detailed battery information
- Implemented touchscreen navigation
- Enhanced temperature monitoring
- Added system status indicators

### Version 1.1.0
- Integrated ESPHome framework
- Added Home Assistant connectivity
- Implemented comprehensive parameter monitoring
- Added individual cell voltage monitoring

### Version 1.0.0
- Initial BMS interface implementation
- Basic parameter request/response system
- Simple display output
- Balance control commands

## Conclusion

The Tesla Model 3 BMS Display Interface represents a comprehensive monitoring solution that provides unprecedented visibility into battery system operations. The three-page interface design ensures both quick status overview and detailed analysis capabilities, while the real-time visualization of all 108 cell voltages enables precise monitoring of battery health and performance.

The system's integration with Home Assistant provides additional automation and monitoring capabilities, making it suitable for both professional and enthusiast applications. The modular ESPHome architecture ensures maintainability and future expandability. 