# Batman IC Hardware Connection Verification

## Tesla Batman Debug Header Pinout (6-pin connector)
```
Pin #1 (Square pin) - SCK  -> ESP32 GPIO16
Pin #2              - MOSI -> ESP32 GPIO17
Pin #3              - MISO -> ESP32 GPIO5
Pin #4              - CS   -> ESP32 GPIO18
Pin #5              - Batman Enable -> ESP32 GPIO19 (kept LOW)
Pin #6              - GND  -> ESP32 GND
```

## Verification Steps

### 1. Physical Connections
- [ ] Check all 6 pins are properly connected
- [ ] Verify pin #1 (square pin) is correctly oriented for SCK
- [ ] Ensure solid connections (no loose wires)
- [ ] Check for shorts between adjacent pins

### 2. Power Requirements
- [ ] Batman IC requires 3.3V or 5V power supply (check datasheet)
- [ ] Ensure sufficient current capacity (typically 50-100mA)
- [ ] Verify ground connection is solid

### 3. SPI Signal Verification (with multimeter)
- [ ] SCK (GPIO16): Should show clock pulses during communication
- [ ] CS (GPIO18): Should go LOW during communication attempts
- [ ] Batman Enable (GPIO19): Should be constantly LOW (0V)
- [ ] MISO (GPIO5): Should show return data from Batman IC

### 4. ESP32 Pin Configuration
According to the firmware:
```
BMB_MOSI = GPIO17   (ESP32 -> Batman IC)
BMB_MISO = GPIO5    (Batman IC -> ESP32)  
BMB_SCK = GPIO16    (ESP32 -> Batman IC)
BMB_CS = GPIO18     (ESP32 -> Batman IC, active LOW)
BMB_ENABLE = GPIO19 (ESP32 -> Batman IC, kept LOW)
```

### 5. Communication Test Commands
After setting `numbmbs = 1`, these commands will test SPI:
```
# Manual SPI test (if available)
batman spi test

# Check for any SPI responses
batman debug on
param get u1
```

## Common Issues

### Issue 1: No SPI Activity
- Check if ESP32 pins are configured correctly
- Verify SPI bus initialization didn't fail
- Ensure numbmbs is set to 1 (not 0)

### Issue 2: SPI Activity but No Valid Responses  
- Check Batman IC power supply
- Verify correct pinout (especially pin #1 orientation)
- Check for damaged Batman IC

### Issue 3: Intermittent Communication
- Check for loose connections
- Verify ground integrity
- Check for EMI interference

## Expected Behavior After Fix

1. **Parameter Values:**
   - numbmbs = 1
   - ExpectedBmbCount = 2
   - ActualBmbCount = 1 (if working)
   - BmbConnectedMask = 1 (bit 0 set)

2. **Cell Readings:**
   - Cell voltages (u1, u7, u15, etc.) should show real mV values
   - No more "OFFLINE" messages in logs
   - Pack voltage should show sum of cell voltages

3. **System Logs:**
   ```
   BMB Connectivity: 1/2 chips connected (Expected: 2, Mask: 0x0001)
     Chip 0: ONLINE (last response X ms ago)
     Chip 1: OFFLINE
   ```

## Troubleshooting Next Steps

If parameter fix doesn't work:
1. Check hardware connections (this document)
2. Try different numbmbs values (0, 1, 2)
3. Enable SPI debugging to see raw communication
4. Consider Batman IC might be damaged
5. Test with oscilloscope to verify SPI signals 