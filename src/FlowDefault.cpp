#include "FlowDefault.h"
#include "FlowTransitions.h"

Flow FlowDefault::disco(const uint8_t bpm) {
    Flow flow;
    flow = flow + FlowTransitions::disco(bpm);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::temp(const uint16_t duration) {
    Flow flow;
    flow = flow + FlowTransitions::temp(duration);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::strobe(const uint16_t duration) {
    Flow flow;
    flow = flow + FlowTransitions::strobe(duration);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::pulse(const uint8_t r, const uint8_t g, const uint8_t b, const uint16_t duration,
                        const uint8_t brightness, const uint16_t count) {
    Flow flow;
    flow = flow + FlowTransitions::pulse(r, g, b, duration, brightness);
    flow.set_count(count);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::strobeColor(const uint16_t duration, const uint8_t brightness) {
    Flow flow;
    flow = flow + FlowTransitions::strobeColor(duration, brightness);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::alarm(const uint16_t duration) {
    Flow flow;
    flow = flow + FlowTransitions::alarm(duration);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::police(const uint16_t duration, const uint8_t brightness) {
    Flow flow;
    flow = flow + FlowTransitions::police(duration, brightness);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::police2(const uint16_t duration, const uint8_t brightness) {
    Flow flow;
    flow = flow + FlowTransitions::police2(duration, brightness);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::lsd(const uint16_t duration, const uint8_t brightness) {
    Flow flow;
    flow = flow + FlowTransitions::lsd(duration, brightness);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::christmas(const uint16_t duration, const uint8_t brightness, const uint16_t speed) {
    Flow flow;
    flow = flow + FlowTransitions::christmas(duration, brightness, speed);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::rgb(const uint32_t duration, const uint8_t brightness, const uint16_t sleep) {
    Flow flow;
    flow = flow + FlowTransitions::rgb(duration, brightness, sleep);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::randomLoop(const uint16_t duration, const uint8_t brightness, const uint16_t count) {
    Flow flow;
    flow = flow + FlowTransitions::randomLoop(duration, brightness, count);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::slowdown(const uint16_t duration, const uint8_t brightness, const uint16_t count) {
    Flow flow;
    flow = flow + FlowTransitions::slowdown(duration, brightness, count);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::home(const uint16_t duration, const uint8_t brightness) {
    Flow flow;
    flow.add_ct(duration, 3200, brightness);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::nightMode(const uint16_t duration, const uint8_t brightness) {
    Flow flow;
    flow.add_rgb(duration, 0xFF, 0x99, 0x00, brightness);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::dateNight(const uint16_t duration, const uint8_t brightness) {
    Flow flow;
    flow.add_rgb(duration, 0xFF, 0x66, 0x00, brightness);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::movie(const uint16_t duration, const uint8_t brightness) {
    Flow flow;
    flow.add_rgb(duration, 0x14, 0x14, 0x32, brightness);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::sunrise() {
    Flow flow;
    flow.add_rgb(50, 0xFF, 0x4D, 0x00, 1);
    flow.add_ct(360000, 1700, 10);
    flow.add_ct(540000, 2700, 100);
    flow.set_count(1);
    flow.setAction(FLOW_STAY);
    return flow;
}

Flow FlowDefault::sunset() {
    Flow flow;
    flow.add_ct(50, 2700, 10);
    flow.add_ct(180000, 1700, 5);
    flow.add_rgb(420000, 0xFF, 0x4C, 0x00, 1);
    flow.set_count(1);
    flow.setAction(FLOW_OFF);
    return flow;
}

Flow FlowDefault::romance() {
    Flow flow;
    flow.add_rgb(4000, 0x59, 0x15, 0x6D, 1);
    flow.add_rgb(4000, 0x66, 0x14, 0x2A, 1);
    flow.set_count(0);
    flow.setAction(FLOW_STAY);
    return flow;
}

Flow FlowDefault::happyBirthday() {
    Flow flow;
    flow.add_rgb(1996, 0xDC, 0x50, 0x19, 80);
    flow.add_rgb(1996, 0xDC, 0x78, 0x1E, 80);
    flow.add_rgb(1996, 0xAA, 0x32, 0x14, 80);
    flow.set_count(0);
    flow.setAction(FLOW_STAY);
    return flow;
}

Flow FlowDefault::candleFlicker() {
    Flow flow;
    flow.add_ct(800, 2700, 50);
    flow.add_ct(800, 2700, 30);
    flow.add_ct(1200, 2700, 80);
    flow.add_ct(800, 2700, 60);
    flow.add_ct(1200, 2700, 90);
    flow.add_ct(2400, 2700, 50);
    flow.add_ct(1200, 2700, 80);
    flow.add_ct(800, 2700, 60);
    flow.add_ct(400, 2700, 70);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}

Flow FlowDefault::teaTime(const uint16_t duration, const uint8_t brightness) {
    Flow flow;
    flow.add_ct(duration, 3000, brightness);
    flow.set_count(0);
    flow.setAction(FLOW_RECOVER);
    return flow;
}
