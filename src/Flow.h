#ifndef YEELIGHTARDUINO_FLOW_H
#define YEELIGHTARDUINO_FLOW_H

#include "Yeelight.h"

struct flow_expression;
/**
 * @class Flow
 * @brief Represents a flow of light effects for a Yeelight device.
 * 
 * The Flow class allows you to create and manipulate a sequence of light effects
 * that can be applied to a Yeelight device. Each effect in the flow can be a
 * combination of RGB color, color temperature, and sleep duration.
 */
class Flow {
private:
    std::vector<flow_expression> flow; /**< The vector of flow expressions representing the sequence of light effects. */
    uint8_t count = 0; /**< The number of times the flow should be repeated. */
public:
    /**
     * @brief Default constructor for the Flow class.
     */
    Flow();

    /**
     * @brief Adds an RGB color effect to the flow.
     * 
     * @param duration The duration of the effect in milliseconds.
     * @param value The RGB color value.
     * @param brightness The brightness level of the effect.
     */
    void add_rgb(uint32_t duration, uint32_t value, int8_t brightness);

    /**
     * @brief Adds an RGB color effect to the flow.
     * 
     * @param duration The duration of the effect in milliseconds.
     * @param r The red component of the RGB color.
     * @param g The green component of the RGB color.
     * @param b The blue component of the RGB color.
     * @param brightness The brightness level of the effect.
     */
    void add_rgb(uint32_t duration, uint8_t r, uint8_t g, uint8_t b, int8_t brightness);

    /**
     * @brief Adds a color temperature effect to the flow.
     * 
     * @param duration The duration of the effect in milliseconds.
     * @param color_temperature The color temperature value.
     * @param brightness The brightness level of the effect.
     */
    void add_ct(uint32_t duration, uint32_t color_temperature, int8_t brightness);

    /**
     * @brief Adds a sleep effect to the flow.
     * 
     * @param duration The duration of the sleep in milliseconds.
     */
    void add_sleep(uint32_t duration);

    /**
     * @brief Adds a custom flow expression to the flow.
     * 
     * @param expression The flow expression to be added.
     */
    void add_expression(flow_expression expression);

    /**
     * @brief Returns the vector of flow expressions representing the flow.
     * 
     * @return The vector of flow expressions.
     */
    std::vector<flow_expression> get_flow();

    /**
     * @brief Clears the flow, removing all flow expressions.
     */
    void clear();

    /**
     * @brief Removes the last flow expression from the flow.
     */
    void remove_last();

    /**
     * @brief Removes the first flow expression from the flow.
     */
    void remove_first();

    /**
     * @brief Removes the flow expression at the specified index.
     * 
     * @param index The index of the flow expression to be removed.
     */
    void remove_at(uint32_t index);

    /**
     * @brief Returns the flow expression at the specified index.
     * 
     * @param index The index of the flow expression.
     * @return The flow expression at the specified index.
     */
    flow_expression operator[](uint32_t index);

    /**
     * @brief Concatenates two flows together.
     * 
     * @param flow The flow to be concatenated.
     * @return The concatenated flow.
     */
    Flow operator+(const Flow &flow);

    /**
     * @brief Concatenates a flow expression to the flow.
     * 
     * @param expression The flow expression to be concatenated.
     * @return The concatenated flow.
     */
    Flow operator+(const flow_expression &expression);

    /**
     * @brief Concatenates a vector of flow expressions to the flow.
     * 
     * @param expressions The vector of flow expressions to be concatenated.
     * @return The concatenated flow.
     */
    Flow operator+(const std::vector<flow_expression> &expressions);

    /**
     * @brief Sets the number of times the flow should be repeated.
     * 
     * @param count The number of times to repeat the flow.
     */
    void set_count(uint8_t count);

    /**
     * @brief Returns the number of times the flow should be repeated.
     * 
     * @return The number of times to repeat the flow.
     */
    uint8_t get_count();

    /**
     * @brief Returns the size of the object.
     *
     * @return The size of the object.
     */
    uint8_t get_size();
};

#endif
