#ifndef AS8510_COULOMBCOUNTER_H
#define AS8510_COULOMBCOUNTER_H

#include <Arduino.h>
#include "Param.h"

// Forward declaration to avoid including AS8510 header in main.cpp
class AS8510;

class AS8510_CoulombCounter {
private:
    AS8510* currentSensor;
    
    // Measurement variables
    float currentReading;
    float prevCurrentReading;
    float powerWatts;
    float energyWh;
    float energyKWh;
    
    // Coulomb counting variables
    float totalCapacityAh;
    float remainingCapacityAh;
    float stateOfChargePercent;
    float fullyChargedVoltage;
    float currentEfficiencyFactor;
    
    // Timing variables
    unsigned long lastUpdateTime;
    unsigned long lastCurrentReadTime;
    unsigned long lastStatusDisplayTime;
    
    // Configuration constants
    static const unsigned long CURRENT_READ_INTERVAL = 100;  // 100ms for more accurate coulomb counting
    static const unsigned long STATUS_DISPLAY_INTERVAL = 10000; // 10 seconds
    
    // Diagnostic state machine
    bool diagnosticInProgress;
    int diagnosticStep;
    unsigned long diagnosticStepTime;
    static const unsigned long DIAGNOSTIC_STEP_INTERVAL = 500;
    HardwareSerial* diagnosticSerial;
    
    // Internal methods
    void updatePowerCalculation(float voltage);
    void updateEnergyAccumulation();
    void updateStateOfCharge();
    void updateParameters();
    
public:
    AS8510_CoulombCounter(uint8_t cs_pin, uint8_t mosi_pin, uint8_t miso_pin, uint8_t sck_pin, int current_gain, int voltage_gain);
    ~AS8510_CoulombCounter();
    
    // Initialization
    bool initialize();
    
    // Main update loop
    void update(float cellSumVoltage);
    
    // Getters
    float getCurrent() const { return currentReading; }
    float getPower() const { return powerWatts; }
    float getEnergyWh() const { return energyWh; }
    float getEnergyKWh() const { return energyKWh; }
    float getStateOfCharge() const { return stateOfChargePercent; }
    float getRemainingCapacity() const { return remainingCapacityAh; }
    
    // Configuration
    void setBatteryCapacity(float capacityAh);
    void setFullyChargedVoltage(float voltage);
    void setCurrentEfficiency(float efficiency);
    void resetEnergyCounters();
    
    // Diagnostic functions
    void startDiagnostics(HardwareSerial& serialPort);
    void runDiagnosticStep();
    void startAS8510NonBlocking(HardwareSerial& serialPort);
    
    // Status display
    void printStatus(HardwareSerial& serialPort);
    void printCurrentSensorStatus(HardwareSerial& serialPort);
    
    // Temperature reading
    float getInternalTemperature();
    
    // Sensor status
    bool isInitialized();
    bool isDataReady();
    bool isAwake();
};

#endif // AS8510_COULOMBCOUNTER_H 