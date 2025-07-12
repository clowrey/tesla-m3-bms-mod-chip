#!/usr/bin/env python3
"""
Tesla Model 3 BMS Test Framework
Automated testing framework for serial port communication with Tesla BMS modification chip
"""

import serial
import time
import threading
import logging
import json
import re
from typing import Dict, List, Optional, Any, Tuple, Callable
from dataclasses import dataclass, field
from enum import Enum
from contextlib import contextmanager
import queue
import sys
from pathlib import Path


class TestResult(Enum):
    """Test result enumeration"""
    PASS = "PASS"
    FAIL = "FAIL"
    SKIP = "SKIP"
    ERROR = "ERROR"


@dataclass
class TestCase:
    """Individual test case representation"""
    name: str
    description: str
    function: Callable
    timeout: int = 30
    setup: Optional[Callable] = None
    teardown: Optional[Callable] = None
    tags: List[str] = field(default_factory=list)
    expected_duration: float = 5.0  # seconds


@dataclass
class TestResults:
    """Test execution results"""
    test_name: str
    result: TestResult
    duration: float
    message: str = ""
    output: str = ""
    error: Optional[Exception] = None
    timestamp: float = field(default_factory=time.time)


class SerialTestInterface:
    """
    Serial communication interface for Tesla BMS testing
    Supports both USB and hardware serial connections
    """
    
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 2.0):
        """
        Initialize serial interface
        
        Args:
            port: Serial port name (e.g., 'COM3', '/dev/ttyUSB0')
            baudrate: Serial communication speed (default: 115200)
            timeout: Read timeout in seconds
        """
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial: Optional[serial.Serial] = None
        self.logger = logging.getLogger(f"SerialTest-{port}")
        self.response_queue = queue.Queue()
        self.reader_thread: Optional[threading.Thread] = None
        self.is_reading = False
        self.command_delay = 0.1  # Delay between commands
        
    def connect(self) -> bool:
        """
        Connect to serial port
        
        Returns:
            bool: True if connection successful
        """
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS
            )
            
            # Clear any existing data
            self.serial.reset_input_buffer()
            self.serial.reset_output_buffer()
            
            # Start reader thread
            self.is_reading = True
            self.reader_thread = threading.Thread(target=self._read_responses, daemon=True)
            self.reader_thread.start()
            
            self.logger.info(f"Connected to {self.port} at {self.baudrate} baud")
            return True
            
        except Exception as e:
            self.logger.error(f"Failed to connect to {self.port}: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from serial port"""
        self.is_reading = False
        if self.reader_thread:
            self.reader_thread.join(timeout=1.0)
        
        if self.serial:
            self.serial.close()
            self.serial = None
        
        self.logger.info(f"Disconnected from {self.port}")
    
    def _read_responses(self):
        """Background thread to read serial responses"""
        buffer = ""
        while self.is_reading and self.serial:
            try:
                if self.serial.in_waiting:
                    data = self.serial.read(self.serial.in_waiting).decode('utf-8', errors='ignore')
                    buffer += data
                    
                    # Process complete lines
                    while '\n' in buffer:
                        line, buffer = buffer.split('\n', 1)
                        line = line.strip()
                        if line:
                            self.response_queue.put(line)
                            self.logger.debug(f"RX: {line}")
                
                time.sleep(0.01)  # Small delay to prevent busy waiting
                
            except Exception as e:
                if self.is_reading:  # Only log if we're still supposed to be reading
                    self.logger.error(f"Error reading from serial: {e}")
                break
    
    def send_command(self, command: str, wait_for_response: bool = True, 
                    timeout: Optional[float] = None, expected_lines: int = 1) -> List[str]:
        """
        Send command and optionally wait for response
        
        Args:
            command: Command to send
            wait_for_response: Whether to wait for response
            timeout: Response timeout (uses default if None)
            expected_lines: Expected number of response lines
            
        Returns:
            List of response lines
        """
        if not self.serial:
            raise RuntimeError("Serial port not connected")
        
        # Clear response queue
        while not self.response_queue.empty():
            try:
                self.response_queue.get_nowait()
            except queue.Empty:
                break
        
        # Send command
        cmd_line = command + '\n'
        self.serial.write(cmd_line.encode('utf-8'))
        self.serial.flush()
        self.logger.debug(f"TX: {command}")
        
        if not wait_for_response:
            return []
        
        # Wait for response
        timeout = timeout if timeout is not None else self.timeout
        responses = []
        start_time = time.time()
        
        while len(responses) < expected_lines and (time.time() - start_time) < timeout:
            try:
                response = self.response_queue.get(timeout=0.1)
                responses.append(response)
            except queue.Empty:
                continue
        
        # Add small delay between commands
        time.sleep(self.command_delay)
        
        return responses
    
    def wait_for_pattern(self, pattern: str, timeout: float = 5.0) -> Optional[str]:
        """
        Wait for a specific pattern in the response
        
        Args:
            pattern: Regex pattern to match
            timeout: Maximum wait time
            
        Returns:
            Matching line or None if timeout
        """
        regex = re.compile(pattern)
        start_time = time.time()
        
        while (time.time() - start_time) < timeout:
            try:
                response = self.response_queue.get(timeout=0.1)
                if regex.search(response):
                    return response
            except queue.Empty:
                continue
        
        return None
    
    def flush_responses(self, timeout: float = 1.0):
        """Flush all pending responses"""
        start_time = time.time()
        while (time.time() - start_time) < timeout:
            try:
                self.response_queue.get_nowait()
            except queue.Empty:
                break
    
    @contextmanager
    def command_session(self):
        """Context manager for command sessions"""
        try:
            yield self
        finally:
            self.flush_responses()


class BMSTestFramework:
    """
    Main test framework for Tesla BMS testing
    Manages test execution, reporting, and serial communication
    """
    
    def __init__(self, config_file: str = "test_config.json"):
        """
        Initialize test framework
        
        Args:
            config_file: Path to test configuration file
        """
        self.config = self._load_config(config_file)
        self.test_cases: List[TestCase] = []
        self.results: List[TestResults] = []
        self.logger = self._setup_logging()
        
        # Serial interfaces
        self.usb_serial: Optional[SerialTestInterface] = None
        self.hw_serial: Optional[SerialTestInterface] = None
        
        # Test state
        self.current_test: Optional[str] = None
        self.test_start_time: float = 0
        
    def _load_config(self, config_file: str) -> Dict[str, Any]:
        """Load test configuration from JSON file"""
        default_config = {
            "usb_serial_port": "COM3",  # Windows default
            "hw_serial_port": "COM4",   # Hardware serial adapter
            "baudrate": 115200,
            "timeout": 2.0,
            "log_level": "INFO",
            "test_output_dir": "test_results",
            "parallel_tests": False,
            "retry_failed_tests": True,
            "max_retries": 3
        }
        
        try:
            with open(config_file, 'r') as f:
                config = json.load(f)
            # Merge with defaults
            for key, value in default_config.items():
                if key not in config:
                    config[key] = value
            return config
        except FileNotFoundError:
            print(f"Config file {config_file} not found, using defaults")
            return default_config
    
    def _setup_logging(self) -> logging.Logger:
        """Setup logging configuration"""
        logger = logging.getLogger("BMSTestFramework")
        logger.setLevel(getattr(logging, self.config["log_level"]))
        
        # Console handler
        console_handler = logging.StreamHandler()
        console_handler.setLevel(logging.INFO)
        console_formatter = logging.Formatter(
            '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
        )
        console_handler.setFormatter(console_formatter)
        logger.addHandler(console_handler)
        
        # File handler
        Path(self.config["test_output_dir"]).mkdir(exist_ok=True)
        file_handler = logging.FileHandler(
            f"{self.config['test_output_dir']}/test_framework.log"
        )
        file_handler.setLevel(logging.DEBUG)
        file_formatter = logging.Formatter(
            '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
        )
        file_handler.setFormatter(file_formatter)
        logger.addHandler(file_handler)
        
        return logger
    
    def setup_serial_connections(self) -> bool:
        """
        Setup serial connections for testing
        
        Returns:
            bool: True if at least one connection successful
        """
        success = False
        
        # Setup USB serial
        if self.config["usb_serial_port"]:
            self.usb_serial = SerialTestInterface(
                self.config["usb_serial_port"],
                self.config["baudrate"],
                self.config["timeout"]
            )
            if self.usb_serial.connect():
                success = True
                self.logger.info("USB serial connection established")
            else:
                self.logger.warning("USB serial connection failed")
        
        # Setup hardware serial
        if self.config["hw_serial_port"]:
            self.hw_serial = SerialTestInterface(
                self.config["hw_serial_port"],
                self.config["baudrate"],
                self.config["timeout"]
            )
            if self.hw_serial.connect():
                success = True
                self.logger.info("Hardware serial connection established")
            else:
                self.logger.warning("Hardware serial connection failed")
        
        return success
    
    def cleanup_serial_connections(self):
        """Cleanup serial connections"""
        if self.usb_serial:
            self.usb_serial.disconnect()
        if self.hw_serial:
            self.hw_serial.disconnect()
    
    def add_test_case(self, test_case: TestCase):
        """Add a test case to the framework"""
        self.test_cases.append(test_case)
        self.logger.debug(f"Added test case: {test_case.name}")
    
    def run_test_case(self, test_case: TestCase) -> TestResults:
        """
        Run a single test case
        
        Args:
            test_case: Test case to run
            
        Returns:
            TestResults: Test execution results
        """
        self.current_test = test_case.name
        self.test_start_time = time.time()
        
        self.logger.info(f"Running test: {test_case.name}")
        
        try:
            # Setup
            if test_case.setup:
                test_case.setup()
            
            # Run test with timeout
            result = self._run_with_timeout(test_case.function, test_case.timeout)
            
            duration = time.time() - self.test_start_time
            
            if result is None:
                return TestResults(
                    test_case.name,
                    TestResult.ERROR,
                    duration,
                    "Test timed out"
                )
            
            return TestResults(
                test_case.name,
                TestResult.PASS,
                duration,
                "Test passed successfully"
            )
            
        except AssertionError as e:
            duration = time.time() - self.test_start_time
            return TestResults(
                test_case.name,
                TestResult.FAIL,
                duration,
                str(e),
                error=e
            )
        
        except Exception as e:
            duration = time.time() - self.test_start_time
            return TestResults(
                test_case.name,
                TestResult.ERROR,
                duration,
                f"Unexpected error: {str(e)}",
                error=e
            )
        
        finally:
            # Teardown
            if test_case.teardown:
                try:
                    test_case.teardown()
                except Exception as e:
                    self.logger.warning(f"Teardown failed for {test_case.name}: {e}")
    
    def _run_with_timeout(self, func: Callable, timeout: int) -> Optional[Any]:
        """Run function with timeout using threading"""
        result = [None]
        exception: List[Optional[Exception]] = [None]
        
        def target():
            try:
                result[0] = func()
            except Exception as e:
                exception[0] = e
        
        thread = threading.Thread(target=target)
        thread.start()
        thread.join(timeout)
        
        if thread.is_alive():
            # Timeout occurred
            return None
        
        if exception[0]:
            raise exception[0]
        
        return result[0]
    
    def run_all_tests(self, filter_tags: Optional[List[str]] = None) -> bool:
        """
        Run all test cases
        
        Args:
            filter_tags: Optional list of tags to filter tests
            
        Returns:
            bool: True if all tests passed
        """
        if not self.test_cases:
            self.logger.warning("No test cases to run")
            return True
        
        # Filter tests by tags
        tests_to_run = self.test_cases
        if filter_tags:
            tests_to_run = [
                test for test in self.test_cases
                if any(tag in test.tags for tag in filter_tags)
            ]
        
        self.logger.info(f"Running {len(tests_to_run)} test cases")
        
        # Run tests
        all_passed = True
        for test_case in tests_to_run:
            result = self.run_test_case(test_case)
            self.results.append(result)
            
            if result.result != TestResult.PASS:
                all_passed = False
                
                # Retry failed tests if configured
                if (self.config["retry_failed_tests"] and 
                    result.result == TestResult.FAIL):
                    self._retry_test(test_case)
        
        # Generate report
        self._generate_report()
        
        return all_passed
    
    def _retry_test(self, test_case: TestCase):
        """Retry a failed test case"""
        for attempt in range(self.config["max_retries"]):
            self.logger.info(f"Retrying test {test_case.name} (attempt {attempt + 1})")
            
            # Wait before retry
            time.sleep(1.0)
            
            result = self.run_test_case(test_case)
            self.results.append(result)
            
            if result.result == TestResult.PASS:
                self.logger.info(f"Test {test_case.name} passed on retry")
                break
    
    def _generate_report(self):
        """Generate test execution report"""
        # Console summary
        total_tests = len(self.results)
        passed = sum(1 for r in self.results if r.result == TestResult.PASS)
        failed = sum(1 for r in self.results if r.result == TestResult.FAIL)
        errors = sum(1 for r in self.results if r.result == TestResult.ERROR)
        
        print(f"\n{'='*50}")
        print(f"TEST EXECUTION SUMMARY")
        print(f"{'='*50}")
        print(f"Total Tests: {total_tests}")
        print(f"Passed: {passed}")
        print(f"Failed: {failed}")
        print(f"Errors: {errors}")
        print(f"Pass Rate: {(passed/total_tests)*100:.1f}%")
        
        # Detailed results
        print(f"\nDETAILED RESULTS:")
        for result in self.results:
            status_symbol = "✓" if result.result == TestResult.PASS else "✗"
            print(f"{status_symbol} {result.test_name} - {result.result.value} ({result.duration:.2f}s)")
            if result.message:
                print(f"  {result.message}")
        
        # Save detailed JSON report
        report_file = f"{self.config['test_output_dir']}/test_report_{int(time.time())}.json"
        with open(report_file, 'w') as f:
            json.dump([{
                'test_name': r.test_name,
                'result': r.result.value,
                'duration': r.duration,
                'message': r.message,
                'timestamp': r.timestamp
            } for r in self.results], f, indent=2)
        
        self.logger.info(f"Detailed report saved to {report_file}")
    
    def get_serial_interface(self, interface_type: str = "usb") -> Optional[SerialTestInterface]:
        """
        Get serial interface for testing
        
        Args:
            interface_type: "usb" or "hw" for hardware serial
            
        Returns:
            SerialTestInterface or None if not available
        """
        if interface_type == "usb":
            return self.usb_serial
        elif interface_type == "hw":
            return self.hw_serial
        else:
            raise ValueError(f"Unknown interface type: {interface_type}")
    
    def assert_response_contains(self, response: str, expected: str, case_sensitive: bool = True):
        """Assert that response contains expected text"""
        if not case_sensitive:
            response = response.lower()
            expected = expected.lower()
        
        if expected not in response:
            raise AssertionError(f"Expected '{expected}' in response, got: '{response}'")
    
    def assert_response_matches(self, response: str, pattern: str):
        """Assert that response matches regex pattern"""
        if not re.search(pattern, response):
            raise AssertionError(f"Response '{response}' does not match pattern '{pattern}'")
    
    def assert_numeric_response(self, response: str, expected_value: float, tolerance: float = 0.01):
        """Assert that response contains a numeric value within tolerance"""
        # Extract numeric value from response
        numbers = re.findall(r'-?\d+\.?\d*', response)
        if not numbers:
            raise AssertionError(f"No numeric value found in response: '{response}'")
        
        actual_value = float(numbers[0])
        if abs(actual_value - expected_value) > tolerance:
            raise AssertionError(
                f"Expected {expected_value} ± {tolerance}, got {actual_value}"
            )


# Test utilities and helper functions
def create_test_config(usb_port: str, hw_port: Optional[str] = None) -> str:
    """Create a test configuration file"""
    config = {
        "usb_serial_port": usb_port,
        "hw_serial_port": hw_port,
        "baudrate": 115200,
        "timeout": 2.0,
        "log_level": "INFO",
        "test_output_dir": "test_results",
        "parallel_tests": False,
        "retry_failed_tests": True,
        "max_retries": 3
    }
    
    config_file = "test_config.json"
    with open(config_file, 'w') as f:
        json.dump(config, f, indent=2)
    
    return config_file


if __name__ == "__main__":
    # Example usage
    print("Tesla BMS Test Framework")
    print("========================")
    
    # Create sample config if it doesn't exist
    if not Path("test_config.json").exists():
        print("Creating sample configuration...")
        create_test_config("COM3", "COM4")
        print("Please edit test_config.json with your serial port settings")
        sys.exit(0)
    
    # Initialize framework
    framework = BMSTestFramework()
    
    # Setup connections
    if not framework.setup_serial_connections():
        print("Failed to establish any serial connections")
        sys.exit(1)
    
    print("Test framework initialized successfully")
    framework.cleanup_serial_connections() 