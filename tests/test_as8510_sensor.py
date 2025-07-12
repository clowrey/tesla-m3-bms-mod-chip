#!/usr/bin/env python3
"""
Tesla BMS AS8510 Current Sensor Tests
Tests for the AS8510 analog front-end current sensor functionality
"""

import time
import re
from typing import Dict, List, Optional, Any, Tuple
from test_framework import BMSTestFramework, TestCase, SerialTestInterface


class AS8510SensorTests:
    """
    AS8510 current sensor test suite for Tesla BMS
    Tests all AS8510-related functionality via serial interface
    """
    
    def __init__(self, framework: BMSTestFramework):
        """
        Initialize AS8510 sensor tests
        
        Args:
            framework: Test framework instance
        """
        self.framework = framework
        self.serial = framework.get_serial_interface("usb")
        self.hw_serial = framework.get_serial_interface("hw")
        
        # Test state tracking
        self.sensor_initialized = False
        self.diagnostic_results = {}
        self.current_readings = []
        self.temperature_readings = []
    
    def create_test_cases(self) -> List[TestCase]:
        """
        Create all AS8510 sensor test cases
        
        Returns:
            List of test cases
        """
        test_cases = []
        
        # Basic sensor initialization tests
        test_cases.extend([
            TestCase(
                name="as8510_start_command",
                description="Test AS8510 start command",
                function=self.test_as8510_start_command,
                tags=["as8510", "start", "initialization", "basic"],
                timeout=20
            ),
            TestCase(
                name="as8510_initialization_status",
                description="Test AS8510 initialization status",
                function=self.test_as8510_initialization_status,
                tags=["as8510", "status", "initialization", "basic"],
                timeout=15
            ),
        ])
        
        # Diagnostic tests
        test_cases.extend([
            TestCase(
                name="as8510_current_diagnostics",
                description="Test AS8510 current diagnostics command",
                function=self.test_as8510_current_diagnostics,
                tags=["as8510", "diagnostics", "current", "basic"],
                timeout=30
            ),
            TestCase(
                name="as8510_error_codes",
                description="Test AS8510 error codes command",
                function=self.test_as8510_error_codes,
                tags=["as8510", "errors", "diagnostics", "basic"],
                timeout=15
            ),
            TestCase(
                name="as8510_saturation_flags",
                description="Test AS8510 saturation flags command",
                function=self.test_as8510_saturation_flags,
                tags=["as8510", "saturation", "diagnostics", "basic"],
                timeout=15
            ),
            TestCase(
                name="as8510_complete_diagnostics",
                description="Test AS8510 complete diagnostics command",
                function=self.test_as8510_complete_diagnostics,
                tags=["as8510", "diagnostics", "complete", "comprehensive"],
                timeout=40
            ),
        ])
        
        # Current measurement tests
        test_cases.extend([
            TestCase(
                name="as8510_current_reading",
                description="Test AS8510 current reading from parameters",
                function=self.test_as8510_current_reading,
                tags=["as8510", "current", "reading", "measurement"],
                timeout=25
            ),
            TestCase(
                name="as8510_current_consistency",
                description="Test AS8510 current reading consistency",
                function=self.test_as8510_current_consistency,
                tags=["as8510", "current", "consistency", "measurement"],
                timeout=35
            ),
            TestCase(
                name="as8510_current_range_validation",
                description="Test AS8510 current reading range validation",
                function=self.test_as8510_current_range_validation,
                tags=["as8510", "current", "validation", "range"],
                timeout=20
            ),
        ])
        
        # Temperature monitoring tests
        test_cases.extend([
            TestCase(
                name="as8510_temperature_reading",
                description="Test AS8510 temperature reading",
                function=self.test_as8510_temperature_reading,
                tags=["as8510", "temperature", "reading", "monitoring"],
                timeout=20
            ),
            TestCase(
                name="as8510_temperature_validation",
                description="Test AS8510 temperature validation",
                function=self.test_as8510_temperature_validation,
                tags=["as8510", "temperature", "validation", "monitoring"],
                timeout=25
            ),
        ])
        
        # Communication and reliability tests
        test_cases.extend([
            TestCase(
                name="as8510_spi_communication",
                description="Test AS8510 SPI communication reliability",
                function=self.test_as8510_spi_communication,
                tags=["as8510", "spi", "communication", "reliability"],
                timeout=45
            ),
            TestCase(
                name="as8510_non_blocking_diagnostics",
                description="Test AS8510 non-blocking diagnostic operation",
                function=self.test_as8510_non_blocking_diagnostics,
                tags=["as8510", "diagnostics", "non-blocking", "reliability"],
                timeout=50
            ),
        ])
        
        # Dual serial interface tests
        test_cases.extend([
            TestCase(
                name="as8510_dual_serial_commands",
                description="Test AS8510 commands on dual serial interfaces",
                function=self.test_as8510_dual_serial_commands,
                tags=["as8510", "dual", "serial", "interfaces"],
                timeout=35
            ),
            TestCase(
                name="as8510_concurrent_diagnostics",
                description="Test concurrent AS8510 diagnostics",
                function=self.test_as8510_concurrent_diagnostics,
                tags=["as8510", "concurrent", "diagnostics", "interfaces"],
                timeout=40
            ),
        ])
        
        # Error handling and edge cases
        test_cases.extend([
            TestCase(
                name="as8510_invalid_commands",
                description="Test invalid AS8510 commands",
                function=self.test_as8510_invalid_commands,
                tags=["as8510", "error", "invalid", "commands"],
                timeout=20
            ),
            TestCase(
                name="as8510_repeated_initialization",
                description="Test repeated AS8510 initialization",
                function=self.test_as8510_repeated_initialization,
                tags=["as8510", "initialization", "repeated", "reliability"],
                timeout=35
            ),
            TestCase(
                name="as8510_diagnostic_interruption",
                description="Test AS8510 diagnostic interruption and recovery",
                function=self.test_as8510_diagnostic_interruption,
                tags=["as8510", "diagnostics", "interruption", "recovery"],
                timeout=40
            ),
        ])
        
        return test_cases
    
    def test_as8510_start_command(self):
        """Test AS8510 start command"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test start as8510 command
            responses = self.serial.send_command("start as8510", expected_lines=1)
            
            if responses:
                response = responses[0]
                # Should contain some indication of starting or initialization
                self.framework.logger.info(f"AS8510 start response: {response}")
                
                # Allow time for initialization
                time.sleep(2.0)
                
                # Check if initialization was successful
                # This depends on the actual implementation
                self.sensor_initialized = True
            else:
                self.framework.logger.warning("No response to start as8510 command")
            
            # Test alias command
            responses = self.serial.send_command("as8510 start", expected_lines=1)
            if responses:
                response = responses[0]
                self.framework.logger.info(f"AS8510 start alias response: {response}")
    
    def test_as8510_initialization_status(self):
        """Test AS8510 initialization status"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Try to get current reading to check if sensor is responding
            responses = self.serial.send_command("param get current", expected_lines=1)
            
            if responses:
                response = responses[0]
                match = re.search(r"current=([-+]?\d*\.?\d+)", response)
                if match:
                    current_value = float(match.group(1))
                    self.framework.logger.info(f"AS8510 current reading: {current_value}A")
                    self.sensor_initialized = True
                else:
                    self.framework.logger.warning("Could not extract current value from response")
            else:
                self.framework.logger.warning("No response for current parameter")
    
    def test_as8510_current_diagnostics(self):
        """Test AS8510 current diagnostics command"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test current diag command
            responses = self.serial.send_command("current diag", expected_lines=5)
            
            if responses:
                diag_output = "\n".join(responses)
                self.framework.logger.info("AS8510 current diagnostics started")
                
                # Allow diagnostics to complete (non-blocking implementation)
                time.sleep(10.0)  # Wait for diagnostic completion
                
                # The diagnostic should run in the background
                # Check for any additional output or status
                self.diagnostic_results['current_diag'] = diag_output
            else:
                self.framework.logger.warning("No response to current diag command")
            
            # Test alias command
            responses = self.serial.send_command("diag current", expected_lines=1)
            if responses:
                self.framework.logger.info("AS8510 diag current alias executed")
    
    def test_as8510_error_codes(self):
        """Test AS8510 error codes command"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test as8510 errors command
            responses = self.serial.send_command("as8510 errors", expected_lines=5)
            
            if responses:
                error_output = "\n".join(responses)
                self.framework.logger.info(f"AS8510 error codes: {error_output}")
                
                # Look for error indicators
                error_patterns = [
                    r"error",
                    r"fault",
                    r"fail",
                    r"timeout",
                ]
                
                for pattern in error_patterns:
                    if re.search(pattern, error_output, re.IGNORECASE):
                        self.framework.logger.warning(f"Potential error detected: {pattern}")
                
                self.diagnostic_results['error_codes'] = error_output
            else:
                self.framework.logger.warning("No response to as8510 errors command")
            
            # Test alias command
            responses = self.serial.send_command("errors", expected_lines=5)
            if responses:
                self.framework.logger.info("AS8510 errors alias executed")
    
    def test_as8510_saturation_flags(self):
        """Test AS8510 saturation flags command"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test as8510 saturation command
            responses = self.serial.send_command("as8510 saturation", expected_lines=5)
            
            if responses:
                saturation_output = "\n".join(responses)
                self.framework.logger.info(f"AS8510 saturation flags: {saturation_output}")
                
                # Look for saturation indicators
                saturation_patterns = [
                    r"saturation",
                    r"overflow",
                    r"underflow",
                    r"clipping",
                ]
                
                for pattern in saturation_patterns:
                    if re.search(pattern, saturation_output, re.IGNORECASE):
                        self.framework.logger.warning(f"Saturation detected: {pattern}")
                
                self.diagnostic_results['saturation_flags'] = saturation_output
            else:
                self.framework.logger.warning("No response to as8510 saturation command")
            
            # Test alias command
            responses = self.serial.send_command("saturation", expected_lines=5)
            if responses:
                self.framework.logger.info("AS8510 saturation alias executed")
    
    def test_as8510_complete_diagnostics(self):
        """Test AS8510 complete diagnostics command"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test as8510 diagnostics command
            responses = self.serial.send_command("as8510 diagnostics", expected_lines=10)
            
            if responses:
                complete_diag_output = "\n".join(responses)
                self.framework.logger.info("AS8510 complete diagnostics executed")
                
                # Should contain comprehensive diagnostic information
                expected_patterns = [
                    r"current",
                    r"sensor",
                    r"status",
                    r"register",
                ]
                
                pattern_found = False
                for pattern in expected_patterns:
                    if re.search(pattern, complete_diag_output, re.IGNORECASE):
                        pattern_found = True
                        break
                
                if not pattern_found:
                    self.framework.logger.warning("No expected diagnostic patterns found")
                
                self.diagnostic_results['complete_diagnostics'] = complete_diag_output
            else:
                self.framework.logger.warning("No response to as8510 diagnostics command")
            
            # Test alias command
            responses = self.serial.send_command("diagnostics", expected_lines=10)
            if responses:
                self.framework.logger.info("AS8510 diagnostics alias executed")
    
    def test_as8510_current_reading(self):
        """Test AS8510 current reading from parameters"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Get current reading
            responses = self.serial.send_command("param get current", expected_lines=1)
            
            if responses:
                response = responses[0]
                match = re.search(r"current=([-+]?\d*\.?\d+)", response)
                if match:
                    current_value = float(match.group(1))
                    
                    # Validate current range (reasonable for BMS application)
                    # Typical range might be -100A to +100A
                    if not (-100.0 <= current_value <= 100.0):
                        self.framework.logger.warning(
                            f"Current reading {current_value}A outside expected range"
                        )
                    
                    self.current_readings.append(current_value)
                    self.framework.logger.info(f"AS8510 current reading: {current_value}A")
                else:
                    raise AssertionError("Could not extract current value from response")
            else:
                raise AssertionError("No response for current parameter")
    
    def test_as8510_current_consistency(self):
        """Test AS8510 current reading consistency"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Take multiple current readings
            readings = []
            
            for i in range(10):
                responses = self.serial.send_command("param get current", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    match = re.search(r"current=([-+]?\d*\.?\d+)", response)
                    if match:
                        current_value = float(match.group(1))
                        readings.append(current_value)
                    else:
                        raise AssertionError(f"Could not extract current value from response: {response}")
                else:
                    raise AssertionError(f"No response for reading {i+1}")
                
                time.sleep(0.5)  # Small delay between readings
            
            # Analyze consistency
            if len(readings) > 1:
                min_current = min(readings)
                max_current = max(readings)
                current_spread = max_current - min_current
                average_current = sum(readings) / len(readings)
                
                # Check for reasonable stability
                # Current might vary depending on system load
                max_allowed_spread = 5.0  # 5A spread
                if current_spread > max_allowed_spread:
                    self.framework.logger.warning(
                        f"Current readings inconsistent: spread {current_spread:.3f}A > {max_allowed_spread}A"
                    )
                
                self.framework.logger.info(
                    f"Current consistency: avg={average_current:.3f}A, "
                    f"range={min_current:.3f}A to {max_current:.3f}A, "
                    f"spread={current_spread:.3f}A"
                )
                
                # Cache readings for other tests
                self.current_readings.extend(readings)
    
    def test_as8510_current_range_validation(self):
        """Test AS8510 current reading range validation"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Get current reading
            responses = self.serial.send_command("param get current", expected_lines=1)
            
            if responses:
                response = responses[0]
                match = re.search(r"current=([-+]?\d*\.?\d+)", response)
                if match:
                    current_value = float(match.group(1))
                    
                    # Validate that current is a reasonable floating-point number
                    if not isinstance(current_value, (int, float)):
                        raise AssertionError(f"Current value is not numeric: {current_value}")
                    
                    # Check for special values that might indicate sensor issues
                    if current_value == float('inf') or current_value == float('-inf'):
                        raise AssertionError(f"Current reading is infinite: {current_value}")
                    
                    if current_value != current_value:  # NaN check
                        raise AssertionError("Current reading is NaN")
                    
                    # Check for extremely large values that might indicate sensor malfunction
                    if abs(current_value) > 1000.0:  # More than 1000A is suspicious
                        self.framework.logger.warning(
                            f"Extremely large current reading: {current_value}A"
                        )
                    
                    self.framework.logger.info(f"Current range validation passed: {current_value}A")
                else:
                    raise AssertionError("Could not extract current value")
            else:
                raise AssertionError("No response for current parameter")
    
    def test_as8510_temperature_reading(self):
        """Test AS8510 temperature reading"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Get AS8510 temperature reading
            responses = self.serial.send_command("param get as8510_temp", expected_lines=1)
            
            if responses:
                response = responses[0]
                match = re.search(r"as8510_temp=([-+]?\d*\.?\d+)", response)
                if match:
                    temp_value = float(match.group(1))
                    
                    # Validate temperature range (reasonable for electronics)
                    if not (-40.0 <= temp_value <= 125.0):
                        self.framework.logger.warning(
                            f"AS8510 temperature {temp_value}°C outside expected range"
                        )
                    
                    self.temperature_readings.append(temp_value)
                    self.framework.logger.info(f"AS8510 temperature: {temp_value}°C")
                else:
                    self.framework.logger.warning("Could not extract AS8510 temperature value")
            else:
                self.framework.logger.warning("No response for as8510_temp parameter")
    
    def test_as8510_temperature_validation(self):
        """Test AS8510 temperature validation"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Take multiple temperature readings
            temp_readings = []
            
            for i in range(5):
                responses = self.serial.send_command("param get as8510_temp", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    match = re.search(r"as8510_temp=([-+]?\d*\.?\d+)", response)
                    if match:
                        temp_value = float(match.group(1))
                        temp_readings.append(temp_value)
                
                time.sleep(1.0)  # Delay between temperature readings
            
            # Analyze temperature stability
            if len(temp_readings) > 1:
                min_temp = min(temp_readings)
                max_temp = max(temp_readings)
                temp_spread = max_temp - min_temp
                
                # Temperature should be relatively stable over short time
                max_allowed_spread = 10.0  # 10°C spread
                if temp_spread > max_allowed_spread:
                    self.framework.logger.warning(
                        f"AS8510 temperature readings unstable: spread {temp_spread:.1f}°C"
                    )
                
                self.framework.logger.info(
                    f"AS8510 temperature stability: {min_temp:.1f}°C to {max_temp:.1f}°C "
                    f"(spread: {temp_spread:.1f}°C)"
                )
    
    def test_as8510_spi_communication(self):
        """Test AS8510 SPI communication reliability"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test repeated parameter access to stress SPI communication
            success_count = 0
            total_attempts = 20
            
            for i in range(total_attempts):
                responses = self.serial.send_command("param get current", expected_lines=1)
                
                if responses and len(responses) > 0:
                    response = responses[0]
                    if "current=" in response:
                        success_count += 1
                
                time.sleep(0.2)  # Small delay between attempts
            
            success_rate = (success_count / total_attempts) * 100
            
            if success_rate < 90.0:  # Less than 90% success rate is concerning
                self.framework.logger.warning(
                    f"AS8510 SPI communication reliability low: {success_rate:.1f}%"
                )
            else:
                self.framework.logger.info(
                    f"AS8510 SPI communication reliability: {success_rate:.1f}%"
                )
            
            # Ensure minimum reliability
            if success_rate < 50.0:
                raise AssertionError(f"AS8510 SPI communication too unreliable: {success_rate:.1f}%")
    
    def test_as8510_non_blocking_diagnostics(self):
        """Test AS8510 non-blocking diagnostic operation"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Start diagnostic
            responses = self.serial.send_command("current diag", expected_lines=1)
            start_time = time.time()
            
            if responses:
                self.framework.logger.info("AS8510 diagnostic started")
                
                # While diagnostic is running, test system responsiveness
                for i in range(10):
                    time.sleep(1.0)
                    
                    # Try to get a parameter - system should still be responsive
                    responses = self.serial.send_command("param get balance", expected_lines=1)
                    
                    if not responses:
                        raise AssertionError(f"System not responsive during diagnostic at iteration {i+1}")
                    
                    # Check if diagnostic is still running or completed
                    elapsed_time = time.time() - start_time
                    if elapsed_time > 30:  # Maximum expected diagnostic time
                        break
                
                self.framework.logger.info("AS8510 non-blocking diagnostic test completed")
            else:
                self.framework.logger.warning("No response to current diag command")
    
    def test_as8510_dual_serial_commands(self):
        """Test AS8510 commands on dual serial interfaces"""
        if not self.serial or not self.hw_serial:
            raise RuntimeError("Both serial interfaces not available")
        
        # Test AS8510 commands on USB serial
        with self.serial.command_session():
            usb_responses = self.serial.send_command("param get current", expected_lines=1)
            usb_result = usb_responses[0] if usb_responses else ""
        
        # Test AS8510 commands on HW serial
        with self.hw_serial.command_session():
            hw_responses = self.hw_serial.send_command("param get current", expected_lines=1)
            hw_result = hw_responses[0] if hw_responses else ""
        
        # Both interfaces should work
        if not usb_result:
            raise AssertionError("USB serial interface failed for AS8510 command")
        
        if not hw_result:
            raise AssertionError("HW serial interface failed for AS8510 command")
        
        # Results should be consistent (within reasonable tolerance)
        usb_match = re.search(r"current=([-+]?\d*\.?\d+)", usb_result)
        hw_match = re.search(r"current=([-+]?\d*\.?\d+)", hw_result)
        
        if usb_match and hw_match:
            usb_current = float(usb_match.group(1))
            hw_current = float(hw_match.group(1))
            
            current_diff = abs(usb_current - hw_current)
            if current_diff > 1.0:  # More than 1A difference is suspicious
                self.framework.logger.warning(
                    f"Large current difference between interfaces: {current_diff:.3f}A"
                )
        
        self.framework.logger.info("AS8510 dual serial interface test completed")
    
    def test_as8510_concurrent_diagnostics(self):
        """Test concurrent AS8510 diagnostics"""
        if not self.serial or not self.hw_serial:
            raise RuntimeError("Both serial interfaces not available")
        
        import threading
        results = {}
        errors = []
        
        def usb_diagnostic():
            try:
                if self.serial:
                    with self.serial.command_session():
                        responses = self.serial.send_command("as8510 errors", expected_lines=5)
                        results['usb'] = responses if responses else []
                else:
                    errors.append("USB serial interface not available")
            except Exception as e:
                errors.append(f"USB error: {e}")
        
        def hw_diagnostic():
            try:
                if self.hw_serial:
                    with self.hw_serial.command_session():
                        responses = self.hw_serial.send_command("as8510 saturation", expected_lines=5)
                        results['hw'] = responses if responses else []
                else:
                    errors.append("HW serial interface not available")
            except Exception as e:
                errors.append(f"HW error: {e}")
        
        # Start concurrent diagnostics
        usb_thread = threading.Thread(target=usb_diagnostic)
        hw_thread = threading.Thread(target=hw_diagnostic)
        
        usb_thread.start()
        hw_thread.start()
        
        usb_thread.join(timeout=15)
        hw_thread.join(timeout=15)
        
        # Check for errors
        if errors:
            raise AssertionError(f"Concurrent diagnostic errors: {errors}")
        
        # Verify results
        if 'usb' not in results or 'hw' not in results:
            raise AssertionError("Concurrent diagnostics failed to complete")
        
        self.framework.logger.info("AS8510 concurrent diagnostics test completed")
    
    def test_as8510_invalid_commands(self):
        """Test invalid AS8510 commands"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test invalid AS8510 commands
            invalid_commands = [
                "as8510 invalid",
                "current invalid",
                "as8510 errors invalid",
                "diagnostics invalid",
                "saturation invalid",
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
            
            self.framework.logger.info("AS8510 invalid command test completed")
    
    def test_as8510_repeated_initialization(self):
        """Test repeated AS8510 initialization"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test repeated start commands
            for i in range(5):
                responses = self.serial.send_command("start as8510", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    self.framework.logger.info(f"AS8510 start attempt {i+1}: {response}")
                
                time.sleep(2.0)  # Allow initialization time
                
                # Verify sensor is still responding
                responses = self.serial.send_command("param get current", expected_lines=1)
                if not responses:
                    raise AssertionError(f"AS8510 not responding after start attempt {i+1}")
            
            self.framework.logger.info("AS8510 repeated initialization test completed")
    
    def test_as8510_diagnostic_interruption(self):
        """Test AS8510 diagnostic interruption and recovery"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Start a long-running diagnostic
            responses = self.serial.send_command("current diag", expected_lines=1)
            
            if responses:
                self.framework.logger.info("AS8510 diagnostic started")
                
                # Wait a bit, then try to interrupt with another command
                time.sleep(2.0)
                
                # Try to start another diagnostic (might be rejected or queued)
                responses = self.serial.send_command("as8510 diagnostics", expected_lines=5)
                
                # System should handle this gracefully
                if responses:
                    self.framework.logger.info("AS8510 concurrent diagnostic handled")
                
                # Wait for completion
                time.sleep(15.0)
                
                # Verify system is still responsive
                responses = self.serial.send_command("param get current", expected_lines=1)
                if not responses:
                    raise AssertionError("AS8510 not responsive after diagnostic interruption")
                
                self.framework.logger.info("AS8510 diagnostic interruption test completed")
            else:
                self.framework.logger.warning("No response to initial diagnostic command")


def register_as8510_tests(framework: BMSTestFramework):
    """
    Register all AS8510 sensor tests with the framework
    
    Args:
        framework: Test framework instance
    """
    as8510_tests = AS8510SensorTests(framework)
    test_cases = as8510_tests.create_test_cases()
    
    for test_case in test_cases:
        framework.add_test_case(test_case)
    
    framework.logger.info(f"Registered {len(test_cases)} AS8510 sensor test cases")


if __name__ == "__main__":
    # Example usage
    from test_framework import BMSTestFramework
    
    # Initialize framework
    framework = BMSTestFramework()
    
    # Register AS8510 tests
    register_as8510_tests(framework)
    
    # Setup connections
    if framework.setup_serial_connections():
        # Run AS8510 tests only
        framework.run_all_tests(filter_tags=["as8510"])
    else:
        print("Failed to establish serial connections")
    
    framework.cleanup_serial_connections() 