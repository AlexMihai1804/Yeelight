#include "Flow.h"
#include <cmath>

Flow::Flow() {
    flow = std::vector<flow_expression>();
}

void Flow::add_rgb(const uint32_t duration, const uint32_t value, const uint8_t brightness) {
    const flow_expression expression = {duration, FLOW_COLOR, value, brightness};
    flow.push_back(expression);
}

void Flow::add_rgb(const uint32_t duration, const uint8_t r, const uint8_t g, const uint8_t b,
                   const uint8_t brightness) {
    const uint32_t value = (r << 16) + (g << 8) + b;
    add_rgb(duration, value, brightness);
}

void Flow::add_ct(const uint32_t duration, const uint32_t color_temperature, const uint8_t brightness) {
    const flow_expression expression = {duration, FLOW_COLOR_TEMPERATURE, color_temperature, brightness};
    flow.push_back(expression);
}

void Flow::add_sleep(const uint32_t duration) {
    const flow_expression expression = {duration, FLOW_SLEEP, 0, 0};
    flow.push_back(expression);
}

void Flow::add_hsv(const uint32_t duration, const uint16_t hue, const uint8_t sat, const uint8_t brightness) {
    float H = fmod(hue, 360.0);
    if (H < 0.0) {
        H += 360.0;
    }
    const float S = static_cast<float>(sat) / 255.0f;
    uint8_t brightness_clamped;
    if (brightness < 0) {
        brightness_clamped = 0;
    } else if (brightness > 100) {
        brightness_clamped = 100;
    } else {
        brightness_clamped = static_cast<uint8_t>(brightness);
    }
    const float V = static_cast<float>(brightness_clamped) / 100.0f;
    const float C = V * S;
    const float H_prime = H / 60.0f;
    const float X = C * (1.0f - std::fabs(std::fmod(H_prime, 2.0f) - 1.0f));
    const float m = V - C;
    float R_prime, G_prime, B_prime;
    if (0.0f <= H_prime && H_prime < 1.0f) {
        R_prime = C;
        G_prime = X;
        B_prime = 0.0f;
    } else if (1.0f <= H_prime && H_prime < 2.0f) {
        R_prime = X;
        G_prime = C;
        B_prime = 0.0f;
    } else if (2.0f <= H_prime && H_prime < 3.0f) {
        R_prime = 0.0f;
        G_prime = C;
        B_prime = X;
    } else if (3.0f <= H_prime && H_prime < 4.0f) {
        R_prime = 0.0f;
        G_prime = X;
        B_prime = C;
    } else if (4.0f <= H_prime && H_prime < 5.0f) {
        R_prime = X;
        G_prime = 0.0f;
        B_prime = C;
    } else if (5.0f <= H_prime && H_prime < 6.0f) {
        R_prime = C;
        G_prime = 0.0f;
        B_prime = X;
    } else {
        R_prime = 0.0f;
        G_prime = 0.0f;
        B_prime = 0.0f;
    }
    const float R = (R_prime + m) * 255.0f;
    const float G = (G_prime + m) * 255.0f;
    const float B = (B_prime + m) * 255.0f;
    const auto r = static_cast<uint8_t>(std::round(R));
    const auto g = static_cast<uint8_t>(std::round(G));
    const auto b = static_cast<uint8_t>(std::round(B));
    const float brightness_calc = 0.299f * r + 0.587f * g + 0.114f * b;
    const auto bright = static_cast<uint8_t>(std::round((brightness_calc / 255.0) * 100.0));
    add_rgb(duration, r, g, b, bright);
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

std::vector<flow_expression> Flow::get_flow() const {
    return flow;
}
