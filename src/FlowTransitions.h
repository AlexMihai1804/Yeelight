/**
 * @file FlowTransitions.h
 * @brief Static methods producing standard flow_expression sequences (disco, pulse, police, etc.).
 */
#ifndef YEELIGHTARDUINO_FLOWTRANSITIONS_H
#define YEELIGHTARDUINO_FLOWTRANSITIONS_H

#include <Flow.h>

/**
 * The FlowTransitions class provides a set of static methods to generate predefined lighting effect
 * sequences represented as a vector of flow_expression. These sequences include various lighting animations
 * with customizable parameters such as duration, brightness, speed, and color configurations.
 */
class FlowTransitions {
public:
    /**
     * Generates a sequence of flow expressions based on the provided beats-per-minute (BPM).
     *
     * @param bpm The beats per minute for generating the flow expressions. Default value is 120.
     * @return A vector containing the generated flow expressions.
     */
    static std::vector<flow_expression> disco(uint8_t bpm = 120);

    /**
     * Generates a temporary list of flow_expression objects based on the given duration.
     *
     * @param duration The time duration in milliseconds (default is 40000). Determines the behavior or scope
     *                 of the created flow expressions.
     *
     * @return A vector of flow_expression objects generated for the specified duration.
     */
    static std::vector<flow_expression> temp(uint16_t duration = 40000);

    /**
     * Generates a series of flow expressions that simulate a strobe effect.
     *
     * The strobe effect creates rapid sequences of on and off states with
     * the given duration for each state.
     *
     * @param duration The duration of each strobe state in milliseconds.
     *                 Defaults to 50 milliseconds.
     * @return A vector of flow_expression objects representing the strobe effect sequence.
     */
    static std::vector<flow_expression> strobe(uint16_t duration = 50);

    /**
     * Generates a sequence of flow expressions for a pulse effect with the specified color and duration.
     *
     * @param r The red component of the color (0-255).
     * @param g The green component of the color (0-255).
     * @param b The blue component of the color (0-255).
     * @param duration Optional parameter. The duration of each pulse in milliseconds. Default is 250.
     * @param brightness Optional parameter. The brightness level of the pulse (0-100). Default is 100.
     * @return A vector containing the flow expressions representing the pulse effect.
     */
    static std::vector<flow_expression> pulse(uint8_t r, uint8_t g, uint8_t b, uint16_t duration = 250,
                                              uint8_t brightness = 100);

    /**
     * Generates a sequence of flow expressions for strobing colors with the specified
     * duration and brightness. The strobe effect alternates colors to create a flashing effect.
     *
     * @param duration The duration of each strobe flash in milliseconds. Default value is 50 milliseconds.
     * @param brightness The brightness level of the strobe effect, represented as a percentage. Default value is 100%.
     * @return A vector containing flow expressions corresponding to the strobe effect.
     */
    static std::vector<flow_expression> strobeColor(uint16_t duration = 50, uint8_t brightness = 100);

    /**
     * Generates a sequence of flow expressions that represent an alarm signal for the specified duration.
     *
     * @param duration The duration of the alarm signal in milliseconds. Defaults to 250 milliseconds if not specified.
     * @return A vector of flow expressions representing the generated alarm signal.
     */
    static std::vector<flow_expression> alarm(uint16_t duration = 250);

    /**
     * Creates a "police" light effect pattern with the specified duration and brightness.
     *
     * The "police" effect alternates light patterns that mimic the appearance of
     * emergency vehicle lights. This method generates a vector of flow_expression
     * instances that define this light effect.
     *
     * @param duration The duration of the effect in milliseconds, default is 300ms.
     *                 This represents the time for one complete cycle of the effect.
     * @param brightness The brightness level of the light effect as a percentage,
     *                   ranging from 0 to 100. The default is 100.
     * @return A vector containing flow_expression objects that define the "police" light effect.
     */
    static std::vector<flow_expression> police(uint16_t duration = 300, uint8_t brightness = 100);

    /**
     * Creates a police-style lighting effect with alternating colors for a given duration and brightness level.
     *
     * @param duration The duration of each flashing cycle in milliseconds. Default value is 250.
     * @param brightness The brightness level of the lights as a percentage (0 to 100). Default value is 100.
     * @return A vector of flow_expression objects representing the police lighting effect configuration.
     */
    static std::vector<flow_expression> police2(uint16_t duration = 250, uint8_t brightness = 100);

    /**
     * Generates a vector of flow_expression objects representing a light pattern sequence described
     * by the Light Sequence Design (LSD).
     *
     * This method creates a predefined series of light states based on the specified sequence duration
     * and brightness level.
     *
     * @param duration The total duration of the sequence in milliseconds. Defaults to 300.
     * @param brightness The brightness level of the lights in the sequence, defined as a percentage (0-100). Defaults to 100.
     * @return A vector containing a sequence of flow_expression objects defining the light pattern.
     */
    static std::vector<flow_expression> lsd(uint16_t duration = 300, uint8_t brightness = 100);

    /**
     * Generates a sequence of flow expressions designed to simulate Christmas-themed effects.
     *
     * @param duration The duration of each flow expression in milliseconds. Default is 250.
     * @param brightness The brightness level of the flow expressions, ranging from 0 to 100. Default is 100.
     * @param sleep The speed at which the flow expressions cycle, in milliseconds. Default is 3000.
     * @return A vector containing the generated Christmas-themed flow expressions.
     */
    static std::vector<flow_expression> christmas(uint16_t duration = 250, uint8_t brightness = 100,
                                                  uint16_t sleep = 3000);

    /**
     * Generates a vector of flow_expression objects representing an RGB light sequence.
     *
     * @param duration The duration of each step in the sequence, in milliseconds. Default is 250ms.
     * @param brightness The brightness level of the light, specified as a percentage (0-100). Default is 100%.
     * @param speed The overall speed of the RGB sequence, in milliseconds. Default is 3000ms.
     *
     * @return A vector of flow_expression objects defining the RGB light sequence.
     */
    static std::vector<flow_expression> rgb(uint16_t duration = 250, uint8_t brightness = 100, uint16_t speed = 3000);

    /**
     * Generates a random loop of flow expressions.
     *
     * This method creates a collection of flow expressions based on the provided
     * duration, brightness, and count parameters. Each flow expression is randomly
     * generated and meant to simulate a dynamic sequence.
     *
     * @param duration The duration (in milliseconds) for each flow expression. Defaults to 750.
     * @param brightness The brightness level (0-255) for the flow expressions. Defaults to 100.
     * @param count The number of flow expressions to generate. Defaults to 9.
     * @return A vector containing the generated flow expressions.
     */
    static std::vector<flow_expression>
    randomLoop(uint16_t duration = 750, uint8_t brightness = 100, uint8_t count = 9);

    /**
     * Generates a sequence of flow expressions for a "slowdown" effect with the specified parameters.
     *
     * @param duration The total duration of the effect in milliseconds. Default value is 2000 ms.
     * @param brightness The brightness level of the effect, ranging from 0 to 100. Default value is 100.
     * @param count The number of flow expressions to generate. Default value is 8.
     * @return A vector of flow expressions representing the "slowdown" effect.
     */
    static std::vector<flow_expression> slowdown(uint16_t duration = 2000, uint8_t brightness = 100, uint8_t count = 8);
};
#endif
