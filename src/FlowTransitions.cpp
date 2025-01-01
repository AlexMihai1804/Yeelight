#include "FlowTransitions.h"
#include <Arduino.h>
std::vector<flow_expression> FlowTransitions::disco(uint8_t bpm) {
    uint32_t duration = 60000 / bpm;
    Flow flow;
    flow.add_hsv(duration, 0, 100, 100);
    flow.add_hsv(duration, 0, 100, 1);
    flow.add_hsv(duration, 90, 100, 100);
    flow.add_hsv(duration, 90, 100, 1);
    flow.add_hsv(duration, 180, 100, 100);
    flow.add_hsv(duration, 180, 100, 1);
    flow.add_hsv(duration, 270, 100, 100);
    flow.add_hsv(duration, 270, 100, 1);
    return flow.get_flow();
}
std::vector<flow_expression> FlowTransitions::temp(uint16_t duration) {
    Flow flow;
    flow.add_ct(duration, 1700, 100);
    flow.add_ct(duration, 6500, 100);
    return flow.get_flow();
}
std::vector<flow_expression> FlowTransitions::strobe(uint16_t duration) {
    Flow flow;
    flow.add_hsv(duration, 0, 0, 100);
    flow.add_hsv(duration, 0, 0, 1);
    return flow.get_flow();
}
std::vector<flow_expression> FlowTransitions::pulse(uint8_t r, uint8_t g, uint8_t b, uint16_t duration, uint8_t brightness) {
    Flow flow;
    flow.add_rgb(duration, r, g, b, brightness);
    flow.add_rgb(duration, r, g, b, 1);
    return flow.get_flow();
}
std::vector<flow_expression> FlowTransitions::strobeColor(uint16_t duration, uint8_t brightness) {
    Flow flow;
    flow.add_hsv(duration, 240, 100, brightness);
    flow.add_hsv(duration, 60, 100, brightness);
    flow.add_hsv(duration, 330, 100, brightness);
    flow.add_hsv(duration, 0, 100, brightness);
    flow.add_hsv(duration, 173, 100, brightness);
    flow.add_hsv(duration, 30, 100, brightness);
    return flow.get_flow();
}
std::vector<flow_expression> FlowTransitions::alarm(uint16_t duration) {
    Flow flow;
    flow.add_hsv(duration, 0, 100, 100);
    flow.add_hsv(duration, 0, 100, 60);
    return flow.get_flow();
}
std::vector<flow_expression> FlowTransitions::police(uint16_t duration, uint8_t brightness) {
    Flow flow;
    flow.add_rgb(duration, 255, 0, 0, brightness);
    flow.add_rgb(duration, 0, 0, 255, brightness);
    return flow.get_flow();
}
std::vector<flow_expression> FlowTransitions::police2(uint16_t duration, uint8_t brightness) {
    Flow flow;
    flow.add_rgb(duration, 255, 0, 0, brightness);
    flow.add_rgb(duration, 0, 0, 255, 1);
    flow.add_rgb(duration, 255, 0, 0, brightness);
    flow.add_sleep(duration);
    flow.add_rgb(duration, 0, 0, 255, brightness);
    flow.add_rgb(duration, 0, 0, 255, 1);
    flow.add_rgb(duration, 0, 0, 255, brightness);
    flow.add_sleep(duration);
    return flow.get_flow();
    }
std::vector<flow_expression> FlowTransitions::lsd(uint16_t duration, uint8_t brightness) {
    Flow flow;
    flow.add_hsv(duration, 3,85,brightness);
    flow.add_hsv(duration, 20,90,brightness);
    flow.add_hsv(duration, 55,95,brightness);
    flow.add_hsv(duration, 93,50,brightness);
    flow.add_hsv(duration, 198,97,brightness);
    return flow.get_flow();
  }
std::vector<flow_expression> FlowTransitions::christmas(uint16_t duration, uint8_t brightness, uint16_t sleep) {
    Flow flow;
    flow.add_hsv(duration, 0, 100, brightness);
    flow.add_sleep(sleep);
    flow.add_hsv(duration, 120, 100, brightness);
    flow.add_sleep(sleep);
    return flow.get_flow();
}
std::vector<flow_expression> FlowTransitions::rgb(uint16_t duration, uint8_t brightness, uint16_t sleep) {
    Flow flow;
    flow.add_hsv(duration, 0, 100, brightness);
    flow.add_sleep(sleep);
    flow.add_hsv(duration, 120, 100, brightness);
    flow.add_sleep(sleep);
    flow.add_hsv(duration, 240, 100, brightness);
    flow.add_sleep(sleep);
    return flow.get_flow();
}
std::vector<flow_expression> FlowTransitions::randomLoop(uint16_t duration, uint8_t brightness, uint8_t count) {
    Flow flow;
    for (int i = 0; i < count; i++) {
        flow.add_hsv(duration, random(0, 360), 100, brightness);
    }
    return flow.get_flow();
}
std::vector<flow_expression> FlowTransitions::slowdown(uint16_t duration, uint8_t brightness, uint8_t count) {
    Flow flow;
    for (int i = 0; i < count; i++) {
        flow.add_hsv(duration*(i+1), random(0,360), 100, brightness);
        }
    return flow.get_flow();
    }