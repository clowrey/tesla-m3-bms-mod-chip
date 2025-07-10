#include <Arduino.h>
#include "BatMan.h"
// #include <TFT_eSPI.h>  // DISABLED to avoid SPI conflicts
#include <SPI.h>
#include "../AS8510-library/as8510.h"
#include <HardwareSerial.h>
#include <cstdint>

/*> 
balance on
Balance ENABLED

> balance status
Balance is currently: ENABLED

> balance off
Balance DISABLED

> help
Available commands:
  balance on / balance enable  - Enable cell balancing
  balance off / balance disable - Disable cell balancing
  balance status / balance     - Show current balance status
  help                         - Show this help message

  */

BATMan batman;
// TFT_eSPI tft = TFT_eSPI();  // DISABLED to avoid SPI conflicts

/* Tesla Shunt Debug Header Pinout

#1 - SCK  (Square pin)  Clock signal (SPI Interface)
#2 - MOSI
#3 - MISO 
#4 - CS 
#5 - INT Digital output Active high Interrupt to indicate data is ready
#6 - GND
*/

// AS8510 Current Sensor Configuration (dedicated HSPI bus at 1MHz)
#define AS8510_CS_PIN 26        // GPIO pin for AS8510 chip select
#define AS8510_MOSI_PIN 33      // GPIO pin for AS8510 MOSI (HSPI)
#define AS8510_MISO_PIN 25      // GPIO pin for AS8510 MISO (HSPI)
#define AS8510_SCK_PIN 32       // GPIO pin for AS8510 SCK (HSPI)
#define SHUNT_RESISTANCE 0.000025296 // 25296nΩ shunt resistance

// Serial Interface Configuration
#define SERIAL2_RX_PIN 39       // GPIO pin for Serial2 RX
#define SERIAL2_TX_PIN 12      // GPIO pin for Serial2 TX
#define SERIAL2_BAUD_RATE 115200 // Baud rate for Serial2

// PWM Configuration for Economizer (moved to avoid conflict with Serial2)
#define ECONOMIZER_PWM_PIN 36  // Changed from 12 to 14 to avoid conflict with Serial2
#define PWM_FREQ 20000        // 20kHz PWM frequency
#define PWM_RESOLUTION 8      // 8-bit resolution (0-255)
#define ECONOMIZER_DUTY 15   // Normal duty cycle (25%)
#define INITIAL_PULSE_TIME 100  // Initial 100% duty cycle time in milliseconds

// Button Configuration
#define BUTTON_PIN 35        // GPIO pin for push button
#define DEBOUNCE_TIME 50     // Debounce time in milliseconds

// Current sensor instance - Updated for new Rust-based AS8510 library
AS8510 currentSensor(26, 33, 25, 32, Gain::Gain100, Gain::Gain25);

// Variables to store previous values for comparison
float prevMinVoltage = 0;
float prevMaxVoltage = 0;
int prevMinCell = 0;
int prevMaxCell = 0;
uint8_t prevDutyCycle = 0;  // Track duty cycle changes

// Current measurement variables
float currentReading = 0;
float prevCurrentReading = 0;
bool currentSensorInitialized = false;

// Balance control variable
bool balanceEnabled = false;

// Button and Economizer state variables
bool economizerEnabled = false;
bool lastButtonState = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long economizerStartTime = 0;
bool initialPulseComplete = false;

// Add global variable for current duty cycle
volatile uint8_t currentDutyCycle = 0;

// Add display update timer variables
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 500; // Update display every 500ms (0.5 second) for faster responsiveness

// Serial command buffers
String serialCommand = "";
String serial2Command = "";

// AS8510 diagnostic state machine variables
bool diagnosticInProgress = false;
int diagnosticStep = 0;
unsigned long diagnosticStepTime = 0;
const unsigned long DIAGNOSTIC_STEP_INTERVAL = 500; // 500ms between diagnostic steps
HardwareSerial* diagnosticSerial = nullptr;

// Function declarations
void runDiagnosticStep();
void startAS8510NonBlocking(HardwareSerial& serialPort);
void processSerialInputs();

// Function to process serial commands (now takes a HardwareSerial reference)
void processSerialCommand(String command, HardwareSerial& serialPort) {
    command.trim(); // Remove whitespace
    // Note: Don't convert to lowercase to preserve parameter name case sensitivity
    
    String lowerCommand = command;
    lowerCommand.toLowerCase();
    
    if (lowerCommand == "balance on" || lowerCommand == "balance enable") {
        balanceEnabled = true;
        Param::SetInt(Param::balance, 1);
        serialPort.println("Balance ENABLED");
    }
    else if (lowerCommand == "balance off" || lowerCommand == "balance disable") {
        balanceEnabled = false;
        Param::SetInt(Param::balance, 0);
        serialPort.println("Balance DISABLED");
    }
    else if (lowerCommand == "balance status" || lowerCommand == "balance") {
        serialPort.printf("Balance is currently: %s\n", balanceEnabled ? "ENABLED" : "DISABLED");
    }
    else if (lowerCommand == "mapping" || lowerCommand == "debug") {
        batman.printHardwareMapping();
    }
    else if (lowerCommand == "bmb registers" || lowerCommand == "bmb debug" || lowerCommand == "registers") {
        serialPort.println("=== Raw BMB Register Data ===");
        batman.printHardwareMapping();
        serialPort.println("Use 'mapping' for basic debug or 'bmb registers' for detailed register analysis");
    }
    else if (lowerCommand == "bmb debug on" || lowerCommand == "register debug on") {
        BATMan::setRegisterDebug(true);
        serialPort.println("BMB register debug ENABLED - will show raw register data during reads");
    }
    else if (lowerCommand == "bmb debug off" || lowerCommand == "register debug off") {
        BATMan::setRegisterDebug(false);
        serialPort.println("BMB register debug DISABLED");
    }
    else if (lowerCommand == "bmb debug status") {
        serialPort.printf("BMB register debug is: %s\n", BATMan::getRegisterDebug() ? "ENABLED" : "DISABLED");
    }
    else if (lowerCommand == "current diag" || lowerCommand == "diag current") {
        if (!diagnosticInProgress) {
            diagnosticInProgress = true;
            diagnosticStep = 0;
            diagnosticStepTime = millis();
            diagnosticSerial = &serialPort;
            serialPort.println("Starting non-blocking AS8510 diagnostics...");
        } else {
            serialPort.println("Diagnostics already in progress. Please wait for completion.");
        }
    }
    else if (lowerCommand == "start as8510" || lowerCommand == "as8510 start") {
        startAS8510NonBlocking(serialPort);
    }
    else if (lowerCommand == "as8510 errors" || lowerCommand == "errors") {
        serialPort.println("Reading AS8510 error codes...");
        currentSensor.printErrorCodes();
    }
    else if (lowerCommand == "as8510 saturation" || lowerCommand == "saturation") {
        serialPort.println("Reading AS8510 saturation flags...");
        currentSensor.printSaturationFlags();
    }
    else if (lowerCommand == "as8510 diagnostics" || lowerCommand == "diagnostics") {
        serialPort.println("Running complete AS8510 diagnostics...");
        currentSensor.printAllDiagnostics();
    }
    // Parameter API commands
    else if (lowerCommand.startsWith("param ")) {
        String paramCommand = command.substring(6); // Remove "param " prefix (preserve original case)
        String lowerParamCommand = paramCommand;
        lowerParamCommand.toLowerCase();
        
        if (lowerParamCommand == "list") {
            Param::PrintAllParams(serialPort);
        }
        else if (lowerParamCommand == "help") {
            Param::PrintParamHelp(serialPort);
        }
        else if (lowerParamCommand.startsWith("get ")) {
            String paramName = paramCommand.substring(4); // Keep original case for parameter name
            Param::PARAM_NUM param = Param::GetParamFromName(paramName.c_str());
            if (param != static_cast<Param::PARAM_NUM>(-1)) {
                Param::PrintParam(param, serialPort);
            } else {
                serialPort.printf("Error: Unknown parameter '%s'\n", paramName.c_str());
            }
        }
        else if (lowerParamCommand.startsWith("set ")) {
            int spacePos = paramCommand.indexOf(' ', 4);
            if (spacePos > 0) {
                String paramName = paramCommand.substring(4, spacePos); // Keep original case for parameter name
                String paramValue = paramCommand.substring(spacePos + 1);
                Param::SetParamFromString(paramName.c_str(), paramValue.c_str(), serialPort);
            } else {
                serialPort.println("Error: Invalid parameter set command. Use: param set <name> <value>");
            }
        }
        else {
            serialPort.println("Error: Unknown parameter command. Use 'param help' for available commands.");
        }
    }
    else if (lowerCommand == "help") {
        serialPort.println("Available commands:");
        serialPort.println("  balance on / balance enable  - Enable cell balancing");
        serialPort.println("  balance off / balance disable - Disable cell balancing");
        serialPort.println("  balance status / balance     - Show current balance status");
        serialPort.println("  mapping / debug              - Show hardware register mapping");
        serialPort.println("  bmb registers / registers    - Show detailed BMB register analysis");
        serialPort.println("  bmb debug on/off             - Enable/disable live BMB register debugging");
        serialPort.println("  current diag                 - Run current sensor diagnostics");
        serialPort.println("  start as8510                 - Explicitly start AS8510 device");
        serialPort.println("  as8510 errors / errors       - Show AS8510 error codes");
        serialPort.println("  as8510 saturation / saturation - Show AS8510 saturation flags");
        serialPort.println("  as8510 diagnostics / diagnostics - Complete AS8510 diagnostics");
        serialPort.println("  param list                   - List all parameters");
        serialPort.println("  param get <name>             - Get parameter value");
        serialPort.println("  param set <name> <value>     - Set parameter value");
        serialPort.println("  param help                   - Show parameter API help");
        serialPort.println("  help                         - Show this help message");
    }
    else if (command.length() > 0) {
        serialPort.printf("Unknown command: '%s'\n", command.c_str());
        serialPort.println("Type 'help' for available commands");
    }
}



// Function to set economizer PWM duty cycle (0-100%)
void setEconomizerDutyCycle(uint8_t dutyCycle) {
    // Convert percentage to 8-bit value (0-255)
    uint32_t pwmValue = (dutyCycle * 255) / 100;
    ledcWrite(ECONOMIZER_PWM_PIN, pwmValue);
    
    // Print duty cycle change to serial
    if (dutyCycle != prevDutyCycle) {
        Serial.print("Economizer duty cycle: ");
        Serial.print(dutyCycle);
        Serial.println("%");
        prevDutyCycle = dutyCycle;
    }
    currentDutyCycle = dutyCycle; // Always update global
}

void updateDisplay(uint8_t currentDutyCycle) {
    // TFT Display disabled to avoid SPI conflicts - all display code commented out
    
    // Get current values (still needed for serial output)
    float minVoltage = batman.getMinVoltage() / 1000.0; // Convert mV to V
    float maxVoltage = batman.getMaxVoltage() / 1000.0; // Convert mV to V
    int minCell = batman.getMinCell();
    int maxCell = batman.getMaxCell();
    
    // Get average cell voltage from parameter system
    float avgVoltage = Param::GetFloat(Param::uavg) / 1000.0; // Convert mV to V
    
    // Get balancing information
    BATMan::BalancingInfo balanceInfo = batman.getBalancingInfo();

    // All TFT display code commented out to avoid SPI conflicts
    /*
    // Clear the display
    tft.fillScreen(TFT_BLACK);
    
    // Set text color and size
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    
    // Display title
    tft.setCursor(10, 10);
    tft.println("Tesla BMS Status");
    
    // Display min voltage
    tft.setCursor(10, 30);
    tft.print("MinV: ");
    tft.print(minVoltage, 3);
    tft.println("V");
    
    // Display min cell number
    tft.setCursor(10, 40);
    tft.print("Min Cell: ");
    tft.println(minCell);
    
    // Display max voltage
    tft.setCursor(10, 60);
    tft.print("MaxV: ");
    tft.print(maxVoltage, 3);
    tft.println("V");
    
    // Display max cell number
    tft.setCursor(10, 70);
    tft.print("Max Cell: ");
    tft.println(maxCell);
    
    // Display voltage delta
    tft.setCursor(10, 90);
    tft.print("Delta: ");
    tft.print(maxVoltage - minVoltage, 3);
    tft.println("V");
    
    // Display current reading
    tft.setCursor(10, 110);
    tft.print("Current: ");
    tft.print(currentReading, 3);
    tft.println("A");
    
    // Display economizer status with duty cycle
    tft.setCursor(10, 130);
    tft.print("Economizer: ");
    tft.print(currentDutyCycle);
    tft.println("%");
    
    // Display balance status
    tft.setCursor(10, 150);
    tft.print("Balance: ");
    if (balanceEnabled) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.println("ON");
    } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.println("OFF");
    }
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // Reset text color
    
    // Display compact balancing information
    tft.setCursor(10, 170);
    tft.print("Balancing: ");
    if (balanceInfo.balancingCells > 0) {
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.print(balanceInfo.balancingCells);
        tft.print(" cells");
        
        // Show first few balancing cell numbers in compact format
        if (balanceInfo.balancingCells <= 8) {
            tft.print(" (");
            for (int i = 0; i < balanceInfo.balancingCells; i++) {
                if (i > 0) tft.print(",");
                tft.print(balanceInfo.balancingCellNumbers[i]);
            }
            tft.print(")");
        } else {
            tft.print(" (");
            for (int i = 0; i < 6; i++) {
                if (i > 0) tft.print(",");
                tft.print(balanceInfo.balancingCellNumbers[i]);
            }
            tft.print("...");
            tft.print(balanceInfo.balancingCellNumbers[balanceInfo.balancingCells-1]);
            tft.print(")");
        }
    } else {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.print("None");
    }
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // Reset text color
    */
    
    // Update previous values
    prevMinVoltage = minVoltage;
    prevMaxVoltage = maxVoltage;
    prevMinCell = minCell;
    prevMaxCell = maxCell;
    prevDutyCycle = currentDutyCycle;
    prevCurrentReading = currentReading;
}

// Function to update parameters from BATMan system data
void updateParametersFromBATMan() {
    // Update cell voltages (u1-u108)
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 15; j++) {
            int cellNumber = batman.getSequentialCellNumber(i, j);
            uint16_t voltage = batman.getVoltage(i, j);
            
            if (cellNumber > 0 && cellNumber <= 108) {
                Param::SetInt(static_cast<Param::PARAM_NUM>(Param::u1 + cellNumber - 1), voltage);
            }
        }
    }
    
    // Update voltage statistics
    Param::SetInt(Param::CellMax, batman.getMaxCell());
    Param::SetInt(Param::CellMin, batman.getMinCell());
    Param::SetInt(Param::umax, batman.getMaxVoltage());
    Param::SetInt(Param::umin, batman.getMinVoltage());
    Param::SetInt(Param::deltaV, batman.getMaxVoltage() - batman.getMinVoltage());
    
    // Update balance status and balancing cell list
    Param::SetInt(Param::balance, balanceEnabled ? 1 : 0);
    Param::SetInt(Param::CellVmax, batman.getMaxVoltage());
    Param::SetInt(Param::CellVmin, batman.getMinVoltage());
    
    // Get balancing information and create comma-separated list
    BATMan::BalancingInfo balanceInfo = batman.getBalancingInfo();
    Param::SetInt(Param::CellsBalancing, balanceInfo.balancingCells);
    
    // Create comma-separated string of balancing cell numbers
    String balanceCellList = "";
    for (int i = 0; i < balanceInfo.balancingCells; i++) {
        if (i > 0) {
            balanceCellList += ",";
        }
        balanceCellList += String(balanceInfo.balancingCellNumbers[i]);
    }
    Param::SetString(Param::BalanceCellList, balanceCellList);
    
    // Update temperature data (if available)
    // Note: This would need to be implemented based on actual temperature data from BATMan
    
    // Update chip voltages (if available)
    // Note: This would need to be implemented based on actual chip voltage data from BATMan
    
    // Update AS8510 current sensor data
    Param::SetFloat(Param::current, currentReading);
    
    // Update AS8510 temperature every time parameters are updated
    if (currentSensor.isInitialized()) {
        float internalTemp = currentSensor.getInternalTemperature();
        Param::SetFloat(Param::as8510_temp, internalTemp);
    } else {
        // If not initialized, set temperature to 0
        Param::SetFloat(Param::as8510_temp, 0.0);
    }
}

// Global variables for non-blocking operation
static unsigned long lastMainLoopTime = 0;
static const unsigned long MAIN_LOOP_INTERVAL = 50; // 50ms interval without blocking delay

// Non-blocking diagnostic function
void runDiagnosticStep() {
    if (!diagnosticInProgress || !diagnosticSerial) return;
    
    unsigned long currentTime = millis();
    if (currentTime - diagnosticStepTime < DIAGNOSTIC_STEP_INTERVAL) return;
    
    switch (diagnosticStep) {
        case 0:
            diagnosticSerial->println("\n=== AS8510 Current Sensor Diagnostics (Rust-based) ===");
            diagnosticSerial->println();
            
            if (!currentSensor.isInitialized()) {
                diagnosticSerial->println("ERROR: AS8510 not initialized!");
                diagnosticSerial->println("Attempting to initialize...");
                if (currentSensor.begin()) {
                    diagnosticSerial->println("AS8510 initialized successfully!");
                } else {
                    diagnosticSerial->println("AS8510 initialization failed!");
                    diagnosticInProgress = false;
                    return;
                }
            }
            break;
            
        case 1:
            diagnosticSerial->printf("Shunt Resistance: %.9f ohms\n", currentSensor.getShuntResistance());
            diagnosticSerial->printf("Device Present: %s\n", currentSensor.isDevicePresent() ? "YES" : "NO");
            diagnosticSerial->printf("Device Awake: %s\n", currentSensor.isAwake() ? "YES" : "NO");
            diagnosticSerial->printf("Data Ready: %s\n", currentSensor.isDataReady() ? "YES" : "NO");
            break;
            
        case 2:
            diagnosticSerial->println("\n--- Key Registers ---");
            diagnosticSerial->println();
            diagnosticSerial->printf("Mode Control (0x0A): 0x%02X\n", currentSensor.readRegister(0x0A));
            diagnosticSerial->printf("Status (0x04): 0x%02X\n", currentSensor.readRegister(0x04));
            diagnosticSerial->printf("PGA Control (0x13): 0x%02X\n", currentSensor.readRegister(0x13));
            break;
            
        case 3:
            diagnosticSerial->printf("Power Control 1 (0x14): 0x%02X\n", currentSensor.readRegister(0x14));
            diagnosticSerial->printf("Power Control 2 (0x15): 0x%02X\n", currentSensor.readRegister(0x15));
            diagnosticSerial->printf("Clock Control (0x08): 0x%02X\n", currentSensor.readRegister(0x08));
            break;
            
        case 4:
            diagnosticSerial->println("\n--- Data Registers ---");
            diagnosticSerial->println();
            diagnosticSerial->printf("Current Data 1 (0x00): 0x%02X\n", currentSensor.readRegister(0x00));
            diagnosticSerial->printf("Current Data 2 (0x01): 0x%02X\n", currentSensor.readRegister(0x01));
            break;
            
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            if (diagnosticStep == 5) {
                diagnosticSerial->println("\n--- Current Measurement Test ---");
                diagnosticSerial->println();
            }
            {
                int measurementNum = diagnosticStep - 4;
                int16_t rawADC = currentSensor.readRawADC(1);
                float current = currentSensor.readCurrent(1);
                
                diagnosticSerial->printf("Measurement %d: Raw ADC = %d, Current = %.6f A\n", 
                                      measurementNum, rawADC, current);
            }
            break;
            
        case 10:
            diagnosticSerial->println("\n--- Status Information ---");
            diagnosticSerial->println();
            currentSensor.printStatus();
            diagnosticSerial->println("=== End AS8510 Diagnostics ===");
            diagnosticInProgress = false;
            diagnosticSerial = nullptr;
            break;
            
        default:
            diagnosticInProgress = false;
            diagnosticSerial = nullptr;
            break;
    }
    
    diagnosticStep++;
    diagnosticStepTime = currentTime;
}

// Non-blocking AS8510 start command
void startAS8510NonBlocking(HardwareSerial& serialPort) {
    static bool startInProgress = false;
    static unsigned long startTime = 0;
    static int startStep = 0;
    
    if (!startInProgress) {
        serialPort.println("Explicitly starting AS8510 device...");
        currentSensor.startDevice();
        startInProgress = true;
        startTime = millis();
        startStep = 0;
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - startTime >= 100) { // 100ms delay equivalent
        uint8_t modCtl = currentSensor.readRegister(0x0A);
        serialPort.printf("Mode Control after start: 0x%02X\n", modCtl);
        if (modCtl & 0x01) {
            serialPort.println("START bit is SET - device should be running");
        } else {
            serialPort.println("START bit is NOT SET - device is not running");
        }
        startInProgress = false;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Tesla Model 3 BMB Interface Starting...");
    
    // Initialize second serial interface
    Serial2.begin(SERIAL2_BAUD_RATE, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN); // RX=12, TX=13
    
    // Initialize the display - Re-enabled on separate SPI controller
    // TFT Display disabled to avoid SPI conflicts
    // tft.init();
    // tft.setRotation(0);
    // tft.fillScreen(TFT_BLACK);
    
    // Initialize PWM for economizer using new ESP32 Arduino core 3.0 API
    ledcAttach(ECONOMIZER_PWM_PIN, PWM_FREQ, PWM_RESOLUTION);
    setEconomizerDutyCycle(0);  // Start with economizer off
    
    // Initialize button pin
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize AS8510 current sensor
    Serial.println("Initializing AS8510 current sensor...");
    Serial.printf("Pin Configuration - CS: %d, MOSI: %d, MISO: %d, SCK: %d\n", 
        AS8510_CS_PIN, AS8510_MOSI_PIN, AS8510_MISO_PIN, AS8510_SCK_PIN);
    Serial.printf("SPI Speed: 1MHz, Shunt Resistance: %.9fΩ (%.0f nΩ)\n", 
        SHUNT_RESISTANCE, SHUNT_RESISTANCE * 1e9);
    
    // Test pin connectivity before initialization
    Serial.println("Testing pin states...");
    pinMode(AS8510_CS_PIN, OUTPUT);
    digitalWrite(AS8510_CS_PIN, HIGH);
    delay(10);
    Serial.printf("CS pin %d set HIGH\n", AS8510_CS_PIN);
    
    // Ensure TFT display SPI doesn't interfere - add delay
    Serial.println("Waiting for SPI bus to settle...");
    delay(100);
    
    // Print SPI bus configuration
    Serial.println("SPI Bus Configuration:");
    Serial.println("  - TFT display: DISABLED to avoid SPI conflicts");
    Serial.println("  - Tesla BMS: 1MHz on HSPI/SPI2_HOST (pins 2,17,15,22) - DEDICATED BUS");
    Serial.println("  - AS8510: 1MHz on VSPI/SPI3_HOST (pins 32,25,33,26) - DEDICATED BUS");
    Serial.println("LCD disabled - BMB on HSPI, AS8510 on VSPI for clean separation");
    
    // Initialize the BATMan interface first
    batman.BatStart();
    
    // Allow BMB to settle before initializing AS8510
    delay(1000);
    
    // Initialize current sensor with new Rust-based library AFTER BMB
    Serial.println("Initializing AS8510 current sensor with Rust-based library...");
    if (currentSensor.begin()) {
        Serial.println("AS8510 initialized successfully!");
    } else {
        Serial.println("AS8510 initialization failed!");
    }
    
    // Set verbose logging to false to disable detailed debug output
    currentSensor.setVerboseLogging(false);
    
    Serial.println("System ready. Commands available on both Serial and Serial2 (pins 12/13)");
    Serial.println("LCD + AS8510 share VSPI bus - BMB on HSPI - All systems enabled - No rewiring needed");
    Serial.println("============ Setup Complete - Starting Main Loop =============");
}

void loop() {
    // Get current time for all timing operations
    unsigned long currentMillis = millis();
    
    // Throttle main loop execution to maintain timing without blocking delays
    if (currentMillis - lastMainLoopTime < MAIN_LOOP_INTERVAL) {
        // Process serial commands even during throttled periods
        processSerialInputs();
        
        // Run non-blocking diagnostic steps if in progress
        runDiagnosticStep();
        
        return;
    }
    lastMainLoopTime = currentMillis;
    
    // Run the BATMan state machine - TESTING: Re-enabled to check if this causes hang
    batman.loop();
    
    // Update parameters from BATMan system data - ENABLED for ESPHome interface
    updateParametersFromBATMan();
    
    // Debug: Show we're alive every 10 seconds with voltage status
    static unsigned long lastHeartbeat = 0;
    if (currentMillis - lastHeartbeat >= 10000) {
        Serial.println("Main loop running - system alive");
        
        // Display voltage status including average
        float minVoltage = batman.getMinVoltage() / 1000.0;
        float maxVoltage = batman.getMaxVoltage() / 1000.0;
        float avgVoltage = Param::GetFloat(Param::uavg) / 1000.0;
        Serial.printf("Voltages - Min: %.3fV, Max: %.3fV, Avg: %.3fV\n", 
                     minVoltage, maxVoltage, avgVoltage);
        
        lastHeartbeat = currentMillis;
    }
    
    // REMOVED: PERIODIC AS8510 RE-INITIALIZATION - This was causing SPI bus conflicts
    // Only initialize once at startup to avoid SPI pin reconfiguration
    
    // SAFE SPI COMMUNICATION - Commented out for ultra-clean output
    // static unsigned long lastForcedRead = 0;
    // if (currentMillis - lastForcedRead >= 5000) { // Every 5 seconds - ALWAYS run for debugging
    //     Serial.printf("SAFE SPI TEST - Time: %lu ms\n", currentMillis);
    //     
    //     // Try ONE simple SPI transaction at a time to prevent crash
    //     static int testStep = 0;
    //     
    //     // Declare variables outside switch to avoid linter errors
    //     uint8_t status;
    //     int16_t rawADC;
    //     float voltage;
    //     
    //     switch(testStep) {
    //         case 0:
    //             Serial.println("Step 0: Reading status register...");
    //             status = currentSensor.getStatus();
    //             Serial.printf("Status: 0x%02X\n", status);
    //             Serial.println("Step 0: COMPLETED");
    //             break;
    //         case 1:
    //             Serial.println("Step 1: Reading raw ADC...");
    //             Serial.println("Step 1: Starting ADC read (this previously hung)...");
    //             rawADC = currentSensor.readRawADC(1);
    //             Serial.printf("Raw ADC: %d\n", rawADC);
    //             Serial.println("Step 1: COMPLETED");
    //             break;
    //         case 2:
    //             Serial.println("Step 2: Skip voltage test");
    //             Serial.println("Step 2: COMPLETED");
    //             break;
    //         case 3:
    //             Serial.println("Step 3: Reading current...");
    //             currentReading = currentSensor.readCurrent(1);
    //             Serial.printf("Current: %.6fA\n", currentReading);
    //             Serial.println("Step 3: COMPLETED");
    //             break;
    //         default:
    //             testStep = -1; // Will wrap to 0
    //             break;
    //     }
    //     testStep++;
    //     
    //     // Add delay between operations to prevent overwhelming
    //     delay(50);
    //     
    //     lastForcedRead = currentMillis;
    // }
    
    // RUST-BASED AS8510 CURRENT MEASUREMENT - Every 2 seconds (reduced frequency for faster main loop)
    static unsigned long lastCurrentRead = 0;
    if (currentMillis - lastCurrentRead >= 2000) {
        lastCurrentRead = currentMillis;
        
        if (currentSensor.isInitialized()) {
            // Get current measurement and update global variable for LCD display
            currentReading = currentSensor.getCurrent();
            
            // Get internal temperature measurement
            float internalTemp = currentSensor.getInternalTemperature();
            
            // Get average cell voltage
            float avgVoltage = Param::GetFloat(Param::uavg) / 1000.0;
            
            // Display current, temperature, and average cell voltage on one line
            Serial.printf("AS8510: %.3fA    %.1f°C    Avg Cell: %.3fV\n", 
                         currentReading, internalTemp, avgVoltage);
            
        } else {
            Serial.println("AS8510 not initialized - attempting restart...");
            currentSensor.startDevice();
        }
    }
    
    // Remove redundant legacy testing section to clean up output
    
    // Current sensor status display - Commented out for ultra-clean output
    // static unsigned long lastStatusDisplay = 0;
    // if (currentMillis - lastStatusDisplay >= 10000) {
    //     lastStatusDisplay = currentMillis;
    //     
    //     Serial.println("┌─── AS8510 Status ───┐");
    //     Serial.println();
    //     Serial.printf("Initialized: %s\n", currentSensor.isInitialized() ? "YES" : "NO");
    //     Serial.printf("Data Ready: %s\n", currentSensor.isInitialized() ? (currentSensor.isDataReady() ? "YES" : "NO") : "N/A");
    //     Serial.printf("Device Awake: %s\n", currentSensor.isInitialized() ? (currentSensor.isAwake() ? "YES" : "NO") : "N/A");
    //     
    //     // Quick test read
    //     int16_t rawADC = currentSensor.readRawADC(1);
    //     Serial.printf("Quick test: Raw ADC = %d\n", rawADC);
    //     
    //     Serial.println("└─────────────────────┘");
    // }
    
    // Check if it's time to update the display
    if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        updateDisplay(currentDutyCycle);
        lastDisplayUpdate = currentMillis;
    }
    
    // Read button state with debouncing
    bool reading = digitalRead(BUTTON_PIN);
    
    // Check if button state has changed
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    
    // If button state is stable for debounce time
    if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {
        if (reading != buttonState) {
            buttonState = reading;
            
            // If button is pressed (LOW due to INPUT_PULLUP)
            if (buttonState == LOW) {
                economizerEnabled = !economizerEnabled;
                if (economizerEnabled) {
                    // Start with 100% duty cycle
                    setEconomizerDutyCycle(100);
                    economizerStartTime = millis();
                    initialPulseComplete = false;
                } else {
                    // Turn off economizer
                    setEconomizerDutyCycle(0);
                    initialPulseComplete = false;
                }
            }
        }
    }
    
    // Handle initial pulse timing
    if (economizerEnabled && !initialPulseComplete) {
        if ((millis() - economizerStartTime) >= INITIAL_PULSE_TIME) {
            setEconomizerDutyCycle(ECONOMIZER_DUTY);  // Set to normal duty cycle
            initialPulseComplete = true;
        }
    }
    
    lastButtonState = reading;
    
    // Process serial commands (moved to separate function for reuse)
    processSerialInputs();
    
    // Run non-blocking diagnostic steps if in progress
    runDiagnosticStep();
}

// Separate function to process serial inputs (can be called more frequently)
void processSerialInputs() {
    // Process serial commands
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (serialCommand.length() > 0) {
                processSerialCommand(serialCommand, Serial);
                serialCommand = "";
            }
        } else {
            serialCommand += c;
        }
    }

    // Process serial2 commands
    while (Serial2.available()) {
        char c = Serial2.read();
        if (c == '\n' || c == '\r') {
            if (serial2Command.length() > 0) {
                processSerialCommand(serial2Command, Serial2);
                serial2Command = "";
            }
        } else {
            serial2Command += c;
        }
    }
} 