#include "Yeelight.h"
#include "Flow.h"
#include <WiFi.h>

const uint8_t ip[] = {192, 168, 1, 100};
Yeelight bulb;

void setup() {
    Serial.begin(115200);

    // Connect to WiFi (replace with your network credentials)
    WiFi.begin("YourWiFiSSID", "YourWiFiPassword");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi!");

    // Connect to the bulb
    if (bulb.connect(ip) == ResponseType::SUCCESS) {
        Serial.println("Connected to Yeelight bulb.");
    } else {
        Serial.println("Error connecting to bulb.");
    }
}

void loop() {
    // Example 1: Changing color with a repeating flow
    Flow flow1;
    flow1.add_rgb(1000, 255, 0, 0, 100); // Red for 1 second
    flow1.add_rgb(1000, 0, 255, 0, 100); // Green for 1 second
    flow1.add_rgb(1000, 0, 0, 255, 100); // Blue for 1 second
    flow1.set_count(3); // Repeat the flow 3 times
    if (bulb.start_flow(flow1) == ResponseType::SUCCESS) {
        Serial.println("Flow 1 started.");
    } else {
        Serial.println("Error starting flow 1.");
    }
    delay(5000); // Wait for 5 seconds

    // Example 2: Stopping the flow
    if (bulb.stop_flow() == ResponseType::SUCCESS) {
        Serial.println("Flow stopped.");
    } else {
        Serial.println("Error stopping flow.");
    }
    delay(5000); // Wait for 5 seconds

    // Example 3: Setting the flow to stay on the last color
    Flow flow2;
    flow2.add_rgb(1000, 255, 0, 0, 50); // Red for 1 second
    flow2.add_rgb(1000, 0, 255, 0, 50); // Green for 1 second
    flow2.add_rgb(1000, 0, 0, 255, 50); // Blue for 1 second
    flow2.setAction(FLOW_STAY); // Stay on the last color
    if (bulb.start_flow(flow2) == ResponseType::SUCCESS) {
        Serial.println("Flow 2 started.");
    } else {
        Serial.println("Error starting flow 2.");
    }
    delay(5000); // Wait for 5 seconds

    // Example 4: Setting the flow to turn off at the end
    Flow flow3;
    flow3.add_rgb(1000, 255, 0, 0, 75); // Red for 1 second
    flow3.add_rgb(1000, 0, 255, 0, 75); // Green for 1 second
    flow3.add_rgb(1000, 0, 0, 255, 75); // Blue for 1 second
    flow3.setAction(FLOW_OFF); // Turn off at the end
    if (bulb.start_flow(flow3) == ResponseType::SUCCESS) {
        Serial.println("Flow 3 started.");
    } else {
        Serial.println("Error starting flow 3.");
    }
    delay(5000); // Wait for 5 seconds

    // Example 5: Running a flow indefinitely (until stopped)
    Flow flow4;
    flow4.add_rgb(1000, 255, 0, 0, 100); // Red for 1 second
    flow4.add_rgb(1000, 0, 255, 0, 100); // Green for 1 second
    flow4.add_rgb(1000, 0, 0, 255, 100); // Blue for 1 second
    flow4.set_count(0); // Repeat indefinitely
    if (bulb.start_flow(flow4) == ResponseType::SUCCESS) {
        Serial.println("Flow 4 started.");
    } else {
        Serial.println("Error starting flow 4.");
    }
    delay(10000); // Wait for 10 seconds

    // Stop the flow
    if (bulb.stop_flow() == ResponseType::SUCCESS) {
        Serial.println("Flow stopped.");
    } else {
        Serial.println("Error stopping flow.");
    }
    delay(5000); // Wait for 5 seconds
}