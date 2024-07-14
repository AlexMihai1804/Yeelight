# Arduino Yeelight Library

This Arduino library provides a simple and efficient way to control Yeelight smart bulbs using Wi-Fi. It's designed to be compatible with a wide range of Arduino boards and *all* Yeelight products that support LAN control.

## Features

* **Broad Compatibility:**  The library is designed to be compatible with various Arduino boards, including ESP32, ESP32-S3, ESP32-C3, ESP8266.
* **Yeelight Bulb Support:**  The library supports all Yeelight models that support LAN control.
* **Comprehensive Control:**  Control the power, brightness, color temperature, RGB color, and color flow of your Yeelight bulbs.
* **Background Light Support:**  Control the background light (if supported by your bulb).

## Getting Started

1. **Install the Library:** The library will be available through the Arduino Library Manager. Simply search for "Yeelight" and install it.
2. **Include the Library:**  Include the library in your Arduino sketch:

```cpp
#include <Yeelight.h>
```
Connect to your Yeelight bulb:

```cpp
// Define the Yeelight device's IP address and port
uint8_t ip[] = {192, 168, 1, 100};  // Replace with your device's IP
uint16_t port = 55443;
// Create a Yeelight object
Yeelight lamp(ip, port);

// Check if the connection was successful
if (lamp.is_connected()) {
    Serial.println("Connected to the Yeelight device");
} else {
    Serial.println("Connection error");
}
```
Start controlling your Yeelight bulb: Refer to the examples below and the documentation for complete control options.
Examples
Basic Commands:
```cpp
// Turn on the light
lamp.turn_on();

// Turn off the light
lamp.turn_off();

// Set the color to warm white
lamp.set_color_temp(2700);

// Set the color to cool white
lamp.set_color_temp(6500);

// Set the brightness to 50%
lamp.set_brightness(50);

// Set the color to red
lamp.set_rgb_color(255, 0, 0);

// Set the color to green
lamp.set_rgb_color(0, 255, 0);

// Set the color to blue
lamp.set_rgb_color(0, 0, 255);
```
Controlling Color Flows:
```cpp
// Create a color flow
Flow myFlow;
myFlow.add_rgb(1000, 255, 0, 0);   // Red for 1 second
myFlow.add_rgb(1000, 0, 255, 0);   // Green for 1 second
myFlow.add_rgb(1000, 0, 0, 255);   // Blue for 1 second

// Start the color flow
lamp.start_flow(myFlow);

// Stop the color flow
lamp.stop_flow();
```
Retrieving Device Properties:
```cpp
// Retrieve device properties
lamp.refreshProperties();

// Display properties
Serial.print("Device Name: ");
Serial.println(lamp.getProperties().name);
Serial.print("Light State: ");
Serial.println(lamp.getProperties().power ? "On" : "Off");
Serial.print("Brightness: ");
Serial.println(lamp.getProperties().bright);
Serial.print("Color Temperature: ");
Serial.println(lamp.getProperties().ct);
```
Adjustment Functions:
```cpp
// Increase brightness by 10%
lamp.adjust_brightness(10);

// Decrease brightness by 10%
lamp.adjust_brightness(-10);

// Increase color temperature by 10%
lamp.adjust_color_temp(10);

// Decrease color temperature by 10%
lamp.adjust_color_temp(-10);
```
### Documentation
For complete documentation of the library, please refer to the Doxygen documentation generated from the header files.
### Testing
This library has been tested with the following hardware and software:
* Arduino Boards: ESP32, ESP32-S3, ESP32-C3, ESP8266
* Yeelight Bulbs: All Yeelight products that support LAN control  
### Future Updates
Here are some features that are planned for future updates to the library:
* Predefined Color Flows: Include a set of pre-defined color flows, like "Disco," "Sunrise," "Sunset," etc.
* Customizable Color Flows: Allow users to define their own custom color flows using a more flexible API.
* Flow Generation: Add functions to generate color flows based on specific parameters (color, brightness, duration, etc.).
* Light Group Control: Support controlling multiple Yeelight bulbs as a group.
### Contributions
Contributions are welcome! Open an issue or submit a pull request.
### License
This library is licensed under the [license] - see the LICENSE file for details.