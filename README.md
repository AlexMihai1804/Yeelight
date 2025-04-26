[![PlatformIO Registry](https://badges.registry.platformio.org/packages/alexmihai1804/library/Yeelight.svg)](https://registry.platformio.org/libraries/alexmihai1804/Yeelight)
[![Arduino Library](https://img.shields.io/badge/Arduino%20Library-Yeelight-blue)](https://docs.arduino.cc/libraries/yeelight/)
![Version](https://img.shields.io/badge/Version-1.2.0-blue.svg)
# Arduino Yeelight Library

A fully asynchronous Arduino/PlatformIO library for discovering and controlling Yeelight smart bulbs over LAN.

## Table of Contents

- [Features](#features)  
- [Requirements](#requirements)  
- [Installation](#installation)  
- [Quick Start](#quick-start)  
- [Examples](#examples)  
- [Project Structure](#project-structure)  
- [Testing](#testing)  
- [Troubleshooting](#troubleshooting)  
- [Contributing](#contributing)  

## ✨ Features

- SSDP auto‑discovery (UDP 1982)  
- Power control: on/off/toggle with smooth or sudden effect  
- Color & temperature: RGB, HSV, CT (1700–6500 K)  
- Brightness adjustment & color flows (custom or predefined)  
- Background channel support  
- Music mode: low‑latency command streaming  
- Fully async (AsyncTCP/ESPAsyncTCP + FreeRTOS)  
- Device state caching and query  
- **Implementation:** see `src/Yeelight.cpp`, `src/Yeelight_enums.h`, `src/Yeelight_structs.h`

## Requirements

- Arduino IDE ≥ 1.8.10 or PlatformIO  
- Board: ESP32 (supporting AsyncTCP + FreeRTOS)  
- Dependencies (Library Manager or lib_deps):  
  - cJSON  
  - AsyncTCP  
  - WiFi (built‑in)  

## Installation

### Arduino IDE
1. Open Arduino IDE.  
2. Go to **Sketch → Include Library → Manage Libraries...**  
3. In the Library Manager search box, type **Yeelight**.  
4. Find **Yeelight** by **alexmihai1804** and click **Install**.  
5. In your sketch add:
```cpp
#include <Yeelight.h>
```

### PlatformIO

Add to `platformio.ini`:

```ini
[env:your_env]
platform = espressif32   
framework = arduino
board = esp32dev         
lib_deps =
  alexmihai1804/Yeelight
```

## Quick Start

> **Important:** You must first connect to WiFi before establishing a connection with any Yeelight device.

```cpp
#include <WiFi.h>
#include <Yeelight.h>

// Wi‑Fi credentials
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

void setup() {
  Serial.begin(115200);
  
  // Step 1: Connect to WiFi first
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  
  // Step 2: Connect to Yeelight device
  // Option 1: Connect using IP
  uint8_t bulbIP[] = {192, 168, 1, 100};
  Yeelight bulb(bulbIP);
  
  // Option 2: Create instance first, then connect
  // Yeelight bulb;
  // bulb.connect(bulbIP);

  if (bulb.is_connected()) {
    Serial.println("✔ Connected to Yeelight");
  } else {
    Serial.println("✖ Failed to connect to Yeelight");
  }

  // Control examples
  bulb.turn_on();                         // on
  bulb.set_color_temp(3000);              // warm white
  bulb.set_rgb_color(0,255,128);          // teal
  bulb.set_brightness(75);                // 75%
  bulb.start_flow(FlowDefault::disco());  // disco flow
}

void loop() {
  // your code
}
```

## 📂 Examples

All sketches live under `lib/Yeelight/examples/`. Click to open:

- [Yeelight_Blink](examples/Yeelight_Blink/Yeelight_Blink.ino) – simple on/off every 0.5 s  
- [Yeelight_White_Basic](examples/Yeelight_White_Basic/Yeelight_White_Basic.ino) – CT & brightness  
- [Yeelight_White_Transition](examples/Yeelight_White_Transition/Yeelight_White_Transition.ino) – smooth → sudden CT  
- [Yeelight_RGB_Red](examples/Yeelight_RGB_Red/Yeelight_RGB_Red.ino) – set red + brightness  
- [Yeelight_RGB_Rainbow](examples/Yeelight_RGB_Rainbow/Yeelight_RGB_Rainbow.ino) – multi‑color transitions  
- [Yeelight_HSV_Yellow](examples/Yeelight_HSV_Yellow/Yeelight_HSV_Yellow.ino) – HSV mode demo  
- [Yeelight_HSV_ColorWheel](examples/Yeelight_HSV_ColorWheel/Yeelight_HSV_ColorWheel.ino) – smooth/sudden HSV changes  
- [Yeelight_Flow_PartyMode](examples/Yeelight_Flow_PartyMode/Yeelight_Flow_PartyMode.ino) – custom Flow start/stop  
- [Yeelight_TimerOff](examples/Yeelight_TimerOff/Yeelight_TimerOff.ino) – cron on/off  
- [Yeelight_DelayedTurnOff](examples/Yeelight_DelayedTurnOff/Yeelight_DelayedTurnOff.ino) – auto_delay_off  
- [Yeelight_ConnectionCheck](examples/Yeelight_ConnectionCheck/Yeelight_ConnectionCheck.ino) – error handling  
- [Yeelight_BulbStatus](examples/Yeelight_BulbStatus/Yeelight_BulbStatus.ino) – getProp refresh  
- [Yeelight_BulbFinder](examples/Yeelight_BulbFinder/Yeelight_BulbFinder.ino) – SSDP discovery  
- [MusicMode](examples/MusicMode/MusicMode.ino) – low‑latency music streaming  
- [BackgroundControl](examples/BackgroundControl/BackgroundControl.ino) – bg_light power/CT/RGB  
- [AdjustDemo](examples/AdjustDemo/AdjustDemo.ino) – adjust_brightness/ct/color  
- [SetDefaultState](examples/SetDefaultState/SetDefaultState.ino) – set_default_state for main/bg  
- [SetDeviceName](examples/SetDeviceName/SetDeviceName.ino) – set_device_name usage  
- [Yeelight_ColorScene_Demo](examples/Yeelight_ColorScene_Demo/Yeelight_ColorScene_Demo.ino) – RGB/HSV/CT scene demo  
- [Yeelight_DevToggle_Demo](examples/Yeelight_DevToggle_Demo/Yeelight_DevToggle_Demo.ino) – toggle main/background/both channels  
- [Yeelight_Background_Scene](examples/Yeelight_Background_Scene/Yeelight_Background_Scene.ino) – background‑only scene demo  
- [Yeelight_FlowDefault_Demo](examples/Yeelight_FlowDefault_Demo/Yeelight_FlowDefault_Demo.ino) – predefined FlowDefault patterns  

## Project Structure

```
lib/Yeelight/
├─ src/
│  ├─ Yeelight.h/.cpp
│  ├─ Yeelight_enums.h
│  ├─ Yeelight_structs.h
│  ├─ Flow.h/.cpp
│  └─ FlowDefault.h/.cpp
├─ examples/
│  ├─ SimpleBlink/
│  ├─ ColorCycle/
│  └─ FlowDemo/
└─ README.md
```

## Testing

- Tested on ESP32 boards  
- Verified basic commands, flows, music mode  
- CI with PlatformIO (add your tests under `test/`)  

## Troubleshooting

- **Discovery fails**: ensure UDP 1982 open on LAN  
- **Timeouts**: enable LAN Control in Yeelight app  
- **Music mode issues**: allow TCP port 55443 inbound  
- **Can't connect to device**: make sure your ESP is connected to WiFi first

## Contributing

1. Fork repo  
2. Create branch: `git checkout -b feature/XYZ`  
3. Commit: `git commit -m "Add XYZ"`  
4. Push & open PR  

Please follow code style and include tests/examples.
