#include "Param.h"
#include <map>

// Storage for parameters
static std::map<Param::PARAM_NUM, int> intParams;
static std::map<Param::PARAM_NUM, float> floatParams;

// Initialize default values
static void initParams() {
    intParams[Param::numbmbs] = 1;  // Set default number of BMBs to 1
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

// Call initialization when the program starts
static struct ParamInitializer {
    ParamInitializer() {
        initParams();
    }
} paramInitializer; 