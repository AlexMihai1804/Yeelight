#ifndef FLOWDEFAULT_H
#define FLOWDEFAULT_H
#include <Flow.h>

/**
 * @class FlowDefault
 * @brief Provides a set of predefined lighting modes and effects.
 *
 * This class defines various static functions to create predefined lighting effects.
 * The effects utilize parameters such as duration, brightness, and color to customize
 * the visual presentation. These functions return instances of the Flow class to execute
 * the effect configuration.
 */
class FlowDefault {
    /**
     * Creates a Flow instance with a disco configuration based on the provided beats per minute (BPM).
     *
     * @param bpm The beats per minute for the disco flow. Defaults to 120 BPM if not specified.
     * @return A Flow object set up with the disco configuration.
     */
public:
    static Flow disco(uint8_t bpm = 120);

    /**
     * Creates a temporary Flow object with a specified duration.
     *
     * @param duration The duration for which the Flow object remains valid, in milliseconds.
     *                 Defaults to 40000 milliseconds.
     * @return A temporary Flow object configured with the given duration.
     */
    static Flow temp(uint16_t duration = 40000);

    /**
     * @brief Creates a strobe effect with a specified duration.
     *
     * This function returns a Flow object that represents a strobe effect.
     * The strobe alternates rapidly between states over the defined duration.
     * By default, the duration is set to 50 if no value is provided.
     *
     * @param duration Duration of a single strobe cycle in milliseconds. Defaults to 50.
     * @return Flow A Flow object representing the strobe effect.
     */
    static Flow strobe(uint16_t duration = 50);

    /**
     * Creates a pulsing effect with the specified color, brightness, duration, and repetitions.
     *
     * @param r The red component of the color (0-255).
     * @param g The green component of the color (0-255).
     * @param b The blue component of the color (0-255).
     * @param duration The duration of each pulse in milliseconds. Default is 250 ms.
     * @param brightness The brightness level of the effect (0-100). Default is 100.
     * @param count The number of pulse repetitions. Default is 1.
     * @return A Flow object representing the configured pulsing effect.
     */
    static Flow pulse(uint8_t r, uint8_t g, uint8_t b, uint16_t duration = 250, uint8_t brightness = 100,
                      uint16_t count = 1);

    /**
     * Generates a strobe effect with a specified color, duration, and brightness.
     *
     * @param duration The duration of each strobe in milliseconds. Defaults to 50.
     * @param brightness The brightness level of the strobe effect, ranging from 0 to 100. Defaults to 100.
     * @return Returns a Flow object representing the configured strobe effect.
     */
    static Flow strobeColor(uint16_t duration = 50, uint8_t brightness = 100);

    /**
     * Generates an alarm with a specified duration.
     *
     * This method triggers an alarm for a defined duration in milliseconds.
     * If no duration is provided, a default value of 250 milliseconds is used.
     *
     * @param duration The duration of the alarm in milliseconds. Defaults to 250.
     * @return A Flow object representing the state of the alarm.
     */
    static Flow alarm(uint16_t duration = 250);

    /**
     * Applies a police light effect to a flow with specified duration and brightness.
     * The effect alternates colors to mimic a police siren light pattern.
     *
     * @param duration The duration of each cycle in milliseconds. Defaults to 300.
     * @param brightness The brightness level of the effect, ranging from 0 to 100. Defaults to 100.
     * @return A Flow object representing the police light effect configuration.
     */
    static Flow police(uint16_t duration = 300, uint8_t brightness = 100);

    /**
     * Creates a "police" flashing light effect with specified duration and brightness settings.
     *
     * @param duration The time in milliseconds for each light pattern cycle. Default is 250ms.
     * @param brightness The intensity of the light as a percentage (0-100). Default is 100.
     * @return A Flow object representing the configured lighting effect.
     */
    static Flow police2(uint16_t duration = 250, uint8_t brightness = 100);

    /**
     * Applies a lighting sequence with specified duration and brightness.
     *
     * This method initiates a flow sequence for lighting effects, with a configurable
     * duration and brightness level. The sequence is predefined and can be used in
     * scenarios requiring aesthetic lighting controls.
     *
     * @param duration The duration of the lighting sequence in milliseconds.
     *        Defaults to 300 if not specified.
     * @param brightness The brightness level of the lighting sequence. A percentage value
     *        ranging from 0 to 100. Defaults to 100 if not specified.
     * @return A Flow object representing the initiated lighting sequence.
     */
    static Flow lsd(uint16_t duration = 300, uint8_t brightness = 100);

    /**
     * Creates and returns a `Flow` object configured with the Christmas theme effect.
     * The Christmas effect alternates between red and green colors with specified parameters for duration,
     * brightness, and speed to create a festive lighting display.
     *
     * @param duration The duration (in milliseconds) for which each color is displayed before switching. Default is 250 ms.
     * @param brightness The brightness level of the colors, ranging from 0 to 100. Default is 100.
     * @param speed The speed (in milliseconds) that determines the transition rate of the effect. Default is 3000 ms.
     * @return A `Flow` object configured with the specified parameters for the Christmas lighting effect.
     */
    static Flow christmas(uint16_t duration = 250, uint8_t brightness = 100, uint16_t speed = 3000);

    /**
     * Creates an RGB flow effect with specified parameters.
     *
     * @param duration The duration of each color transition in milliseconds.
     *                 Defaults to 250 milliseconds if not specified.
     * @param brightness The brightness level of the effect, ranging from 0 to 100.
     *                   Defaults to 100 if not specified.
     * @param sleep The sleep duration in milliseconds before the effect restarts.
     *              Defaults to 3000 milliseconds if not specified.
     * @return A Flow object configured with the specified RGB settings.
     */
    static Flow rgb(uint16_t duration = 250, uint8_t brightness = 100, uint16_t sleep = 3000);

    /**
     * Generates a randomized flow pattern in a loop with configurable duration, brightness, and count.
     *
     * @param duration The duration of each cycle in milliseconds. Defaults to 750 if not specified.
     * @param brightness The brightness level of the flow, ranging from 0 to 255. Defaults to 100 if not specified.
     * @param count The number of iterations or elements in the flow. Defaults to 9 if not specified.
     * @return A Flow object representing the randomized flow pattern.
     */
    static Flow randomLoop(uint16_t duration = 750, uint8_t brightness = 100, uint16_t count = 9);

    /**
     * Creates a Flow object that simulates a slowdown effect with configurable duration, brightness, and count.
     *
     * @param duration The duration of the slowdown effect in milliseconds. Defaults to 2000 if not specified.
     * @param brightness The brightness level as a percentage (0-100). Defaults to 100 if not specified.
     * @param count The number of iterations or steps in the slowdown effect. Defaults to 8 if not specified.
     * @return A Flow object representing the configured slowdown effect.
     */
    static Flow slowdown(uint16_t duration = 2000, uint8_t brightness = 100, uint16_t count = 8);

    /**
     * Resets the device state to home configuration with specified settings for duration and brightness.
     *
     * @param duration The time duration in milliseconds for setting the device to the home configuration. Default is 500 milliseconds.
     * @param brightness The brightness level as a percentage (0-100). Default is 80%.
     * @return A Flow object representing the result of the home configuration process.
     */
    static Flow home(uint16_t duration = 500, uint8_t brightness = 80);

    /**
     * Activates the night mode by adjusting the brightness and setting a duration for the effect.
     *
     * @param duration Specifies the duration of the night mode effect in milliseconds.
     *                 Defaults to 500 if not provided.
     * @param brightness Specifies the brightness level for night mode.
     *                   Defaults to 1 if not provided.
     * @return Returns a Flow object representing the configured night mode.
     */
    static Flow nightMode(uint16_t duration = 500, uint8_t brightness = 1);

    /**
     * Creates and returns a Flow object configured for a "date night" lighting theme.
     *
     * This method initializes a lighting flow with a default or specified duration and brightness
     * level commonly used for a romantic or cozy atmosphere.
     *
     * @param duration The duration of the lighting flow in milliseconds. Defaults to 500 if not specified.
     * @param brightness The brightness level of the lighting flow, ranging from 0 to 100. Defaults to 50 if not specified.
     * @return A Flow object representing the configured "date night" light flow.
     */
    static Flow dateNight(uint16_t duration = 500, uint8_t brightness = 50);

    /**
     * Generates a Flow object configured to mimic a movie-like lighting effect.
     *
     * @param duration The duration of the movie effect in milliseconds. Default value is 500.
     * @param brightness The brightness level of the effect, ranging from 0 to 100. Default value is 50.
     * @return A Flow object configured with the specified duration and brightness settings for the movie effect.
     */
    static Flow movie(uint16_t duration = 500, uint8_t brightness = 50);

    /**
     * Returns a Flow object that represents the sunrise state or behavior.
     *
     * @return A Flow object encapsulating the sunrise representation.
     */
    static Flow sunrise();

    /**
     * @brief Creates and returns a Flow object representing the "sunset" state.
     *
     * This method statically generates a Flow instance configured to encapsulate
     * behavior or properties associated with a "sunset" state. This may involve
     * specific color schemes, patterns, or contextual data related to sunset.
     *
     * @return A Flow object representing the "sunset" state.
     */
    static Flow sunset();

    /**
     * Generates and returns a Flow object representing a romantic storyline or theme.
     *
     * This method encapsulates logic to construct a Flow specifically tailored for
     * representing romance, potentially involving themes of affection, relationships,
     * and emotional connections.
     *
     * @return A Flow object that represents a romantic theme.
     */
    static Flow romance();

    /**
     * Generates a Flow object representing the "Happy Birthday" sequence.
     *
     * This method is used to create a specific Flow that corresponds
     * to the "Happy Birthday" sequence. The implementation details
     * and the exact structure of the Flow are determined by the context
     * of the application where this method is used.
     *
     * @return A Flow object representing the "Happy Birthday" sequence.
     */
    static Flow happyBirthday();

    /**
     * Generates a Flow object representing a flickering candle effect.
     *
     * This method encapsulates logic to create a realistic simulation
     * of a flickering candle, which can be used in graphical or light control applications.
     *
     * @return A Flow object that simulates the flickering of a candle.
     */
    static Flow candleFlicker();

    /**
     * Prepares and returns a Flow object to simulate a tea time lighting effect.
     *
     * This method creates a lighting flow effect that resembles a calm and cozy tea time atmosphere.
     * The duration and brightness can be adjusted through the parameters to customize the effect.
     *
     * @param duration The duration of the flow effect in milliseconds. Defaults to 500 ms.
     * @param brightness The brightness level of the effect as a percentage (0-100). Defaults to 50.
     * @return A Flow object representing the tea time lighting effect.
     */
    static Flow teaTime(uint16_t duration = 500, uint8_t brightness = 50);
};
#endif
