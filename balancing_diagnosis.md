# Tesla BMS Balancing Diagnostics

## Issue Description
Cells at 3.69V are being balanced despite appearing to be the "lowest" cells in the graph. This indicates that the true minimum cell voltage in the pack is actually lower than 3.69V.

## Balancing Logic Explanation
The Tesla BMS balances any cell that is more than 20mV above the lowest cell voltage:
```
Balance if: Cell_Voltage > (Minimum_Cell_Voltage + 20mV)
```

## Diagnostic Commands

### 1. Check Current Minimum Cell Voltage
```bash
# Get the actual minimum cell voltage
param get umin
param get CellMin

# Get voltage statistics
param get umax
param get deltaV
param get CellsBalancing
```

### 2. Get Complete Cell Voltage List
```bash
# Get all cell voltages (u1-u108)
param list | grep "^u[0-9]"

# Or get specific ranges
param get u1 u2 u3 u4 u5 u6 u7 u8 u9 u10
param get u11 u12 u13 u14 u15 u16 u17 u18 u19 u20
# ... continue for all cells
```

### 3. Check Balance Status
```bash
# Check balance cell list
param get BalanceCellList

# Check balance enable status
param get balance
```

### 4. Enable Debug Mode
```bash
# Enable detailed BMS debug output
debug bms on

# Watch the serial console for detailed cell voltage information
# Look for the "Individual Cell Voltages" section
```

## What to Look For

### 1. Identify the True Minimum Cell
- Look for cells with voltage < 3.69V
- Check for cells with extremely low readings (< 1V)
- Identify damaged or disconnected cells

### 2. Check for Problem Cells
- Cells reading 0V or very low voltages
- Cells with unrealistic readings
- Cells that might be physically damaged

### 3. Verify Balancing Threshold
- Calculate: `minimum_voltage + 20mV`
- Confirm cells above this threshold are being balanced
- Verify cells below this threshold are NOT being balanced

## Expected Findings

If cells at 3.69V are being balanced, you should find:
- At least one cell with voltage < 3.67V (3.69V - 20mV)
- The `umin` parameter shows a voltage lower than 3.67V
- The `CellMin` parameter points to a cell not visible in your graph

## Potential Solutions

### 1. If a Cell is Damaged/Disconnected
- Identify the physically damaged cell
- Check wiring connections
- Consider replacing the cell or BMB module

### 2. If a Cell is Reading Incorrectly
- Check BMB communication
- Verify SPI connections
- Reset the BMS system

### 3. If the Minimum is Correct
- Your balancing is working properly
- The 3.69V cells are correctly being balanced
- Consider adjusting the balancing hysteresis if needed

## Manual Verification

You can also manually check by:
1. Temporarily disabling balancing: `balance off`
2. Wait for voltage readings to stabilize
3. Check all cell voltages: `param list | grep "^u[0-9]"`
4. Find the true minimum
5. Re-enable balancing: `balance on`

## Hysteresis Adjustment (If Needed)

If you want to change the 20mV hysteresis:
- This requires modifying the firmware
- Edit `BalHys` in `src/BatMan.cpp`
- Recompile and upload the firmware

## Contact Information
If you need further assistance, provide:
- Output of `param get umin CellMin umax deltaV`
- Complete cell voltage list
- Debug output showing individual cell voltages
