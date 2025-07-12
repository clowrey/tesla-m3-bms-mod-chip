#!/usr/bin/env python3
"""
Tesla BMS Parameter API Tests
Tests for the serial parameter API functionality
"""

import time
import re
from typing import Dict, List, Optional, Any
from test_framework import BMSTestFramework, TestCase, SerialTestInterface


class ParameterAPITests:
    """
    Parameter API test suite for Tesla BMS
    Tests all parameter-related functionality via serial interface
    """
    
    def __init__(self, framework: BMSTestFramework):
        """
        Initialize parameter API tests
        
        Args:
            framework: Test framework instance
        """
        self.framework = framework
        self.serial = framework.get_serial_interface("usb")
        self.hw_serial = framework.get_serial_interface("hw")
        
        # Test data
        self.test_parameters = {
            # Integer parameters
            "balance": [0, 1],
            "numbmbs": [1, 2, 4],
            "CellsPresent": [23, 25, 48, 96],
            
            # Float parameters (if any)
            "current": [0.0, 1.5, -2.3],
            "as8510_temp": [25.0, 35.5, -10.2],
            
            # Voltage parameters (in mV)
            "u1": [3700, 3800, 4200],
            "u2": [3750, 3850, 4150],
            "umax": [4200, 4000, 3900],
            "umin": [3700, 3650, 3800],
            "uavg": [3850, 3900, 4000],
            
            # String parameters
            "BalanceCellList": ["1,2,3", "5,10,15", ""],
        }
        
        # Expected parameter patterns
        self.parameter_patterns = {
            "balance": r"balance=[01]",
            "numbmbs": r"numbmbs=\d+",
            "CellsPresent": r"CellsPresent=\d+",
            "current": r"current=[-+]?\d*\.?\d+",
            "as8510_temp": r"as8510_temp=[-+]?\d*\.?\d+",
            "u1": r"u1=\d+",
            "BalanceCellList": r"BalanceCellList=.*",
        }
    
    def create_test_cases(self) -> List[TestCase]:
        """
        Create all parameter API test cases
        
        Returns:
            List of test cases
        """
        test_cases = []
        
        # Basic functionality tests
        test_cases.extend([
            TestCase(
                name="param_help_command",
                description="Test param help command",
                function=self.test_param_help_command,
                tags=["parameter", "help", "basic"],
                timeout=10
            ),
            TestCase(
                name="param_list_command",
                description="Test param list command",
                function=self.test_param_list_command,
                tags=["parameter", "list", "basic"],
                timeout=15
            ),
            TestCase(
                name="param_get_valid_parameters",
                description="Test param get with valid parameters",
                function=self.test_param_get_valid_parameters,
                tags=["parameter", "get", "basic"],
                timeout=20
            ),
            TestCase(
                name="param_get_invalid_parameters",
                description="Test param get with invalid parameters",
                function=self.test_param_get_invalid_parameters,
                tags=["parameter", "get", "error"],
                timeout=10
            ),
            TestCase(
                name="param_set_valid_parameters",
                description="Test param set with valid parameters",
                function=self.test_param_set_valid_parameters,
                tags=["parameter", "set", "basic"],
                timeout=30
            ),
            TestCase(
                name="param_set_invalid_parameters",
                description="Test param set with invalid parameters",
                function=self.test_param_set_invalid_parameters,
                tags=["parameter", "set", "error"],
                timeout=15
            ),
        ])
        
        # Cell voltage tests
        test_cases.extend([
            TestCase(
                name="param_cell_voltage_range",
                description="Test cell voltage parameter range (u1-u108)",
                function=self.test_param_cell_voltage_range,
                tags=["parameter", "voltage", "cells"],
                timeout=60
            ),
            TestCase(
                name="param_voltage_statistics",
                description="Test voltage statistics parameters",
                function=self.test_param_voltage_statistics,
                tags=["parameter", "voltage", "stats"],
                timeout=20
            ),
        ])
        
        # Balance control tests
        test_cases.extend([
            TestCase(
                name="param_balance_control",
                description="Test balance control parameter",
                function=self.test_param_balance_control,
                tags=["parameter", "balance", "control"],
                timeout=20
            ),
            TestCase(
                name="param_balance_cell_list",
                description="Test balance cell list parameter",
                function=self.test_param_balance_cell_list,
                tags=["parameter", "balance", "list"],
                timeout=15
            ),
        ])
        
        # Dual serial interface tests
        test_cases.extend([
            TestCase(
                name="param_dual_serial_consistency",
                description="Test parameter consistency between USB and HW serial",
                function=self.test_param_dual_serial_consistency,
                tags=["parameter", "dual", "serial"],
                timeout=30
            ),
            TestCase(
                name="param_concurrent_access",
                description="Test concurrent parameter access on both interfaces",
                function=self.test_param_concurrent_access,
                tags=["parameter", "concurrent", "serial"],
                timeout=25
            ),
        ])
        
        # Error handling tests
        test_cases.extend([
            TestCase(
                name="param_malformed_commands",
                description="Test malformed parameter commands",
                function=self.test_param_malformed_commands,
                tags=["parameter", "error", "malformed"],
                timeout=15
            ),
            TestCase(
                name="param_boundary_values",
                description="Test parameter boundary values",
                function=self.test_param_boundary_values,
                tags=["parameter", "boundary", "validation"],
                timeout=20
            ),
        ])
        
        return test_cases
    
    def test_param_help_command(self):
        """Test the param help command"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            responses = self.serial.send_command("param help", expected_lines=10)
            
            # Should get help text
            help_text = "\n".join(responses)
            self.framework.assert_response_contains(help_text, "param list", case_sensitive=False)
            self.framework.assert_response_contains(help_text, "param get", case_sensitive=False)
            self.framework.assert_response_contains(help_text, "param set", case_sensitive=False)
    
    def test_param_list_command(self):
        """Test the param list command"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            responses = self.serial.send_command("param list", expected_lines=50)
            
            # Should get parameter list
            param_list = "\n".join(responses)
            
            # Check for expected parameters
            expected_params = ["balance", "numbmbs", "CellsPresent", "u1", "u2", "umax", "umin"]
            for param in expected_params:
                self.framework.assert_response_contains(param_list, f"{param}=", case_sensitive=False)
    
    def test_param_get_valid_parameters(self):
        """Test param get with valid parameters"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test common parameters
            test_params = ["balance", "numbmbs", "CellsPresent", "u1", "u2", "umax", "umin"]
            
            for param in test_params:
                responses = self.serial.send_command(f"param get {param}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    # Should contain parameter=value format
                    self.framework.assert_response_matches(response, rf"{param}=.*")
                else:
                    raise AssertionError(f"No response for param get {param}")
    
    def test_param_get_invalid_parameters(self):
        """Test param get with invalid parameters"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test invalid parameters
            invalid_params = ["invalid_param", "notexist", "u109", "u0", "balance2"]
            
            for param in invalid_params:
                responses = self.serial.send_command(f"param get {param}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    # Should contain error message
                    self.framework.assert_response_contains(response, "Error", case_sensitive=False)
                    self.framework.assert_response_contains(response, "Unknown parameter", case_sensitive=False)
    
    def test_param_set_valid_parameters(self):
        """Test param set with valid parameters"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test setting and getting balance parameter
            original_balance = self._get_parameter_value("balance")
            
            # Set to opposite value
            new_value = "1" if original_balance == "0" else "0"
            
            # Set parameter
            responses = self.serial.send_command(f"param set balance {new_value}", expected_lines=1)
            if responses:
                response = responses[0]
                self.framework.assert_response_contains(response, "Set balance", case_sensitive=False)
            
            # Verify it was set
            time.sleep(0.5)  # Small delay for parameter update
            current_value = self._get_parameter_value("balance")
            if current_value != new_value:
                raise AssertionError(f"Parameter not set correctly: expected {new_value}, got {current_value}")
            
            # Restore original value
            self.serial.send_command(f"param set balance {original_balance}", expected_lines=1)
    
    def test_param_set_invalid_parameters(self):
        """Test param set with invalid parameters"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test invalid parameters
            invalid_tests = [
                ("invalid_param", "123"),
                ("u109", "4000"),
                ("balance", "invalid_value"),
                ("numbmbs", "-1"),
            ]
            
            for param, value in invalid_tests:
                responses = self.serial.send_command(f"param set {param} {value}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    # Should contain error message
                    self.framework.assert_response_contains(response, "Error", case_sensitive=False)
    
    def test_param_cell_voltage_range(self):
        """Test cell voltage parameter range (u1-u108)"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test first few and last few cell parameters
            test_cells = [1, 2, 3, 4, 5, 104, 105, 106, 107, 108]
            
            for cell in test_cells:
                param = f"u{cell}"
                responses = self.serial.send_command(f"param get {param}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    # Should contain voltage value (typically in mV)
                    self.framework.assert_response_matches(response, rf"{param}=\d+")
                else:
                    raise AssertionError(f"No response for param get {param}")
    
    def test_param_voltage_statistics(self):
        """Test voltage statistics parameters"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test voltage statistics
            stats_params = ["umax", "umin", "uavg", "deltaV", "CellMax", "CellMin"]
            
            for param in stats_params:
                responses = self.serial.send_command(f"param get {param}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    # Should contain numeric value
                    self.framework.assert_response_matches(response, rf"{param}=\d+")
                else:
                    raise AssertionError(f"No response for param get {param}")
    
    def test_param_balance_control(self):
        """Test balance control parameter"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Get current balance state
            original_balance = self._get_parameter_value("balance")
            
            # Test setting balance to 1
            responses = self.serial.send_command("param set balance 1", expected_lines=1)
            if responses:
                self.framework.assert_response_contains(responses[0], "Set balance", case_sensitive=False)
            
            # Verify it was set
            time.sleep(0.5)
            current_balance = self._get_parameter_value("balance")
            if current_balance != "1":
                raise AssertionError(f"Balance parameter not set to 1: got {current_balance}")
            
            # Test setting balance to 0
            responses = self.serial.send_command("param set balance 0", expected_lines=1)
            if responses:
                self.framework.assert_response_contains(responses[0], "Set balance", case_sensitive=False)
            
            # Verify it was set
            time.sleep(0.5)
            current_balance = self._get_parameter_value("balance")
            if current_balance != "0":
                raise AssertionError(f"Balance parameter not set to 0: got {current_balance}")
            
            # Restore original state
            self.serial.send_command(f"param set balance {original_balance}", expected_lines=1)
    
    def test_param_balance_cell_list(self):
        """Test balance cell list parameter"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Get current balance cell list
            responses = self.serial.send_command("param get BalanceCellList", expected_lines=1)
            
            if responses:
                response = responses[0]
                # Should contain BalanceCellList parameter
                self.framework.assert_response_matches(response, r"BalanceCellList=.*")
            else:
                raise AssertionError("No response for param get BalanceCellList")
    
    def test_param_dual_serial_consistency(self):
        """Test parameter consistency between USB and HW serial"""
        if not self.serial or not self.hw_serial:
            raise RuntimeError("Both serial interfaces not available")
        
        # Test parameters on both interfaces
        test_params = ["balance", "numbmbs", "CellsPresent", "u1", "u2"]
        
        for param in test_params:
            # Get from USB serial
            with self.serial.command_session():
                usb_responses = self.serial.send_command(f"param get {param}", expected_lines=1)
                usb_value = usb_responses[0] if usb_responses else ""
            
            # Get from HW serial
            with self.hw_serial.command_session():
                hw_responses = self.hw_serial.send_command(f"param get {param}", expected_lines=1)
                hw_value = hw_responses[0] if hw_responses else ""
            
            # Values should be identical
            if usb_value != hw_value:
                raise AssertionError(f"Parameter {param} inconsistent: USB='{usb_value}', HW='{hw_value}'")
    
    def test_param_concurrent_access(self):
        """Test concurrent parameter access on both interfaces"""
        if not self.serial or not self.hw_serial:
            raise RuntimeError("Both serial interfaces not available")
        
        # Test concurrent access
        import threading
        import time
        
        results = {}
        errors = []
        
        def access_usb():
            try:
                if self.serial:
                    with self.serial.command_session():
                        responses = self.serial.send_command("param get balance", expected_lines=1)
                        results['usb'] = responses[0] if responses else ""
                else:
                    errors.append("USB serial interface not available")
            except Exception as e:
                errors.append(f"USB error: {e}")
        
        def access_hw():
            try:
                if self.hw_serial:
                    with self.hw_serial.command_session():
                        responses = self.hw_serial.send_command("param get balance", expected_lines=1)
                        results['hw'] = responses[0] if responses else ""
                else:
                    errors.append("HW serial interface not available")
            except Exception as e:
                errors.append(f"HW error: {e}")
        
        # Start concurrent access
        usb_thread = threading.Thread(target=access_usb)
        hw_thread = threading.Thread(target=access_hw)
        
        usb_thread.start()
        hw_thread.start()
        
        usb_thread.join(timeout=5)
        hw_thread.join(timeout=5)
        
        # Check for errors
        if errors:
            raise AssertionError(f"Concurrent access errors: {errors}")
        
        # Check results
        if 'usb' not in results or 'hw' not in results:
            raise AssertionError("Concurrent access failed to get results")
    
    def test_param_malformed_commands(self):
        """Test malformed parameter commands"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test malformed commands
            malformed_commands = [
                "param",
                "param get",
                "param set",
                "param set balance",
                "param invalid_command",
                "param get invalid param name",
                "param set balance 1 extra",
            ]
            
            for cmd in malformed_commands:
                responses = self.serial.send_command(cmd, expected_lines=1)
                
                if responses:
                    response = responses[0]
                    # Should contain error message
                    self.framework.assert_response_contains(response, "Error", case_sensitive=False)
    
    def test_param_boundary_values(self):
        """Test parameter boundary values"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test boundary values for balance parameter
            boundary_tests = [
                ("balance", "0"),    # Minimum
                ("balance", "1"),    # Maximum
                ("balance", "2"),    # Over maximum (should error)
                ("balance", "-1"),   # Under minimum (should error)
            ]
            
            for param, value in boundary_tests:
                responses = self.serial.send_command(f"param set {param} {value}", expected_lines=1)
                
                if responses:
                    response = responses[0]
                    if value in ["0", "1"]:
                        # Should succeed
                        self.framework.assert_response_contains(response, "Set", case_sensitive=False)
                    else:
                        # Should error
                        self.framework.assert_response_contains(response, "Error", case_sensitive=False)
    
    def _get_parameter_value(self, param_name: str) -> str:
        """
        Get current value of a parameter
        
        Args:
            param_name: Name of parameter to get
            
        Returns:
            Parameter value as string
        """
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        responses = self.serial.send_command(f"param get {param_name}", expected_lines=1)
        
        if responses:
            response = responses[0]
            # Extract value from "param=value" format
            match = re.search(rf"{param_name}=([^,\s]+)", response)
            if match:
                return match.group(1)
        
        raise RuntimeError(f"Could not get value for parameter {param_name}")


def register_parameter_tests(framework: BMSTestFramework):
    """
    Register all parameter API tests with the framework
    
    Args:
        framework: Test framework instance
    """
    param_tests = ParameterAPITests(framework)
    test_cases = param_tests.create_test_cases()
    
    for test_case in test_cases:
        framework.add_test_case(test_case)
    
    framework.logger.info(f"Registered {len(test_cases)} parameter API test cases")


if __name__ == "__main__":
    # Example usage
    from test_framework import BMSTestFramework
    
    # Initialize framework
    framework = BMSTestFramework()
    
    # Register parameter tests
    register_parameter_tests(framework)
    
    # Setup connections
    if framework.setup_serial_connections():
        # Run parameter tests only
        framework.run_all_tests(filter_tags=["parameter"])
    else:
        print("Failed to establish serial connections")
    
    framework.cleanup_serial_connections() 