#ifndef YEELIGHTARDUINO_FLOW_H
#define YEELIGHTARDUINO_FLOW_H

#include "Yeelight.h"

struct flow_expression;

class Flow {
private:
    std::vector<flow_expression> flow;
    uint8_t count = 0;
public:
    Flow();

    void add_rgb(uint32_t duration, uint32_t value, int8_t brightness);

    void add_rgb(uint32_t duration, uint8_t r, uint8_t g, uint8_t b, int8_t brightness);

    void add_ct(uint32_t duration, uint32_t color_temperature, int8_t brightness);

    void add_sleep(uint32_t duration);

    void add_expression(flow_expression expression);

    std::vector<flow_expression> get_flow();

    void clear();

    void remove_last();

    void remove_first();

    void remove_at(uint32_t index);

    flow_expression operator[](uint32_t index);

    Flow operator+(const Flow &flow);

    Flow operator+(const flow_expression &expression);

    Flow operator+(const std::vector<flow_expression> &expressions);

    void set_count(uint8_t count);

    uint8_t get_count();
};

#endif
