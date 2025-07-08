# Tesla BMS Display - ESPHome Project

This ESPHome project creates a touchscreen display interface for the Tesla Model 3 BMS (Battery Management System) using an ESP32-S3 microcontroller with a JC4832W535 QSPI LCD display.

## Features

- **Real-time BMS Monitoring**: Displays all Tesla BMS parameters via UART interface
- **Touchscreen LCD Display**: Shows key metrics on a 480x320 LCD screen with touch interface
- **LVGL Interface**: Modern graphical user interface with buttons and real-time updates
- **Home Assistant Integration**: Full integration with Home Assistant for remote monitoring
- **Web Interface**: Built-in web server for configuration and monitoring
- **OTA Updates**: Over-the-air firmware updates
- **Parameter Control**: Enable/disable cell balancing via touchscreen buttons
- **Comprehensive Sensor Coverage**: All 108+ BMS parameters monitored (individual cell voltages u1-u108)

## Hardware Requirements

- **ESP32-S3 Development Board** (e.g., ESP32-S3-DevKitC-1)
- **JC4832W535 QSPI LCD Display** (480x320 resolution with touch)
- **Serial Connection** to Tesla BMS (UART)
- **Power Supply** (5V/3.3V as required by your board)

## Pin Configuration

### UART Connection (Tesla BMS)
- **TX**: GPIO43
- **RX**: GPIO44
- **Baud Rate**: 115200

### QSPI LCD Display (JC4832W535)
- **CLK**: GPIO47
- **Data Pins**: GPIO21, GPIO48, GPIO40, GPIO39
- **CS**: GPIO45
- **Backlight**: GPIO1 (PWM controlled)

### I2C Touchscreen (AXS15231)
- **SDA**: GPIO4
- **SCL**: GPIO8

### Memory Configuration
- **PSRAM**: Octal mode at 80MHz
- **Flash**: 16MB

## Installation

1. **Install ESPHome**:
   ```bash
   pip install esphome
   ```

2. **Clone or download this project**:
   ```bash
   git clone <repository-url>
   cd tesla-m3-bms-mod-chip/esphome-interface
   ```

3. **Configure secrets**:
   - Edit `secrets.yaml` and update with your WiFi credentials
   - Update API encryption key for Home Assistant
   - Set an OTA password

4. **Compile and upload**:
   ```bash
   esphome run tesla_bms_display.yaml
   ```

## Configuration

### Main Configuration File
The main configuration is in `tesla_bms_display.yaml`. This file includes:

- **Hardware Configuration**: ESP32-S3 board, UART, QSPI LCD display with touch
- **Sensors**: All Tesla BMS parameters as template sensors
- **LVGL Display**: Modern touchscreen interface with real-time data
- **Automation**: Batch parameter updates with staggered intervals
- **Web Interface**: Built-in web server for configuration

### Parameter Mapping

The system automatically maps Tesla BMS parameters to ESPHome sensors:

#### System Parameters
- `numbmbs`: Number of BMB boards
- `LoopCnt`: Loop counter
- `LoopState`: Current loop state
- `CellsPresent`: Number of cells present
- `CellsBalancing`: Number of cells currently balancing

#### Voltage Parameters
- `umax`/`umin`: Maximum/minimum cell voltages (converted from mV to V)
- `deltaV`: Voltage difference between max and min cells
- `uavg`: Average cell voltage
- `udc`: DC pack voltage
- `CellMax`/`CellMin`: Cell numbers with max/min voltage
- `u1` through `u108`: Individual cell voltages (converted from mV to V)

#### Temperature Parameters
- `Chipt0`: Chip temperature
- `Cellt0_0`/`Cellt0_1`: Cell temperatures
- `TempMax`/`TempMin`: Maximum/minimum temperatures

#### Control Parameters
- `balance`: Balance control status (0=disabled, 1=enabled)

## Usage

Plotly graph code:

type: custom:plotly-graph
raw_plotly_config: true
title: Battery 0
fn: |-
  $ex {
    vars.x = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
    vars.y = [1, 2, 3, 4, 5, 6, 7, 8];
    vars.z = [];
      

    for (let j = 0; j < 108; j++) {
      vars.z[j] = [];
      // Calculate the index in the 1D array corresponding to the 2D matrix element
      let index = j;

      // Populate the matrix with values from the 1D array
      vars.z[j][j] = hass.states["sensor.tesla_bms_display_u" + (index).toString()].state
    }
    //console.log(vars.x);
    //console.log(vars.y);
    //console.log(vars.z);
    

    for (let j = 0; j < 108; j++) {
      // Calculate the index in the 1D array corresponding to the 2D matrix element
      let index = j;
      vars.x[index] = index;
      // Populate the matrix with values from the 1D array
      console.log(index);
      vars.y[index] = hass.states["sensor.tesla_bms_display_u" + (index).toString()].state
    }

    
    vars.ymin = Math.min(...vars.y) - (50.0 / 1000);
    vars.ymax = Math.max(...vars.y) + (50.0 / 1000);
    console.log(vars.x);
    console.log(vars.y);
    
  }
entities:
  - entity: ""
    color: "#ff0000"
    type: bar
    x: $ex vars.x
    "y": $ex vars.y
    z: $ex vars.z
    texttemplate: "%{y}"
    refresh_interval: 2
layout:
  xaxis:
    showgrid: false
    zeroline: false
    showticklabels: false
    ticks: false
    nticks: 1
    visible: false
    height: 410
  yaxis:
    range:
      - $ex vars.ymin
      - $ex vars.ymax
config:
  displayModeBar: false
  scrollZoom: false


### Display Interface
The touchscreen LCD shows:
- **System Status**: BMB count, connection status, loop information
- **Voltage Information**: Max/min voltages, delta, cell numbers
- **Temperature**: Current chip temperature
- **Balance Status**: ON/OFF with color coding (green=on, red=off)
- **Control Buttons**: "Balance ON" and "Balance OFF" touchscreen buttons
- **Real-time Clock**: Current time from NTP

### Serial Communication
The system communicates with the Tesla BMS using the Parameter API:

- **Batch Updates**: Parameters are requested in batches every 10 seconds
- **Staggered Requests**: Cell voltages requested in 6 batches to avoid overwhelming the BMS
- **Fast Parsing**: UART data parsed every 100ms for responsive updates
- **Manual Control**: Touch buttons for balance enable/disable

### Home Assistant Integration
Once connected to Home Assistant:
- All sensors appear as entities (108+ individual cell voltages)
- Balance status and control
- System parameters (BMB count, loop state, etc.)
- Temperature monitoring
- Historical data logging

## API Commands

The system sends these commands to the Tesla BMS:

```bash
param get <parameter>   # Get specific parameter (e.g., param get u1)
balance on             # Enable cell balancing
balance off            # Disable cell balancing
balance status         # Check balance status
```

## Parameter Request Schedule

Cell voltages are requested in batches to manage UART traffic:
- **Batch 1**: u1-u20 (immediate)
- **Batch 2**: u21-u40 (2s delay)
- **Batch 3**: u41-u60 (4s delay)
- **Batch 4**: u61-u80 (6s delay)
- **Batch 5**: u81-u100 (8s delay)
- **Batch 6**: u101-u108 (9s delay)

System parameters are requested separately with minimal delays.

## Troubleshooting

### Common Issues

1. **No Display Output**:
   - Check QSPI pin connections (CLK: 47, Data: 21,48,40,39, CS: 45)
   - Verify JC4832W535 display compatibility
   - Check power supply and backlight (GPIO1)

2. **Touch Not Working**:
   - Verify I2C connections (SDA: 4, SCL: 8)
   - Check AXS15231 touchscreen controller
   - Ensure proper calibration settings

3. **No Serial Communication**:
   - Verify UART pins (TX: GPIO43, RX: GPIO44)
   - Check baud rate (115200)
   - Ensure Tesla BMS is powered and responding

4. **WiFi Connection Issues**:
   - Check WiFi credentials in secrets.yaml
   - Verify network availability
   - Check signal strength

5. **Parameters Not Updating**:
   - Check UART connection
   - Verify Tesla BMS is responding to "param get" commands
   - Check log output for parsing errors

### Debug Mode
Enable debug logging by modifying the logger section:

```yaml
logger:
  level: DEBUG
  logs:
    uart: DEBUG
    lvgl: DEBUG
```

## Custom Components

### tesla_bms_uart Component
Located in `external_components/tesla_bms_uart/`, this component:
- Parses UART responses in "parameter: value" format
- Registers sensors for automatic parameter mapping
- Handles Tesla BMS-specific protocol nuances

## Customization

### Adding New Parameters
To add new parameters, update the parsing lambda in the interval section:

```yaml
# In the 100ms interval lambda
else if (param == "NewParam") { id(new_sensor).publish_state(value); sensor_updated = true; }
```

### Display Layout
Modify the LVGL page configuration to change the layout:

```yaml
lvgl:
  pages:
    - id: main_page
      # Custom layout here
```

### Update Intervals
Change parameter update frequency by modifying the interval sections:

```yaml
interval:
  - interval: 20s  # Change from 10s to 20s
```

## Performance Notes

- **PSRAM Required**: Large configuration requires external PSRAM
- **Batch Processing**: Cell voltage requests are batched to prevent UART overflow
- **Memory Management**: ESP-IDF framework optimized for size and performance
- **Touch Responsiveness**: 100ms parsing interval ensures responsive touch interface

## Files Structure

```
esphome-interface/
├── tesla_bms_display.yaml    # Main configuration (1100+ lines)
├── cell_voltage_sensors.yaml       # Individual cell voltage sensors (u1-u108)
├── secrets.yaml                     # WiFi and API credentials
├── external_components/
│   └── tesla_bms_uart/             # Custom UART parser component
│       ├── tesla_bms_uart.h
│       ├── tesla_bms_uart.cpp
│       ├── component.yaml
│       └── __init__.py
└── README.md                       # This file
```

## Security Notes

- Keep your `secrets.yaml` file secure and never commit it to version control
- Use strong passwords for OTA updates
- Consider using WPA3 WiFi networks
- Regularly update ESPHome and firmware

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly with actual Tesla BMS hardware
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Support

For issues and questions:
1. Check the troubleshooting section
2. Review ESPHome documentation
3. Check Tesla BMS Parameter API documentation
4. Open an issue on the repository

## Changelog

### Version 2.0.0
- Updated to JC4832W535 QSPI display (480x320)
- Added touchscreen interface with balance control buttons
- Implemented LVGL modern UI framework
- Added batch parameter processing for better performance
- Custom tesla_bms_uart component for improved parsing
- ESP32-S3 with PSRAM support
- Staggered cell voltage requests to prevent UART overflow

### Version 1.0.0
- Initial release with basic parameter monitoring 