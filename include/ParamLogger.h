#ifndef PARAM_LOGGER_H
#define PARAM_LOGGER_H

#include <Arduino.h>
#include "Param.h"

class ParamLogger {
private:
    static unsigned long lastLogTime;
    static const unsigned long LOG_INTERVAL = 3000; // 3 seconds in milliseconds
    
    // Helper methods for formatted output
    static void logCellVoltages();
    static void logVoltageStatistics();
    static void logCurrentAndPower();
    static void logTemperatureData();
    static void logSystemStatus();

public:
    // Initialize the logger
    static void begin();
    
    // Main logging method - call this from main loop
    static void update();
    
    // Force immediate log output (useful for debugging)
    static void forceLog();
};

#endif // PARAM_LOGGER_H 