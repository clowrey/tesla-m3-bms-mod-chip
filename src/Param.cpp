#include "../include/Param.h"
#include <map>
#include <Arduino.h>
#include <cstring>

// Storage for parameters
static std::map<Param::PARAM_NUM, int> intParams;
static std::map<Param::PARAM_NUM, float> floatParams;
static std::map<Param::PARAM_NUM, String> stringParams;

// Parameter name mapping
static const char* paramNames[] = {
    // System parameters
    "numbmbs", "LoopCnt", "LoopState", "CellsPresent", "CellsBalancing", "BalanceCellList",
    
    // Cell voltage parameters
    "u1", "u2", "u3", "u4", "u5", "u6", "u7", "u8", "u9", "u10",
    "u11", "u12", "u13", "u14", "u15", "u16", "u17", "u18", "u19", "u20",
    "u21", "u22", "u23", "u24", "u25", "u26", "u27", "u28", "u29", "u30",
    "u31", "u32", "u33", "u34", "u35", "u36", "u37", "u38", "u39", "u40",
    "u41", "u42", "u43", "u44", "u45", "u46", "u47", "u48", "u49", "u50",
    "u51", "u52", "u53", "u54", "u55", "u56", "u57", "u58", "u59", "u60",
    "u61", "u62", "u63", "u64", "u65", "u66", "u67", "u68", "u69", "u70",
    "u71", "u72", "u73", "u74", "u75", "u76", "u77", "u78", "u79", "u80",
    "u81", "u82", "u83", "u84", "u85", "u86", "u87", "u88", "u89", "u90",
    "u91", "u92", "u93", "u94", "u95", "u96", "u97", "u98", "u99", "u100",
    "u101", "u102", "u103", "u104", "u105", "u106", "u107", "u108",
    
    // Voltage statistics
    "CellMax", "CellMin", "umax", "umin", "deltaV", "udc", "uavg", "chargeVlim", "dischargeVlim",
    
    // Balance control
    "balance", "CellVmax", "CellVmin",
    
    // Temperature parameters
    "Chipt0", "Cellt0_0", "Cellt0_1", "TempMax", "TempMin",
    
    // Chip voltages
    "ChipV1", "ChipV2", "ChipV3", "ChipV4", "ChipV5", "ChipV6", "ChipV7", "ChipV8",
    
    // Chip supplies
    "Chip1_5V", "Chip2_5V",
    
    // Cell counts per chip
    "Chip1Cells", "Chip2Cells", "Chip3Cells", "Chip4Cells",
    
    // AS8510 Current Sensor parameters
    "current", "as8510_temp"
};

// Initialize default values
static void initParams() {
    intParams[Param::numbmbs] = 1;  // Set default number of BMBs to 1
    intParams[Param::balance] = 0;  // Balance disabled by default
    intParams[Param::LoopCnt] = 0;
    intParams[Param::LoopState] = 0;
    intParams[Param::CellsPresent] = 0;
    intParams[Param::CellsBalancing] = 0;
    
    // Initialize string parameters
    stringParams[Param::BalanceCellList] = "";
    
    // Initialize all cell voltages to 0
    for (int i = Param::u1; i <= Param::u108; i++) {
        intParams[static_cast<Param::PARAM_NUM>(i)] = 0;
    }
    
    // Initialize voltage statistics
    intParams[Param::CellMax] = 0;
    intParams[Param::CellMin] = 0;
    intParams[Param::umax] = 0;
    intParams[Param::umin] = 0;
    intParams[Param::deltaV] = 0;
    intParams[Param::udc] = 0;
    intParams[Param::uavg] = 0;
    intParams[Param::chargeVlim] = 0;
    intParams[Param::dischargeVlim] = 0;
    
    // Initialize balance control
    intParams[Param::CellVmax] = 0;
    intParams[Param::CellVmin] = 0;
    
    // Initialize temperature parameters
    intParams[Param::Chipt0] = 0;
    intParams[Param::Cellt0_0] = 0;
    intParams[Param::Cellt0_1] = 0;
    intParams[Param::TempMax] = 0;
    intParams[Param::TempMin] = 0;
    
    // Initialize chip voltages
    intParams[Param::ChipV1] = 0;
    intParams[Param::ChipV2] = 0;
    intParams[Param::ChipV3] = 0;
    intParams[Param::ChipV4] = 0;
    intParams[Param::ChipV5] = 0;
    intParams[Param::ChipV6] = 0;
    intParams[Param::ChipV7] = 0;
    intParams[Param::ChipV8] = 0;
    
    // Initialize chip supplies
    intParams[Param::Chip1_5V] = 0;
    intParams[Param::Chip2_5V] = 0;
    
    // Initialize cell counts per chip
    intParams[Param::Chip1Cells] = 0;
    intParams[Param::Chip2Cells] = 0;
    intParams[Param::Chip3Cells] = 0;
    intParams[Param::Chip4Cells] = 0;
    
    // Initialize AS8510 Current Sensor parameters
    floatParams[Param::current] = 0.0f;
    floatParams[Param::as8510_temp] = 0.0f;
}

int Param::GetInt(PARAM_NUM param) {
    return intParams[param];
}

void Param::SetInt(PARAM_NUM param, int value) {
    intParams[param] = value;
}

float Param::GetFloat(PARAM_NUM param) {
    return floatParams[param];
}

void Param::SetFloat(PARAM_NUM param, float value) {
    floatParams[param] = value;
}

String Param::GetString(PARAM_NUM param) {
    return stringParams[param];
}

void Param::SetString(PARAM_NUM param, const String& value) {
    stringParams[param] = value;
}

const char* Param::GetParamName(PARAM_NUM param) {
    if (param >= 0 && param < sizeof(paramNames) / sizeof(paramNames[0])) {
        return paramNames[param];
    }
    return "unknown";
}

Param::PARAM_NUM Param::GetParamFromName(const char* name) {
    for (int i = 0; i < sizeof(paramNames) / sizeof(paramNames[0]); i++) {
        if (strcmp(paramNames[i], name) == 0) {
            return static_cast<PARAM_NUM>(i);
        }
    }
    return static_cast<PARAM_NUM>(-1); // Invalid parameter
}

void Param::PrintAllParams() {
    Serial.println("\n=== All Parameters ===");
    for (int i = 0; i < sizeof(paramNames) / sizeof(paramNames[0]); i++) {
        PrintParam(static_cast<PARAM_NUM>(i));
    }
    Serial.println("=====================\n");
}

void Param::PrintParam(PARAM_NUM param) {
    const char* name = GetParamName(param);
    if (strcmp(name, "unknown") == 0) {
        Serial.printf("Parameter %d: INVALID\n", param);
        return;
    }
    
    // Check if parameter exists in stringParams
    if (stringParams.find(param) != stringParams.end()) {
        Serial.printf("%s=%s\n", name, stringParams[param].c_str());
    }
    // Check if parameter exists in intParams
    else if (intParams.find(param) != intParams.end()) {
        Serial.printf("%s=%d\n", name, intParams[param]);
    }
    // Check if parameter exists in floatParams
    else if (floatParams.find(param) != floatParams.end()) {
        Serial.printf("%s=%.3f\n", name, floatParams[param]);
    }
    else {
        Serial.printf("%s=<not set>\n", name);
    }
}

bool Param::SetParamFromString(const char* name, const char* value) {
    PARAM_NUM param = GetParamFromName(name);
    if (param == static_cast<PARAM_NUM>(-1)) {
        Serial.printf("Error: Unknown parameter '%s'\n", name);
        return false;
    }
    
    // Try to parse as integer first
    char* endptr;
    long intValue = strtol(value, &endptr, 10);
    if (*endptr == '\0') {
        // Successfully parsed as integer
        SetInt(param, intValue);
        Serial.printf("Set %s = %d\n", name, intValue);
        return true;
    }
    
    // Try to parse as float
    float floatValue = strtof(value, &endptr);
    if (*endptr == '\0') {
        // Successfully parsed as float
        SetFloat(param, floatValue);
        Serial.printf("Set %s = %.3f\n", name, floatValue);
        return true;
    }
    
    Serial.printf("Error: Could not parse value '%s' for parameter '%s'\n", value, name);
    return false;
}

void Param::PrintParamHelp() {
    Serial.println("\n=== Parameter API Help ===");
    Serial.println("Commands:");
    Serial.println("  param list                    - List all parameters");
    Serial.println("  param get <name>              - Get parameter value");
    Serial.println("  param set <name> <value>      - Set parameter value");
    Serial.println("  param help                    - Show this help");
    Serial.println("");
    Serial.println("Examples:");
    Serial.println("  param get balance             - Get balance status");
    Serial.println("  param set balance 1           - Enable balance");
    Serial.println("  param set numbmbs 2           - Set number of BMBs to 2");
    Serial.println("  param set u1 4200             - Set cell 1 voltage to 4200mV");
    Serial.println("  param set ChipV1 3.3          - Set chip 1 voltage to 3.3V");
    Serial.println("");
    Serial.println("Parameter Categories:");
    Serial.println("  System: numbmbs, LoopCnt, LoopState, CellsPresent, CellsBalancing");
    Serial.println("  Cell Voltages: u1-u108 (cell voltages in mV)");
    Serial.println("  Voltage Stats: CellMax, CellMin, umax, umin, deltaV, udc, uavg");
    Serial.println("  Balance: balance, CellVmax, CellVmin");
    Serial.println("  Temperature: Chipt0, Cellt0_0, Cellt0_1, TempMax, TempMin");
    Serial.println("  Chip Voltages: ChipV1-ChipV8");
    Serial.println("  Chip Supplies: Chip1_5V, Chip2_5V");
    Serial.println("  Cell Counts: Chip1Cells, Chip2Cells, Chip3Cells, Chip4Cells");
    Serial.println("");
    Serial.println("Common Parameters:");
    Serial.println("  balance     - Balance control (0=off, 1=on)");
    Serial.println("  numbmbs     - Number of BMB boards");
    Serial.println("  u1-u108     - Individual cell voltages in mV");
    Serial.println("  umax/umin   - Maximum/minimum cell voltages");
    Serial.println("  deltaV      - Voltage difference between max and min cells");
    Serial.println("  CellMax/CellMin - Cell numbers with max/min voltage");
    Serial.println("=======================\n");
}

// Overloaded methods for specific serial ports
void Param::PrintAllParams(HardwareSerial& serialPort) {
    serialPort.println("\n=== All Parameters ===");
    for (int i = 0; i < sizeof(paramNames) / sizeof(paramNames[0]); i++) {
        PrintParam(static_cast<PARAM_NUM>(i), serialPort);
    }
    serialPort.println("=====================\n");
}

void Param::PrintParam(PARAM_NUM param, HardwareSerial& serialPort) {
    const char* name = GetParamName(param);
    if (strcmp(name, "unknown") == 0) {
        serialPort.printf("Parameter %d: INVALID\n", param);
        return;
    }
    
    // Check if parameter exists in stringParams
    if (stringParams.find(param) != stringParams.end()) {
        serialPort.printf("%s=%s\n", name, stringParams[param].c_str());
    }
    // Check if parameter exists in intParams
    else if (intParams.find(param) != intParams.end()) {
        serialPort.printf("%s=%d\n", name, intParams[param]);
    }
    // Check if parameter exists in floatParams
    else if (floatParams.find(param) != floatParams.end()) {
        serialPort.printf("%s=%.3f\n", name, floatParams[param]);
    }
    else {
        serialPort.printf("%s=<not set>\n", name);
    }
}

bool Param::SetParamFromString(const char* name, const char* value, HardwareSerial& serialPort) {
    PARAM_NUM param = GetParamFromName(name);
    if (param == static_cast<PARAM_NUM>(-1)) {
        serialPort.printf("Error: Unknown parameter '%s'\n", name);
        return false;
    }
    
    // Try to parse as integer first
    char* endptr;
    long intValue = strtol(value, &endptr, 10);
    if (*endptr == '\0') {
        // Successfully parsed as integer
        SetInt(param, intValue);
        serialPort.printf("Set %s = %d\n", name, intValue);
        return true;
    }
    
    // Try to parse as float
    float floatValue = strtof(value, &endptr);
    if (*endptr == '\0') {
        // Successfully parsed as float
        SetFloat(param, floatValue);
        serialPort.printf("Set %s = %.3f\n", name, floatValue);
        return true;
    }
    
    serialPort.printf("Error: Could not parse value '%s' for parameter '%s'\n", value, name);
    return false;
}

void Param::PrintParamHelp(HardwareSerial& serialPort) {
    serialPort.println("\n=== Parameter API Help ===");
    serialPort.println("Commands:");
    serialPort.println("  param list                    - List all parameters");
    serialPort.println("  param get <name>              - Get parameter value");
    serialPort.println("  param set <name> <value>      - Set parameter value");
    serialPort.println("  param help                    - Show this help");
    serialPort.println("");
    serialPort.println("Examples:");
    serialPort.println("  param get balance             - Get balance status");
    serialPort.println("  param set balance 1           - Enable balance");
    serialPort.println("  param set numbmbs 2           - Set number of BMBs to 2");
    serialPort.println("  param set u1 4200             - Set cell 1 voltage to 4200mV");
    serialPort.println("  param set ChipV1 3.3          - Set chip 1 voltage to 3.3V");
    serialPort.println("");
    serialPort.println("Parameter Categories:");
    serialPort.println("  System: numbmbs, LoopCnt, LoopState, CellsPresent, CellsBalancing");
    serialPort.println("  Cell Voltages: u1-u108 (cell voltages in mV)");
    serialPort.println("  Voltage Stats: CellMax, CellMin, umax, umin, deltaV, udc, uavg");
    serialPort.println("  Balance: balance, CellVmax, CellVmin");
    serialPort.println("  Temperature: Chipt0, Cellt0_0, Cellt0_1, TempMax, TempMin");
    serialPort.println("  Chip Voltages: ChipV1-ChipV8");
    serialPort.println("  Chip Supplies: Chip1_5V, Chip2_5V");
    serialPort.println("  Cell Counts: Chip1Cells, Chip2Cells, Chip3Cells, Chip4Cells");
    serialPort.println("");
    serialPort.println("Common Parameters:");
    serialPort.println("  balance     - Balance control (0=off, 1=on)");
    serialPort.println("  numbmbs     - Number of BMB boards");
    serialPort.println("  u1-u108     - Individual cell voltages in mV");
    serialPort.println("  umax/umin   - Maximum/minimum cell voltages");
    serialPort.println("  deltaV      - Voltage difference between max and min cells");
    serialPort.println("  CellMax/CellMin - Cell numbers with max/min voltage");
    serialPort.println("=======================\n");
}

// Call initialization when the program starts
static struct ParamInitializer {
    ParamInitializer() {
        initParams();
    }
} paramInitializer; 