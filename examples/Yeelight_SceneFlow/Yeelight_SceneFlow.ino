#include <Yeelight.h>
#include <WiFi.h>
#include <Flow.h>

#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASS "YourWiFiPassword"
const uint8_t bulbIP[] = {192,168,1,100};

Yeelight bulb;

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status()!=WL_CONNECTED) { delay(500); Serial.print("."); }
    if (bulb.connect(bulbIP)!=ResponseType::SUCCESS) {
        Serial.println("Connect error"); while(true) delay(1000);
    }
    // build a Flow for the scene
    Flow flow;
    flow.add_rgb(1000,255,0,0,100);
    flow.add_sleep(500);
    flow.add_rgb(1000,0,255,0,100);
    flow.set_count(5);
    flow.setAction(FLOW_OFF);
    // send the scene flow
    if (bulb.set_scene_flow(flow) == ResponseType::SUCCESS) {
        Serial.println("Scene flow started");
    } else {
        Serial.println("Error starting scene");
    }
}

void loop() {
    // on button press we could stop the scene
    // if (buttonPressed()) bulb.stop_flow();
}
