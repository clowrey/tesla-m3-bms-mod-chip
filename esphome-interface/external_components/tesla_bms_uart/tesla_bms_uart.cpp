#include "tesla_bms_uart.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include <cmath>

namespace esphome {
namespace tesla_bms_uart {

static const char *const TAG = "tesla_bms_uart";

void TeslaBmsUartComponent::setup() {
  ESP_LOGI(TAG, "Tesla BMS UART component setup complete - %d sensors, %d voltage sensors, %d text sensors registered", 
           sensors_.size(), voltage_sensors_.size(), text_sensors_.size());
}

void TeslaBmsUartComponent::loop() {
  while (available()) {
    char c = read();
    if (c == '\n' || c == '\r') {
      if (!rx_buffer_.empty()) {
        parse_line(rx_buffer_);
        rx_buffer_.clear();
      }
    } else {
      if (rx_buffer_.length() < 200) {
        rx_buffer_ += c;
      } else {
        rx_buffer_.clear();  // Prevent buffer overflow
      }
    }
  }
}

void TeslaBmsUartComponent::register_sensor(const std::string &param, sensor::Sensor *sensor) {
  sensors_[param] = sensor;
}

void TeslaBmsUartComponent::register_text_sensor(const std::string &param, text_sensor::TextSensor *sensor) {
  text_sensors_[param] = sensor;
}

void TeslaBmsUartComponent::register_voltage_sensor(const std::string &param, sensor::Sensor *sensor) {
  voltage_sensors_[param] = sensor;
}

void TeslaBmsUartComponent::set_data_complete_callback(std::function<void()> callback) {
  data_complete_callback_ = callback;
}

void TeslaBmsUartComponent::parse_line(const std::string &line) {
  // Handle DATA_COMPLETE signal
  if (line == "DATA_COMPLETE") {
    if (data_complete_callback_) {
      data_complete_callback_();
    }
    return;
  }
  
  // Skip comments
  if (line.find("#") != std::string::npos) {
    return;
  }
  
  // Parse parameter=value lines
  size_t pos = line.find('=');
  if (pos == std::string::npos) {
    return;
  }
  
  std::string param = line.substr(0, pos);
  std::string value_str = line.substr(pos + 1);
  
  // Trim whitespace
  param.erase(0, param.find_first_not_of(" \t\r\n"));
  param.erase(param.find_last_not_of(" \t\r\n") + 1);
  value_str.erase(0, value_str.find_first_not_of(" \t\r\n"));
  value_str.erase(value_str.find_last_not_of(" \t\r\n") + 1);
  
  if (param.empty()) {
    return;
  }
  
  // Handle text parameters first
  auto text_it = text_sensors_.find(param);
  if (text_it != text_sensors_.end() && text_it->second != nullptr) {
    text_it->second->publish_state(value_str);
    return;
  }
  
  // Handle numeric parameters
  bool is_nan = (value_str == "nan" || value_str == "NaN" || value_str == "NAN");
  bool is_numeric = (!value_str.empty() && (isdigit(value_str[0]) || value_str[0] == '.' || value_str[0] == '-'));
  
  if (is_numeric || is_nan) {
    float value = is_nan ? NAN : atof(value_str.c_str());
    
    // Check for voltage sensors (mV to V conversion)
    auto voltage_it = voltage_sensors_.find(param);
    if (voltage_it != voltage_sensors_.end() && voltage_it->second != nullptr) {
      voltage_it->second->publish_state(value / 1000.0);
      return;
    }
    
    // Check for regular sensors
    auto sensor_it = sensors_.find(param);
    if (sensor_it != sensors_.end() && sensor_it->second != nullptr) {
      sensor_it->second->publish_state(value);
      return;
    }
    
    // Handle individual cell voltages (u1-u108 or 1-108)
    handle_cell_voltage(param, value);
  }
}

void TeslaBmsUartComponent::handle_cell_voltage(const std::string &param, float value_mv) {
  int cell_num = 0;
  
  // Parse cell number from parameter name
  if ((param.substr(0,1) == "u" && param.length() >= 2) || 
      (isdigit(param[0]) && param.length() >= 1 && param.length() <= 3)) {
    
    if (param.substr(0,1) == "u") {
      cell_num = atoi(param.substr(1).c_str());
    } else {
      cell_num = atoi(param.c_str());
    }
    
    if (!std::isnan(value_mv) && value_mv >= 10.0 && cell_num >= 1 && cell_num <= 108) {
      // Look for registered cell voltage sensor
      char cell_param[16];
      snprintf(cell_param, sizeof(cell_param), "u%03d", cell_num);
      
      auto cell_it = voltage_sensors_.find(cell_param);
      if (cell_it != voltage_sensors_.end() && cell_it->second != nullptr) {
        cell_it->second->publish_state(value_mv / 1000.0);  // Convert mV to V
      }
    }
  }
}

}  // namespace tesla_bms_uart
}  // namespace esphome 