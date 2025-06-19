# Parameter API Documentation

This document describes the serial parameter API for the Tesla Model 3 BMS interface.

## Overview

The parameter API allows you to read and write all system parameters via serial commands. Parameters are automatically synchronized with the BATMan BMS system data.

## Commands

### List All Parameters
```
param list
```
Lists all parameters with their current values.

### Get Parameter Value
```
param get <parameter_name>
```
Gets the current value of a specific parameter.

### Set Parameter Value
```
param set <parameter_name> <value>
```
Sets a parameter to a specific value. Values can be integers or floats.

### Help
```
param help
```
Shows detailed help information about the parameter API.

## Parameter Categories

### System Parameters
- `numbmbs` - Number of BMB boards
- `LoopCnt` - Loop counter
- `LoopState` - Current loop state
- `CellsPresent` - Number of cells present
- `CellsBalancing` - Number of cells currently balancing

### Cell Voltages (u1-u108)
Individual cell voltages in millivolts:
- `u1` through `u108` - Cell voltages (e.g., `u1` = cell 1 voltage)

### Voltage Statistics
- `CellMax` - Cell number with maximum voltage
- `CellMin` - Cell number with minimum voltage
- `umax` - Maximum cell voltage (mV)
- `umin` - Minimum cell voltage (mV)
- `deltaV` - Voltage difference between max and min cells
- `udc` - DC voltage
- `uavg` - Average cell voltage
- `chargeVlim` - Charge voltage limit
- `dischargeVlim` - Discharge voltage limit

### Balance Control
- `balance` - Balance control (0=disabled, 1=enabled)
- `CellVmax` - Maximum cell voltage for balancing
- `CellVmin` - Minimum cell voltage for balancing

### Temperature Parameters
- `Chipt0` - Chip temperature
- `Cellt0_0` - Cell temperature 0
- `Cellt0_1` - Cell temperature 1
- `TempMax` - Maximum temperature
- `TempMin` - Minimum temperature

### Chip Voltages
- `ChipV1` through `ChipV8` - Individual chip voltages

### Chip Supplies
- `Chip1_5V` - Chip 1 5V supply
- `Chip2_5V` - Chip 2 5V supply

### Cell Counts per Chip
- `Chip1Cells` - Number of cells on chip 1
- `Chip2Cells` - Number of cells on chip 2
- `Chip3Cells` - Number of cells on chip 3
- `Chip4Cells` - Number of cells on chip 4

## Examples

### Enable Cell Balancing
```
param set balance 1
```

### Get Current Balance Status
```
param get balance
```

### Get Cell 1 Voltage
```
param get u1
```

### Set Cell 1 Voltage (for testing)
```
param set u1 4200
```

### Get Maximum Cell Voltage
```
param get umax
```

### Get Voltage Delta
```
param get deltaV
```

### List All Parameters
```
param list
```

## Integration with BATMan System

The parameter API automatically synchronizes with the BATMan BMS system:

1. **Automatic Updates**: Parameters are updated every loop cycle with real BMS data
2. **Cell Voltage Mapping**: Cell voltages (u1-u108) are automatically mapped from the hardware register positions
3. **Statistics Calculation**: Voltage statistics are calculated from actual cell data
4. **Balance Status**: Balance control parameters reflect the actual system state

## Error Handling

The API provides clear error messages for:
- Unknown parameters
- Invalid values
- Malformed commands

## Usage Notes

- All cell voltages are in millivolts (mV)
- Temperature values are in degrees Celsius
- Balance control is binary (0=off, 1=on)
- Parameters are automatically initialized with default values
- The API is case-insensitive for parameter names 