#include <Arduino.h>
#include "BatMan.h"
#include <TFT_eSPI.h>
#include <SPI.h>

BATMan batman;
TFT_eSPI tft = TFT_eSPI();

// PWM Configuration for Economizer
#define ECONOMIZER_PWM_PIN 12  // GPIO pin for PWM output
#define PWM_FREQ 20000        // 20kHz PWM frequency
#define PWM_RESOLUTION 8      // 8-bit resolution (0-255)
#define ECONOMIZER_DUTY 25    // Normal duty cycle (25%)
#define INITIAL_PULSE_TIME 100  // Initial 100% duty cycle time in milliseconds

// Button Configuration
#define BUTTON_PIN 35        // GPIO pin for push button
#define DEBOUNCE_TIME 50     // Debounce time in milliseconds


// Variables to store previous values for comparison
float prevMinVoltage = 0;
float prevMaxVoltage = 0;
int prevMinCell = 0;
int prevMaxCell = 0;
uint8_t prevDutyCycle = 0;  // Track duty cycle changes

// Button and Economizer state variables
bool economizerEnabled = false;
bool lastButtonState = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long economizerStartTime = 0;
bool initialPulseComplete = false;

// Add global variable for current duty cycle
volatile uint8_t currentDutyCycle = 0;

// Add display update timer variables
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 1000; // Update display every 1000ms (1 second)

// Function to set economizer PWM duty cycle (0-100%)
void setEconomizerDutyCycle(uint8_t dutyCycle) {
    // Convert percentage to 8-bit value (0-255)
    uint32_t pwmValue = (dutyCycle * 255) / 100;
    ledcWrite(ECONOMIZER_PWM_PIN, pwmValue);
    
    // Print duty cycle change to serial
    if (dutyCycle != prevDutyCycle) {
        Serial.print("Economizer duty cycle: ");
        Serial.print(dutyCycle);
        Serial.println("%");
        prevDutyCycle = dutyCycle;
    }
    currentDutyCycle = dutyCycle; // Always update global
}

void updateDisplay(uint8_t currentDutyCycle) {
    // Get current values
    float minVoltage = batman.getMinVoltage();
    float maxVoltage = batman.getMaxVoltage();
    int minCell = batman.getMinCell();
    int maxCell = batman.getMaxCell();

        
    // Clear the display
    tft.fillScreen(TFT_BLACK);
    
    // Set text color and size
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    
    // Display title
    tft.setCursor(10, 10);
    tft.println("Tesla BMS Status");
    
    // Display min voltage
    tft.setCursor(10, 30);
    tft.print("MinV: ");
    tft.print(minVoltage, 3);
    tft.println("V");
    
    // Display min cell number
    tft.setCursor(10, 40);
    tft.print("Min Cell: ");
    tft.println(minCell);
    
    // Display max voltage
    tft.setCursor(10, 60);
    tft.print("MaxV: ");
    tft.print(maxVoltage, 3);
    tft.println("V");
    
    // Display max cell number
    tft.setCursor(10, 70);
    tft.print("Max Cell: ");
    tft.println(maxCell);
    
    // Display voltage delta
    tft.setCursor(10, 90);
    tft.print("Delta: ");
    tft.print(maxVoltage - minVoltage, 3);
    tft.println("V");
    
    // Display economizer status with duty cycle
    tft.setCursor(10, 110);
    tft.print("Economizer: ");
    tft.print(currentDutyCycle);
    tft.println("%");
    
    // Update previous values
    prevMinVoltage = minVoltage;
    prevMaxVoltage = maxVoltage;
    prevMinCell = minCell;
    prevMaxCell = maxCell;
    prevDutyCycle = currentDutyCycle;

}

void setup() {
    Serial.begin(115200);
    Serial.println("Tesla Model 3 BMB Interface Starting...");
    
    // Initialize the display
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    
    // Initialize PWM for economizer using new ESP32 Arduino core 3.0 API
    ledcAttach(ECONOMIZER_PWM_PIN, PWM_FREQ, PWM_RESOLUTION);
    setEconomizerDutyCycle(0);  // Start with economizer off
    
    // Initialize button pin
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize the BATMan interface
    batman.BatStart();
}

void loop() {
    // Run the BATMan state machine
    batman.loop();
    
    // Check if it's time to update the display
    unsigned long currentMillis = millis();
    if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        updateDisplay(currentDutyCycle);
        lastDisplayUpdate = currentMillis;
    }
    
    // Read button state with debouncing
    bool reading = digitalRead(BUTTON_PIN);
    
    // Check if button state has changed
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    
    // If button state is stable for debounce time
    if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {
        if (reading != buttonState) {
            buttonState = reading;
            
            // If button is pressed (LOW due to INPUT_PULLUP)
            if (buttonState == LOW) {
                economizerEnabled = !economizerEnabled;
                if (economizerEnabled) {
                    // Start with 100% duty cycle
                    setEconomizerDutyCycle(100);
                    economizerStartTime = millis();
                    initialPulseComplete = false;
                } else {
                    // Turn off economizer
                    setEconomizerDutyCycle(0);
                    initialPulseComplete = false;
                }
            }
        }
    }
    
    // Handle initial pulse timing
    if (economizerEnabled && !initialPulseComplete) {
        if ((millis() - economizerStartTime) >= INITIAL_PULSE_TIME) {
            setEconomizerDutyCycle(ECONOMIZER_DUTY);  // Set to normal duty cycle
            initialPulseComplete = true;
        }
    }
    
    lastButtonState = reading;
    
    // Add a small delay to prevent overwhelming the serial output
    delay(100);
} 