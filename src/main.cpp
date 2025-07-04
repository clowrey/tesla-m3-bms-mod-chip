#include <Arduino.h>
#include "BatMan.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include "../AS8510-library/as8510.h"
#include <HardwareSerial.h>

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
TFT_eSPI tft = TFT_eSPI();

/* Tesla Shunt Debug Header Pinout

#1 - SCK  (Square pin)  Clock signal (SPI Interface)
#2 - MOSI
#3 - MISO 
#4 - CS 
#5 - INT Digital output Active high Interrupt to indicate data is ready
#6 - GND
*/

// AS8510 Current Sensor Configuration
#define AS8510_CS_PIN 26        // GPIO pin for AS8510 chip select
#define AS8510_MOSI_PIN 33      // GPIO pin for AS8510 MOSI
#define AS8510_MISO_PIN 25     // GPIO pin for AS8510 MISO  
#define AS8510_SCK_PIN 32       // GPIO pin for AS8510 SCK
#define SHUNT_RESISTANCE 0.0001 // 100µΩ shunt resistance

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

AS8510 currentSensor(AS8510_CS_PIN, AS8510_MOSI_PIN, AS8510_MISO_PIN, AS8510_SCK_PIN, SHUNT_RESISTANCE);

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
const unsigned long DISPLAY_UPDATE_INTERVAL = 1000; // Update display every 1000ms (1 second)

// Serial command buffers
String serialCommand = "";
String serial2Command = "";

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
    // Get current values
    float minVoltage = batman.getMinVoltage() / 1000.0; // Convert mV to V
    float maxVoltage = batman.getMaxVoltage() / 1000.0; // Convert mV to V
    int minCell = batman.getMinCell();
    int maxCell = batman.getMaxCell();
    
    // Get balancing information
    BATMan::BalancingInfo balanceInfo = batman.getBalancingInfo();

        
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
            if (cellNumber > 0 && cellNumber <= 108) {
                Param::SetInt(static_cast<Param::PARAM_NUM>(Param::u1 + cellNumber - 1), batman.getVoltage(i, j));
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
}

void setup() {
    Serial.begin(115200);
    Serial.println("Tesla Model 3 BMB Interface Starting...");
    
    // Initialize second serial interface
    Serial2.begin(SERIAL2_BAUD_RATE, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN); // RX=12, TX=13
    
    // Initialize the display
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    
    // Initialize PWM for economizer using new ESP32 Arduino core 3.0 API
    ledcAttach(ECONOMIZER_PWM_PIN, PWM_FREQ, PWM_RESOLUTION);
    setEconomizerDutyCycle(0);  // Start with economizer off
    
    // Initialize button pin
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize AS8510 current sensor
    Serial.println("Initializing AS8510 current sensor...");
    if (currentSensor.begin()) {
        currentSensorInitialized = true;
        Serial.println("AS8510 current sensor initialized successfully");
        
        // Verify device is awake
        if (currentSensor.isAwake()) {
            Serial.println("AS8510: Device is awake and ready for measurements");
        } else {
            Serial.println("AS8510: Warning - Device may still be in sleep mode");
            // Try to wake up again
            currentSensor.wakeUp();
            delay(10);
        }
        
        // Configure AS8510 settings
        currentSensor.setPGAGain(1, PGA_GAIN_4X);  // Channel 1: 4x gain for current measurement
        currentSensor.setSampleRate(RATE_1000_SPS); // 1000 samples per second
        currentSensor.enableContinuousMode(true);   // Enable continuous mode
        
        Serial.printf("AS8510 configured - CS: %d, MOSI: %d, MISO: %d, SCK: %d, Shunt: %.6fΩ\n", 
            AS8510_CS_PIN, AS8510_MOSI_PIN, AS8510_MISO_PIN, AS8510_SCK_PIN, SHUNT_RESISTANCE);
    } else {
        Serial.println("Failed to initialize AS8510 current sensor");
        currentSensorInitialized = false;
    }
    
    // Initialize the BATMan interface
    batman.BatStart();
    
    Serial.println("System ready. Commands available on both Serial and Serial2 (pins 12/13)");
}

void loop() {
    // Run the BATMan state machine
    batman.loop();
    
    // Update parameters from BATMan system data
    updateParametersFromBATMan();
    
    // Get current time for all timing operations
    unsigned long currentMillis = millis();
    
    // Read current from AS8510 sensor
    if (currentSensorInitialized && currentSensor.isDataReady()) {
        currentReading = currentSensor.readCurrent(1); // Read current from channel 1
        
        // Print current reading to serial (only when it changes significantly)
        if (abs(currentReading - prevCurrentReading) > 0.001) { // 1mA threshold
            Serial.printf("Current: %.3fA\n", currentReading);
        }
    }
    
    // Print periodic status update to serial
    static unsigned long lastSerialUpdate = 0;
    if (currentMillis - lastSerialUpdate >= 5000) { // Every 5 seconds
        Serial.println("\n=== BMS Status Update ===");
        Serial.printf("Min Voltage: %.3fV (Cell %d)\n", batman.getMinVoltage()/1000.0, batman.getMinCell());
        Serial.printf("Max Voltage: %.3fV (Cell %d)\n", batman.getMaxVoltage()/1000.0, batman.getMaxCell());
        Serial.printf("Voltage Delta: %.3fV\n", (batman.getMaxVoltage() - batman.getMinVoltage())/1000.0);
        Serial.printf("Current: %.3fA\n", currentReading);
        Serial.printf("Balance: %s\n", balanceEnabled ? "ON" : "OFF");
        Serial.printf("Economizer: %d%%\n", currentDutyCycle);
        Serial.println("========================\n");
        
        lastSerialUpdate = currentMillis;
    }
    
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
    
    // Add a small delay to prevent overwhelming the serial output
    delay(100);
    
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