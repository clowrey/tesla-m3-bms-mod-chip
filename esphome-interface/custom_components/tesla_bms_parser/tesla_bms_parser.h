#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/automation.h"
#include <string>
#include <map>
#include <vector>

namespace esphome {
namespace tesla_bms_parser {

class TeslaBMSParser : public Component, public uart::UARTDevice {
 public:
  TeslaBMSParser(uart::UARTComponent *parent) : uart::UARTDevice(parent) {}
  
  void setup() override;
  void loop() override;
  void dump_config() override;
  
  // Parameter registration methods
  void register_system_sensor(sensor::Sensor *sensor, const std::string &param_name);
  void register_voltage_sensor(sensor::Sensor *sensor, const std::string &param_name);
  void register_temperature_sensor(sensor::Sensor *sensor, const std::string &param_name);
  void register_chip_sensor(sensor::Sensor *sensor, const std::string &param_name);
  void register_balance_switch(switch_::Switch *switch_obj);
  
  // Command methods
  void send_command(const std::string &command);
  void request_all_parameters();
  void request_parameter(const std::string &param_name);
  
  // Callback for parameter updates
  void add_on_parameter_update_callback(std::function<void(const std::string &, float)> &&callback) {
    this->parameter_update_callback_.add(std::move(callback));
  }

 protected:
  void parse_parameter_line(const std::string &line);
  void parse_param_list_response(const std::string &data);
  void parse_single_param_response(const std::string &data);
  void update_sensor_value(const std::string &param_name, float value);
  
  // Parameter storage
  std::map<std::string, float> parameter_values_;
  std::map<std::string, sensor::Sensor*> registered_sensors_;
  std::map<std::string, switch_::Switch*> registered_switches_;
  
  // UART buffer
  std::string uart_buffer_;
  
  // Timing
  uint32_t last_request_time_ = 0;
  uint32_t request_interval_ = 5000; // 5 seconds
  
  // Callbacks
  CallbackManager<void(const std::string &, float)> parameter_update_callback_;
  
  // State
  bool waiting_for_response_ = false;
  uint32_t response_timeout_ = 0;
  static const uint32_t RESPONSE_TIMEOUT_MS = 3000;
};

// Parameter categories for easier management
enum class ParameterCategory {
  SYSTEM,
  VOLTAGE,
  TEMPERATURE,
  CHIP,
  BALANCE
};

struct ParameterInfo {
  std::string name;
  ParameterCategory category;
  std::string unit;
  int decimals;
  float min_value;
  float max_value;
};

// Global parameter definitions
extern const std::vector<ParameterInfo> PARAMETER_DEFINITIONS;

}  // namespace tesla_bms_parser
}  // namespace esphome 