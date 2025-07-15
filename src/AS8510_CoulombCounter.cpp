#include "../include/AS8510_CoulombCounter.h"
#include "../AS8510-library/as8510.h"

AS8510_CoulombCounter::AS8510_CoulombCounter(uint8_t cs_pin, uint8_t mosi_pin, uint8_t miso_pin, uint8_t sck_pin, int current_gain, int voltage_gain) : 
    currentSensor(new AS8510(cs_pin, mosi_pin, miso_pin, sck_pin, static_cast<Gain>(current_gain), static_cast<Gain>(voltage_gain))),
    currentReading(0),
    prevCurrentReading(0),
    powerWatts(0),
    energyWh(0),
    energyKWh(0),
    totalCapacityAh(75.0),  // Default Tesla Model 3 battery capacity
    remainingCapacityAh(75.0),
    stateOfChargePercent(100.0),
    fullyChargedVoltage(4.2),  // Default fully charged cell voltage
    currentEfficiencyFactor(0.95),  // Default 95% efficiency
    lastUpdateTime(0),
    lastCurrentReadTime(0),
    lastStatusDisplayTime(0),
    diagnosticInProgress(false),
    diagnosticStep(0),
    diagnosticStepTime(0),
    diagnosticSerial(nullptr)
{
    // Initialize timing
    lastUpdateTime = millis();
    lastCurrentReadTime = millis();
    lastStatusDisplayTime = millis();
}

AS8510_CoulombCounter::~AS8510_CoulombCounter() {
    delete currentSensor;
}

bool AS8510_CoulombCounter::initialize() {
    if (!currentSensor) {
        Serial.println("AS8510_CoulombCounter: Current sensor not created");
        return false;
    }
    
    // Initialize AS8510 sensor
    if (!currentSensor->begin()) {
        Serial.println("AS8510_CoulombCounter: Failed to initialize AS8510 sensor");
        return false;
    }
    
    // Set verbose logging to false for clean output
    currentSensor->setVerboseLogging(false);
    
    // Load configuration from parameters
    totalCapacityAh = Param::GetFloat(Param::BatteryCapacityAh);
    fullyChargedVoltage = Param::GetFloat(Param::FullyChargedVoltage);
    currentEfficiencyFactor = Param::GetFloat(Param::CurrentEfficiency);
    
    // Initialize energy counters from stored parameters
    energyWh = Param::GetFloat(Param::EnergyWh);
    energyKWh = energyWh / 1000.0;
    
    Serial.println("AS8510_CoulombCounter: Initialized successfully");
    Serial.printf("Battery Capacity: %.1f Ah, Fully Charged Voltage: %.2f V, Efficiency: %.2f%%\n", 
                  totalCapacityAh, fullyChargedVoltage, currentEfficiencyFactor * 100.0);
    
    return true;
}

void AS8510_CoulombCounter::update(float cellSumVoltage) {
    unsigned long currentTime = millis();
    
    // Update current reading at specified interval
    if (currentTime - lastCurrentReadTime >= CURRENT_READ_INTERVAL) {
        lastCurrentReadTime = currentTime;
        
        if (currentSensor && currentSensor->isInitialized()) {
            prevCurrentReading = currentReading;
            currentReading = currentSensor->getCurrent();
            
            // Update power calculation
            updatePowerCalculation(cellSumVoltage);
            
            // Update energy accumulation
            updateEnergyAccumulation();
            
            // Update state of charge
            updateStateOfCharge();
            
            // Update parameters for API and ESPHome
            updateParameters();
            
            // Display status periodically
            if (currentTime - lastStatusDisplayTime >= STATUS_DISPLAY_INTERVAL) {
                lastStatusDisplayTime = currentTime;
                float internalTemp = getInternalTemperature();
                Serial.printf("AS8510: %.3fA  %.3fW  %.2fWh  %.1f°C  SOC: %.1f%%\n", 
                             currentReading, powerWatts, energyWh, internalTemp, stateOfChargePercent);
            }
        } else {
            Serial.println("AS8510_CoulombCounter: Sensor not initialized - attempting restart...");
            if (currentSensor) {
                currentSensor->startDevice();
            }
        }
    }
}

void AS8510_CoulombCounter::updatePowerCalculation(float voltage) {
    // Calculate instantaneous power: P = V × I
    powerWatts = voltage * currentReading;
    
    // Handle charging vs discharging
    // Positive current = charging, negative current = discharging
    // Power sign indicates direction: positive = charging, negative = discharging
}

void AS8510_CoulombCounter::updateEnergyAccumulation() {
    unsigned long currentTime = millis();
    
    // Calculate time delta in hours
    float timeDeltaHours = (currentTime - lastUpdateTime) / 3600000.0;  // Convert ms to hours
    
    // Accumulate energy: E = ∫(P × dt)
    float energyDelta = powerWatts * timeDeltaHours;
    energyWh += energyDelta;
    energyKWh = energyWh / 1000.0;
    
    // Update coulomb counting
    // Convert power to charge: Q = P × dt / V
    // For coulomb counting, we integrate current over time
    float chargeDelta = currentReading * timeDeltaHours;  // Ah
    
    // Apply efficiency factor
    if (currentReading > 0) {
        // Charging - apply efficiency loss
        chargeDelta *= currentEfficiencyFactor;
    }
    
    // Update remaining capacity
    remainingCapacityAh += chargeDelta;
    
    // Clamp remaining capacity to valid range
    if (remainingCapacityAh > totalCapacityAh) {
        remainingCapacityAh = totalCapacityAh;
    } else if (remainingCapacityAh < 0) {
        remainingCapacityAh = 0;
    }
    
    lastUpdateTime = currentTime;
}

void AS8510_CoulombCounter::updateStateOfCharge() {
    // Calculate SOC as percentage of total capacity
    stateOfChargePercent = (remainingCapacityAh / totalCapacityAh) * 100.0;
    
    // Clamp to valid range
    if (stateOfChargePercent > 100.0) {
        stateOfChargePercent = 100.0;
    } else if (stateOfChargePercent < 0.0) {
        stateOfChargePercent = 0.0;
    }
}

void AS8510_CoulombCounter::updateParameters() {
    // Update current measurement
    Param::SetFloat(Param::current, currentReading);
    
    // Update power calculation
    Param::SetFloat(Param::PowerWatts, powerWatts);
    
    // Update energy measurements
    Param::SetFloat(Param::EnergyWh, energyWh);
    Param::SetFloat(Param::EnergyKWh, energyKWh);
    
    // Update coulomb counting results
    Param::SetFloat(Param::StateOfCharge, stateOfChargePercent);
    Param::SetFloat(Param::RemainingCapacityAh, remainingCapacityAh);
    
    // Update AS8510 temperature
    if (currentSensor && currentSensor->isInitialized()) {
        float internalTemp = currentSensor->getInternalTemperature();
        Param::SetFloat(Param::as8510_temp, internalTemp);
    } else {
        Param::SetFloat(Param::as8510_temp, 0.0);
    }
}

void AS8510_CoulombCounter::setBatteryCapacity(float capacityAh) {
    totalCapacityAh = capacityAh;
    Param::SetFloat(Param::BatteryCapacityAh, capacityAh);
    Serial.printf("Battery capacity set to %.1f Ah\n", capacityAh);
}

void AS8510_CoulombCounter::setFullyChargedVoltage(float voltage) {
    fullyChargedVoltage = voltage;
    Param::SetFloat(Param::FullyChargedVoltage, voltage);
    Serial.printf("Fully charged voltage set to %.2f V\n", voltage);
}

void AS8510_CoulombCounter::setCurrentEfficiency(float efficiency) {
    currentEfficiencyFactor = efficiency;
    Param::SetFloat(Param::CurrentEfficiency, efficiency);
    Serial.printf("Current efficiency set to %.2f%%\n", efficiency * 100.0);
}

void AS8510_CoulombCounter::resetEnergyCounters() {
    energyWh = 0;
    energyKWh = 0;
    remainingCapacityAh = totalCapacityAh;
    stateOfChargePercent = 100.0;
    
    // Update parameters
    Param::SetFloat(Param::EnergyWh, energyWh);
    Param::SetFloat(Param::EnergyKWh, energyKWh);
    Param::SetFloat(Param::StateOfCharge, stateOfChargePercent);
    Param::SetFloat(Param::RemainingCapacityAh, remainingCapacityAh);
    
    Serial.println("Energy counters reset to zero, SOC reset to 100%");
}

void AS8510_CoulombCounter::startDiagnostics(HardwareSerial& serialPort) {
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

void AS8510_CoulombCounter::runDiagnosticStep() {
    if (!diagnosticInProgress || !diagnosticSerial || !currentSensor) return;
    
    unsigned long currentTime = millis();
    if (currentTime - diagnosticStepTime < DIAGNOSTIC_STEP_INTERVAL) return;
    
    switch (diagnosticStep) {
        case 0:
            diagnosticSerial->println("\n=== AS8510 Current Sensor Diagnostics (Rust-based) ===");
            diagnosticSerial->println();
            
            if (!currentSensor->isInitialized()) {
                diagnosticSerial->println("ERROR: AS8510 not initialized!");
                diagnosticSerial->println("Attempting to initialize...");
                if (currentSensor->begin()) {
                    diagnosticSerial->println("AS8510 initialized successfully!");
                } else {
                    diagnosticSerial->println("AS8510 initialization failed!");
                    diagnosticInProgress = false;
                    return;
                }
            }
            break;
            
        case 1:
            diagnosticSerial->printf("Shunt Resistance: %.9f ohms\n", currentSensor->getShuntResistance());
            diagnosticSerial->printf("Device Present: %s\n", currentSensor->isDevicePresent() ? "YES" : "NO");
            diagnosticSerial->printf("Device Awake: %s\n", currentSensor->isAwake() ? "YES" : "NO");
            diagnosticSerial->printf("Data Ready: %s\n", currentSensor->isDataReady() ? "YES" : "NO");
            break;
            
        case 2:
            diagnosticSerial->println("\n--- Key Registers ---");
            diagnosticSerial->println();
            diagnosticSerial->printf("Mode Control (0x0A): 0x%02X\n", currentSensor->readRegister(0x0A));
            diagnosticSerial->printf("Status (0x04): 0x%02X\n", currentSensor->readRegister(0x04));
            diagnosticSerial->printf("PGA Control (0x13): 0x%02X\n", currentSensor->readRegister(0x13));
            break;
            
        case 3:
            diagnosticSerial->printf("Power Control 1 (0x14): 0x%02X\n", currentSensor->readRegister(0x14));
            diagnosticSerial->printf("Power Control 2 (0x15): 0x%02X\n", currentSensor->readRegister(0x15));
            diagnosticSerial->printf("Clock Control (0x08): 0x%02X\n", currentSensor->readRegister(0x08));
            break;
            
        case 4:
            diagnosticSerial->println("\n--- Data Registers ---");
            diagnosticSerial->println();
            diagnosticSerial->printf("Current Data 1 (0x00): 0x%02X\n", currentSensor->readRegister(0x00));
            diagnosticSerial->printf("Current Data 2 (0x01): 0x%02X\n", currentSensor->readRegister(0x01));
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
                int16_t rawADC = currentSensor->readRawADC(1);
                float current = currentSensor->readCurrent(1);
                
                diagnosticSerial->printf("Measurement %d: Raw ADC = %d, Current = %.6f A\n", 
                                      measurementNum, rawADC, current);
            }
            break;
            
        case 10:
            diagnosticSerial->println("\n--- Status Information ---");
            diagnosticSerial->println();
            currentSensor->printStatus();
            diagnosticSerial->println();
            diagnosticSerial->println("--- Coulomb Counter Status ---");
            diagnosticSerial->printf("Power: %.3f W\n", powerWatts);
            diagnosticSerial->printf("Energy: %.2f Wh (%.3f kWh)\n", energyWh, energyKWh);
            diagnosticSerial->printf("State of Charge: %.1f%%\n", stateOfChargePercent);
            diagnosticSerial->printf("Remaining Capacity: %.1f Ah / %.1f Ah\n", remainingCapacityAh, totalCapacityAh);
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

void AS8510_CoulombCounter::startAS8510NonBlocking(HardwareSerial& serialPort) {
    if (!currentSensor) {
        serialPort.println("AS8510_CoulombCounter: No sensor available");
        return;
    }
    
    serialPort.println("Explicitly starting AS8510 device...");
    currentSensor->startDevice();
    
    // Wait a bit and check status
    delay(100);
    uint8_t modCtl = currentSensor->readRegister(0x0A);
    serialPort.printf("Mode Control after start: 0x%02X\n", modCtl);
    if (modCtl & 0x01) {
        serialPort.println("START bit is SET - device should be running");
    } else {
        serialPort.println("START bit is NOT SET - device is not running");
    }
}

void AS8510_CoulombCounter::printStatus(HardwareSerial& serialPort) {
    serialPort.println("┌─── AS8510 Coulomb Counter Status ───┐");
    serialPort.println();
    serialPort.printf("Current: %.6f A\n", currentReading);
    serialPort.printf("Power: %.3f W\n", powerWatts);
    serialPort.printf("Energy: %.2f Wh (%.3f kWh)\n", energyWh, energyKWh);
    serialPort.printf("State of Charge: %.1f%%\n", stateOfChargePercent);
    serialPort.printf("Remaining Capacity: %.1f Ah / %.1f Ah\n", remainingCapacityAh, totalCapacityAh);
    serialPort.printf("Internal Temperature: %.1f°C\n", getInternalTemperature());
    serialPort.printf("Initialized: %s\n", isInitialized() ? "YES" : "NO");
    serialPort.printf("Data Ready: %s\n", isDataReady() ? "YES" : "NO");
    serialPort.printf("Device Awake: %s\n", isAwake() ? "YES" : "NO");
    serialPort.println("└─────────────────────────────────────┘");
}

void AS8510_CoulombCounter::printCurrentSensorStatus(HardwareSerial& serialPort) {
    if (!currentSensor) {
        serialPort.println("AS8510_CoulombCounter: No sensor available");
        return;
    }
    
    serialPort.println("┌─── AS8510 Sensor Status ───┐");
    serialPort.println();
    serialPort.printf("Initialized: %s\n", currentSensor->isInitialized() ? "YES" : "NO");
    serialPort.printf("Data Ready: %s\n", currentSensor->isInitialized() ? (currentSensor->isDataReady() ? "YES" : "NO") : "N/A");
    serialPort.printf("Device Awake: %s\n", currentSensor->isInitialized() ? (currentSensor->isAwake() ? "YES" : "NO") : "N/A");
    
    // Quick test read
    if (currentSensor->isInitialized()) {
        int16_t rawADC = currentSensor->readRawADC(1);
        serialPort.printf("Quick test: Raw ADC = %d\n", rawADC);
    }
    
    serialPort.println("└─────────────────────────────┘");
}

float AS8510_CoulombCounter::getInternalTemperature() {
    if (currentSensor && currentSensor->isInitialized()) {
        return currentSensor->getInternalTemperature();
    }
    return 0.0;
}

bool AS8510_CoulombCounter::isInitialized() {
    return currentSensor ? currentSensor->isInitialized() : false;
}

bool AS8510_CoulombCounter::isDataReady() {
    return currentSensor ? currentSensor->isDataReady() : false;
}

bool AS8510_CoulombCounter::isAwake() {
    return currentSensor ? currentSensor->isAwake() : false;
} 