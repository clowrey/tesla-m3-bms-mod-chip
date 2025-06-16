#include <Arduino.h>
#include "BatMan.h"
#include <TFT_eSPI.h>
#include <SPI.h>

BATMan batman;
TFT_eSPI tft = TFT_eSPI();

// Variables to store previous values for comparison
float prevMinVoltage = 0;
float prevMaxVoltage = 0;
int prevMinCell = 0;
int prevMaxCell = 0;

void updateDisplay() {
    // Get current values
    float minVoltage = batman.getMinVoltage();
    float maxVoltage = batman.getMaxVoltage();
    int minCell = batman.getMinCell();
    int maxCell = batman.getMaxCell();

    // Only update if values have changed
    if (minVoltage != prevMinVoltage || maxVoltage != prevMaxVoltage || 
        minCell != prevMinCell || maxCell != prevMaxCell) {
        
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
        
        // Update previous values
        prevMinVoltage = minVoltage;
        prevMaxVoltage = maxVoltage;
        prevMinCell = minCell;
        prevMaxCell = maxCell;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Tesla Model 3 BMB Interface Starting...");
    
    // Initialize the display
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    
    // Initialize the BATMan interface
    batman.BatStart();
}

void loop() {
    // Run the BATMan state machine
    batman.loop();
    
    // Update the display
    updateDisplay();
    
    // Add a small delay to prevent overwhelming the serial output
    delay(100);
} 