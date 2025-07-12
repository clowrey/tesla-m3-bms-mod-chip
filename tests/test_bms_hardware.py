#!/usr/bin/env python3
"""
Tesla BMS Hardware Tests
Tests for the BMS hardware communication and functionality
"""

import time
import re
from typing import Dict, List, Optional, Any, Tuple
from test_framework import BMSTestFramework, TestCase, SerialTestInterface


class BMSHardwareTests:
    """
    BMS hardware test suite for Tesla BMS
    Tests all hardware-related functionality via serial interface
    """
    
    def __init__(self, framework: BMSTestFramework):
        """
        Initialize BMS hardware tests
        
        Args:
            framework: Test framework instance
        """
        self.framework = framework
        self.serial = framework.get_serial_interface("usb")
        self.hw_serial = framework.get_serial_interface("hw")
        
        # Test state tracking
        self.original_debug_state = None
        self.cell_voltage_cache = {}
        self.temperature_cache = {}
    
    def create_test_cases(self) -> List[TestCase]:
        """
        Create all BMS hardware test cases
        
        Returns:
            List of test cases
        """
        test_cases = []
        
        # Basic hardware communication tests
        test_cases.extend([
            TestCase(
                name="bms_hardware_mapping",
                description="Test BMS hardware mapping command",
                function=self.test_bms_hardware_mapping,
                tags=["bms", "hardware", "mapping", "basic"],
                timeout=15
            ),
            TestCase(
                name="bms_register_access",
                description="Test BMB register access commands",
                function=self.test_bms_register_access,
                tags=["bms", "registers", "access", "basic"],
                timeout=20
            ),
            TestCase(
                name="bms_debug_control",
                description="Test BMB debug control commands",
                function=self.test_bms_debug_control,
                setup=self.setup_debug_test,
                teardown=self.teardown_debug_test,
                tags=["bms", "debug", "control", "basic"],
                timeout=25
            ),
        ])
        
        # Cell voltage tests
        test_cases.extend([
            TestCase(
                name="bms_cell_voltage_reading",
                description="Test cell voltage reading from hardware",
                function=self.test_bms_cell_voltage_reading,
                tags=["bms", "voltage", "cells", "reading"],
                timeout=30
            ),
            TestCase(
                name="bms_voltage_consistency",
                description="Test voltage reading consistency",
                function=self.test_bms_voltage_consistency,
                tags=["bms", "voltage", "consistency", "validation"],
                timeout=35
            ),
            TestCase(
                name="bms_voltage_statistics",
                description="Test voltage statistics calculation",
                function=self.test_bms_voltage_statistics,
                tags=["bms", "voltage", "statistics", "validation"],
                timeout=25
            ),
        ])
        
        # Temperature monitoring tests
        test_cases.extend([
            TestCase(
                name="bms_temperature_monitoring",
                description="Test temperature monitoring functionality",
                function=self.test_bms_temperature_monitoring,
                tags=["bms", "temperature", "monitoring", "sensors"],
                timeout=20
            ),
            TestCase(
                name="bms_temperature_validation",
                description="Test temperature reading validation",
                function=self.test_bms_temperature_validation,
                tags=["bms", "temperature", "validation", "sensors"],
                timeout=25
            ),
        ])
        
        # BMS state machine tests
        test_cases.extend([
            TestCase(
                name="bms_loop_state_monitoring",
                description="Test BMS loop state monitoring",
                function=self.test_bms_loop_state_monitoring,
                tags=["bms", "state", "loop", "monitoring"],
                timeout=30
            ),
            TestCase(
                name="bms_system_parameters",
                description="Test BMS system parameters",
                function=self.test_bms_system_parameters,
                tags=["bms", "system", "parameters", "validation"],
                timeout=20
            ),
        ])
        
        # Hardware diagnostic tests
        test_cases.extend([
            TestCase(
                name="bms_hardware_diagnostics",
                description="Test BMS hardware diagnostic capabilities",
                function=self.test_bms_hardware_diagnostics,
                tags=["bms", "diagnostics", "hardware", "validation"],
                timeout=40
            ),
            TestCase(
                name="bms_register_debug_output",
                description="Test BMB register debug output",
                function=self.test_bms_register_debug_output,
                setup=self.setup_debug_test,
                teardown=self.teardown_debug_test,
                tags=["bms", "debug", "registers", "output"],
                timeout=35
            ),
        ])
        
        # Data integrity tests
        test_cases.extend([
            TestCase(
                name="bms_data_integrity",
                description="Test BMS data integrity over time",
                function=self.test_bms_data_integrity,
                tags=["bms", "data", "integrity", "validation"],
                timeout=60
            ),
            TestCase(
                name="bms_concurrent_access",
                description="Test concurrent BMS data access",
                function=self.test_bms_concurrent_access,
                tags=["bms", "concurrent", "access", "validation"],
                timeout=45
            ),
        ])
        
        # Error handling tests
        test_cases.extend([
            TestCase(
                name="bms_error_handling",
                description="Test BMS error handling and recovery",
                function=self.test_bms_error_handling,
                tags=["bms", "error", "handling", "recovery"],
                timeout=30
            ),
            TestCase(
                name="bms_invalid_commands",
                description="Test invalid BMS commands",
                function=self.test_bms_invalid_commands,
                tags=["bms", "error", "invalid", "commands"],
                timeout=20
            ),
        ])
        
        return test_cases
    
    def setup_debug_test(self):
        """Setup for debug-related tests"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        # Save original debug state
        self.original_debug_state = self._get_debug_state()
        
        # Start with debug off
        self._set_debug_state(False)
        time.sleep(0.5)
    
    def teardown_debug_test(self):
        """Teardown for debug-related tests"""
        if not self.serial:
            return
        
        # Restore original debug state
        if self.original_debug_state is not None:
            self._set_debug_state(self.original_debug_state)
        
        # Clear any pending responses
        self.serial.flush_responses()
    
    def test_bms_hardware_mapping(self):
        """Test BMS hardware mapping command"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test mapping command
            responses = self.serial.send_command("mapping", expected_lines=10)
            
            if responses:
                mapping_output = "\n".join(responses)
                
                # Should contain information about hardware mapping
                # Look for common BMS-related information
                expected_patterns = [
                    r"[Cc]ell",      # Cell information
                    r"[Vv]oltage",   # Voltage information
                    r"[Tt]emp",      # Temperature information
                    r"\d+",          # Numeric values
                ]
                
                for pattern in expected_patterns:
                    if not re.search(pattern, mapping_output):
                        self.framework.logger.warning(f"Pattern '{pattern}' not found in mapping output")
                
                self.framework.logger.info("Hardware mapping command executed successfully")
            else:
                raise AssertionError("No response to mapping command")
            
            # Test debug alias
            responses = self.serial.send_command("debug", expected_lines=5)
            if responses:
                debug_output = "\n".join(responses)
                self.framework.logger.info("Debug alias command executed successfully")
    
    def test_bms_register_access(self):
        """Test BMB register access commands"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test register access commands
            register_commands = ["bmb registers", "registers"]
            
            for cmd in register_commands:
                responses = self.serial.send_command(cmd, expected_lines=5)
                
                if responses:
                    register_output = "\n".join(responses)
                    
                    # Should contain register-related information
                    expected_patterns = [
                        r"[Rr]egister",   # Register keyword
                        r"[Dd]ata",       # Data keyword
                        r"[Aa]nalysis",   # Analysis keyword
                    ]
                    
                    pattern_found = False
                    for pattern in expected_patterns:
                        if re.search(pattern, register_output):
                            pattern_found = True
                            break
                    
                    if not pattern_found:
                        self.framework.logger.warning(f"No expected patterns found in '{cmd}' output")
                    
                    self.framework.logger.info(f"Register command '{cmd}' executed successfully")
                else:
                    self.framework.logger.warning(f"No response to '{cmd}' command")
    
    def test_bms_debug_control(self):
        """Test BMB debug control commands"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test debug on
            responses = self.serial.send_command("bmb debug on", expected_lines=1)
            if responses:
                response = responses[0]
                self.framework.assert_response_contains(response, "debug", case_sensitive=False)
                self.framework.assert_response_contains(response, "enabled", case_sensitive=False)
            
            # Verify debug state
            time.sleep(0.5)
            debug_state = self._get_debug_state()
            if not debug_state:
                self.framework.logger.warning("Debug state not properly set to enabled")
            
            # Test debug off
            responses = self.serial.send_command("bmb debug off", expected_lines=1)
            if responses:
                response = responses[0]
                self.framework.assert_response_contains(response, "debug", case_sensitive=False)
                self.framework.assert_response_contains(response, "disabled", case_sensitive=False)
            
            # Verify debug state
            time.sleep(0.5)
            debug_state = self._get_debug_state()
            if debug_state:
                self.framework.logger.warning("Debug state not properly set to disabled")
            
            # Test debug status
            responses = self.serial.send_command("bmb debug status", expected_lines=1)
            if responses:
                response = responses[0]
                self.framework.assert_response_contains(response, "debug", case_sensitive=False)
    
    def test_bms_cell_voltage_reading(self):
        """Test cell voltage reading from hardware"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Read cell voltages for first few cells
            test_cells = [1, 2, 3, 4, 5]
            voltages = {}
            
            for cell in test_cells:
                responses = self.serial.send_command(f"param get u{cell}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    # Extract voltage value
                    match = re.search(rf"u{cell}=(\d+)", response)
                    if match:
                        voltage_mv = int(match.group(1))
                        voltages[cell] = voltage_mv
                        
                        # Validate voltage range (reasonable for Li-ion cells)
                        if not (2500 <= voltage_mv <= 4500):
                            self.framework.logger.warning(
                                f"Cell {cell} voltage {voltage_mv}mV outside expected range"
                            )
                    else:
                        raise AssertionError(f"Could not extract voltage for cell {cell}")
                else:
                    raise AssertionError(f"No response for cell {cell} voltage")
            
            # Cache voltages for other tests
            self.cell_voltage_cache.update(voltages)
            
            # Verify we got readings for all test cells
            if len(voltages) != len(test_cells):
                raise AssertionError(f"Got {len(voltages)} voltages, expected {len(test_cells)}")
            
            self.framework.logger.info(f"Successfully read voltages for {len(voltages)} cells")
    
    def test_bms_voltage_consistency(self):
        """Test voltage reading consistency"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Read the same cell voltage multiple times
            test_cell = 1
            readings = []
            
            for i in range(5):
                responses = self.serial.send_command(f"param get u{test_cell}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    match = re.search(rf"u{test_cell}=(\d+)", response)
                    if match:
                        voltage_mv = int(match.group(1))
                        readings.append(voltage_mv)
                    else:
                        raise AssertionError(f"Could not extract voltage from response: {response}")
                else:
                    raise AssertionError(f"No response for reading {i+1}")
                
                time.sleep(0.5)  # Small delay between readings
            
            # Check consistency (should be within reasonable tolerance)
            if len(readings) > 1:
                min_voltage = min(readings)
                max_voltage = max(readings)
                voltage_spread = max_voltage - min_voltage
                
                # Allow for some variation due to measurement noise
                max_allowed_spread = 50  # 50mV
                if voltage_spread > max_allowed_spread:
                    self.framework.logger.warning(
                        f"Voltage readings inconsistent: spread {voltage_spread}mV > {max_allowed_spread}mV"
                    )
                
                self.framework.logger.info(
                    f"Voltage consistency check: {min_voltage}-{max_voltage}mV (spread: {voltage_spread}mV)"
                )
    
    def test_bms_voltage_statistics(self):
        """Test voltage statistics calculation"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Get voltage statistics
            stats_params = ["umax", "umin", "uavg", "deltaV", "CellMax", "CellMin"]
            stats = {}
            
            for param in stats_params:
                responses = self.serial.send_command(f"param get {param}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    match = re.search(rf"{param}=(\d+)", response)
                    if match:
                        value = int(match.group(1))
                        stats[param] = value
                    else:
                        raise AssertionError(f"Could not extract {param} value")
                else:
                    raise AssertionError(f"No response for {param}")
            
            # Validate statistics relationships
            if "umax" in stats and "umin" in stats and "deltaV" in stats:
                calculated_delta = stats["umax"] - stats["umin"]
                actual_delta = stats["deltaV"]
                
                # Allow for small calculation differences
                if abs(calculated_delta - actual_delta) > 5:
                    self.framework.logger.warning(
                        f"Delta calculation mismatch: calc={calculated_delta}, actual={actual_delta}"
                    )
            
            # Validate cell numbers
            if "CellMax" in stats and "CellMin" in stats:
                cell_max = stats["CellMax"]
                cell_min = stats["CellMin"]
                
                # Cell numbers should be in valid range
                if not (1 <= cell_max <= 108):
                    self.framework.logger.warning(f"CellMax {cell_max} outside valid range")
                if not (1 <= cell_min <= 108):
                    self.framework.logger.warning(f"CellMin {cell_min} outside valid range")
            
            self.framework.logger.info(f"Voltage statistics: {stats}")
    
    def test_bms_temperature_monitoring(self):
        """Test temperature monitoring functionality"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test temperature parameters
            temp_params = ["Chipt0", "Cellt0_0", "Cellt0_1", "TempMax", "TempMin"]
            temperatures = {}
            
            for param in temp_params:
                responses = self.serial.send_command(f"param get {param}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    match = re.search(rf"{param}=([+-]?\d+)", response)
                    if match:
                        temp_value = int(match.group(1))
                        temperatures[param] = temp_value
                        
                        # Validate temperature range (reasonable for BMS operation)
                        if not (-40 <= temp_value <= 85):
                            self.framework.logger.warning(
                                f"Temperature {param} = {temp_value}°C outside expected range"
                            )
                    else:
                        self.framework.logger.warning(f"Could not extract {param} value")
                else:
                    self.framework.logger.warning(f"No response for {param}")
            
            # Cache temperatures for other tests
            self.temperature_cache.update(temperatures)
            
            self.framework.logger.info(f"Temperature readings: {temperatures}")
    
    def test_bms_temperature_validation(self):
        """Test temperature reading validation"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Get temperature statistics
            if "TempMax" in self.temperature_cache and "TempMin" in self.temperature_cache:
                temp_max = self.temperature_cache["TempMax"]
                temp_min = self.temperature_cache["TempMin"]
                
                # Validate temperature relationship
                if temp_max < temp_min:
                    raise AssertionError(f"TempMax ({temp_max}) < TempMin ({temp_min})")
                
                # Check temperature spread
                temp_spread = temp_max - temp_min
                if temp_spread > 50:  # More than 50°C spread might indicate an issue
                    self.framework.logger.warning(
                        f"Large temperature spread: {temp_spread}°C"
                    )
            
            # Test temperature consistency over time
            responses = self.serial.send_command("param get Chipt0", expected_lines=1)
            if responses:
                response = responses[0]
                match = re.search(r"Chipt0=([+-]?\d+)", response)
                if match:
                    current_temp = int(match.group(1))
                    
                    # Temperature should be relatively stable
                    if "Chipt0" in self.temperature_cache:
                        previous_temp = self.temperature_cache["Chipt0"]
                        temp_change = abs(current_temp - previous_temp)
                        
                        # Allow for reasonable temperature variation
                        if temp_change > 20:  # More than 20°C change is suspicious
                            self.framework.logger.warning(
                                f"Large temperature change: {temp_change}°C"
                            )
    
    def test_bms_loop_state_monitoring(self):
        """Test BMS loop state monitoring"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Monitor loop state parameters
            loop_params = ["LoopCnt", "LoopState", "CellsPresent"]
            initial_state = {}
            
            for param in loop_params:
                responses = self.serial.send_command(f"param get {param}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    match = re.search(rf"{param}=(\d+)", response)
                    if match:
                        value = int(match.group(1))
                        initial_state[param] = value
                    else:
                        raise AssertionError(f"Could not extract {param} value")
                else:
                    raise AssertionError(f"No response for {param}")
            
            # Wait and check again
            time.sleep(2.0)
            
            for param in loop_params:
                responses = self.serial.send_command(f"param get {param}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    match = re.search(rf"{param}=(\d+)", response)
                    if match:
                        new_value = int(match.group(1))
                        
                        if param == "LoopCnt":
                            # Loop count should increase
                            if new_value <= initial_state[param]:
                                self.framework.logger.warning(
                                    f"LoopCnt not increasing: {initial_state[param]} -> {new_value}"
                                )
                        elif param == "CellsPresent":
                            # Cell count should remain stable
                            if new_value != initial_state[param]:
                                self.framework.logger.warning(
                                    f"CellsPresent changed: {initial_state[param]} -> {new_value}"
                                )
            
            self.framework.logger.info(f"Loop state monitoring: {initial_state}")
    
    def test_bms_system_parameters(self):
        """Test BMS system parameters"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test system parameters
            system_params = ["numbmbs", "CellsPresent", "CellsBalancing"]
            system_state = {}
            
            for param in system_params:
                responses = self.serial.send_command(f"param get {param}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    match = re.search(rf"{param}=(\d+)", response)
                    if match:
                        value = int(match.group(1))
                        system_state[param] = value
                        
                        # Validate parameter ranges
                        if param == "numbmbs":
                            if not (1 <= value <= 8):
                                self.framework.logger.warning(f"numbmbs {value} outside expected range")
                        elif param == "CellsPresent":
                            if not (1 <= value <= 108):
                                self.framework.logger.warning(f"CellsPresent {value} outside expected range")
                        elif param == "CellsBalancing":
                            if not (0 <= value <= 108):
                                self.framework.logger.warning(f"CellsBalancing {value} outside expected range")
                    else:
                        raise AssertionError(f"Could not extract {param} value")
                else:
                    raise AssertionError(f"No response for {param}")
            
            self.framework.logger.info(f"System parameters: {system_state}")
    
    def test_bms_hardware_diagnostics(self):
        """Test BMS hardware diagnostic capabilities"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test various diagnostic commands
            diagnostic_commands = [
                "mapping",
                "bmb registers",
                "param list",
            ]
            
            for cmd in diagnostic_commands:
                responses = self.serial.send_command(cmd, expected_lines=5)
                
                if responses:
                    output = "\n".join(responses)
                    
                    # Check for common error indicators
                    error_patterns = [
                        r"error",
                        r"fail",
                        r"timeout",
                        r"communication.*error",
                    ]
                    
                    for pattern in error_patterns:
                        if re.search(pattern, output, re.IGNORECASE):
                            self.framework.logger.warning(f"Potential error in '{cmd}': {pattern}")
                    
                    self.framework.logger.info(f"Diagnostic command '{cmd}' completed")
                else:
                    self.framework.logger.warning(f"No response to diagnostic command: {cmd}")
    
    def test_bms_register_debug_output(self):
        """Test BMB register debug output"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Enable debug
            self.serial.send_command("bmb debug on", expected_lines=1)
            time.sleep(0.5)
            
            # Get some parameter that should trigger register reads
            responses = self.serial.send_command("param get u1", expected_lines=1)
            
            # Allow time for debug output
            time.sleep(1.0)
            
            # Check for additional debug output
            # This depends on the specific debug implementation
            self.framework.logger.info("Register debug test completed")
            
            # Disable debug
            self.serial.send_command("bmb debug off", expected_lines=1)
    
    def test_bms_data_integrity(self):
        """Test BMS data integrity over time"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Sample data multiple times
            sample_count = 10
            sample_interval = 2.0  # seconds
            
            data_samples = []
            
            for i in range(sample_count):
                sample = {}
                
                # Sample key parameters
                params = ["u1", "u2", "umax", "umin", "CellsPresent", "LoopCnt"]
                
                for param in params:
                    responses = self.serial.send_command(f"param get {param}", expected_lines=1)
                    
                    if responses:
                        response = responses[0]
                        match = re.search(rf"{param}=(\d+)", response)
                        if match:
                            sample[param] = int(match.group(1))
                
                data_samples.append(sample)
                
                if i < sample_count - 1:  # Don't wait after the last sample
                    time.sleep(sample_interval)
            
            # Analyze data integrity
            for param in params:
                values = [sample.get(param, 0) for sample in data_samples]
                
                if param == "LoopCnt":
                    # Loop count should be monotonically increasing
                    if not all(values[i] <= values[i+1] for i in range(len(values)-1)):
                        self.framework.logger.warning(f"LoopCnt not monotonically increasing")
                
                elif param in ["CellsPresent"]:
                    # These should remain constant
                    if len(set(values)) > 1:
                        self.framework.logger.warning(f"{param} values not constant: {set(values)}")
                
                elif param in ["u1", "u2", "umax", "umin"]:
                    # Voltages should be reasonable and stable
                    if len(values) > 1:
                        min_val = min(values)
                        max_val = max(values)
                        spread = max_val - min_val
                        
                        if spread > 100:  # More than 100mV spread
                            self.framework.logger.warning(f"{param} spread too large: {spread}mV")
            
            self.framework.logger.info(f"Data integrity test completed with {len(data_samples)} samples")
    
    def test_bms_concurrent_access(self):
        """Test concurrent BMS data access"""
        if not self.serial or not self.hw_serial:
            raise RuntimeError("Both serial interfaces not available")
        
        import threading
        results = {}
        errors = []
        
        def usb_access():
            try:
                if self.serial:
                    with self.serial.command_session():
                        responses = self.serial.send_command("param get u1", expected_lines=1)
                        results['usb'] = responses[0] if responses else ""
                else:
                    errors.append("USB serial interface not available")
            except Exception as e:
                errors.append(f"USB error: {e}")
        
        def hw_access():
            try:
                if self.hw_serial:
                    with self.hw_serial.command_session():
                        responses = self.hw_serial.send_command("param get u2", expected_lines=1)
                        results['hw'] = responses[0] if responses else ""
                else:
                    errors.append("HW serial interface not available")
            except Exception as e:
                errors.append(f"HW error: {e}")
        
        # Start concurrent access
        usb_thread = threading.Thread(target=usb_access)
        hw_thread = threading.Thread(target=hw_access)
        
        usb_thread.start()
        hw_thread.start()
        
        usb_thread.join(timeout=10)
        hw_thread.join(timeout=10)
        
        # Check for errors
        if errors:
            raise AssertionError(f"Concurrent access errors: {errors}")
        
        # Validate results
        if 'usb' in results and results['usb']:
            self.framework.assert_response_matches(results['usb'], r"u1=\d+")
        
        if 'hw' in results and results['hw']:
            self.framework.assert_response_matches(results['hw'], r"u2=\d+")
        
        self.framework.logger.info("Concurrent BMS access test completed")
    
    def test_bms_error_handling(self):
        """Test BMS error handling and recovery"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test various error conditions
            # Note: These tests depend on specific error handling in the BMS
            
            # Test with potentially invalid parameters
            invalid_params = ["u0", "u109", "invalidparam"]
            
            for param in invalid_params:
                responses = self.serial.send_command(f"param get {param}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    # Should get an error response
                    self.framework.assert_response_contains(response, "Error", case_sensitive=False)
                
                # System should still be responsive after error
                time.sleep(0.5)
                responses = self.serial.send_command("param get u1", expected_lines=1)
                if not responses:
                    raise AssertionError(f"System not responsive after error with {param}")
            
            self.framework.logger.info("Error handling test completed")
    
    def test_bms_invalid_commands(self):
        """Test invalid BMS commands"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test invalid commands
            invalid_commands = [
                "invalid_command",
                "bmb invalid",
                "registers invalid",
                "debug invalid",
                "mapping invalid",
            ]
            
            for cmd in invalid_commands:
                responses = self.serial.send_command(cmd, expected_lines=1)
                
                if responses:
                    response = responses[0]
                    # Should get unknown command or error response
                    self.framework.assert_response_contains(
                        response, 
                        "Unknown command", 
                        case_sensitive=False
                    )
                
                # System should remain responsive
                time.sleep(0.2)
                responses = self.serial.send_command("help", expected_lines=5)
                if not responses:
                    raise AssertionError(f"System not responsive after invalid command: {cmd}")
            
            self.framework.logger.info("Invalid command test completed")
    
    def _get_debug_state(self) -> bool:
        """
        Get current debug state
        
        Returns:
            True if debug is enabled, False otherwise
        """
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        responses = self.serial.send_command("bmb debug status", expected_lines=1)
        if responses:
            response = responses[0]
            return "ENABLED" in response.upper()
        
        return False
    
    def _set_debug_state(self, enabled: bool):
        """
        Set debug state
        
        Args:
            enabled: True to enable debug, False to disable
        """
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        command = "bmb debug on" if enabled else "bmb debug off"
        responses = self.serial.send_command(command, expected_lines=1)
        
        if responses:
            expected_text = "ENABLED" if enabled else "DISABLED"
            self.framework.assert_response_contains(responses[0], expected_text, case_sensitive=False)


def register_bms_hardware_tests(framework: BMSTestFramework):
    """
    Register all BMS hardware tests with the framework
    
    Args:
        framework: Test framework instance
    """
    bms_tests = BMSHardwareTests(framework)
    test_cases = bms_tests.create_test_cases()
    
    for test_case in test_cases:
        framework.add_test_case(test_case)
    
    framework.logger.info(f"Registered {len(test_cases)} BMS hardware test cases")


if __name__ == "__main__":
    # Example usage
    from test_framework import BMSTestFramework
    
    # Initialize framework
    framework = BMSTestFramework()
    
    # Register BMS hardware tests
    register_bms_hardware_tests(framework)
    
    # Setup connections
    if framework.setup_serial_connections():
        # Run BMS hardware tests only
        framework.run_all_tests(filter_tags=["bms"])
    else:
        print("Failed to establish serial connections")
    
    framework.cleanup_serial_connections() 