#include "tesla_bms_parser.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include <algorithm>
#include <sstream>

namespace esphome {
namespace tesla_bms_parser {

static const char *const TAG = "tesla_bms_parser";

// Parameter definitions based on the Param.h file
const std::vector<ParameterInfo> PARAMETER_DEFINITIONS = {
  // System parameters
  {"numbmbs", ParameterCategory::SYSTEM, "boards", 0, 0, 10},
  {"LoopCnt", ParameterCategory::SYSTEM, "count", 0, 0, 999999},
  {"LoopState", ParameterCategory::SYSTEM, "state", 0, 0, 255},
  {"CellsPresent", ParameterCategory::SYSTEM, "cells", 0, 0, 108},
  {"CellsBalancing", ParameterCategory::SYSTEM, "cells", 0, 0, 108},
  
  // Voltage statistics
  {"CellMax", ParameterCategory::VOLTAGE, "cell", 0, 1, 108},
  {"CellMin", ParameterCategory::VOLTAGE, "cell", 0, 1, 108},
  {"umax", ParameterCategory::VOLTAGE, "mV", 0, 2000, 5000},
  {"umin", ParameterCategory::VOLTAGE, "mV", 0, 2000, 5000},
  {"deltaV", ParameterCategory::VOLTAGE, "mV", 0, 0, 1000},
  {"udc", ParameterCategory::VOLTAGE, "mV", 0, 0, 500000},
  {"uavg", ParameterCategory::VOLTAGE, "mV", 0, 2000, 5000},
  {"chargeVlim", ParameterCategory::VOLTAGE, "mV", 0, 3000, 5000},
  {"dischargeVlim", ParameterCategory::VOLTAGE, "mV", 0, 2000, 4000},
  
  // Balance control
  {"balance", ParameterCategory::BALANCE, "status", 0, 0, 1},
  {"CellVmax", ParameterCategory::VOLTAGE, "mV", 0, 3000, 5000},
  {"CellVmin", ParameterCategory::VOLTAGE, "mV", 0, 2000, 4000},
  
  // Temperature parameters
  {"Chipt0", ParameterCategory::TEMPERATURE, "°C", 1, -40, 125},
  {"Cellt0_0", ParameterCategory::TEMPERATURE, "°C", 1, -40, 125},
  {"Cellt0_1", ParameterCategory::TEMPERATURE, "°C", 1, -40, 125},
  {"TempMax", ParameterCategory::TEMPERATURE, "°C", 1, -40, 125},
  {"TempMin", ParameterCategory::TEMPERATURE, "°C", 1, -40, 125},
  
  // Chip voltages
  {"ChipV1", ParameterCategory::CHIP, "V", 2, 0, 5},
  {"ChipV2", ParameterCategory::CHIP, "V", 2, 0, 5},
  {"ChipV3", ParameterCategory::CHIP, "V", 2, 0, 5},
  {"ChipV4", ParameterCategory::CHIP, "V", 2, 0, 5},
  {"ChipV5", ParameterCategory::CHIP, "V", 2, 0, 5},
  {"ChipV6", ParameterCategory::CHIP, "V", 2, 0, 5},
  {"ChipV7", ParameterCategory::CHIP, "V", 2, 0, 5},
  {"ChipV8", ParameterCategory::CHIP, "V", 2, 0, 5},
  
  // Chip supplies
  {"Chip1_5V", ParameterCategory::CHIP, "V", 2, 0, 6},
  {"Chip2_5V", ParameterCategory::CHIP, "V", 2, 0, 6},
  
  // Cell counts per chip
  {"Chip1Cells", ParameterCategory::SYSTEM, "cells", 0, 0, 30},
  {"Chip2Cells", ParameterCategory::SYSTEM, "cells", 0, 0, 30},
  {"Chip3Cells", ParameterCategory::SYSTEM, "cells", 0, 0, 30},
  {"Chip4Cells", ParameterCategory::SYSTEM, "cells", 0, 0, 30}
};

// Add cell voltage parameters (u1-u108)
void add_cell_voltage_parameters() {
  for (int i = 1; i <= 108; i++) {
    std::string param_name = "u" + std::to_string(i);
    PARAMETER_DEFINITIONS.push_back({param_name, ParameterCategory::VOLTAGE, "mV", 0, 2000, 5000});
  }
}

void TeslaBMSParser::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Tesla BMS Parser");
  
  // Initialize parameter values
  for (const auto &param : PARAMETER_DEFINITIONS) {
    parameter_values_[param.name] = 0.0f;
  }
  
  // Add cell voltage parameters
  add_cell_voltage_parameters();
  
  ESP_LOGCONFIG(TAG, "Tesla BMS Parser setup complete");
}

void TeslaBMSParser::loop() {
  uint32_t now = millis();
  
  // Read UART data
  while (this->available()) {
    uint8_t c;
    if (this->read_byte(&c)) {
      if (c == '\n' || c == '\r') {
        if (!uart_buffer_.empty()) {
          parse_parameter_line(uart_buffer_);
          uart_buffer_.clear();
        }
      } else {
        uart_buffer_ += (char) c;
      }
    }
  }
  
  // Periodic parameter requests
  if (now - last_request_time_ > request_interval_) {
    if (!waiting_for_response_) {
      request_all_parameters();
      last_request_time_ = now;
      waiting_for_response_ = true;
      response_timeout_ = now + RESPONSE_TIMEOUT_MS;
    }
  }
  
  // Check for response timeout
  if (waiting_for_response_ && now > response_timeout_) {
    ESP_LOGW(TAG, "Response timeout, retrying...");
    waiting_for_response_ = false;
  }
}

void TeslaBMSParser::dump_config() {
  ESP_LOGCONFIG(TAG, "Tesla BMS Parser:");
  ESP_LOGCONFIG(TAG, "  Request Interval: %d ms", request_interval_);
  ESP_LOGCONFIG(TAG, "  Registered Sensors: %d", registered_sensors_.size());
  ESP_LOGCONFIG(TAG, "  Registered Switches: %d", registered_switches_.size());
}

void TeslaBMSParser::register_system_sensor(sensor::Sensor *sensor, const std::string &param_name) {
  registered_sensors_[param_name] = sensor;
  ESP_LOGCONFIG(TAG, "Registered system sensor: %s", param_name.c_str());
}

void TeslaBMSParser::register_voltage_sensor(sensor::Sensor *sensor, const std::string &param_name) {
  registered_sensors_[param_name] = sensor;
  ESP_LOGCONFIG(TAG, "Registered voltage sensor: %s", param_name.c_str());
}

void TeslaBMSParser::register_temperature_sensor(sensor::Sensor *sensor, const std::string &param_name) {
  registered_sensors_[param_name] = sensor;
  ESP_LOGCONFIG(TAG, "Registered temperature sensor: %s", param_name.c_str());
}

void TeslaBMSParser::register_chip_sensor(sensor::Sensor *sensor, const std::string &param_name) {
  registered_sensors_[param_name] = sensor;
  ESP_LOGCONFIG(TAG, "Registered chip sensor: %s", param_name.c_str());
}

void TeslaBMSParser::register_balance_switch(switch_::Switch *switch_obj) {
  registered_switches_["balance"] = switch_obj;
  ESP_LOGCONFIG(TAG, "Registered balance switch");
}

void TeslaBMSParser::send_command(const std::string &command) {
  ESP_LOGD(TAG, "Sending command: %s", command.c_str());
  this->write_str(command + "\n");
}

void TeslaBMSParser::request_all_parameters() {
  send_command("param list");
}

void TeslaBMSParser::request_parameter(const std::string &param_name) {
  send_command("param get " + param_name);
}

void TeslaBMSParser::parse_parameter_line(const std::string &line) {
  ESP_LOGD(TAG, "Parsing line: %s", line.c_str());
  
  // Check if this is a parameter list response
  if (line.find("param list") != std::string::npos || line.find("Parameter List:") != std::string::npos) {
    waiting_for_response_ = false;
    return;
  }
  
  // Check if this is a single parameter response
  if (line.find("param get") != std::string::npos) {
    waiting_for_response_ = false;
    return;
  }
  
  // Parse parameter line format: "param_name: value"
  size_t colon_pos = line.find(':');
  if (colon_pos != std::string::npos) {
    std::string param_name = line.substr(0, colon_pos);
    std::string value_str = line.substr(colon_pos + 1);
    
    // Trim whitespace
    param_name.erase(0, param_name.find_first_not_of(" \t"));
    param_name.erase(param_name.find_last_not_of(" \t") + 1);
    value_str.erase(0, value_str.find_first_not_of(" \t"));
    value_str.erase(value_str.find_last_not_of(" \t") + 1);
    
    // Convert value to float
    try {
      float value = std::stof(value_str);
      update_sensor_value(param_name, value);
    } catch (const std::exception &e) {
      ESP_LOGW(TAG, "Failed to parse value '%s' for parameter '%s': %s", 
               value_str.c_str(), param_name.c_str(), e.what());
    }
  }
  
  // Check for balance status responses
  if (line.find("Balance ENABLED") != std::string::npos) {
    update_sensor_value("balance", 1.0f);
  } else if (line.find("Balance DISABLED") != std::string::npos) {
    update_sensor_value("balance", 0.0f);
  }
}

void TeslaBMSParser::update_sensor_value(const std::string &param_name, float value) {
  // Store the value
  parameter_values_[param_name] = value;
  
  // Update registered sensor
  auto sensor_it = registered_sensors_.find(param_name);
  if (sensor_it != registered_sensors_.end()) {
    sensor_it->second->publish_state(value);
  }
  
  // Update registered switch (for balance control)
  if (param_name == "balance") {
    auto switch_it = registered_switches_.find("balance");
    if (switch_it != registered_switches_.end()) {
      if (value > 0.5f) {
        switch_it->second->publish_state(true);
      } else {
        switch_it->second->publish_state(false);
      }
    }
  }
  
  // Call callback
  this->parameter_update_callback_.call(param_name, value);
  
  ESP_LOGD(TAG, "Updated parameter %s = %.3f", param_name.c_str(), value);
}

}  // namespace tesla_bms_parser
}  // namespace esphome 