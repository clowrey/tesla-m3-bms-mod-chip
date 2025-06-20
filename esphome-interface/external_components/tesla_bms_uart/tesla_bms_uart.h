#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include <map>
#include <string>

namespace esphome {
namespace tesla_bms_uart {

class TeslaBmsUartComponent : public Component, public uart::UARTDevice {
 public:
  explicit TeslaBmsUartComponent(uart::UARTComponent *parent);
  void loop() override;
  void setup() override;

  // Register a sensor for a given parameter name
  void register_sensor(const std::string &param, sensor::Sensor *sensor);
  
  // Set the UART component
  void set_uart(uart::UARTComponent *uart) { this->parent_ = uart; }

 protected:
  std::string rx_buffer_;
  std::map<std::string, sensor::Sensor *> sensors_;
  void parse_line(const std::string &line);
};

}  // namespace tesla_bms_uart
}  // namespace esphome 