# Tesla Model 3 BMB Interface

This project implements an interface for the Tesla Model 3 Battery Management Board (BMB) using an ESP32 microcontroller.

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