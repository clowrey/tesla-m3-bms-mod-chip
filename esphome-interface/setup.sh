#!/bin/bash

# Tesla BMS Display - ESPHome Setup Script
# This script helps set up the ESPHome project for Tesla BMS monitoring

set -e

echo "=== Tesla BMS Display - ESPHome Setup ==="
echo ""

# Check if ESPHome is installed
if ! command -v esphome &> /dev/null; then
    echo "ESPHome not found. Installing..."
    pip install esphome
else
    echo "ESPHome found: $(esphome version)"
fi

# Check if secrets file exists
if [ ! -f "secrets.yaml" ]; then
    echo ""
    echo "Creating secrets.yaml template..."
    cat > secrets.yaml << EOF
# WiFi Configuration
wifi_ssid: "Your_WiFi_SSID"
wifi_password: "Your_WiFi_Password"

# Home Assistant API
api_encryption_key: "Your_API_Encryption_Key_Here"

# OTA Updates
ota_password: "Your_OTA_Password"
EOF
    echo "Created secrets.yaml template. Please edit it with your credentials."
else
    echo "secrets.yaml already exists."
fi

# Check if fonts directory exists
if [ ! -d "fonts" ]; then
    echo ""
    echo "Creating fonts directory..."
    mkdir -p fonts
    echo "Please download OpenSans fonts and place them in the fonts/ directory:"
    echo "  - OpenSans-Regular-12.pcf"
    echo "  - OpenSans-Bold-16.pcf"
    echo "  - OpenSans-Bold-20.pcf"
fi

echo ""
echo "=== Setup Complete ==="
echo ""
echo "Next steps:"
echo "1. Edit secrets.yaml with your WiFi credentials and API key"
echo "2. Download OpenSans fonts to the fonts/ directory"
echo "3. Connect your ESP32-S3 board"
echo "4. Run: esphome run tesla_bms_display_simple.yaml"
echo ""
echo "For individual cell monitoring, include cell_voltage_sensors.yaml"
echo "For custom components, use tesla_bms_display.yaml"
echo ""
echo "Documentation: README.md" 