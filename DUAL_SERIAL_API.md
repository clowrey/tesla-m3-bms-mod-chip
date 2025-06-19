# Dual Serial Interface API

This document describes the dual serial interface setup for the Tesla Model 3 BMS interface.

## Overview

The system now supports two serial interfaces for accessing the parameter API and system commands:

1. **Primary Serial Interface** - Standard USB serial connection
2. **Secondary Serial Interface** - Hardware serial on pins 12 (RX) and 13 (TX)

## Hardware Configuration

### Pin Assignments
- **Serial (Primary)**: USB connection (default)
- **Serial2 (Secondary)**: 
  - RX: GPIO 12
  - TX: GPIO 13
  - Baud Rate: 115200
  - Configuration: 8N1 (8 data bits, no parity, 1 stop bit)

### Pin Conflicts Resolved
- **Economizer PWM**: Moved from GPIO 12 to GPIO 14 to avoid conflict with Serial2 RX
- **Serial2**: Uses GPIO 12 (RX) and GPIO 13 (TX)

## Usage

### Connecting to Serial2
You can connect to the secondary serial interface using:
- USB-to-TTL converter
- Logic analyzer
- Another microcontroller
- Serial terminal software

### Command Availability
All commands are available on both serial interfaces:
- Balance control commands
- Parameter API commands
- System status commands
- Help commands

### Examples

#### Primary Serial (USB)
```bash
# Connect via USB
param list
param get balance
param set balance 1
```

#### Secondary Serial (Pins 12/13)
```bash
# Connect via pins 12/13
param list
param get balance
param set balance 1
```

## Output Synchronization

Both serial interfaces receive:
- System startup messages
- Current sensor readings
- Periodic BMS status updates (every 5 seconds)
- Economizer duty cycle changes
- All command responses
- Error messages

## Benefits

1. **Dual Access**: Access the system from two different interfaces simultaneously
2. **Debugging**: Use one interface for monitoring while using the other for commands
3. **Integration**: Connect to other systems via the hardware serial interface
4. **Redundancy**: Backup communication path if one interface fails
5. **Development**: Test commands on one interface while monitoring on another

## Configuration

### Baud Rate
Both interfaces operate at 115200 baud by default.

### Pin Configuration
```cpp
// Serial2 configuration
#define SERIAL2_BAUD_RATE 115200
Serial2.begin(SERIAL2_BAUD_RATE, SERIAL_8N1, 12, 13); // RX=12, TX=13
```

### Economizer Pin Change
```cpp
// PWM Configuration for Economizer (moved to avoid conflict with Serial2)
#define ECONOMIZER_PWM_PIN 14  // Changed from 12 to 14
```

## Troubleshooting

### Serial2 Not Responding
1. Check physical connections (RX/TX may need to be swapped)
2. Verify baud rate settings
3. Ensure no other devices are using pins 12/13
4. Check for pin conflicts with other peripherals

### Command Not Working
1. Ensure proper line endings (CR/LF)
2. Check command syntax
3. Verify parameter names are correct
4. Use 'help' command to see available options

### Performance Issues
1. Both interfaces share the same processing time
2. High command frequency may affect system responsiveness
3. Consider using one interface for monitoring and one for commands

## Advanced Usage

### Simultaneous Monitoring
You can monitor the system from both interfaces simultaneously:
- Use Serial for real-time monitoring
- Use Serial2 for parameter configuration

### External Integration
The Serial2 interface can be used to:
- Connect to external monitoring systems
- Interface with other microcontrollers
- Provide data logging capabilities
- Enable remote control functionality

### Custom Applications
Develop custom applications that:
- Read parameters via Serial2
- Control the system remotely
- Log data to external systems
- Integrate with home automation systems 