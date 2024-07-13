#include "Yeelight.h"
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

    // Refresh the bulb properties
    if (bulb.refreshProperties() == ResponseType::SUCCESS) {
        Serial.println("Bulb properties refreshed.");
    } else {
        Serial.println("Error refreshing properties.");
    }
}

void loop() {
    // Display the bulb properties
    YeelightProperties props = bulb.getProperties();
    Serial.print("Power: ");
    Serial.println(props.power ? "On" : "Off");

    Serial.print("Brightness: ");
    Serial.println(props.bright);

    Serial.print("Color temperature: ");
    Serial.println(props.ct);

    Serial.print("RGB: ");
    Serial.println(props.rgb);

    Serial.print("Hue: ");
    Serial.println(props.hue);

    Serial.print("Saturation: ");
    Serial.println(props.sat);

    Serial.print("Color Mode: ");
    switch (props.color_mode) {
        case Color_mode::COLOR_MODE_RGB:
            Serial.println("RGB");
            break;
        case Color_mode::COLOR_MODE_COLOR_TEMPERATURE:
            Serial.println("Color Temperature");
            break;
        case Color_mode::COLOR_MODE_HSV:
            Serial.println("HSV");
            break;
        default:
            Serial.println("Unknown");
            break;
    }

    // ... Display the rest of the properties ...

    delay(5000); // Wait for 5 seconds
}