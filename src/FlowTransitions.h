#ifndef FLOWTRANSITIONS_H
#define FLOWTRANSITIONS_H
#include <Flow.h>

class FlowTransitions {
public:
    static std::vector<flow_expression> disco(uint8_t bpm = 120);

    static std::vector<flow_expression> temp(uint16_t duration = 40000);

    static std::vector<flow_expression> strobe(uint16_t duration = 50);

    static std::vector<flow_expression> pulse(uint8_t r, uint8_t g, uint8_t b, uint16_t duration = 250,
                                              uint8_t brightness = 100);

    static std::vector<flow_expression> strobeColor(uint16_t duration = 50, uint8_t brightness = 100);

    static std::vector<flow_expression> alarm(uint16_t duration = 250);

    static std::vector<flow_expression> police(uint16_t duration = 300, uint8_t brightness = 100);

    static std::vector<flow_expression> police2(uint16_t duration = 250, uint8_t brightness = 100);

    static std::vector<flow_expression> lsd(uint16_t duration = 300, uint8_t brightness = 100);

    static std::vector<flow_expression> christmas(uint16_t duration = 250, uint8_t brightness = 100,
                                                  uint16_t speed = 3000);

    static std::vector<flow_expression> rgb(uint16_t duration = 250, uint8_t brightness = 100, uint16_t speed = 3000);

    static std::vector<flow_expression>
    randomLoop(uint16_t duration = 750, uint8_t brightness = 100, uint8_t count = 9);

    static std::vector<flow_expression> slowdown(uint16_t duration = 2000, uint8_t brightness = 100, uint8_t count = 8);
};
#endif
