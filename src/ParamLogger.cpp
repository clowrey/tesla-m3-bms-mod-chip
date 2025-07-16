#include "../include/ParamLogger.h"
#include <cmath>  // For std::isnan

// Initialize static variables
unsigned long ParamLogger::lastLogTime = 0;

void ParamLogger::begin() {
    Serial.println("==================== BMS Parameter Logger Started ====================");
    Serial.println("Logging key parameters every 3 seconds:");
    Serial.println("- Cell voltages (1, 7, 15)");
    Serial.println("- Voltage statistics (min/max/avg)");
    Serial.println("- Current and power data");
    Serial.println("- Temperature readings");
    Serial.println("- System status");
    Serial.println("=======================================================================");
    
    // Force first log immediately
    forceLog();
}

void ParamLogger::update() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastLogTime >= LOG_INTERVAL) {
        forceLog();
        lastLogTime = currentTime;
    }
}

void ParamLogger::forceLog() {
    unsigned long timestamp = millis();
    
    Serial.println();
    Serial.printf("===== BMS PARAM LOG [%lu ms] =====\n", timestamp);
    
    logCellVoltages();
    logVoltageStatistics(); 
    logCurrentAndPower();
    logTemperatureData();
    logSystemStatus();
    
    Serial.println("================================");
}

void ParamLogger::logCellVoltages() {
    Serial.println("Cell Voltages:");
    
    // Log specific cells requested: 1, 7, 15
    float cell1 = Param::GetFloat(Param::u1);
    float cell7 = Param::GetFloat(Param::u7);
    float cell15 = Param::GetFloat(Param::u15);
    
    Serial.printf("  Cell 1:  ");
    if (std::isnan(cell1)) {
        Serial.println("OFFLINE");
    } else {
        Serial.printf("%.0f mV\n", cell1);
    }
    
    Serial.printf("  Cell 7:  ");
    if (std::isnan(cell7)) {
        Serial.println("OFFLINE");
    } else {
        Serial.printf("%.0f mV\n", cell7);
    }
    
    Serial.printf("  Cell 15: ");
    if (std::isnan(cell15)) {
        Serial.println("OFFLINE");
    } else {
        Serial.printf("%.0f mV\n", cell15);
    }
    
    // Also log a few more key cells for context
    float cell48 = Param::GetFloat(Param::u48); // Middle cell
    float cell96 = Param::GetFloat(Param::u96); // Near end cell
    
    Serial.printf("  Cell 48: ");
    if (std::isnan(cell48)) {
        Serial.println("OFFLINE");
    } else {
        Serial.printf("%.0f mV\n", cell48);
    }
    
    Serial.printf("  Cell 96: ");
    if (std::isnan(cell96)) {
        Serial.println("OFFLINE");
    } else {
        Serial.printf("%.0f mV\n", cell96);
    }
}

void ParamLogger::logVoltageStatistics() {
    Serial.println("Voltage Statistics:");
    
    int cellMin = Param::GetInt(Param::CellMin);
    int cellMax = Param::GetInt(Param::CellMax);
    float vMin = Param::GetFloat(Param::umin);
    float vMax = Param::GetFloat(Param::umax);
    float vAvg = Param::GetFloat(Param::uavg);
    float deltaV = Param::GetFloat(Param::deltaV);
    float packV = Param::GetFloat(Param::udc);
    
    Serial.printf("  Min: Cell %d = %.0f mV\n", cellMin, vMin);
    Serial.printf("  Max: Cell %d = %.0f mV\n", cellMax, vMax);
    Serial.printf("  Avg: %.1f mV\n", vAvg);
    Serial.printf("  Delta: %.0f mV\n", deltaV);
    Serial.printf("  Pack: %.2f V\n", packV);
}

void ParamLogger::logCurrentAndPower() {
    Serial.println("Current & Power:");
    
    float current = Param::GetFloat(Param::current);
    float power = Param::GetFloat(Param::PowerWatts);
    float energy = Param::GetFloat(Param::EnergyWh);
    float soc = Param::GetFloat(Param::StateOfCharge);
    float remainingAh = Param::GetFloat(Param::RemainingCapacityAh);
    
    Serial.printf("  Current: %.2f A\n", current);
    Serial.printf("  Power: %.1f W\n", power);
    Serial.printf("  Energy: %.1f Wh\n", energy);
    Serial.printf("  SOC: %.1f%%\n", soc);
    Serial.printf("  Remaining: %.1f Ah\n", remainingAh);
}

void ParamLogger::logTemperatureData() {
    Serial.println("Temperature:");
    
    float as8510Temp = Param::GetFloat(Param::as8510_temp);
    int chipTemp = Param::GetInt(Param::Chipt0);
    int cellTemp1 = Param::GetInt(Param::Cellt0_0);
    int cellTemp2 = Param::GetInt(Param::Cellt0_1);
    
    Serial.printf("  AS8510: %.1f°C\n", as8510Temp);
    
    if (chipTemp > 0) {
        Serial.printf("  Chip: %d°C\n", chipTemp);
    } else {
        Serial.println("  Chip: No data");
    }
    
    if (cellTemp1 > 0 || cellTemp2 > 0) {
        Serial.printf("  Cells: %d°C, %d°C\n", cellTemp1, cellTemp2);
    } else {
        Serial.println("  Cells: No data");
    }
}

void ParamLogger::logSystemStatus() {
    Serial.println("System Status:");
    
    int cellsPresent = Param::GetInt(Param::CellsPresent);
    int cellsBalancing = Param::GetInt(Param::CellsBalancing);
    int balanceEnabled = Param::GetInt(Param::balance);
    String balanceCellList = Param::GetString(Param::BalanceCellList);
    int actualBmbs = Param::GetInt(Param::ActualBmbCount);
    int expectedBmbs = Param::GetInt(Param::ExpectedBmbCount);
    int loopState = Param::GetInt(Param::LoopState);
    
    Serial.printf("  Cells Present: %d\n", cellsPresent);
    Serial.printf("  BMBs: %d/%d\n", actualBmbs, expectedBmbs);
    Serial.printf("  Loop State: %d\n", loopState);
    Serial.printf("  Balance: %s\n", balanceEnabled ? "ON" : "OFF");
    
    if (cellsBalancing > 0) {
        Serial.printf("  Balancing %d cells: %s\n", cellsBalancing, balanceCellList.c_str());
    } else {
        Serial.println("  No active balancing");
    }
} 