# Tesla Model 3 BMB Interface

This project implements an interface for the Tesla Model 3 Battery Management Board (BMB) using an ESP32 microcontroller.

Batman code created by Damien and Tom here: https://github.com/damienmaguire/Tesla-M3-Bms-Software

## Example

```cpp
=== Cell Voltage Information ===
Total Cells Present: 23
Max Cell Voltage: 3.792V (Cell 13)
Min Cell Voltage: 3.770V (Cell 4)
Voltage Delta: 0.022V
Cells Balancing: 0

Individual Cell Voltages:
Cell 1: 3.789V
Cell 2: 3.789V
Cell 3: 3.790V
Cell 4: 3.770V (MIN)
Cell 5: 3.790V
Cell 6: 3.789V
Cell 7: 3.790V
Cell 8: 3.789V
Cell 9: 3.790V
Cell 10: 3.790V
Cell 11: 3.790V
Cell 12: 3.791V
Cell 13: 3.792V (MAX)
Cell 14: 3.792V
Cell 15: 3.791V
Cell 16: 3.791V
Cell 17: 3.791V
Cell 18: 3.790V
Cell 19: 3.792V
Cell 20: 3.791V
Cell 21: 3.791V
Cell 22: 3.787V
Cell 23: 3.790V
==============================

=== Auxiliary Voltage Information ===
Total Pack Voltage: 87.10V
Average Cell Voltage: 0.00V
Charge Voltage Limit: 0.00V
Discharge Voltage Limit: 0.00V
Chip1 5V Supply: 5.02V
Chip2 5V Supply: 5.02V
===================================

=== Temperature Information ===
Max Temperature: 29.0°C
Min Temperature: 25.0°C
```

## Project Structure

- `src/` - Source files
  - `main.cpp` - Main program entry point
  - `BatMan.cpp` - Battery Management implementation
  - `Param.cpp` - Parameter management
- `include/` - Header files
  - `BatMan.h` - Battery Management interface
  - `Param.h` - Parameter definitions

## Requirements

- PlatformIO
- ESP32 development board
- Arduino framework

## Building and Uploading

1. Clone this repository
2. Open the project in PlatformIO
3. Build the project:
   ```
   pio run
   ```
4. Upload to ESP32:
   ```
   pio run -t upload
   ```
5. Monitor serial output:
   ```
   pio device monitor
   ```

## License

[Add your license here]

## Contributing

[Add contribution guidelines here] 
