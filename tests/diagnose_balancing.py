#!/usr/bin/env python3
"""
Tesla BMS Balancing Diagnostics Script
Connects to the BMS and checks actual minimum cell voltage to explain balancing behavior
"""

import serial
import time
import sys

def main():
    # Try different common COM ports
    ports_to_try = ['COM3', 'COM4', 'COM5', 'COM6', 'COM7', 'COM8', 'COM9', 'COM10']
    
    print('=== Tesla BMS Balancing Diagnostics ===')
    print('\nTrying to connect to BMS...')
    
    ser = None
    connected_port = None
    
    for port in ports_to_try:
        try:
            print(f'Trying {port}...')
            ser = serial.Serial(port, 115200, timeout=2.0)
            time.sleep(1)  # Give it time to settle
            
            # Clear buffers
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            # Test with a simple command
            ser.write(b'param get umin\n')
            ser.flush()
            time.sleep(0.5)
            
            response = ser.readline().decode().strip()
            if response and ('umin' in response or ':' in response):
                print(f'✅ Connected to {port}')
                connected_port = port
                break
            else:
                ser.close()
                print(f'❌ {port} - no valid response')
        except Exception as e:
            print(f'❌ {port} - {e}')
            if ser:
                ser.close()
                ser = None
    
    if ser and connected_port:
        print(f'\n🔗 Using {connected_port} for diagnostics')
        
        try:
            # Get voltage statistics
            print('\n1. Voltage Statistics:')
            commands = ['param get umin', 'param get umax', 'param get CellMin', 'param get CellMax', 'param get deltaV']
            for cmd in commands:
                try:
                    ser.write((cmd + '\n').encode())
                    ser.flush()
                    time.sleep(0.2)
                    response = ser.readline().decode().strip()
                    if response:
                        print(f'  {response}')
                    else:
                        print(f'  {cmd}: No response')
                except Exception as e:
                    print(f'  {cmd}: Error - {e}')
            
            # Get balancing status
            print('\n2. Balancing Status:')
            balance_commands = ['param get balance', 'param get CellsBalancing', 'param get BalanceCellList']
            for cmd in balance_commands:
                try:
                    ser.write((cmd + '\n').encode())
                    ser.flush()
                    time.sleep(0.2)
                    response = ser.readline().decode().strip()
                    if response:
                        print(f'  {response}')
                    else:
                        print(f'  {cmd}: No response')
                except Exception as e:
                    print(f'  {cmd}: Error - {e}')
            
            # Check first 20 cells for low voltages
            print('\n3. First 20 Cell Voltages (checking for low cells):')
            for i in range(1, 21):
                try:
                    cmd = f'param get u{i}'
                    ser.write((cmd + '\n').encode())
                    ser.flush()
                    time.sleep(0.1)
                    response = ser.readline().decode().strip()
                    if response and ':' in response:
                        voltage_str = response.split(':')[1].strip()
                        try:
                            voltage = float(voltage_str)
                            if voltage < 3670:  # Less than 3.67V (3.69V - 20mV threshold)
                                print(f'  ⚠️  Cell {i}: {voltage}mV (BELOW THRESHOLD)')
                            elif voltage < 3690:  # Close to the 3.69V we see being balanced
                                print(f'  📍 Cell {i}: {voltage}mV (NEAR THRESHOLD)')
                            else:
                                print(f'  Cell {i}: {voltage}mV')
                        except ValueError:
                            print(f'  Cell {i}: {voltage_str} (invalid format)')
                    else:
                        print(f'  Cell {i}: {response or "No response"}')
                except Exception as e:
                    print(f'  Cell {i}: Error - {e}')
            
            # Check additional cells for extremely low voltages
            print('\n4. Checking additional cells (u21-u50):')
            low_cells_found = []
            for i in range(21, 51):
                try:
                    cmd = f'param get u{i}'
                    ser.write((cmd + '\n').encode())
                    ser.flush()
                    time.sleep(0.1)
                    response = ser.readline().decode().strip()
                    if response and ':' in response:
                        voltage_str = response.split(':')[1].strip()
                        try:
                            voltage = float(voltage_str)
                            if voltage < 3670:  # Below threshold
                                low_cells_found.append((i, voltage))
                                print(f'  ⚠️  Cell {i}: {voltage}mV (BELOW THRESHOLD)')
                            elif voltage < 100:  # Extremely low
                                low_cells_found.append((i, voltage))
                                print(f'  🚨 Cell {i}: {voltage}mV (CRITICALLY LOW)')
                        except ValueError:
                            pass
                except Exception:
                    pass
            
            if low_cells_found:
                print(f'\n🔍 Found {len(low_cells_found)} cells below threshold:')
                for cell_num, voltage in low_cells_found:
                    print(f'   Cell {cell_num}: {voltage}mV')
            else:
                print('\n✅ No critically low cells found in u21-u50 range')
        
        finally:
            ser.close()
            print(f'\n🔌 Disconnected from {connected_port}')
    else:
        print('\n❌ Could not connect to any serial port')
        print('\nManual steps:')
        print('1. Check if BMS is powered on')
        print('2. Check USB cable connection')
        print('3. Check Windows Device Manager for COM ports')
        print('4. Try running: param get umin')
    
    print('\n=== Diagnostics Complete ===')

if __name__ == '__main__':
    main() 