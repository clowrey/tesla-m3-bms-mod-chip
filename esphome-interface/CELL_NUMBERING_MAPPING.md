# Cell Numbering Mapping Between Serial Monitor and ESPHome Interface

## Problem Description (RESOLVED)
The cell numbers in the serial monitor didn't line up with the ones in the ESPHome logging - they appeared to be offset by 1.

**ROOT CAUSE FOUND:** The `updateParametersFromBATMan()` function in main.cpp was incorrectly overwriting the cell voltage parameters set by the main BATMan system, causing misalignment when dead cells were present.

## Root Cause
The two systems use different numbering conventions:

| System | Numbering Convention | Example |
|--------|---------------------|---------|
| **Main Firmware (Serial Monitor)** | 1-based sequential | "Cell 1", "Cell 2", "Cell 3", etc. |
| **ESPHome Interface** | 0-based display positions | Position 0, Position 1, Position 2, etc. |

## Detailed Mapping

### First 10 Cells Example:
| Serial Monitor | Parameter | ESPHome Display Position | ESPHome Sensor |
|---------------|-----------|-------------------------|----------------|
| Cell 1        | u1        | Position 0              | id(u1)         |
| Cell 2        | u2        | Position 1              | id(u2)         |
| Cell 3        | u3        | Position 2              | id(u3)         |
| Cell 4        | u4        | Position 3              | id(u4)         |
| Cell 5        | u5        | Position 4              | id(u5)         |
| Cell 6        | u6        | Position 5              | id(u6)         |
| Cell 7        | u7        | Position 6              | id(u7)         |
| Cell 8        | u8        | Position 7              | id(u8)         |
| Cell 9        | u9        | Position 8              | id(u9)         |
| Cell 10       | u10       | Position 9              | id(u10)        |

## Balancing Example
If the serial monitor shows:
```
Cell 5 (Chip0:R4): 3.850V (BALANCING-Bit4)
Cell 12 (Chip0:R11): 3.855V (BALANCING-Bit11)
```

Then the ESPHome interface will show:
- Balance cell list: "5,12" (1-based from BMS)
- Visual display: Bars at positions 4 and 11 (0-based display positions)
- ESPHome logs: u5 and u12 sensors are balancing

## Key Points
1. **The systems are working correctly** - they just use different numbering conventions
2. **Serial Monitor uses 1-based numbering** to match human-readable cell identification
3. **ESPHome uses 0-based numbering** internally for array indexing and display positioning
4. **Both systems refer to the same physical cells** - just with different numbering

## Visual Reference
```
Serial Monitor: Cell 1  Cell 2  Cell 3  Cell 4  Cell 5  ...
ESPHome Display: Pos 0  Pos 1   Pos 2   Pos 3   Pos 4   ...
Parameters:       u1     u2      u3      u4      u5     ...
```

## Fix Applied
The redundant cell voltage parameter setting in `updateParametersFromBATMan()` has been removed. Now only the main BATMan system sets the u1-u108 parameters, ensuring proper alignment:

**Before Fix:**
- Serial Monitor: "Cell 1" data
- ESPHome: Data appeared in u2 (offset by 1)

**After Fix:**
- Serial Monitor: "Cell 1" data  
- ESPHome: Data correctly appears in u1 (properly aligned)

## Cell Numbering Now Correctly Aligned
The cell numbering is now properly synchronized between both systems. 