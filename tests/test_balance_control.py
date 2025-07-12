#!/usr/bin/env python3
"""
Tesla BMS Balance Control Tests
Tests for the balance control functionality via serial interface
"""

import time
import re
from typing import Dict, List, Optional, Any
from test_framework import BMSTestFramework, TestCase, SerialTestInterface


class BalanceControlTests:
    """
    Balance control test suite for Tesla BMS
    Tests all balance-related functionality via serial interface
    """
    
    def __init__(self, framework: BMSTestFramework):
        """
        Initialize balance control tests
        
        Args:
            framework: Test framework instance
        """
        self.framework = framework
        self.serial = framework.get_serial_interface("usb")
        self.hw_serial = framework.get_serial_interface("hw")
        
        # Test state tracking
        self.original_balance_state = None
        self.test_start_time = None
    
    def create_test_cases(self) -> List[TestCase]:
        """
        Create all balance control test cases
        
        Returns:
            List of test cases
        """
        test_cases = []
        
        # Basic balance control tests
        test_cases.extend([
            TestCase(
                name="balance_on_command",
                description="Test balance on command",
                function=self.test_balance_on_command,
                setup=self.setup_balance_test,
                teardown=self.teardown_balance_test,
                tags=["balance", "command", "basic"],
                timeout=15
            ),
            TestCase(
                name="balance_off_command",
                description="Test balance off command",
                function=self.test_balance_off_command,
                setup=self.setup_balance_test,
                teardown=self.teardown_balance_test,
                tags=["balance", "command", "basic"],
                timeout=15
            ),
            TestCase(
                name="balance_status_command",
                description="Test balance status command",
                function=self.test_balance_status_command,
                tags=["balance", "status", "basic"],
                timeout=10
            ),
            TestCase(
                name="balance_enable_disable_aliases",
                description="Test balance enable/disable command aliases",
                function=self.test_balance_enable_disable_aliases,
                setup=self.setup_balance_test,
                teardown=self.teardown_balance_test,
                tags=["balance", "aliases", "basic"],
                timeout=20
            ),
        ])
        
        # Parameter synchronization tests
        test_cases.extend([
            TestCase(
                name="balance_parameter_sync",
                description="Test synchronization between balance commands and parameter API",
                function=self.test_balance_parameter_sync,
                setup=self.setup_balance_test,
                teardown=self.teardown_balance_test,
                tags=["balance", "parameter", "sync"],
                timeout=25
            ),
            TestCase(
                name="balance_state_persistence",
                description="Test balance state persistence",
                function=self.test_balance_state_persistence,
                setup=self.setup_balance_test,
                teardown=self.teardown_balance_test,
                tags=["balance", "persistence", "state"],
                timeout=30
            ),
        ])
        
        # Balance cell list tests
        test_cases.extend([
            TestCase(
                name="balance_cell_list_monitoring",
                description="Test balance cell list parameter monitoring",
                function=self.test_balance_cell_list_monitoring,
                setup=self.setup_balance_test,
                teardown=self.teardown_balance_test,
                tags=["balance", "cell", "list"],
                timeout=35
            ),
            TestCase(
                name="balance_cells_balancing_count",
                description="Test CellsBalancing parameter updates",
                function=self.test_balance_cells_balancing_count,
                setup=self.setup_balance_test,
                teardown=self.teardown_balance_test,
                tags=["balance", "count", "cells"],
                timeout=30
            ),
        ])
        
        # Dual serial interface tests
        test_cases.extend([
            TestCase(
                name="balance_dual_serial_consistency",
                description="Test balance control consistency across serial interfaces",
                function=self.test_balance_dual_serial_consistency,
                setup=self.setup_balance_test,
                teardown=self.teardown_balance_test,
                tags=["balance", "dual", "serial"],
                timeout=35
            ),
            TestCase(
                name="balance_concurrent_commands",
                description="Test concurrent balance commands on different interfaces",
                function=self.test_balance_concurrent_commands,
                setup=self.setup_balance_test,
                teardown=self.teardown_balance_test,
                tags=["balance", "concurrent", "commands"],
                timeout=40
            ),
        ])
        
        # Error handling and edge cases
        test_cases.extend([
            TestCase(
                name="balance_invalid_commands",
                description="Test invalid balance commands",
                function=self.test_balance_invalid_commands,
                tags=["balance", "error", "invalid"],
                timeout=15
            ),
            TestCase(
                name="balance_rapid_toggle",
                description="Test rapid balance on/off toggling",
                function=self.test_balance_rapid_toggle,
                setup=self.setup_balance_test,
                teardown=self.teardown_balance_test,
                tags=["balance", "rapid", "toggle"],
                timeout=45
            ),
        ])
        
        return test_cases
    
    def setup_balance_test(self):
        """Setup for balance control tests"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        # Record original balance state
        self.original_balance_state = self._get_balance_state()
        self.test_start_time = time.time()
        
        # Ensure we start with a known state (balance off)
        self._set_balance_state(False)
        time.sleep(0.5)  # Allow state to settle
    
    def teardown_balance_test(self):
        """Teardown for balance control tests"""
        if not self.serial:
            return
        
        # Restore original balance state
        if self.original_balance_state is not None:
            self._set_balance_state(self.original_balance_state)
        
        # Clear any pending responses
        self.serial.flush_responses()
    
    def test_balance_on_command(self):
        """Test balance on command"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Send balance on command
            responses = self.serial.send_command("balance on", expected_lines=1)
            
            if responses:
                response = responses[0]
                self.framework.assert_response_contains(response, "Balance ENABLED", case_sensitive=False)
            else:
                raise AssertionError("No response to balance on command")
            
            # Verify state change
            time.sleep(0.5)
            current_state = self._get_balance_state()
            if not current_state:
                raise AssertionError("Balance state not changed to enabled")
    
    def test_balance_off_command(self):
        """Test balance off command"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # First enable balance
            self.serial.send_command("balance on", expected_lines=1)
            time.sleep(0.5)
            
            # Send balance off command
            responses = self.serial.send_command("balance off", expected_lines=1)
            
            if responses:
                response = responses[0]
                self.framework.assert_response_contains(response, "Balance DISABLED", case_sensitive=False)
            else:
                raise AssertionError("No response to balance off command")
            
            # Verify state change
            time.sleep(0.5)
            current_state = self._get_balance_state()
            if current_state:
                raise AssertionError("Balance state not changed to disabled")
    
    def test_balance_status_command(self):
        """Test balance status command"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test status when balance is off
            self._set_balance_state(False)
            time.sleep(0.5)
            
            responses = self.serial.send_command("balance status", expected_lines=1)
            if responses:
                response = responses[0]
                self.framework.assert_response_contains(response, "DISABLED", case_sensitive=False)
            else:
                raise AssertionError("No response to balance status command")
            
            # Test status when balance is on
            self._set_balance_state(True)
            time.sleep(0.5)
            
            responses = self.serial.send_command("balance status", expected_lines=1)
            if responses:
                response = responses[0]
                self.framework.assert_response_contains(response, "ENABLED", case_sensitive=False)
            else:
                raise AssertionError("No response to balance status command")
    
    def test_balance_enable_disable_aliases(self):
        """Test balance enable/disable command aliases"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test balance enable alias
            responses = self.serial.send_command("balance enable", expected_lines=1)
            if responses:
                response = responses[0]
                self.framework.assert_response_contains(response, "Balance ENABLED", case_sensitive=False)
            
            # Verify state
            time.sleep(0.5)
            current_state = self._get_balance_state()
            if not current_state:
                raise AssertionError("Balance enable alias failed")
            
            # Test balance disable alias
            responses = self.serial.send_command("balance disable", expected_lines=1)
            if responses:
                response = responses[0]
                self.framework.assert_response_contains(response, "Balance DISABLED", case_sensitive=False)
            
            # Verify state
            time.sleep(0.5)
            current_state = self._get_balance_state()
            if current_state:
                raise AssertionError("Balance disable alias failed")
    
    def test_balance_parameter_sync(self):
        """Test synchronization between balance commands and parameter API"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test command to parameter sync
            self.serial.send_command("balance on", expected_lines=1)
            time.sleep(0.5)
            
            # Check parameter value
            param_value = self._get_parameter_value("balance")
            if param_value != "1":
                raise AssertionError(f"Parameter not synced: expected 1, got {param_value}")
            
            # Test parameter to command sync
            self.serial.send_command("param set balance 0", expected_lines=1)
            time.sleep(0.5)
            
            # Check command state
            responses = self.serial.send_command("balance status", expected_lines=1)
            if responses:
                response = responses[0]
                self.framework.assert_response_contains(response, "DISABLED", case_sensitive=False)
            else:
                raise AssertionError("No response to balance status command")
    
    def test_balance_state_persistence(self):
        """Test balance state persistence"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Set balance on
            self.serial.send_command("balance on", expected_lines=1)
            time.sleep(0.5)
            
            # Check state multiple times with delays
            for i in range(5):
                current_state = self._get_balance_state()
                if not current_state:
                    raise AssertionError(f"Balance state not persistent at check {i+1}")
                time.sleep(1.0)
            
            # Set balance off
            self.serial.send_command("balance off", expected_lines=1)
            time.sleep(0.5)
            
            # Check state persistence
            for i in range(5):
                current_state = self._get_balance_state()
                if current_state:
                    raise AssertionError(f"Balance off state not persistent at check {i+1}")
                time.sleep(1.0)
    
    def test_balance_cell_list_monitoring(self):
        """Test balance cell list parameter monitoring"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Enable balance
            self.serial.send_command("balance on", expected_lines=1)
            time.sleep(2.0)  # Allow balancing to start
            
            # Check balance cell list
            responses = self.serial.send_command("param get BalanceCellList", expected_lines=1)
            if responses:
                response = responses[0]
                self.framework.assert_response_matches(response, r"BalanceCellList=.*")
                
                # Extract the cell list
                match = re.search(r"BalanceCellList=(.+)", response)
                if match:
                    cell_list = match.group(1).strip()
                    self.framework.logger.info(f"Current balance cell list: {cell_list}")
                    
                    # If cells are being balanced, the list should not be empty
                    # (This depends on actual cell voltages and balance thresholds)
                    if cell_list and cell_list != "":
                        # Validate cell list format (comma-separated numbers)
                        cells = cell_list.split(',')
                        for cell in cells:
                            if cell.strip():
                                try:
                                    cell_num = int(cell.strip())
                                    if cell_num < 1 or cell_num > 108:
                                        raise AssertionError(f"Invalid cell number in list: {cell_num}")
                                except ValueError:
                                    raise AssertionError(f"Invalid cell number format: {cell}")
            else:
                raise AssertionError("No response for BalanceCellList parameter")
    
    def test_balance_cells_balancing_count(self):
        """Test CellsBalancing parameter updates"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Start with balance off
            self.serial.send_command("balance off", expected_lines=1)
            time.sleep(0.5)
            
            # Check initial count
            count_off = self._get_parameter_value("CellsBalancing")
            if count_off != "0":
                self.framework.logger.warning(f"CellsBalancing not 0 when balance off: {count_off}")
            
            # Enable balance
            self.serial.send_command("balance on", expected_lines=1)
            time.sleep(2.0)  # Allow balancing to start
            
            # Check count after enabling
            count_on = self._get_parameter_value("CellsBalancing")
            
            # The count should be a valid integer
            try:
                count_value = int(count_on)
                if count_value < 0:
                    raise AssertionError(f"Invalid CellsBalancing count: {count_value}")
                self.framework.logger.info(f"CellsBalancing count: {count_value}")
            except ValueError:
                raise AssertionError(f"Invalid CellsBalancing format: {count_on}")
            
            # Disable balance
            self.serial.send_command("balance off", expected_lines=1)
            time.sleep(1.0)
            
            # Check count after disabling
            count_off_final = self._get_parameter_value("CellsBalancing")
            if count_off_final != "0":
                self.framework.logger.warning(f"CellsBalancing not 0 after balance off: {count_off_final}")
    
    def test_balance_dual_serial_consistency(self):
        """Test balance control consistency across serial interfaces"""
        if not self.serial or not self.hw_serial:
            raise RuntimeError("Both serial interfaces not available")
        
        # Test balance on via USB, check via HW serial
        with self.serial.command_session():
            self.serial.send_command("balance on", expected_lines=1)
            time.sleep(0.5)
        
        with self.hw_serial.command_session():
            responses = self.hw_serial.send_command("balance status", expected_lines=1)
            if responses:
                response = responses[0]
                self.framework.assert_response_contains(response, "ENABLED", case_sensitive=False)
            else:
                raise AssertionError("No response from HW serial for balance status")
        
        # Test balance off via HW serial, check via USB
        with self.hw_serial.command_session():
            self.hw_serial.send_command("balance off", expected_lines=1)
            time.sleep(0.5)
        
        with self.serial.command_session():
            responses = self.serial.send_command("balance status", expected_lines=1)
            if responses:
                response = responses[0]
                self.framework.assert_response_contains(response, "DISABLED", case_sensitive=False)
            else:
                raise AssertionError("No response from USB serial for balance status")
    
    def test_balance_concurrent_commands(self):
        """Test concurrent balance commands on different interfaces"""
        if not self.serial or not self.hw_serial:
            raise RuntimeError("Both serial interfaces not available")
        
        import threading
        results = {}
        errors = []
        
        def usb_command():
            try:
                if self.serial:
                    with self.serial.command_session():
                        responses = self.serial.send_command("balance on", expected_lines=1)
                        results['usb'] = responses[0] if responses else ""
                else:
                    errors.append("USB serial interface not available")
            except Exception as e:
                errors.append(f"USB error: {e}")
        
        def hw_command():
            try:
                if self.hw_serial:
                    with self.hw_serial.command_session():
                        responses = self.hw_serial.send_command("balance status", expected_lines=1)
                        results['hw'] = responses[0] if responses else ""
                else:
                    errors.append("HW serial interface not available")
            except Exception as e:
                errors.append(f"HW error: {e}")
        
        # Start concurrent commands
        usb_thread = threading.Thread(target=usb_command)
        hw_thread = threading.Thread(target=hw_command)
        
        usb_thread.start()
        hw_thread.start()
        
        usb_thread.join(timeout=10)
        hw_thread.join(timeout=10)
        
        # Check for errors
        if errors:
            raise AssertionError(f"Concurrent command errors: {errors}")
        
        # Verify results
        if 'usb' in results and results['usb']:
            self.framework.assert_response_contains(results['usb'], "ENABLED", case_sensitive=False)
        
        if 'hw' in results and results['hw']:
            # HW serial should eventually show the enabled state
            time.sleep(1.0)  # Allow state to propagate
            with self.hw_serial.command_session():
                responses = self.hw_serial.send_command("balance status", expected_lines=1)
                if responses:
                    self.framework.assert_response_contains(responses[0], "ENABLED", case_sensitive=False)
    
    def test_balance_invalid_commands(self):
        """Test invalid balance commands"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Test invalid balance commands
            invalid_commands = [
                "balance",           # No argument
                "balance invalid",   # Invalid argument
                "balance on off",    # Too many arguments
                "balance 1",         # Numeric argument
                "balance true",      # Boolean-like argument
            ]
            
            for cmd in invalid_commands:
                responses = self.serial.send_command(cmd, expected_lines=1)
                if responses:
                    response = responses[0]
                    # Should either be unknown command or help message
                    self.framework.assert_response_contains(
                        response, 
                        "Unknown command", 
                        case_sensitive=False
                    )
    
    def test_balance_rapid_toggle(self):
        """Test rapid balance on/off toggling"""
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        with self.serial.command_session():
            # Rapid toggle test
            for i in range(10):
                # Toggle balance on
                responses = self.serial.send_command("balance on", expected_lines=1)
                if responses:
                    self.framework.assert_response_contains(responses[0], "ENABLED", case_sensitive=False)
                
                # Short delay
                time.sleep(0.2)
                
                # Toggle balance off
                responses = self.serial.send_command("balance off", expected_lines=1)
                if responses:
                    self.framework.assert_response_contains(responses[0], "DISABLED", case_sensitive=False)
                
                # Short delay
                time.sleep(0.2)
            
            # Final state check
            time.sleep(1.0)
            final_state = self._get_balance_state()
            if final_state:
                raise AssertionError("Balance state inconsistent after rapid toggling")
    
    def _get_balance_state(self) -> bool:
        """
        Get current balance state
        
        Returns:
            True if balance is enabled, False otherwise
        """
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        responses = self.serial.send_command("balance status", expected_lines=1)
        if responses:
            response = responses[0]
            return "ENABLED" in response.upper()
        
        raise RuntimeError("Could not get balance state")
    
    def _set_balance_state(self, enabled: bool):
        """
        Set balance state
        
        Args:
            enabled: True to enable balance, False to disable
        """
        if not self.serial:
            raise RuntimeError("USB serial interface not available")
        
        command = "balance on" if enabled else "balance off"
        responses = self.serial.send_command(command, expected_lines=1)
        
        if responses:
            expected_text = "ENABLED" if enabled else "DISABLED"
            self.framework.assert_response_contains(responses[0], expected_text, case_sensitive=False)
    
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


def register_balance_tests(framework: BMSTestFramework):
    """
    Register all balance control tests with the framework
    
    Args:
        framework: Test framework instance
    """
    balance_tests = BalanceControlTests(framework)
    test_cases = balance_tests.create_test_cases()
    
    for test_case in test_cases:
        framework.add_test_case(test_case)
    
    framework.logger.info(f"Registered {len(test_cases)} balance control test cases")


if __name__ == "__main__":
    # Example usage
    from test_framework import BMSTestFramework
    
    # Initialize framework
    framework = BMSTestFramework()
    
    # Register balance tests
    register_balance_tests(framework)
    
    # Setup connections
    if framework.setup_serial_connections():
        # Run balance tests only
        framework.run_all_tests(filter_tags=["balance"])
    else:
        print("Failed to establish serial connections")
    
    framework.cleanup_serial_connections() 