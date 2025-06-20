@echo off
REM Tesla BMS Display - ESPHome Setup Script
REM This script helps set up the ESPHome project for Tesla BMS monitoring

echo === Tesla BMS Display - ESPHome Setup ===
echo.

REM Check if ESPHome is installed
esphome version >nul 2>&1
if %errorlevel% neq 0 (
    echo ESPHome not found. Installing...
    pip install esphome
) else (
    echo ESPHome found:
    esphome version
)

REM Check if secrets file exists
if not exist "secrets.yaml" (
    echo.
    echo Creating secrets.yaml template...
    (
        echo # WiFi Configuration
        echo wifi_ssid: "Your_WiFi_SSID"
        echo wifi_password: "Your_WiFi_Password"
        echo.
        echo # Home Assistant API
        echo api_encryption_key: "Your_API_Encryption_Key_Here"
        echo.
        echo # OTA Updates
        echo ota_password: "Your_OTA_Password"
    ) > secrets.yaml
    echo Created secrets.yaml template. Please edit it with your credentials.
) else (
    echo secrets.yaml already exists.
)

REM Check if fonts directory exists
if not exist "fonts" (
    echo.
    echo Creating fonts directory...
    mkdir fonts
    echo Please download OpenSans fonts and place them in the fonts/ directory:
    echo   - OpenSans-Regular-12.pcf
    echo   - OpenSans-Bold-16.pcf
    echo   - OpenSans-Bold-20.pcf
)

echo.
echo === Setup Complete ===
echo.
echo Next steps:
echo 1. Edit secrets.yaml with your WiFi credentials and API key
echo 2. Download OpenSans fonts to the fonts/ directory
echo 3. Connect your ESP32-S3 board
echo 4. Run: esphome run tesla_bms_display_simple.yaml
echo.
echo For individual cell monitoring, include cell_voltage_sensors.yaml
echo For custom components, use tesla_bms_display.yaml
echo.
echo Documentation: README.md
pause 