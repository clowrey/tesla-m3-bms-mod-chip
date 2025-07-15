# STM32F411RE Conversion Plan
# Tesla Model 3 BMS Mod Chip Port from ESP32

## Overview
This document outlines the complete conversion plan for porting the Tesla BMS mod chip from ESP32 to STM32F411RE Nucleo board using CN7 and CN10 morpho connectors. The conversion includes switching from external ADS1115 ADC to internal STM32 ADC.

## Hardware Specifications

### STM32F411RE Key Specs
- ARM Cortex-M4 @ 84MHz (vs ESP32 @ 240MHz)
- 128KB RAM, 512KB Flash
- 3x SPI, 3x I2C, 3x UART/USART
- 12-bit ADC with 16 channels
- Multiple timer units for PWM
- Morpho connectors: CN7 (left), CN10 (right)

## Pin Mapping Conversion

### Current ESP32 → STM32F411RE Mapping

| Function | ESP32 Pin | STM32F411RE Pin | Connector | Notes |
|----------|-----------|-----------------|-----------|--------|
| **Tesla BMS SPI** | | | | |
| BMS_SCK | GPIO 2 | PA5 (SPI1_SCK) | CN7-30 | Primary SPI |
| BMS_MISO | GPIO 15 | PA6 (SPI1_MISO) | CN7-28 | Primary SPI |
| BMS_MOSI | GPIO 17 | PA7 (SPI1_MOSI) | CN7-32 | Primary SPI |
| BMS_CS | GPIO 22 | PB6 | CN7-34 | GPIO |
| **AS8510 Current Sensor** | | | | |
| AS8510_SCK | GPIO 25 | PB13 (SPI2_SCK) | CN10-26 | Secondary SPI |
| AS8510_MISO | GPIO 27 | PB14 (SPI2_MISO) | CN10-28 | Secondary SPI |
| AS8510_MOSI | GPIO 26 | PB15 (SPI2_MOSI) | CN10-30 | Secondary SPI |
| AS8510_CS | GPIO 14 | PC6 | CN10-32 | GPIO |
| **ADS1115 I2C → Internal ADC** | | | | |
| ~~ADS1115_SDA~~ | ~~GPIO 32~~ | **PA0 (ADC1_IN0)** | **CN7-28** | **Batt-Neg** |
| ~~ADS1115_SCL~~ | ~~GPIO 33~~ | **PA1 (ADC1_IN1)** | **CN7-30** | **Batt-Pos** |
| | | **PA4 (ADC1_IN4)** | **CN7-32** | **Link-Pos** |
| | | **PB0 (ADC1_IN8)** | **CN7-34** | **Link-Neg** |
| | | **PC0 (ADC1_IN10)** | **CN10-38** | **1.5V Reference** |
| **Serial Interfaces** | | | | |
| Serial2_RX | GPIO 22 | PA3 (USART2_RX) | CN10-37 | ESPHome |
| Serial2_TX | GPIO 23 | PA2 (USART2_TX) | CN10-35 | ESPHome |
| Serial (Debug) | USB | USB | USB | Virtual COM |
| **PWM Outputs** | | | | |
| Pack Contactors | GPIO 4 | PC7 (TIM3_CH2) | CN7-38 | 20kHz PWM |
| Pre-charge Relay | GPIO 21 | PC8 (TIM3_CH3) | CN7-36 | 20kHz PWM |

## ADC Conversion: ADS1115 → Internal ADC

### Current ADS1115 Implementation
```cpp
// External ADC with I2C interface
Adafruit_ADS1115 ads;
int16_t adc0 = ads.readADC_Differential_0_3();
float battContactorNeg = ads.computeVolts(adc0) * VOLTAGE_SCALE_FACTOR;
```

### New Internal ADC Implementation
```cpp
// Internal 12-bit ADC with DMA
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

// Multi-channel ADC configuration
uint32_t adcBuffer[5];  // Batt-Neg, Batt-Pos, Link-Pos, Link-Neg, 1.5V Reference
```

### ADC Channel Mapping
| Signal | Old ADS1115 | New STM32 ADC | Pin | Connector |
|--------|-------------|---------------|-----|-----------|
| Batt-Neg | Channel 0 | ADC1_IN0 | PA0 | CN7-28 |
| Batt-Pos | Channel 1 | ADC1_IN1 | PA1 | CN7-30 |
| Link-Pos | Channel 2 | ADC1_IN4 | PA4 | CN7-32 |
| Link-Neg | Channel 3 | ADC1_IN8 | PB0 | CN7-34 |
| **1.5V Reference** | **N/A** | **ADC1_IN10** | **PC0** | **CN10-38** |

### ADC Configuration Requirements
- **Resolution**: 12-bit (4096 levels vs ADS1115's 16-bit)
- **Reference**: 3.3V internal reference
- **Channels**: 5 channels (4 voltage + 1 reference)
- **Sampling**: Continuous with DMA
- **Conversion Time**: ~1µs per channel @ 84MHz
- **Measurement Method**: Differential (signal - 1.5V reference)
- **Scaling**: 25V input = 0.067V ADC (same 373.13 scale factor)

### 1.5V Reference Channel Strategy
The original system uses a 1.5V center reference for all voltage measurements. By dedicating one ADC channel to continuously monitor this reference voltage, we achieve several benefits:

**Benefits of Reference Channel:**
1. **Drift Compensation**: Automatically compensates for reference voltage drift
2. **Temperature Stability**: Corrects for temperature-related variations
3. **Supply Voltage Independence**: Reduces impact of VCC fluctuations
4. **Improved Accuracy**: Maintains precision even with component aging
5. **True Differential Measurement**: Matches original ADS1115 differential operation

**Implementation Details:**
- **Reference Input**: PC0 (ADC1_IN10) continuously samples 1.5V reference
- **Hardware Connection**: Connect 1.5V reference point to PC0 (CN10-38)
- **Differential Calculation**: Each voltage = (Signal_ADC - Reference_ADC) × Scale_Factor
- **Automatic Calibration**: Reference variations are automatically subtracted
- **Real-time Correction**: Every measurement is referenced to current 1.5V value

**Hardware Requirements:**
- **1.5V Reference Connection**: Route the system's 1.5V reference to PC0 (CN10-38)
- **Input Protection**: Consider adding a voltage follower or buffer if needed
- **Noise Filtering**: Add appropriate filtering on reference line for stable readings

## Software Framework Options

### Option 1: STM32CubeIDE + HAL (Recommended)
- **Pros**: Full STM32 ecosystem, debugging tools, code generation
- **Cons**: Departure from Arduino-style code
- **Migration Effort**: Medium-High

### Option 2: PlatformIO + STM32duino
- **Pros**: Minimal code changes, familiar Arduino API
- **Cons**: Less optimized, potential library limitations
- **Migration Effort**: Low-Medium

### Option 3: Native STM32 HAL
- **Pros**: Maximum performance and control
- **Cons**: Significant code rewrite required
- **Migration Effort**: High

## Step-by-Step Migration Plan

### Phase 1: Project Setup and Pin Mapping
1. **Create STM32CubeIDE project**
   - Target: STM32F411RE
   - Generate initialization code for all peripherals
   
2. **Define pin mapping header**
   ```cpp
   // stm32_pin_map.h
   #define TESLA_BMS_CS_PIN     GPIO_PIN_6   // PB6
   #define TESLA_BMS_SPI        SPI1
   #define AS8510_CS_PIN        GPIO_PIN_6   // PC6
   #define AS8510_SPI           SPI2
   #define PACK_CONTACTORS_PWM  TIM3_CHANNEL_2
   #define PRECHARGE_RELAY_PWM  TIM3_CHANNEL_3
   ```

### Phase 2: Interface Conversions

#### 2.1 SPI Interface Conversion
```cpp
// Replace ESP32 SPI calls
// OLD: SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
// NEW: 
HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, txData, size, timeout);
```

#### 2.2 UART Interface Conversion
```cpp
// Replace Serial/Serial2 calls
// OLD: Serial2.println("data");
// NEW: 
HAL_UART_Transmit(&huart2, (uint8_t*)data, strlen(data), HAL_MAX_DELAY);
```

#### 2.3 PWM Interface Conversion
```cpp
// Replace ESP32 LEDC calls
// OLD: ledcWrite(PACK_CONTACTORS_PWM_PIN, pwmValue);
// NEW:
__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwmValue);
```

### Phase 3: ADC Implementation

#### 3.1 ADC Configuration
```cpp
// ADC1 configuration for 5-channel continuous conversion
ADC_HandleTypeDef hadc1;
ADC_ChannelConfTypeDef sConfig = {0};

// Configure ADC1
hadc1.Instance = ADC1;
hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
hadc1.Init.Resolution = ADC_RESOLUTION_12B;
hadc1.Init.ScanConvMode = ENABLE;
hadc1.Init.ContinuousConvMode = ENABLE;
hadc1.Init.DiscontinuousConvMode = DISABLE;
hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
hadc1.Init.NbrOfConversion = 5;
hadc1.Init.DMAContinuousRequests = ENABLE;

// Configure individual channels
// Channel 0: PA0 (Batt-Neg)
sConfig.Channel = ADC_CHANNEL_0;
sConfig.Rank = 1;
sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
HAL_ADC_ConfigChannel(&hadc1, &sConfig);

// Channel 1: PA1 (Batt-Pos)
sConfig.Channel = ADC_CHANNEL_1;
sConfig.Rank = 2;
HAL_ADC_ConfigChannel(&hadc1, &sConfig);

// Channel 4: PA4 (Link-Pos)
sConfig.Channel = ADC_CHANNEL_4;
sConfig.Rank = 3;
HAL_ADC_ConfigChannel(&hadc1, &sConfig);

// Channel 8: PB0 (Link-Neg)
sConfig.Channel = ADC_CHANNEL_8;
sConfig.Rank = 4;
HAL_ADC_ConfigChannel(&hadc1, &sConfig);

// Channel 10: PC0 (1.5V Reference)
sConfig.Channel = ADC_CHANNEL_10;
sConfig.Rank = 5;
HAL_ADC_ConfigChannel(&hadc1, &sConfig);
```

#### 3.2 DMA Configuration
```cpp
// DMA for continuous ADC conversion
DMA_HandleTypeDef hdma_adc1;
uint32_t adcBuffer[5];  // Batt-Neg, Batt-Pos, Link-Pos, Link-Neg, 1.5V Reference

// Configure DMA
hdma_adc1.Instance = DMA2_Stream0;
hdma_adc1.Init.Channel = DMA_CHANNEL_0;
hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
hdma_adc1.Init.Mode = DMA_CIRCULAR;
```

#### 3.3 Voltage Calculation (Differential with Reference)
```cpp
// Convert ADC values to voltages using differential measurement
float convertAdcToVoltage(uint32_t adcValue, uint32_t referenceValue) {
    const float VREF = 3.3f;
    const float ADC_RESOLUTION = 4096.0f;
    const float VOLTAGE_SCALE_FACTOR = 25.0f / 0.067f;  // 373.13
    
    // Calculate differential voltage (signal - 1.5V reference)
    float signalVoltage = (adcValue * VREF) / ADC_RESOLUTION;
    float referenceVoltage = (referenceValue * VREF) / ADC_RESOLUTION;
    float differentialVoltage = signalVoltage - referenceVoltage;
    
    return differentialVoltage * VOLTAGE_SCALE_FACTOR;
}

// Update voltage readings with reference subtraction
void updatePackVoltages() {
    uint32_t reference1_5V = adcBuffer[4];  // PC0 - 1.5V Reference
    
    // Calculate differential voltages referenced to 1.5V center
    battContactorNeg = convertAdcToVoltage(adcBuffer[0], reference1_5V);  // PA0
    battContactorPos = convertAdcToVoltage(adcBuffer[1], reference1_5V);  // PA1
    battLinkPos = convertAdcToVoltage(adcBuffer[2], reference1_5V);       // PA4
    battLinkNeg = convertAdcToVoltage(adcBuffer[3], reference1_5V);       // PB0
    
    battContactorSum = fabs(battContactorPos) + fabs(battContactorNeg);
}
```

### Phase 4: Library Adaptations

#### 4.1 BATMan Library
- **No changes needed** - operates at protocol level
- Verify SPI timing compatibility at 84MHz

#### 4.2 AS8510 Library
- **Pin definitions**: Update to STM32 GPIO
- **SPI calls**: Replace with HAL_SPI functions
- **Timing**: Verify 1MHz SPI clock generation

#### 4.3 Parameter System
- **No changes needed** - pure software implementation
- Verify UART output formatting

### Phase 5: Timing and Performance

#### 5.1 Main Loop Timing
```cpp
// Maintain 50ms main loop timing
static uint32_t lastMainLoopTime = 0;
const uint32_t MAIN_LOOP_INTERVAL = 50;

if (HAL_GetTick() - lastMainLoopTime >= MAIN_LOOP_INTERVAL) {
    // Main loop execution
    lastMainLoopTime = HAL_GetTick();
}
```

#### 5.2 Timer Configuration
```cpp
// Configure system timer for millisecond timing
// Use HAL_GetTick() instead of millis()
```

## Code Structure Changes

### Required Header Changes
```cpp
// Replace Arduino/ESP32 includes
#include <Arduino.h>           // REMOVE
#include <SPI.h>               // REMOVE
#include <HardwareSerial.h>    // REMOVE
#include <Wire.h>              // REMOVE
#include <Adafruit_ADS1X15.h>  // REMOVE

// Add STM32 includes
#include "stm32f4xx_hal.h"     // ADD
#include "main.h"              // ADD
#include "stm32_pin_map.h"     // ADD
```

### Function Signature Changes
```cpp
// Replace Arduino-style functions
void setup() → void SystemInit()
void loop() → void MainLoop()
Serial.println() → HAL_UART_Transmit()
millis() → HAL_GetTick()
delay() → HAL_Delay()
```

## Testing Strategy

### Phase 1 Testing: Basic Functionality
1. **GPIO Test**: Verify all pin assignments
2. **SPI Test**: Communication with Tesla BMS
3. **UART Test**: Serial communication
4. **PWM Test**: Contactor control

### Phase 2 Testing: ADC Functionality
1. **ADC Calibration**: Verify voltage scaling
2. **Reference Channel Test**: Validate 1.5V reference reading accuracy
3. **Differential Measurement**: Test signal - reference calculation
4. **DMA Operation**: Continuous conversion of all 5 channels
5. **Voltage Accuracy**: Compare differential results with known values

### Phase 3 Testing: System Integration
1. **AS8510 Communication**: Current sensor
2. **BATMan Integration**: Full BMS communication
3. **Parameter System**: All parameters working
4. **ESPHome Interface**: Serial data transmission

### Phase 4 Testing: Performance Validation
1. **Timing Verification**: Main loop timing
2. **SPI Speed**: Tesla BMS communication reliability
3. **ADC Sampling Rate**: Continuous conversion
4. **Memory Usage**: RAM/Flash utilization

## Performance Considerations

### CPU Speed Impact
- **STM32F411RE**: 84MHz vs ESP32 240MHz
- **Impact**: 35% of ESP32 performance
- **Mitigation**: Optimize SPI timing, use DMA

### Memory Usage
- **RAM**: 128KB vs ESP32 ~300KB
- **Flash**: 512KB vs ESP32 4MB
- **Current Usage**: Well within limits

### Real-time Requirements
- **Main Loop**: 50ms interval (easily achievable)
- **SPI Communication**: 1MHz (compatible)
- **ADC Sampling**: Continuous DMA (faster than ADS1115)

## Migration Timeline

### Week 1: Setup and Basic Interfaces
- [ ] STM32CubeIDE project setup
- [ ] Pin mapping definitions
- [ ] SPI interface conversion
- [ ] UART interface conversion

### Week 2: ADC Implementation
- [ ] Internal ADC configuration
- [ ] DMA setup for continuous conversion
- [ ] Voltage scaling calibration
- [ ] Remove ADS1115 dependencies

### Week 3: System Integration
- [ ] AS8510 library adaptation
- [ ] BATMan interface testing
- [ ] Parameter system validation
- [ ] PWM contactor control

### Week 4: Testing and Optimization
- [ ] Full system testing
- [ ] Performance optimization
- [ ] ESPHome interface validation
- [ ] Documentation update

## Hardware Wiring Changes

### CN7 Connector Pinout
```
Pin 28: PA0 (ADC1_IN0) → Batt-Neg voltage divider
Pin 30: PA5 (SPI1_SCK) → Tesla BMS SCK
Pin 30: PA1 (ADC1_IN1) → Batt-Pos voltage divider  
Pin 32: PA7 (SPI1_MOSI) → Tesla BMS MOSI
Pin 32: PA4 (ADC1_IN4) → Link-Pos voltage divider
Pin 34: PB6 (GPIO) → Tesla BMS CS
Pin 34: PB0 (ADC1_IN8) → Link-Neg voltage divider
Pin 36: PC8 (TIM3_CH3) → Pre-charge relay PWM
Pin 38: PC7 (TIM3_CH2) → Pack contactors PWM
```

### CN10 Connector Pinout
```
Pin 26: PB13 (SPI2_SCK) → AS8510 SCK
Pin 28: PB14 (SPI2_MISO) → AS8510 MISO
Pin 30: PB15 (SPI2_MOSI) → AS8510 MOSI
Pin 32: PC6 (GPIO) → AS8510 CS
Pin 35: PA2 (USART2_TX) → ESPHome TX
Pin 37: PA3 (USART2_RX) → ESPHome RX
Pin 38: PC0 (ADC1_IN10) → 1.5V Reference voltage
```

## Risk Mitigation

### Technical Risks
1. **SPI Timing**: Test with oscilloscope
2. **ADC Accuracy**: Calibrate with known voltages
3. **Memory Constraints**: Monitor usage
4. **Real-time Performance**: Profile execution times

### Mitigation Strategies
1. **Incremental Migration**: One interface at a time
2. **Hardware Validation**: Test each connection
3. **Rollback Plan**: Keep ESP32 version functional
4. **Documentation**: Record all changes

## Success Criteria

### Functional Requirements
- [ ] All Tesla BMS communication working
- [ ] AS8510 current sensor operational
- [ ] Pack voltage monitoring accurate
- [ ] Contactor control functional
- [ ] ESPHome interface working
- [ ] Parameter system complete

### Performance Requirements
- [ ] 50ms main loop timing maintained
- [ ] SPI communication stable at 1MHz
- [ ] ADC sampling faster than ADS1115
- [ ] Memory usage within limits
- [ ] System stability over 24+ hours

## Conclusion

This conversion plan provides a comprehensive roadmap for migrating from ESP32 to STM32F411RE while improving the system by replacing the external ADS1115 with internal ADC. The STM32 platform offers better real-time performance, lower power consumption, and integrated ADC functionality that simplifies the hardware design.

The key advantages of this migration:
1. **Integrated ADC**: Eliminates I2C dependency and improves sampling speed
2. **1.5V Reference Channel**: Provides drift compensation and improved accuracy
3. **Real-time Performance**: Better deterministic timing with 5-channel DMA
4. **Professional Platform**: STM32 ecosystem for automotive applications
5. **Cost Reduction**: Eliminates external ADC component
6. **Improved Reliability**: Fewer external components, better temperature stability
7. **True Differential Measurement**: Maintains original system's differential approach

The migration is feasible with moderate effort, maintaining all existing functionality while improving system performance and reliability. 