#include "Flow.h"

Flow::Flow() {
    flow = std::vector<flow_expression>();
}

void Flow::add_rgb(uint32_t duration, uint32_t value, int8_t brightness) {
    flow_expression expression = {duration, FLOW_COLOR, value, brightness};
    flow.push_back(expression);
}

void Flow::add_rgb(uint32_t duration, uint8_t r, uint8_t g, uint8_t b, int8_t brightness) {
    uint32_t value = (r << 16) + (g << 8) + b;
    add_rgb(duration, value, brightness);
}

void Flow::add_ct(uint32_t duration, uint32_t color_temperature, int8_t brightness) {
    flow_expression expression = {duration, FLOW_COLOR_TEMPERATURE, color_temperature, brightness};
    flow.push_back(expression);
}

void Flow::add_sleep(uint32_t duration) {
    flow_expression expression = {duration, FLOW_SLEEP, 0, 0};
    flow.push_back(expression);
}

void Flow::add_expression(flow_expression expression) {
    flow.push_back(expression);
}

std::vector<flow_expression> Flow::get_flow() {
    return flow;
}

void Flow::clear() {
    flow.clear();
}

void Flow::remove_last() {
    flow.pop_back();
}

void Flow::remove_first() {
    flow.erase(flow.begin());
}

void Flow::remove_at(uint32_t index) {
    flow.erase(flow.begin() + index);
}

flow_expression Flow::operator[](uint32_t index) {
    return flow[index];
}

Flow Flow::operator+(const Flow &flow) {
    Flow new_flow = Flow();
    new_flow.flow = this->flow;
    new_flow.flow.insert(new_flow.flow.end(), flow.flow.begin(), flow.flow.end());
    return new_flow;
}

Flow Flow::operator+(const flow_expression &expression) {
    Flow new_flow = Flow();
    new_flow.flow = this->flow;
    new_flow.flow.push_back(expression);
    return new_flow;
}

Flow Flow::operator+(const std::vector<flow_expression> &expressions) {
    Flow new_flow = Flow();
    new_flow.flow = this->flow;
    new_flow.flow.insert(new_flow.flow.end(), expressions.begin(), expressions.end());
    return new_flow;
}

void Flow::set_count(uint8_t count) {
    this->count = count;
}

uint8_t Flow::get_count() {
    return count;
}

uint8_t Flow::get_size() {
    return flow.size();
}

flow_action Flow::getAction() const {
    return action;
}

void Flow::setAction(flow_action new_action) {
    action = new_action;
}
