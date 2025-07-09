#include "tesla_bms_uart.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace tesla_bms_uart {

static const char *const TAG = "tesla_bms_uart";

TeslaBmsUartComponent::TeslaBmsUartComponent(uart::UARTComponent *parent) : uart::UARTDevice(parent) {}

void TeslaBmsUartComponent::setup() {
  ESP_LOGI(TAG, "Tesla BMS UART component setup complete");
  ESP_LOGI(TAG, "Registered %d sensors", sensors_.size());
}

void TeslaBmsUartComponent::loop() {
  while (available()) {
    char c = read();
    if (c == '\n' || c == '\r') {
      if (!rx_buffer_.empty()) {
        ESP_LOGD(TAG, "Received line: %s", rx_buffer_.c_str());
        parse_line(rx_buffer_);
        rx_buffer_.clear();
      }
    } else {
      rx_buffer_ += c;
    }
  }
}

void TeslaBmsUartComponent::register_sensor(const std::string &param, sensor::Sensor *sensor) {
  sensors_[param] = sensor;
  ESP_LOGI(TAG, "Registered sensor for parameter: %s", param.c_str());
}

void TeslaBmsUartComponent::parse_line(const std::string &line) {
  ESP_LOGI(TAG, "DEBUG: Parsing line: '%s' (length: %d)", line.c_str(), line.length());
  
  size_t pos = line.find('=');
  if (pos == std::string::npos) {
    ESP_LOGW(TAG, "DEBUG: No '=' found in line: '%s'", line.c_str());
    return;
  }
  
  std::string param = line.substr(0, pos);
  std::string value_str = line.substr(pos + 1);
  
  ESP_LOGI(TAG, "DEBUG: Raw param='%s', value_str='%s'", param.c_str(), value_str.c_str());
  
  // Trim whitespace
  param.erase(0, param.find_first_not_of(" \t\r\n"));
  param.erase(param.find_last_not_of(" \t\r\n") + 1);
  value_str.erase(0, value_str.find_first_not_of(" \t\r\n"));
  value_str.erase(value_str.find_last_not_of(" \t\r\n") + 1);
  
  ESP_LOGI(TAG, "DEBUG: Trimmed param='%s', value_str='%s'", param.c_str(), value_str.c_str());
  
  float value = atof(value_str.c_str());
  ESP_LOGI(TAG, "DEBUG: Converted value=%f", value);
  
  auto it = sensors_.find(param);
  if (it != sensors_.end() && it->second != nullptr) {
    ESP_LOGI(TAG, "DEBUG: Found sensor for '%s', publishing value %f", param.c_str(), value);
    it->second->publish_state(value);
    ESP_LOGI(TAG, "DEBUG: Successfully published %s = %f", param.c_str(), value);
  } else {
    ESP_LOGW(TAG, "DEBUG: Unknown parameter: '%s' (value: '%s') - Sensor not found", param.c_str(), value_str.c_str());
    ESP_LOGI(TAG, "DEBUG: Available sensors: %d", sensors_.size());
    for (const auto& sensor : sensors_) {
      ESP_LOGI(TAG, "DEBUG: Registered sensor: '%s'", sensor.first.c_str());
    }
  }
}

}  // namespace tesla_bms_uart
}  // namespace esphome 