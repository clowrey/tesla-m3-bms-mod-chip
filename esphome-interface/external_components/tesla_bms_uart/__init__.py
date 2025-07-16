import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ['uart']
CODEOWNERS = ['@tesla-bms']

tesla_bms_uart_ns = cg.esphome_ns.namespace('tesla_bms_uart')
TeslaBmsUartComponent = tesla_bms_uart_ns.class_('TeslaBmsUartComponent', cg.Component, uart.UARTDevice)

CONF_UART_ID = 'uart_id'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(TeslaBmsUartComponent),
    cv.Required(CONF_UART_ID): cv.use_id(uart.UARTComponent),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    uart_component = await cg.get_variable(config[CONF_UART_ID])
    cg.add(var.set_uart_parent(uart_component)) 