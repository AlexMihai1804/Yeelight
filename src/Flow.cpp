#include "Flow.h"

Flow::Flow() {
    flow = std::vector<flow_expression>();
}

void Flow::add_rgb(const uint32_t duration, const uint32_t value, const int8_t brightness) {
    const flow_expression expression = {duration, FLOW_COLOR, value, brightness};
    flow.push_back(expression);
}

void Flow::add_rgb(const uint32_t duration, const uint8_t r, const uint8_t g, const uint8_t b, const int8_t brightness) {
    const uint32_t value = (r << 16) + (g << 8) + b;
    add_rgb(duration, value, brightness);
}

void Flow::add_ct(const uint32_t duration, const uint32_t color_temperature, const int8_t brightness) {
    const flow_expression expression = {duration, FLOW_COLOR_TEMPERATURE, color_temperature, brightness};
    flow.push_back(expression);
}

void Flow::add_sleep(const uint32_t duration) {
    const flow_expression expression = {duration, FLOW_SLEEP, 0, 0};
    flow.push_back(expression);
}

void Flow::add_expression(const flow_expression &expression) {
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

void Flow::remove_at(const uint32_t index) {
    flow.erase(flow.begin() + index);
}

flow_expression Flow::operator[](const uint32_t index) const {
    return flow[index];
}

Flow Flow::operator+(const Flow &flow) const {
    auto new_flow = Flow();
    new_flow.flow = this->flow;
    new_flow.flow.insert(new_flow.flow.end(), flow.flow.begin(), flow.flow.end());
    return new_flow;
}

Flow Flow::operator+(const flow_expression &expression) const {
    auto new_flow = Flow();
    new_flow.flow = this->flow;
    new_flow.flow.push_back(expression);
    return new_flow;
}

Flow Flow::operator+(const std::vector<flow_expression> &expressions) const {
    auto new_flow = Flow();
    new_flow.flow = this->flow;
    new_flow.flow.insert(new_flow.flow.end(), expressions.begin(), expressions.end());
    return new_flow;
}

void Flow::set_count(const uint8_t count) {
    this->count = count;
}

uint8_t Flow::get_count() const {
    return count;
}

uint8_t Flow::get_size() const {
    return flow.size();
}

flow_action Flow::getAction() const {
    return action;
}

void Flow::setAction(const flow_action new_action) {
    action = new_action;
}
