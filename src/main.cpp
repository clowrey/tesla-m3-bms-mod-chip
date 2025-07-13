#include <Arduino.h>
#include "BatMan.h"
#include <SPI.h>
#include "../AS8510-library/as8510.h"
#include <HardwareSerial.h>
#include <cstdint>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

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

/* Tesla Shunt Debug Header Pinout

#1 - SCK  (Square pin)  Clock signal (SPI Interface)
#2 - MOSI
#3 - MISO 
#4 - CS 
#5 - INT Digital output Active high Interrupt to indicate data is ready
#6 - GND
*/

// AS8510 Current Sensor Configuration (dedicated VSPI bus at 1MHz)
#define AS8510_CS_PIN 14        // GPIO pin for AS8510 chip select
#define AS8510_MOSI_PIN 26      // GPIO pin for AS8510 MOSI (VSPI)
#define AS8510_MISO_PIN 27      // GPIO pin for AS8510 MISO (VSPI)
#define AS8510_SCK_PIN 25       // GPIO pin for AS8510 SCK (VSPI)
#define SHUNT_RESISTANCE 0.000025296 // 25296nΩ shunt resistance

// ADS1115 ADC Configuration (I2C interface)
#define ADS1115_I2C_SDA 32      // GPIO pin for I2C SDA
#define ADS1115_I2C_SCL 33      // GPIO pin for I2C SCL
#define ADS1115_I2C_FREQ 400000 // I2C frequency (400kHz)
#define ADS1115_ADDRESS 0x48    // Default I2C address (ADDR pin to GND)

// Serial Interface Configuration
#define SERIAL2_RX_PIN 22       // GPIO pin for Serial2 RX
#define SERIAL2_TX_PIN 23      // GPIO pin for Serial2 TX
#define SERIAL2_BAUD_RATE 115200 // Baud rate for Serial2

// PWM Configuration for Economizer (moved to avoid conflict with Serial2)
#define ECONOMIZER_PWM_PIN 4  // Changed from 12 to 14 to avoid conflict with Serial2
#define PWM_FREQ 20000        // 20kHz PWM frequency
#define PWM_RESOLUTION 8      // 8-bit resolution (0-255)
#define ECONOMIZER_DUTY 15   // Normal duty cycle (25%)
#define INITIAL_PULSE_TIME 100  // Initial 100% duty cycle time in milliseconds

// Button Configuration
#define BUTTON_PIN 35        // GPIO pin for push button
#define DEBOUNCE_TIME 50     // Debounce time in milliseconds

// Current sensor instance - Updated for new Rust-based AS8510 library
AS8510 currentSensor(AS8510_CS_PIN, AS8510_MOSI_PIN, AS8510_MISO_PIN, AS8510_SCK_PIN, Gain::Gain100, Gain::Gain25);

// ADS1115 ADC instance
Adafruit_ADS1115 ads;

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

// ADS1115 voltage measurement variables
float packCellVoltage1 = 0;     // Channel 0: Pack cell voltage 1
float packCellVoltage2 = 0;     // Channel 1: Pack cell voltage 2
float packLinkVoltage1 = 0;     // Channel 2: Pack link voltage 1 (after contactors)
float packLinkVoltage2 = 0;     // Channel 3: Pack link voltage 2 (after contactors)
bool ads1115Initialized = false;

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

// Timer variables
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 500; // Update interval for value tracking

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
    else if (lowerCommand == "ads1115 read" || lowerCommand == "pack voltages") {
        if (ads1115Initialized) {
            serialPort.println("=== ADS1115 Pack Voltage Readings ===");
            serialPort.printf("Pack Cell Voltage 1 (Ch0): %.3fV\n", packCellVoltage1);
            serialPort.printf("Pack Cell Voltage 2 (Ch1): %.3fV\n", packCellVoltage2);
            serialPort.printf("Pack Link Voltage 1 (Ch2): %.3fV\n", packLinkVoltage1);
            serialPort.printf("Pack Link Voltage 2 (Ch3): %.3fV\n", packLinkVoltage2);
            serialPort.println("=====================================");
        } else {
            serialPort.println("ADS1115 not initialized!");
        }
    }
    else if (lowerCommand == "ads1115 status" || lowerCommand == "adc status") {
        serialPort.printf("ADS1115 Initialized: %s\n", ads1115Initialized ? "YES" : "NO");
        if (ads1115Initialized) {
            serialPort.printf("I2C Address: 0x%02X\n", ADS1115_ADDRESS);
            serialPort.printf("I2C Pins: SDA=%d, SCL=%d\n", ADS1115_I2C_SDA, ADS1115_I2C_SCL);
        }
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
        serialPort.println("  ads1115 read / pack voltages - Read all ADS1115 pack voltages");
        serialPort.println("  ads1115 status / adc status  - Show ADS1115 ADC status");
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
    // Get current values for tracking previous values
    float minVoltage = batman.getMinVoltage() / 1000.0; // Convert mV to V
    float maxVoltage = batman.getMaxVoltage() / 1000.0; // Convert mV to V
    int minCell = batman.getMinCell();
    int maxCell = batman.getMaxCell();
    
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
    // REMOVED: Individual cell voltage updating - this is now handled by BATMan.upDateCellVolts()
    // to avoid parameter conflicts and ensure proper cell numbering alignment
    // The main BATMan system already correctly sets u1-u108 parameters
    
    // DEBUG: Verify first few cell parameters are being set correctly
    static unsigned long lastDebugOutput = 0;
    if (millis() - lastDebugOutput >= 15000) { // Every 15 seconds
        Serial.println("=== Cell Parameter Debug ===");
        for (int i = 1; i <= 5; i++) {
            float voltage = Param::GetFloat(static_cast<Param::PARAM_NUM>(Param::u1 + i - 1));
            Serial.printf("u%d = %.0fmV\n", i, voltage);
        }
        Serial.println("===========================");
        lastDebugOutput = millis();
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
    
    // Update ADS1115 pack voltage data (using spare chip voltage parameters)
    if (ads1115Initialized) {
        // Store pack voltages in chip voltage parameters (repurposed)
        Param::SetFloat(Param::ChipV1, packCellVoltage1);  // Pack Cell Voltage 1
        Param::SetFloat(Param::ChipV2, packCellVoltage2);  // Pack Cell Voltage 2  
        Param::SetFloat(Param::ChipV3, packLinkVoltage1);  // Pack Link Voltage 1 (after contactors)
        Param::SetFloat(Param::ChipV4, packLinkVoltage2);  // Pack Link Voltage 2 (after contactors)
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
    

    
    // Initialize PWM for economizer using new ESP32 Arduino core 3.0 API
    ledcAttach(ECONOMIZER_PWM_PIN, PWM_FREQ, PWM_RESOLUTION);
    setEconomizerDutyCycle(0);  // Start with economizer off
    
    // Initialize button pin
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize I2C for ADS1115
    Serial.println("Initializing I2C for ADS1115...");
    Wire.begin(ADS1115_I2C_SDA, ADS1115_I2C_SCL);
    Wire.setClock(ADS1115_I2C_FREQ);
    Serial.printf("I2C Configuration - SDA: %d, SCL: %d, Freq: %dHz\n", 
        ADS1115_I2C_SDA, ADS1115_I2C_SCL, ADS1115_I2C_FREQ);
    
    // Initialize ADS1115 ADC
    Serial.println("Initializing ADS1115 ADC...");
    if (ads.begin(ADS1115_ADDRESS)) {
        Serial.printf("ADS1115 initialized successfully at address 0x%02X!\n", ADS1115_ADDRESS);
        
        // Set gain and data rate
        ads.setGain(GAIN_FOUR);     // ±1.024V range (1 bit = 0.03125mV)
        ads.setDataRate(RATE_ADS1115_860SPS);  // 860 samples per second
        
        Serial.println("ADS1115 Configuration:");
        Serial.println("  - Gain: ±1.024V (1 bit = 0.03125mV)");
        Serial.println("  - Data Rate: 860 SPS");
        Serial.println("  - Channel 0: Pack Cell Voltage 1");
        Serial.println("  - Channel 1: Pack Cell Voltage 2");
        Serial.println("  - Channel 2: Pack Link Voltage 1 (after contactors)");
        Serial.println("  - Channel 3: Pack Link Voltage 2 (after contactors)");
        
        ads1115Initialized = true;
    } else {
        Serial.printf("ADS1115 initialization failed at address 0x%02X!\n", ADS1115_ADDRESS);
        ads1115Initialized = false;
    }
    
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
    
    // Wait for SPI bus to settle before initialization
    Serial.println("Waiting for SPI bus to settle...");
    delay(100);
    
    // Print SPI bus configuration
    Serial.println("SPI Bus Configuration:");
    Serial.println("  - Tesla BMS: 1MHz on HSPI/SPI2_HOST (pins 2,17,15,22) - DEDICATED BUS");
    Serial.println("  - AS8510: 1MHz on VSPI/SPI3_HOST (SCK=25, MISO=27, MOSI=26, CS=14) - DEDICATED BUS");
    Serial.println("BMB on HSPI, AS8510 on VSPI for clean separation");
    
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
    
    Serial.println("System ready. Commands available on both Serial and Serial2 (pins 22/23)");
    Serial.printf("AS8510 on VSPI bus - BMB on HSPI - ADS1115 on I2C (%s) - All systems enabled\n", 
                  ads1115Initialized ? "OK" : "FAILED");
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
            // Get current measurement and update global variable
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
    
    // ADS1115 PACK VOLTAGE MEASUREMENT - Every 3 seconds
    static unsigned long lastVoltageRead = 0;
    if (currentMillis - lastVoltageRead >= 3000) {
        lastVoltageRead = currentMillis;
        
        if (ads1115Initialized) {
            // Read all 4 channels
            int16_t adc0 = ads.readADC_SingleEnded(0);
            int16_t adc1 = ads.readADC_SingleEnded(1);
            int16_t adc2 = ads.readADC_SingleEnded(2);
            int16_t adc3 = ads.readADC_SingleEnded(3);
            
            // Convert to voltages (assuming ±1.024V range, 1 bit = 0.03125mV)
            packCellVoltage1 = ads.computeVolts(adc0);
            packCellVoltage2 = ads.computeVolts(adc1);
            packLinkVoltage1 = ads.computeVolts(adc2);
            packLinkVoltage2 = ads.computeVolts(adc3);
            
            // Display pack voltages on one line
            Serial.printf("Pack: Cell1=%.3fV Cell2=%.3fV Link1=%.3fV Link2=%.3fV\n", 
                         packCellVoltage1, packCellVoltage2, packLinkVoltage1, packLinkVoltage2);
            
        } else {
            Serial.println("ADS1115 not initialized - pack voltage readings unavailable");
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
    
    // Check if it's time to update tracked values
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