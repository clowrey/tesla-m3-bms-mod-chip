# Tesla BMS Display - ESPHome Project

This ESPHome project creates a display interface for the Tesla Model 3 BMS (Battery Management System) using an ESP32-S3 microcontroller with a JC3248W535C LCD display.

## Features

- **Real-time BMS Monitoring**: Displays all Tesla BMS parameters via serial interface
- **LCD Display**: Shows key metrics on a 240x320 LCD screen
- **Home Assistant Integration**: Full integration with Home Assistant for remote monitoring
- **Web Interface**: Built-in web server for configuration and monitoring
- **OTA Updates**: Over-the-air firmware updates
- **Parameter Control**: Enable/disable cell balancing remotely
- **Comprehensive Sensor Coverage**: All 108+ BMS parameters monitored

## Hardware Requirements

- **ESP32-S3 Development Board** (e.g., ESP32-S3-DevKitC-1)
- **JC3248W535C LCD Display** (240x320 resolution)
- **Serial Connection** to Tesla BMS (UART)
- **Power Supply** (5V/3.3V as required by your board)

## Pin Configuration

### UART Connection (Tesla BMS)
- **TX**: GPIO17
- **RX**: GPIO16
- **Baud Rate**: 115200

### LCD Display (JC3248W535C)
- **Data Pins**: GPIO23, GPIO19, GPIO18, GPIO5, GPIO4, GPIO2, GPIO1, GPIO0
- **DC**: GPIO15
- **CS**: GPIO14
- **Reset**: GPIO13
- **Backlight**: GPIO12

### I2C (if needed)
- **SDA**: GPIO21
- **SCL**: GPIO22

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
   - Copy `secrets.yaml` and update with your WiFi credentials
   - Generate an API encryption key for Home Assistant
   - Set an OTA password

4. **Compile and upload**:
   ```bash
   esphome run tesla_bms_display_simple.yaml
   ```

## Configuration

### Main Configuration File
The main configuration is in `tesla_bms_display_simple.yaml`. This file includes:

- **Hardware Configuration**: ESP32-S3 board, UART, LCD display
- **Sensors**: All Tesla BMS parameters as template sensors
- **Display**: LCD interface with real-time data
- **Automation**: Automatic parameter updates and display refresh
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
- `umax`/`umin`: Maximum/minimum cell voltages
- `deltaV`: Voltage difference between max and min cells
- `uavg`: Average cell voltage
- `CellMax`/`CellMin`: Cell numbers with max/min voltage
- `u1` through `u108`: Individual cell voltages

#### Temperature Parameters
- `Chipt0`: Chip temperature
- `Cellt0_0`/`Cellt0_1`: Cell temperatures
- `TempMax`/`TempMin`: Maximum/minimum temperatures

#### Control Parameters
- `balance`: Balance control status (0=disabled, 1=enabled)

## Usage

### Serial Communication
The system communicates with the Tesla BMS using the Parameter API:

- **Automatic Updates**: Parameters are requested every 5 seconds
- **Manual Refresh**: Use the "Refresh Parameters" button
- **Balance Control**: Use "Enable Balance" / "Disable Balance" buttons

### Display Interface
The LCD shows:
- **System Status**: Connection status, loop information
- **Voltage Information**: Max/min voltages, delta, cell numbers
- **Temperature**: Current chip temperature
- **Balance Status**: ON/OFF with color coding
- **Real-time Clock**: Current time from NTP

### Home Assistant Integration
Once connected to Home Assistant:
- All sensors appear as entities
- Control switches for balance enable/disable
- Automation capabilities for alerts and control
- Historical data logging

## API Commands

The system sends these commands to the Tesla BMS:

```bash
param list          # Get all parameters
param get <name>    # Get specific parameter
balance on          # Enable cell balancing
balance off         # Disable cell balancing
balance status      # Check balance status
```

## Troubleshooting

### Common Issues

1. **No Display Output**:
   - Check pin connections
   - Verify display driver (ST7789)
   - Check power supply

2. **No Serial Communication**:
   - Verify UART pins (GPIO16/17)
   - Check baud rate (115200)
   - Ensure Tesla BMS is powered and responding

3. **WiFi Connection Issues**:
   - Check WiFi credentials in secrets.yaml
   - Verify network availability
   - Check signal strength

4. **Parameter Not Updating**:
   - Check UART connection
   - Verify Tesla BMS is responding
   - Check log output for errors

### Debug Mode
Enable debug logging by modifying the logger section:

```yaml
logger:
  level: DEBUG
  logs:
    uart: DEBUG
    tesla_bms_parser: DEBUG
```

## Customization

### Adding New Parameters
To add new parameters, edit the text sensor lambda in the configuration:

```yaml
text_sensor:
  - platform: uart
    id: tesla_bms_response
    on_value:
      then:
        - lambda: |-
            // Add new parameter mapping here
            if (param_name == "NewParam") {
              id(new_sensor).publish_state(value);
            }
```

### Display Layout
Modify the display page lambda to change the layout:

```yaml
display:
  - platform: st7789
    pages:
      - id: main_page
        lambda: |-
          // Custom display code here
```

### Update Intervals
Change parameter update frequency:

```yaml
interval:
  - interval: 10s  # Change from 5s to 10s
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
4. Test thoroughly
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Support

For issues and questions:
1. Check the troubleshooting section
2. Review ESPHome documentation
3. Check Tesla BMS documentation
4. Open an issue on the repository

## Changelog

### Version 1.0.0
- Initial release
- Basic parameter monitoring
- LCD display interface
- Home Assistant integration
- Balance control functionality 