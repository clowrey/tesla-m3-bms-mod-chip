#include <Arduino.h>
#include "BatMan.h"

BATMan batman;

void setup() {
    Serial.begin(115200);
    Serial.println("Tesla Model 3 BMB Interface Starting...");
    
    // Initialize the BATMan interface
    batman.BatStart();
}

void loop() {
    // Run the BATMan state machine
    batman.loop();
    
    // Add a small delay to prevent overwhelming the serial output
    delay(100);
} 