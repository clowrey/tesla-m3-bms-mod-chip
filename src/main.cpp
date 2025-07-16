#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <cmath>  // For std::isnan
#include "BatMan.h"
#include "Param.h"
#include "AS8510_CoulombCounter.h"
#include "ParamLogger.h"

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
#define SERIAL2_BAUD_RATE 921600 // Baud rate for Serial2

// PWM Configuration for PackContactors (moved to avoid conflict with Serial2)
#define PACK_CONTACTORS_PWM_PIN 4  // Changed from 12 to 14 to avoid conflict with Serial2
#define PRE_CHARGE_RELAY_PWM_PIN 13   // Pre Charge Relay on pin 21
#define PWM_FREQ 10000        // 10kHz PWM frequency
#define PWM_RESOLUTION 8      // 8-bit resolution (0-255)
#define PACK_CONTACTORS_DUTY 15   // Normal duty cycle (25%)
#define PRE_CHARGE_RELAY_DUTY 15     // Normal duty cycle (25%)
#define INITIAL_PULSE_TIME 200  // Initial 100% duty cycle time in milliseconds

// Button Configuration - REMOVED: Physical button control replaced with serial API

// AS8510 Coulomb Counter instance (manages AS8510 internally)
AS8510_CoulombCounter coulombCounter(AS8510_CS_PIN, AS8510_MOSI_PIN, AS8510_MISO_PIN, AS8510_SCK_PIN, 100, 25);

// ADS1115 ADC instance
Adafruit_ADS1115 ads;

// Variables to store previous values for comparison
float prevMinVoltage = 0;
float prevMaxVoltage = 0;
int prevMinCell = 0;
int prevMaxCell = 0;
uint8_t prevDutyCycle = 0;  // Track duty cycle changes



// ADS1115 voltage measurement variables
float battContactorPos = 0;     // Channel 1: Batt-Pos
float battContactorNeg = 0;     // Channel 0: Batt-Neg
float battContactorSum = 0;     // Sum of absolute values: |battContactorPos| + |battContactorNeg|
float battLinkPos = 0;     // Channel 2: Link-Pos (after contactors)
float battLinkNeg = 0;     // Channel 3: Link-Neg (after contactors)
bool ads1115Initialized = false;

// Balance control variable
bool balanceEnabled = false;

// PackContactors and Pre Charge Relay state variables
bool packContactorsEnabled = false;
bool preChargeRelayEnabled = false;
unsigned long packContactorsStartTime = 0;
unsigned long preChargeRelayStartTime = 0;
bool initialPulseComplete = false;
bool preChargeRelayInitialPulseComplete = false;

// Add global variable for current duty cycle
volatile uint8_t currentDutyCycle = 0;

// Timer variables
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 500; // Update interval for value tracking

// Serial command buffers
String serialCommand = "";
String serial2Command = "";

// Function declarations
void processSerialInputs();
void sendAllParametersToESPHome();
void sendAllParametersToStream(Stream& stream);
void setPackContactorsDutyCycle(uint8_t dutyCycle);
void setPreChargeRelayDutyCycle(uint8_t dutyCycle);

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
    else if (lowerCommand == "pack contactors on" || lowerCommand == "pack contactors enable") {
        if (!packContactorsEnabled) {
            packContactorsEnabled = true;
            setPackContactorsDutyCycle(100);
            packContactorsStartTime = millis();
            initialPulseComplete = false;
        }
        serialPort.println("Pack Contactors ENABLED");
    }
    else if (lowerCommand == "pack contactors off" || lowerCommand == "pack contactors disable") {
        packContactorsEnabled = false;
        setPackContactorsDutyCycle(0);
        initialPulseComplete = false;
        serialPort.println("Pack Contactors DISABLED");
    }
    else if (lowerCommand == "pack contactors status" || lowerCommand == "pack contactors") {
        serialPort.printf("Pack Contactors are currently: %s\n", packContactorsEnabled ? "ENABLED" : "DISABLED");
    }
    else if (lowerCommand == "pre charge relay on" || lowerCommand == "pre charge relay enable") {
        if (!preChargeRelayEnabled) {
            preChargeRelayEnabled = true;
            setPreChargeRelayDutyCycle(100);
            preChargeRelayStartTime = millis();
            preChargeRelayInitialPulseComplete = false;
        }
        serialPort.println("Pre Charge Relay ENABLED");
    }
    else if (lowerCommand == "pre charge relay off" || lowerCommand == "pre charge relay disable") {
        preChargeRelayEnabled = false;
        setPreChargeRelayDutyCycle(0);
        preChargeRelayInitialPulseComplete = false;
        serialPort.println("Pre Charge Relay DISABLED");
    }
    else if (lowerCommand == "pre charge relay status" || lowerCommand == "pre charge relay") {
        serialPort.printf("Pre Charge Relay is currently: %s\n", preChargeRelayEnabled ? "ENABLED" : "DISABLED");
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
        coulombCounter.startDiagnostics(serialPort);
    }
    else if (lowerCommand == "start as8510" || lowerCommand == "as8510 start") {
        coulombCounter.startAS8510NonBlocking(serialPort);
    }
    else if (lowerCommand == "as8510 errors" || lowerCommand == "errors") {
        serialPort.println("AS8510 error codes are included in 'coulomb status' or 'current diag'");
        coulombCounter.printCurrentSensorStatus(serialPort);
    }
    else if (lowerCommand == "as8510 saturation" || lowerCommand == "saturation") {
        serialPort.println("AS8510 saturation flags are included in 'coulomb status' or 'current diag'");
        coulombCounter.printCurrentSensorStatus(serialPort);
    }
    else if (lowerCommand == "as8510 diagnostics" || lowerCommand == "diagnostics") {
        serialPort.println("Running complete AS8510 diagnostics via coulomb counter...");
        coulombCounter.startDiagnostics(serialPort);
    }
    else if (lowerCommand == "coulomb status" || lowerCommand == "coulomb") {
        coulombCounter.printStatus(serialPort);
    }
    else if (lowerCommand == "coulomb reset" || lowerCommand == "reset coulomb") {
        coulombCounter.resetEnergyCounters();
        serialPort.println("Coulomb counters reset successfully");
    }
    else if (lowerCommand.startsWith("coulomb capacity ")) {
        String capacityStr = lowerCommand.substring(17);
        float capacity = capacityStr.toFloat();
        if (capacity > 0) {
            coulombCounter.setBatteryCapacity(capacity);
        } else {
            serialPort.println("Error: Invalid battery capacity. Must be positive number.");
        }
    }
    else if (lowerCommand.startsWith("coulomb voltage ")) {
        String voltageStr = lowerCommand.substring(16);
        float voltage = voltageStr.toFloat();
        if (voltage > 0) {
            coulombCounter.setFullyChargedVoltage(voltage);
        } else {
            serialPort.println("Error: Invalid fully charged voltage. Must be positive number.");
        }
    }
    else if (lowerCommand.startsWith("coulomb efficiency ")) {
        String efficiencyStr = lowerCommand.substring(19);
        float efficiency = efficiencyStr.toFloat();
        if (efficiency > 0 && efficiency <= 1.0) {
            coulombCounter.setCurrentEfficiency(efficiency);
        } else {
            serialPort.println("Error: Invalid efficiency. Must be between 0 and 1.0 (e.g., 0.95 for 95%).");
        }
    }
    else if (lowerCommand == "ads1115 read" || lowerCommand == "pack voltages") {
        if (ads1115Initialized) {
            serialPort.println("=== ADS1115 Pack Voltage Readings ===");
            serialPort.printf("Batt-Neg (Channel 0): %.3fV\n", battContactorNeg);
            serialPort.printf("Batt-Pos (Channel 1): %.3fV\n", battContactorPos);
            serialPort.printf("Batt-Sum (Absolute Total): %.3fV\n", battContactorSum);
            serialPort.printf("Link-Pos - Post-Contactors (Channel 2): %.3fV\n", battLinkPos);
            serialPort.printf("Link-Neg - Post-Contactors (Channel 3): %.3fV\n", battLinkNeg);
            serialPort.println("Note: Link voltages measured after contactor closure");
            serialPort.println("=============================================");
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
        serialPort.println("  pack contactors on / pack contactors enable  - Enable pack contactors");
        serialPort.println("  pack contactors off / pack contactors disable - Disable pack contactors");
        serialPort.println("  pack contactors status / pack contactors      - Show pack contactors status");
        serialPort.println("  pre charge relay on / pre charge relay enable - Enable pre charge relay");
        serialPort.println("  pre charge relay off / pre charge relay disable - Disable pre charge relay");
        serialPort.println("  pre charge relay status / pre charge relay    - Show pre charge relay status");
        serialPort.println("  mapping / debug              - Show hardware register mapping");
        serialPort.println("  bmb registers / registers    - Show detailed BMB register analysis");
        serialPort.println("  bmb debug on/off             - Enable/disable live BMB register debugging");
        serialPort.println("  current diag                 - Run current sensor diagnostics");
        serialPort.println("  start as8510                 - Explicitly start AS8510 device");
        serialPort.println("  as8510 errors / errors       - Show AS8510 error codes");
        serialPort.println("  as8510 saturation / saturation - Show AS8510 saturation flags");
        serialPort.println("  as8510 diagnostics / diagnostics - Complete AS8510 diagnostics");
        serialPort.println("  coulomb status / coulomb     - Show coulomb counter status (power, energy, SOC)");
        serialPort.println("  coulomb reset / reset coulomb - Reset energy counters and SOC to 100%");
        serialPort.println("  coulomb capacity <Ah>        - Set battery capacity in Ah (e.g., 75)");
        serialPort.println("  coulomb voltage <V>          - Set fully charged cell voltage (e.g., 4.2)");
        serialPort.println("  coulomb efficiency <factor>  - Set current efficiency (0-1.0, e.g., 0.95)");
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



// Function to set pack contactors PWM duty cycle (0-100%)
void setPackContactorsDutyCycle(uint8_t dutyCycle) {
    // Convert percentage to 8-bit value (0-255)
    uint32_t pwmValue = (dutyCycle * 255) / 100;
    ledcWrite(PACK_CONTACTORS_PWM_PIN, pwmValue);
    
    // Print duty cycle change to serial
    if (dutyCycle != prevDutyCycle) {
        Serial.print("PackContactors duty cycle: ");
        Serial.print(dutyCycle);
        Serial.println("%");
        prevDutyCycle = dutyCycle;
    }
    currentDutyCycle = dutyCycle; // Always update global
}

// Function to set pre charge relay PWM duty cycle (0-100%)
void setPreChargeRelayDutyCycle(uint8_t dutyCycle) {
    // Convert percentage to 8-bit value (0-255)
    uint32_t pwmValue = (dutyCycle * 255) / 100;
    ledcWrite(PRE_CHARGE_RELAY_PWM_PIN, pwmValue);
    
    // Print duty cycle change to serial
    Serial.print("PreChargeRelay duty cycle: ");
    Serial.print(dutyCycle);
    Serial.println("%");
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
}

// Function to update parameters from BATMan system data
void updateParametersFromBATMan() {
    // REMOVED: Cell voltage setting - this is now handled by upDateCellVolts() to avoid off-by-one errors
    // The main BATMan system already sets u1-u108 parameters correctly
    
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
    
    // Update AS8510 current sensor data - handled by coulomb counter
    
    // Update AS8510 temperature every time parameters are updated - handled by coulomb counter
    if (coulombCounter.isInitialized()) {
        float internalTemp = coulombCounter.getInternalTemperature();
        Param::SetFloat(Param::as8510_temp, internalTemp);
    } else {
        // If not initialized, set temperature to 0
        Param::SetFloat(Param::as8510_temp, 0.0);
    }
    
    // Update ADS1115 pack voltage data
    if (ads1115Initialized) {
        // Store pack voltages in dedicated ADS1115 parameters
        Param::SetFloat(Param::battContactorPos, battContactorPos);  // Batt-Pos
        Param::SetFloat(Param::battContactorNeg, battContactorNeg);  // Batt-Neg
        Param::SetFloat(Param::udc, battContactorSum);      // Battery sum (absolute values, total pack voltage)
        Param::SetFloat(Param::battLinkPos, battLinkPos);  // Link-Pos (after contactors)
        Param::SetFloat(Param::battLinkNeg, battLinkNeg);  // Link-Neg (after contactors)
    }
}

// Global variables for non-blocking operation
static unsigned long lastMainLoopTime = 0;
static const unsigned long MAIN_LOOP_INTERVAL = 50; // 50ms interval without blocking delay





void sendAllParametersToESPHome() {
    // Send all parameters to Serial2 (ESPHome) using the generic function
    sendAllParametersToStream(Serial2);
    
    // Keep the debug output for cell 7 specifically for troubleshooting
    float cell7Voltage = Param::GetFloat(Param::u7);
    if (!std::isnan(cell7Voltage) && cell7Voltage > 0) {
        Serial.printf("DEBUG Cell7: enum=%d, cellID=7, stored_value=%.1f, sending=%.0f\n", 
                     Param::u7, 7, cell7Voltage, cell7Voltage);
    }
}

// Generic function to send all parameters to any Stream (Serial, Serial2, etc.)
void sendAllParametersToStream(Stream& stream) {
    // Send all parameters in param=value format
    
    // System parameters
    stream.printf("numbmbs=%d\n", Param::GetInt(Param::numbmbs));
    stream.printf("LoopCnt=%d\n", Param::GetInt(Param::LoopCnt));
    stream.printf("LoopState=%d\n", Param::GetInt(Param::LoopState));
    stream.printf("CellsPresent=%d\n", Param::GetInt(Param::CellsPresent));
    stream.printf("CellsBalancing=%d\n", Param::GetInt(Param::CellsBalancing));
    stream.printf("balance=%d\n", Param::GetInt(Param::balance));
    stream.printf("BalanceCellList=%s\n", Param::GetString(Param::BalanceCellList).c_str());
    
    // BMB Connectivity parameters
    stream.printf("ActualBmbCount=%d\n", Param::GetInt(Param::ActualBmbCount));
    stream.printf("ExpectedBmbCount=%d\n", Param::GetInt(Param::ExpectedBmbCount));
    stream.printf("BmbConnectedMask=%d\n", Param::GetInt(Param::BmbConnectedMask));
    
    // Contactor states
    stream.printf("packContactors=%d\n", packContactorsEnabled ? 1 : 0);
    stream.printf("preChargeRelay=%d\n", preChargeRelayEnabled ? 1 : 0);
    
    // Voltage statistics
    stream.printf("CellMax=%d\n", Param::GetInt(Param::CellMax));
    stream.printf("CellMin=%d\n", Param::GetInt(Param::CellMin));
    stream.printf("umax=%.0f\n", Param::GetFloat(Param::umax));
    stream.printf("umin=%.0f\n", Param::GetFloat(Param::umin));
    stream.printf("deltaV=%.0f\n", Param::GetFloat(Param::deltaV));
    stream.printf("uavg=%.3f\n", Param::GetFloat(Param::uavg));
    stream.printf("udc=%.2f\n", Param::GetFloat(Param::udc));
    stream.printf("CellVoltageSum=%.2f\n", Param::GetFloat(Param::CellVoltageSum));
    
    // Temperature parameters
    stream.printf("Chipt0=%d\n", Param::GetInt(Param::Chipt0));
    stream.printf("Cellt0_0=%d\n", Param::GetInt(Param::Cellt0_0));
    stream.printf("Cellt0_1=%d\n", Param::GetInt(Param::Cellt0_1));
    stream.printf("TempMax=%d\n", Param::GetInt(Param::TempMax));
    stream.printf("TempMin=%d\n", Param::GetInt(Param::TempMin));
    
    // ADS1115 pack voltages
    stream.printf("battContactorPos=%.3f\n", Param::GetFloat(Param::battContactorPos));  // Batt-Pos
    stream.printf("battContactorNeg=%.3f\n", Param::GetFloat(Param::battContactorNeg));  // Batt-Neg
    stream.printf("battLinkPos=%.3f\n", Param::GetFloat(Param::battLinkPos));  // Link-Pos
    stream.printf("battLinkNeg=%.3f\n", Param::GetFloat(Param::battLinkNeg));  // Link-Neg
    
    // AS8510 Current and Temperature
    stream.printf("current=%.3f\n", Param::GetFloat(Param::current));
    stream.printf("as8510_temp=%.1f\n", Param::GetFloat(Param::as8510_temp));
    
    // AS8510 Coulomb Counting
    stream.printf("PowerWatts=%.3f\n", Param::GetFloat(Param::PowerWatts));
    stream.printf("EnergyWh=%.2f\n", Param::GetFloat(Param::EnergyWh));
    stream.printf("EnergyKWh=%.3f\n", Param::GetFloat(Param::EnergyKWh));
    stream.printf("StateOfCharge=%.1f\n", Param::GetFloat(Param::StateOfCharge));
    stream.printf("RemainingCapacityAh=%.1f\n", Param::GetFloat(Param::RemainingCapacityAh));
    stream.printf("BatteryCapacityAh=%.1f\n", Param::GetFloat(Param::BatteryCapacityAh));
    stream.printf("FullyChargedVoltage=%.2f\n", Param::GetFloat(Param::FullyChargedVoltage));
    stream.printf("CurrentEfficiency=%.2f\n", Param::GetFloat(Param::CurrentEfficiency));
    
    // Send individual cell voltages in optimized format
    // Format: cellID=voltage (e.g., 1=3770, 2=3814, etc.) or cellID=nan for offline cells
    int cellID = 1;
    for (int i = Param::u1; i <= Param::u108; i++) {
        float cellVoltage = Param::GetFloat(static_cast<Param::PARAM_NUM>(i));
        
        // Send all configured cells, including NaN values for offline BMBs
        if (std::isnan(cellVoltage)) {
            // Send NaN as "nan" string for offline BMB cells
            stream.printf("%d=nan\n", cellID);
            cellID++;
        } else if (cellVoltage > 0) {
            // Send valid voltage readings
            stream.printf("%d=%.0f\n", cellID, cellVoltage);
            cellID++;
        }
        // Skip cells with 0 voltage (unused cell positions)
    }
    
    // Send end marker to indicate complete data packet
    stream.println("DATA_COMPLETE");
}

void setup() {
    Serial.begin(115200);
    Serial.println("Tesla Model 3 BMB Interface Starting...");
    
    // Initialize second serial interface
    Serial2.begin(SERIAL2_BAUD_RATE, SERIAL_8N1, SERIAL2_RX_PIN, SERIAL2_TX_PIN); // RX=12, TX=13
    

    
    // Initialize PWM for pack contactors using new ESP32 Arduino core 3.0 API
    ledcAttach(PACK_CONTACTORS_PWM_PIN, PWM_FREQ, PWM_RESOLUTION);
    setPackContactorsDutyCycle(0);  // Start with pack contactors off
    
    // Initialize PWM for pre charge relay using new ESP32 Arduino core 3.0 API
    ledcAttach(PRE_CHARGE_RELAY_PWM_PIN, PWM_FREQ, PWM_RESOLUTION);
    setPreChargeRelayDutyCycle(0);  // Start with pre charge relay off
    
    // Button initialization removed - contactors now controlled via serial API
    
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
        ads.setGain(GAIN_EIGHT);     // ±0.512V range (1 bit = 0.015625mV)
        ads.setDataRate(RATE_ADS1115_860SPS);  // 860 samples per second
        
        Serial.println("ADS1115 Configuration:");
        Serial.println("  - Gain: ±0.512V (1 bit = 0.015625mV)");
        Serial.println("  - Data Rate: 860 SPS");
        Serial.println("  - Channel 0: Batt-Neg");
        Serial.println("  - Channel 1: Batt-Pos");
        Serial.println("  - Channel 2: Link-Pos (after contactors)");
        Serial.println("  - Channel 3: Link-Neg (after contactors)");
        
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
    
    // Initialize AS8510 Coulomb Counter (handles AS8510 initialization internally)
    Serial.println("Initializing AS8510 Coulomb Counter...");
    if (coulombCounter.initialize()) {
        Serial.println("AS8510 Coulomb Counter initialized successfully!");
    } else {
        Serial.println("AS8510 Coulomb Counter initialization failed!");
    }
    
    Serial.println("System ready. Commands available on both Serial and Serial2 (pins 22/23)");
    Serial.printf("AS8510 on VSPI bus - BMB on HSPI - ADS1115 on I2C (%s) - All systems enabled\n", 
                  ads1115Initialized ? "OK" : "FAILED");
    Serial.println("============ Setup Complete - Starting Main Loop =============");
    
    // Initialize parameter logger
    ParamLogger::begin();
}

void loop() {
    // Get current time for all timing operations
    unsigned long currentMillis = millis();
    
    // Auto-send all parameters to ESPHome once per second
    static unsigned long lastESPHomeUpdate = 0;
    if (currentMillis - lastESPHomeUpdate >= 1000) {
        sendAllParametersToESPHome();
        lastESPHomeUpdate = currentMillis;
    }
    
    // Debug: Send Serial2 data stream to Serial every 10 seconds for monitoring
    static unsigned long lastSerialDebugUpdate = 0;
    if (currentMillis - lastSerialDebugUpdate >= 10000) {
        Serial.println("\n=== Serial2 Data Stream Debug (10s interval) ===");
        sendAllParametersToStream(Serial);
        Serial.println("=== End Serial2 Data Stream Debug ===\n");
        lastSerialDebugUpdate = currentMillis;
    }
    
    // Throttle main loop execution to maintain timing without blocking delays
    if (currentMillis - lastMainLoopTime < MAIN_LOOP_INTERVAL) {
        // Process serial commands even during throttled periods
        processSerialInputs();
        
        // Run non-blocking diagnostic steps if in progress
        coulombCounter.runDiagnosticStep();
        
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
    
    // AS8510 COULOMB COUNTER UPDATE - Uses cell sum voltage for power calculation
    // Update coulomb counter with battery voltage (use cell sum voltage as pack voltage)
    float cellSumVoltage = Param::GetFloat(Param::CellVoltageSum) / 1000.0;  // Convert mV to V
    coulombCounter.update(cellSumVoltage);
    
    // Display coulomb counter data every 5 seconds
    static unsigned long lastCoulombDisplay = 0;
    if (currentMillis - lastCoulombDisplay >= 5000) {
        lastCoulombDisplay = currentMillis;
        Serial.printf("Coulomb: %.2fA | %.1fW | %.1fWh | %.2fkWh | SOC: %.1f%% | Remaining: %.1fAh\n", 
                     Param::GetFloat(Param::current), 
                     Param::GetFloat(Param::PowerWatts), 
                     Param::GetFloat(Param::EnergyWh), 
                     Param::GetFloat(Param::EnergyKWh), 
                     Param::GetFloat(Param::StateOfCharge), 
                     Param::GetFloat(Param::RemainingCapacityAh));
    }
    
    // ADS1115 PACK VOLTAGE MEASUREMENT - Every 3 seconds
    static unsigned long lastVoltageRead = 0;
    if (currentMillis - lastVoltageRead >= 3000) {
        lastVoltageRead = currentMillis;
        
        if (ads1115Initialized) {
            // Read all channels using differential mode
            int16_t adc0 = ads.readADC_Differential_0_3();
            int16_t adc1 = ads.readADC_Differential_1_3();
            int16_t adc2 = ads.readADC_Differential_2_3();
            int16_t adc3 = ads.readADC_Differential_2_3();  // Will be changed later
            
            // Convert to voltages (±0.512V range, 1 bit = 0.015625mV)
            // Apply scaling factor: 25V input = 0.067V ADC, so scale factor = 373.13
            const float VOLTAGE_SCALE_FACTOR = 25.0 / 0.067;  // 373.13
            battContactorPos = ads.computeVolts(adc1) * VOLTAGE_SCALE_FACTOR;  // A1 = Batt positive
            battContactorNeg = ads.computeVolts(adc0) * VOLTAGE_SCALE_FACTOR;  // A0 = Batt negative
            battContactorSum = fabs(battContactorPos) + fabs(battContactorNeg);  // Calculate sum of absolute battery voltages
            battLinkPos = ads.computeVolts(adc2) * VOLTAGE_SCALE_FACTOR;
            battLinkNeg = ads.computeVolts(adc3) * VOLTAGE_SCALE_FACTOR;
            
            // Display pack voltages with descriptive labels
            Serial.printf("ADS1115 Pack Voltages: Batt-Neg=%.3fV Batt-Pos=%.3fV Batt-Sum(abs)=%.3fV Link-Pos(post-contactors)=%.3fV Link-Neg(post-contactors)=%.3fV\n", 
                         battContactorNeg, battContactorPos, battContactorSum, battLinkPos, battLinkNeg);
            
        } else {
            Serial.println("ADS1115 ADC not initialized - pack voltage readings unavailable");
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
    
    // Button handling removed - contactors now controlled via serial API
    
    // Handle initial pulse timing
    if (packContactorsEnabled && !initialPulseComplete) {
        if ((millis() - packContactorsStartTime) >= INITIAL_PULSE_TIME) {
            setPackContactorsDutyCycle(PACK_CONTACTORS_DUTY);  // Set to normal duty cycle
            initialPulseComplete = true;
        }
    }
    
    // Handle pre charge relay initial pulse timing
    if (preChargeRelayEnabled && !preChargeRelayInitialPulseComplete) {
        if ((millis() - preChargeRelayStartTime) >= INITIAL_PULSE_TIME) {
            setPreChargeRelayDutyCycle(PRE_CHARGE_RELAY_DUTY);  // Set to normal duty cycle
            preChargeRelayInitialPulseComplete = true;
        }
    }
    
    // Button state tracking removed - contactors now controlled via serial API
    
    // Process serial commands (moved to separate function for reuse)
    processSerialInputs();
    
    // Update parameter logger (every 3 seconds)
    ParamLogger::update();
    
    // Run non-blocking diagnostic steps if in progress
    coulombCounter.runDiagnosticStep();
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