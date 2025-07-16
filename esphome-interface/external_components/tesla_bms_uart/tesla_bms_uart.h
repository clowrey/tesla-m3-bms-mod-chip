#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <map>
#include <string>
#include <functional>

namespace esphome {
namespace tesla_bms_uart {

class TeslaBmsUartComponent : public Component, public uart::UARTDevice {
 public:
  TeslaBmsUartComponent() = default;
  void loop() override;
  void setup() override;

  // Register a sensor for a given parameter name
  void register_sensor(const std::string &param, sensor::Sensor *sensor);
  
  // Register a text sensor for string parameters
  void register_text_sensor(const std::string &param, text_sensor::TextSensor *sensor);
  
  // Register a sensor with unit conversion (mV to V)
  void register_voltage_sensor(const std::string &param, sensor::Sensor *sensor);
  
  // Register callback for DATA_COMPLETE events
  void set_data_complete_callback(std::function<void()> callback);
  
  // Set the UART parent (called by ESPHome setup)
  void set_uart_parent(uart::UARTComponent *parent) { this->parent_ = parent; }

 protected:
  std::string rx_buffer_;
  std::map<std::string, sensor::Sensor *> sensors_;
  std::map<std::string, sensor::Sensor *> voltage_sensors_;  // Sensors that need mV->V conversion
  std::map<std::string, text_sensor::TextSensor *> text_sensors_;
  std::function<void()> data_complete_callback_;
  
  void parse_line(const std::string &line);
  void handle_cell_voltage(const std::string &param, float value_mv);
};

}  // namespace tesla_bms_uart
}  // namespace esphome 