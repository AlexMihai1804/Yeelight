/**
 * @file Yeelight.h
 * @brief Header file for the Yeelight class and related structures and enums.
 * 
 * This file defines the Yeelight class, which provides methods for controlling Yeelight devices.
 * It also includes various structures and enums used by the Yeelight class.
 */
#ifndef YEELIGHTARDUINO_YEELIGHT_H
#define YEELIGHTARDUINO_YEELIGHT_H

#include <cJSON.h>
#include <cstdint>
#include <WiFiClient.h>
#include <vector>
#include <Flow.h>
#include <Yeelight_enums.h>
#include <Yeelight_structs.h>

/**
 * @class Yeelight
 * @brief Represents a Yeelight device.
 *
 * The Yeelight class provides methods to control and interact with a Yeelight device.
 * It allows you to control various aspects of the device such as power, brightness, color, and more.
 * You can also discover Yeelight devices on the network and retrieve information about their supported methods.
 */
class Yeelight {
private:
    uint8_t ip[4]{}; // The IP address of the Yeelight device
    uint16_t port; // The port number to connect to
    WiFiClient client; // The client object for establishing a connection
    SupportedMethods supported_methods; // The supported methods of the Yeelight device
    uint16_t timeout = 5000; // The timeout value for communication with the device
    uint8_t max_retry = 3; // The maximum number of retries for failed commands
    YeelightProperties properties; // The properties of the Yeelight device
    /**
     * Parses the discovery response and returns a YeelightDevice object.
     *
     * @param response The discovery response to parse.
     * @return A YeelightDevice object representing the parsed response.
     */
    static YeelightDevice parseDiscoveryResponse(const char *response);

    /**
     * Sends a start_cf command to the Yeelight device.
     *
     * @param count The number of times to repeat the flow.
     * @param action The action to be performed in the flow.
     * @param size The size of the flow expression.
     * @param flow The flow expression.
     * @return The response type of the command.
     */
    ResponseType start_cf_command(uint8_t count, flow_action action, uint8_t size, const flow_expression *flow);

    /**
     * Sends a stop_cf command to the Yeelight device.
     *
     * @return The response type of the command.
     */
    ResponseType stop_cf_command();

    /**
     * Establishes a connection with the Yeelight device.
     */
    ResponseType connect();

    /**
     * Sends a command to the Yeelight device.
     *
     * @param method The method of the command.
     * @param params The parameters of the command.
     * @return The response type of the command.
     */

    ResponseType send_command(const char *method, cJSON *params);

    /**
     * Sends a bg_set_power command to the Yeelight device.
     *
     * @param power The power state to set.
     * @param effect The effect to apply.
     * @param duration The duration of the effect.
     * @param mode The mode to set.
     * @return The response type of the command.
     */
    ResponseType
    bg_set_power_command(bool power, effect effect = EFFECT_SMOOTH, uint16_t duration = 500, mode mode = MODE_CURRENT);

    /**
     * Sends a set_power command to the Yeelight device.
     *
     * @param power The power state to set.
     * @param effect The effect to apply.
     * @param duration The duration of the effect.
     * @param mode The mode to set.
     * @return The response type of the command.
     */
    ResponseType
    set_power_command(bool power, effect effect = EFFECT_SMOOTH, uint16_t duration = 500, mode mode = MODE_CURRENT);

    /**
     * Sends a bg_toggle command to the Yeelight device.
     *
     * @return The response type of the command.
     */
    ResponseType bg_toggle_command();

    /**
     * Sends a toggle command to the Yeelight device.
     *
     * @return The response type of the command.
     */
    ResponseType toggle_command();

    /**
     * Sends a dev_toggle command to the Yeelight device.
     *
     * @return The response type of the command.
     */
    ResponseType dev_toggle_command();

    /**
     * Checks the response from the Yeelight device.
     *
     * @return The response type of the command.
     */
    ResponseType checkResponse();

    /**
     * Sends a set_ct_abx command to the Yeelight device.
     *
     * @param ct_value The color temperature value to set.
     * @param effect The effect to apply.
     * @param duration The duration of the effect.
     * @return The response type of the command.
     */
    ResponseType set_ct_abx_command(uint16_t ct_value, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * Sends a set_scene_ct command to the Yeelight device.
     *
     * @param ct The color temperature to set.
     * @param bright The brightness to set.
     * @return The response type of the command.
     */
    ResponseType set_scene_ct_command(uint16_t ct, uint8_t bright);

    /**
     * Sends a bg_set_ct_abx command to the Yeelight device.
     *
     * @param ct_value The color temperature value to set.
     * @param effect The effect to apply.
     * @param duration The duration of the effect.
     * @return The response type of the command.
     */
    ResponseType bg_set_ct_abx_command(uint16_t ct_value, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * Sends a bg_set_scene_ct command to the Yeelight device.
     *
     * @param ct The color temperature to set.
     * @param bright The brightness to set.
     * @return The response type of the command.
     */
    ResponseType bg_set_scene_ct_command(uint16_t ct, uint8_t bright);

    /**
     * Sends a set_scene_rgb command to the Yeelight device.
     *
     * @param r The red value to set.
     * @param g The green value to set.
     * @param b The blue value to set.
     * @param bright The brightness to set.
     * @return The response type of the command.
     */
    ResponseType set_scene_rgb_command(uint8_t r, uint8_t g, uint8_t b, uint8_t bright);

    /**
     * Sends a bg_set_rgb command to the Yeelight device.
     *
     * @param r The red value to set.
     * @param g The green value to set.
     * @param b The blue value to set.
     * @param effect The effect to apply.
     * @param duration The duration of the effect.
     * @return The response type of the command.
     */
    ResponseType
    bg_set_rgb_command(uint8_t r, uint8_t g, uint8_t b, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * Sends a bg_set_scene_rgb command to the Yeelight device.
     *
     * @param r The red value to set.
     * @param g The green value to set.
     * @param b The blue value to set.
     * @param bright The brightness to set.
     * @return The response type of the command.
     */
    ResponseType bg_set_scene_rgb_command(uint8_t r, uint8_t g, uint8_t b, uint8_t bright);

    /**
     * Sends a set_rgb command to the Yeelight device.
     *
     * @param r The red value to set.
     * @param g The green value to set.
     * @param b The blue value to set.
     * @param effect The effect to apply.
     * @param duration The duration of the effect.
     * @return The response type of the command.
     */
    ResponseType
    set_rgb_command(uint8_t r, uint8_t g, uint8_t b, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * Sends a set_bright command to the Yeelight device.
     *
     * @param bright The brightness to set.
     * @param effect The effect to apply.
     * @param duration The duration of the effect.
     * @return The response type of the command.
     */
    ResponseType set_bright_command(uint8_t bright, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * Sends a bg_set_bright command to the Yeelight device.
     *
     * @param bright The brightness to set.
     * @param effect The effect to apply.
     * @param duration The duration of the effect.
     * @return The response type of the command.
     */
    ResponseType bg_set_bright_command(uint8_t bright, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * Sends a set_scene_hsv command to the Yeelight device.
     *
     * @param hue The hue value to set.
     * @param sat The saturation value to set.
     * @param bright The brightness to set.
     * @return The response type of the command.
     */
    ResponseType set_scene_hsv_command(uint8_t hue, uint8_t sat, uint8_t bright);

    /**
     * Sends a set_hsv command to the Yeelight device.
     *
     * @param hue The hue value to set.
     * @param sat The saturation value to set.
     * @param effect The effect to apply.
     * @param duration The duration of the effect.
     * @return The response type of the command.
     */
    ResponseType set_hsv_command(uint16_t hue, uint8_t sat, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * Sends a bg_set_hsv command to the Yeelight device.
     *
     * @param hue The hue value to set.
     * @param sat The saturation value to set.
     * @param effect The effect to apply.
     * @param duration The duration of the effect.
     * @return The response type of the command.
     */
    ResponseType bg_set_hsv_command(uint16_t hue, uint8_t sat, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * Sends a bg_set_scene_hsv command to the Yeelight device.
     *
     * @param hue The hue value to set.
     * @param sat The saturation value to set.
     * @param bright The brightness to set.
     * @return The response type of the command.
     */
    ResponseType bg_set_scene_hsv_command(uint8_t hue, uint8_t sat, uint8_t bright);

    /**
     * Sends a set_scene_auto_delay_off command to the Yeelight device.
     *
     * @param brightness The brightness to set.
     * @param duration The duration of the delay.
     * @return The response type of the command.
     */
    ResponseType set_scene_auto_delay_off_command(uint8_t brightness, uint32_t duration);

    /**
     * Sends a bg_set_scene_auto_delay_off command to the Yeelight device.
     *
     * @param brightness The brightness to set.
     * @param duration The duration of the delay.
     * @return The response type of the command.
     */
    ResponseType bg_set_scene_auto_delay_off_command(uint8_t brightness, uint32_t duration);

    /**
     * Sends a cron_add command to the Yeelight device.
     *
     * @param time The time to add to the cron.
     * @return The response type of the command.
     */
    ResponseType cron_add_command(uint32_t time);

    /**
     * Sends a cron_del command to the Yeelight device.
     *
     * @return The response type of the command.
     */
    ResponseType cron_del_command();

    /**
     * Sends a set_default command to the Yeelight device.
     *
     * @return The response type of the command.
     */
    ResponseType set_default();

    /**
     * Sends a bg_set_default command to the Yeelight device.
     *
     * @return The response type of the command.
     */
    ResponseType bg_set_default();

    /**
     * Sends a set_name command to the Yeelight device.
     *
     * @param name The name to set.
     * @return The response type of the command.
     */
    ResponseType set_name_command(const char *name);

    /**
     * Sends an adjust_bright command to the Yeelight device.
     *
     * @param percentage The percentage to adjust the brightness.
     * @param duration The duration of the adjustment.
     * @return The response type of the command.
     */
    ResponseType adjust_bright_command(int8_t percentage, uint16_t duration);

    /**
     * Sends an adjust_ct command to the Yeelight device.
     *
     * @param percentage The percentage to adjust the color temperature.
     * @param duration The duration of the adjustment.
     * @return The response type of the command.
     */
    ResponseType adjust_ct_command(int8_t percentage, uint16_t duration);

    /**
     * Sends an adjust_color command to the Yeelight device.
     *
     * @param percentage The percentage to adjust the color.
     * @param duration The duration of the adjustment.
     * @return The response type of the command.
     */
    ResponseType adjust_color_command(int8_t percentage, uint16_t duration);

    /**
     * Sends a bg_adjust_bright command to the Yeelight device.
     *
     * @param percentage The percentage to adjust the brightness.
     * @param duration The duration of the adjustment.
     * @return The response type of the command.
     */
    ResponseType bg_adjust_bright_command(int8_t percentage, uint16_t duration);

    /**
     * Sends a bg_adjust_ct command to the Yeelight device.
     *
     * @param percentage The percentage to adjust the color temperature.
     * @param duration The duration of the adjustment.
     * @return The response type of the command.
     */
    ResponseType bg_adjust_ct_command(int8_t percentage, uint16_t duration);

    /**
     * Sends a bg_adjust_color command to the Yeelight device.
     *
     * @param percentage The percentage to adjust the color.
     * @param duration The duration of the adjustment.
     * @return The response type of the command.
     */
    ResponseType bg_adjust_color_command(int8_t percentage, uint16_t duration);

    /**
     * Starts a color flow command in the background.
     *
     * This function sends a command to start a color flow in the background to the Yeelight device.
     * The color flow is a series of predefined or custom light effects that can be played on the device.
     *
     * @param count The number of times the flow should be repeated. Use 0 to repeat indefinitely.
     * @param action The action to be performed at the end of the flow.
     * @param size The size of the flow expression array.
     * @param flow An array of flow expressions that define the color flow.
     *
     * @return The response type indicating the success or failure of the command.
     */
    ResponseType bg_start_cf_command(uint8_t count, flow_action action, uint8_t size, const flow_expression *flow);

    /**
     * Stops the color flow effect on the Yeelight device.
     *
     * @return The response type indicating the success or failure of the command.
     */
    ResponseType bg_stop_cf_command();

    /**
     * Sets the scene CF (Color Flow) command.
     *
     * This function allows you to set the scene CF command for controlling the Yeelight device's color flow.
     *
     * @param count The number of times the color flow should be executed.
     * @param action The action to be performed in the color flow.
     * @param size The size of the flow expression.
     * @param flow A pointer to the flow expression array.
     * @return The response type indicating the success or failure of the command.
     */
    ResponseType set_scene_cf_command(uint32_t count, flow_action action, uint32_t size, const flow_expression *flow);

    /**
     * Sets the background light to a specific scene using the CF (Color Flow) command.
     *
     * @param count The total number of commands in the flow.
     * @param action The action to be performed after the flow is executed.
     * @param size The size of the flow expression.
     * @param flow An array of flow expressions that define the flow.
     * @return The response type indicating the success or failure of the command.
     */
    ResponseType bg_set_scene_cf_command(uint32_t count, flow_action action, uint32_t size, const flow_expression *flow);

public:
    /**
     * Discovers Yeelight devices on the network.
     *
     * This function scans the network for Yeelight devices and returns a vector of YeelightDevice objects
     * representing the discovered devices.
     *
     * @param waitTimeMs The time to wait for device discovery, in milliseconds. Default is 5000ms.
     * @return A vector of YeelightDevice objects representing the discovered devices.
     */
    static std::vector<YeelightDevice> discoverYeelightDevices(int waitTimeMs = 5000);

    /**
     * @brief Constructs a Yeelight object with the specified IP address and port.
     *
     * @param ip The IP address of the Yeelight device as an array of 4 bytes.
     * @param port The port number to connect to the Yeelight device. Default is 55443.
     */
    explicit Yeelight(const uint8_t ip[4], uint16_t port = 55443);

    /**
     * @brief Constructs a Yeelight object with the specified device.
     *
     * @param device The YeelightDevice object representing the device to connect to.
     */
    explicit Yeelight(const YeelightDevice &device);

    /**
     * @brief Constructs a new Yeelight object.
     */
    Yeelight();

    /**
     * @brief Connects to the Yeelight device at the specified IP address and port.
     *
     * This function establishes a connection with the Yeelight device at the given IP address and port.
     *
     * @param ip The IP address of the Yeelight device as an array of 4 bytes.
     * @param port The port number to connect to. The default value is 55443.
     * @return The response type indicating the success or failure of the connection.
     */
    ResponseType connect(const uint8_t ip[4], uint16_t port = 55443);

    /**
     * Establishes a connection with the specified Yeelight device.
     *
     * @param device The Yeelight device to connect to.
     * @return The response type indicating the success or failure of the connection.
     */
    ResponseType connect(const YeelightDevice &device);

    /**
     * Checks if the Yeelight device is connected.
     *
     * @return true if the device is connected, false otherwise.
     */
    bool is_connected();

    /**
     * Retrieves the supported methods of the Yeelight device.
     *
     * @return The supported methods as an instance of the SupportedMethods enum.
     */
    SupportedMethods getSupportedMethods() const;

    /**
     * Refreshes the list of supported methods for the Yeelight device.
     * This function sends a request to the device to retrieve the updated list of supported methods.
     * After calling this function, the list of supported methods can be accessed using the appropriate getter function.
     */
    void refreshSupportedMethods();

    /**
     * Refreshes the properties of the Yeelight device.
     *
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType refreshProperties();

    /**
     * Retrieves the properties of the Yeelight device.
     *
     * @return The YeelightProperties object containing the device properties.
     */
    YeelightProperties getProperties();

    /**
     * @brief Destructor for the Yeelight class.
     */
    ~Yeelight();

    /**
     * Starts a flow on the Yeelight device.
     *
     * @param flow The flow to be started.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType start_flow(Flow flow, LightType lightType = AUTO);

    /**
     * Stops the flow effect on the Yeelight device.
     *
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType stop_flow(LightType lightType = AUTO);

    /**
     * Sets the power state of the Yeelight device.
     *
     * @param power The desired power state. Set to `true` to turn on the device, or `false` to turn it off.
     * @param lightType The type of light to control. Use the `LightType` enum to specify the light type.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_power(bool power, LightType lightType = AUTO);

    /**
     * Sets the power state of the Yeelight device.
     *
     * @param power The desired power state (true for on, false for off).
     * @param effect The effect to apply when changing the power state.
     * @param lightType The type of light to control (optional, default is AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_power(bool power, effect effect, LightType lightType = AUTO);

    /**
     * Sets the power state of the Yeelight device.
     *
     * @param power The desired power state (true for on, false for off).
     * @param effect The effect to apply when changing the power state.
     * @param duration The duration of the effect in milliseconds.
     * @param lightType The type of light to control (optional, defaults to AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_power(bool power, effect effect, uint16_t duration, LightType lightType = AUTO);

    /**
     * Sets the power state of the Yeelight device.
     *
     * @param power The desired power state (true for on, false for off).
     * @param mode The mode in which the power state should be set.
     * @param lightType The type of light to control (optional, default is AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_power(bool power, mode mode, LightType lightType = AUTO);

    /**
     * Sets the power state of the Yeelight device.
     *
     * @param power The desired power state (true for on, false for off).
     * @param effect The effect to apply when changing the power state.
     * @param mode The mode to set the device to.
     * @param lightType The type of light to control (optional, defaults to AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_power(bool power, effect effect, mode mode, LightType lightType = AUTO);

    /**
     * Sets the power state of the Yeelight device.
     *
     * @param power     The desired power state (true for on, false for off).
     * @param effect    The effect to apply when changing the power state.
     * @param duration  The duration of the effect in milliseconds.
     * @param mode      The mode to set the device to.
     * @param lightType The type of light to control.
     * @return          The response type indicating the success or failure of the operation.
     */
    ResponseType set_power(bool power, effect effect, uint16_t duration, mode mode, LightType lightType = AUTO);

    /**
     * Toggles the power state of the Yeelight device.
     *
     * @param lightType The type of light to toggle power for. Defaults to AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType toggle_power(LightType lightType = AUTO);

    /**
     * Turns on the Yeelight device.
     *
     * @param lightType The type of light to turn on. Defaults to AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType turn_on(LightType lightType = AUTO);

    /**
     * Turns on the Yeelight device with the specified effect and light type.
     *
     * @param effect The effect to be applied when turning on the device.
     * @param lightType The type of light to be used. Defaults to AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType turn_on(effect effect, LightType lightType = AUTO);

    /**
     * Turns on the Yeelight device with the specified effect and duration.
     *
     * @param effect The effect to apply when turning on the light.
     * @param duration The duration of the effect in milliseconds.
     * @param lightType The type of light to turn on (optional, default is AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType turn_on(effect effect, uint16_t duration, LightType lightType = AUTO);

    /**
     * Turns on the Yeelight device with the specified mode and light type.
     *
     * @param mode The mode to set for the Yeelight device.
     * @param lightType The type of light to use (optional, default is AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType turn_on(mode mode, LightType lightType = AUTO);

    /**
     * Turns on the Yeelight device with the specified effect, mode, and light type.
     *
     * @param effect The effect to apply when turning on the device.
     * @param mode The mode to set for the device.
     * @param lightType The type of light to use (optional, defaults to AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType turn_on(effect effect, mode mode, LightType lightType = AUTO);

    /**
     * Turns on the Yeelight device with the specified effect, duration, mode, and light type.
     *
     * @param effect The effect to apply when turning on the device.
     * @param duration The duration of the effect in milliseconds.
     * @param mode The mode to set for the device.
     * @param lightType The type of light to use (optional, defaults to AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType turn_on(effect effect, uint16_t duration, mode mode, LightType lightType = AUTO);

    /**
     * Turns off the specified light.
     *
     * @param lightType The type of light to turn off. Defaults to AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType turn_off(LightType lightType = AUTO);

    /**
     * Turns off the Yeelight device.
     *
     * @param effect The effect to apply when turning off the light. Default is `AUTO`.
     * @param lightType The type of light to turn off. Default is `AUTO`.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType turn_off(effect effect, LightType lightType = AUTO);

    /**
     * Turns off the Yeelight device.
     *
     * @param effect The effect to apply when turning off the light.
     * @param duration The duration of the effect in milliseconds.
     * @param lightType The type of light to turn off. Defaults to AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType turn_off(effect effect, uint16_t duration, LightType lightType = AUTO);

    /**
     * Turns off the Yeelight device.
     *
     * @param mode The mode to use for turning off the device.
     * @param lightType The type of light to turn off (optional, default is AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType turn_off(mode mode, LightType lightType = AUTO);

    /**
     * Turns off the Yeelight device.
     *
     * @param effect The effect to apply when turning off the light.
     * @param mode The mode to use when turning off the light.
     * @param lightType The type of light to turn off. Defaults to AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType turn_off(effect effect, mode mode, LightType lightType = AUTO);

    /**
     * Turns off the Yeelight device with the specified effect, duration, mode, and light type.
     *
     * @param effect The effect to apply when turning off the device.
     * @param duration The duration of the effect in milliseconds.
     * @param mode The mode to use when turning off the device.
     * @param lightType The type of light to turn off. Defaults to AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType turn_off(effect effect, uint16_t duration, mode mode, LightType lightType = AUTO);

    /**
     * Sets the color temperature of the Yeelight device.
     *
     * @param ct_value The desired color temperature value.
     * @param lightType The type of light to set the color temperature for. Default is AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_color_temp(uint16_t ct_value, LightType lightType = AUTO);

    /**
     * Sets the color temperature of the Yeelight device.
     *
     * @param ct_value The color temperature value to set.
     * @param effect The effect to apply when setting the color temperature.
     * @param lightType The type of light to set the color temperature for (optional, default is AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_color_temp(uint16_t ct_value, effect effect, LightType lightType = AUTO);

    /**
     * Sets the color temperature of the Yeelight device.
     *
     * @param ct_value The color temperature value to set.
     * @param effect The effect to apply when setting the color temperature.
     * @param duration The duration of the effect in milliseconds.
     * @param lightType The type of light to set the color temperature for (optional, default is AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_color_temp(uint16_t ct_value, effect effect, uint16_t duration, LightType lightType = AUTO);

    /**
     * Sets the color temperature and brightness of the Yeelight device.
     *
     * @param ct_value The color temperature value to set.
     * @param bright The brightness value to set.
     * @param lightType The type of light to set. Default is AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_color_temp(uint16_t ct_value, uint8_t bright, LightType lightType = AUTO);

    /**
     * @brief Sets the brightness of the Yeelight device.
     *
     * This function allows you to set the brightness of the Yeelight device to a specific value.
     * The brightness value should be in the range of 0 to 100, where 0 represents the minimum brightness
     * and 100 represents the maximum brightness.
     *
     * @param bright The brightness value to set (0-100).
     * @param lightType The type of light to set the brightness for (optional, default is AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_brightness(uint8_t bright, LightType lightType = AUTO);

    /**
     * Sets the brightness of the Yeelight device.
     *
     * @param bright The brightness value to set (0-100).
     * @param effect The effect to apply to the brightness change.
     * @param lightType The type of light to set the brightness for (optional, default is AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_brightness(uint8_t bright, effect effect, LightType lightType = AUTO);

    /**
     * Sets the brightness of the Yeelight device.
     *
     * @param bright The brightness value to set (0-100).
     * @param effect The effect to apply to the brightness change.
     * @param duration The duration of the effect in milliseconds.
     * @param lightType The type of light to apply the brightness change to (default: AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_brightness(uint8_t bright, effect effect, uint16_t duration, LightType lightType = AUTO);

    /**
     * Sets the RGB color of the Yeelight device.
     *
     * @param r The red component of the RGB color (0-255).
     * @param g The green component of the RGB color (0-255).
     * @param b The blue component of the RGB color (0-255).
     * @param lightType The type of light to set the color for (default: AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_rgb_color(uint8_t r, uint8_t g, uint8_t b, LightType lightType = AUTO);

    /**
     * Sets the RGB color of the Yeelight device.
     *
     * @param r The red component of the RGB color (0-255).
     * @param g The green component of the RGB color (0-255).
     * @param b The blue component of the RGB color (0-255).
     * @param effect The effect to apply to the color change.
     * @param lightType The type of light to apply the color change to (optional, default is AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_rgb_color(uint8_t r, uint8_t g, uint8_t b, effect effect, LightType lightType = AUTO);

    /**
     * Sets the RGB color of the Yeelight device.
     *
     * @param r The red component of the RGB color (0-255).
     * @param g The green component of the RGB color (0-255).
     * @param b The blue component of the RGB color (0-255).
     * @param effect The effect to apply to the color change.
     * @param duration The duration of the color change in milliseconds.
     * @param lightType The type of light to apply the color change to (default is AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType
    set_rgb_color(uint8_t r, uint8_t g, uint8_t b, effect effect, uint16_t duration, LightType lightType = AUTO);

    /**
     * Sets the RGB color and brightness of the Yeelight device.
     *
     * @param r The red component of the RGB color (0-255).
     * @param g The green component of the RGB color (0-255).
     * @param b The blue component of the RGB color (0-255).
     * @param bright The brightness level (0-100).
     * @param lightType The type of light to control (default: AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_rgb_color(uint8_t r, uint8_t g, uint8_t b, uint8_t bright, LightType lightType = AUTO);

    /**
     * Sets the color of the Yeelight device using HSV (Hue, Saturation, Value) values.
     *
     * @param hue The hue value (0-65535) representing the color.
     * @param sat The saturation value (0-100) representing the intensity of the color.
     * @param lightType The type of light to set the color for. Default is AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_hsv_color(uint16_t hue, uint8_t sat, LightType lightType = AUTO);

    /**
     * Sets the color of the Yeelight device using HSV (Hue, Saturation, Value) values.
     *
     * @param hue The hue value (0-65535) representing the color.
     * @param sat The saturation value (0-100) representing the intensity of the color.
     * @param effect The effect to be applied to the color change.
     * @param lightType The type of light to apply the color change to (optional, default is AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_hsv_color(uint16_t hue, uint8_t sat, effect effect, LightType lightType = AUTO);

    /**
     * Sets the color of the Yeelight device using HSV color model.
     *
     * @param hue The hue value (0-359) representing the color.
     * @param sat The saturation value (0-100) representing the intensity of the color.
     * @param effect The effect to be applied to the color change.
     * @param duration The duration of the color change in milliseconds.
     * @param lightType The type of light to apply the color change to (default: AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_hsv_color(uint16_t hue, uint8_t sat, effect effect, uint16_t duration, LightType lightType = AUTO);

    /**
     * Sets the color of the Yeelight device using HSV (Hue, Saturation, Value) values.
     *
     * @param hue The hue value (0-65535) representing the color.
     * @param sat The saturation value (0-100) representing the intensity of the color.
     * @param bright The brightness value (0-100) representing the brightness of the color.
     * @param lightType The type of light to set the color for. Default is AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_hsv_color(uint16_t hue, uint8_t sat, uint8_t bright, LightType lightType = AUTO);

    /**
     * Sets the scene to a specific RGB color and brightness.
     *
     * @param r The red component of the RGB color (0-255).
     * @param g The green component of the RGB color (0-255).
     * @param b The blue component of the RGB color (0-255).
     * @param bright The brightness level (0-100).
     * @param lightType The type of light to set the scene for (optional, default is AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_scene_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t bright, LightType lightType = AUTO);

    /**
     * Sets the scene of the Yeelight device to the specified HSV values.
     *
     * @param hue The hue value (0-359) representing the color.
     * @param sat The saturation value (0-100) representing the intensity of the color.
     * @param bright The brightness value (0-100) representing the brightness of the color.
     * @param lightType The type of light to set the scene for. Defaults to AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_scene_hsv(uint16_t hue, uint8_t sat, uint8_t bright, LightType lightType = AUTO);

    /**
     * Sets the scene color temperature for the Yeelight device.
     *
     * @param ct The color temperature value to set.
     * @param bright The brightness value to set.
     * @param lightType The type of light to set the scene for. Defaults to AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_scene_color_temperature(uint16_t ct, uint8_t bright, LightType lightType = AUTO);

    /**
     * Sets the scene to auto delay off.
     *
     * This function sets the scene to auto delay off with the specified brightness and duration.
     *
     * @param brightness The brightness level to set (0-100).
     * @param duration The duration in milliseconds for the delay off.
     * @param lightType The type of light to set the scene for (default: AUTO).
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_scene_auto_delay_off(uint8_t brightness, uint32_t duration, LightType lightType = AUTO);

    /**
     * Sets the turn off delay for the Yeelight device.
     *
     * This function allows you to set the duration for the turn off delay of the Yeelight device.
     * The turn off delay is the time it takes for the device to automatically turn off after being turned on.
     *
     * @param duration The duration of the turn off delay in milliseconds.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_turn_off_delay(uint32_t duration);

    /**
     * Removes the turn off delay for the Yeelight device.
     *
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType remove_turn_off_delay();

    /**
     * Sets the default state for the specified light type.
     *
     * @param lightType The type of light for which the default state should be set. Defaults to AUTO.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_default_state(LightType lightType = AUTO);

    /**
     * @brief Sets the name of the device.
     *
     * This function allows you to set the name of the device.
     *
     * @param name The new name for the device.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_device_name(const char *name);

    /**
     * @brief Sets the device name.
     *
     * This function sets the name of the device.
     *
     * @param name The new name for the device.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_device_name(const std::string &name);

    /**
     * Adjusts the brightness of the Yeelight device.
     *
     * This function allows you to adjust the brightness of the Yeelight device by specifying a percentage value.
     * The brightness can be adjusted for a specific light type or for all light types (AUTO).
     *
     * @param percentage The percentage value to adjust the brightness. Positive values increase the brightness,
     *                   while negative values decrease the brightness.
     * @param lightType  (Optional) The type of light to adjust the brightness for. Default is AUTO, which adjusts
     *                   the brightness for all light types.
     *
     * @return The response type indicating the success or failure of the brightness adjustment.
     */
    ResponseType adjust_brightness(int8_t percentage, LightType lightType = AUTO);

    /**
     * Adjusts the brightness of the Yeelight device.
     *
     * This function allows you to adjust the brightness of the Yeelight device by specifying a percentage value.
     * The brightness adjustment can be applied gradually over a specified duration.
     *
     * @param percentage The percentage value to adjust the brightness. Positive values increase the brightness,
     *                   while negative values decrease the brightness.
     * @param duration   The duration over which the brightness adjustment should be applied, in milliseconds.
     * @param lightType  (Optional) The type of light to adjust the brightness for. Defaults to AUTO, which adjusts
     *                   the brightness for all types of lights.
     *
     * @return The response type indicating the success or failure of the brightness adjustment.
     */
    ResponseType adjust_brightness(int8_t percentage, uint16_t duration, LightType lightType = AUTO);

    /**
     * Adjusts the color temperature of the Yeelight device.
     *
     * This function allows you to adjust the color temperature of the Yeelight device by a certain percentage.
     * The color temperature represents the warmth or coolness of the light emitted by the device.
     *
     * @param percentage The percentage by which to adjust the color temperature. A positive value increases the temperature,
     *                   while a negative value decreases it.
     * @param lightType  (Optional) The type of light to adjust the color temperature for. By default, it is set to AUTO,
     *                   which means the adjustment will be applied to all lights.
     *
     * @return The response type indicating the success or failure of the adjustment.
     */
    ResponseType adjust_color_temp(int8_t percentage, LightType lightType = AUTO);

    /**
     * Adjusts the color temperature of the Yeelight device.
     *
     * This function allows you to adjust the color temperature of the Yeelight device by specifying a percentage and duration.
     * The percentage parameter represents the percentage change in color temperature, where a positive value increases the color temperature and a negative value decreases it.
     * The duration parameter specifies the time it takes to complete the color temperature adjustment, in milliseconds.
     * The lightType parameter is optional and can be used to specify the type of light to adjust. If not provided, the function will automatically determine the light type.
     *
     * @param percentage The percentage change in color temperature.
     * @param duration The duration of the color temperature adjustment in milliseconds.
     * @param lightType The type of light to adjust (optional).
     * @return The response type of the operation.
     */
    ResponseType adjust_color_temp(int8_t percentage, uint16_t duration, LightType lightType = AUTO);

    /**
     * Adjusts the color of the Yeelight device by a specified percentage.
     *
     * @param percentage The percentage by which to adjust the color. Positive values increase the color, while negative values decrease it.
     * @param lightType (Optional) The type of light to adjust. Defaults to AUTO, which adjusts all lights.
     * @return The response type indicating the success or failure of the adjustment.
     */
    ResponseType adjust_color(int8_t percentage, LightType lightType = AUTO);

    /**
     * Adjusts the color of the Yeelight device.
     *
     * This function allows you to adjust the color of the Yeelight device by specifying the percentage of color adjustment,
     * the duration of the adjustment, and the type of light to adjust.
     *
     * @param percentage The percentage of color adjustment. Positive values increase the color intensity, while negative values decrease it.
     * @param duration The duration of the color adjustment in milliseconds.
     * @param lightType The type of light to adjust. By default, it is set to AUTO, which means the adjustment will be applied to all lights.
     * @return The response type indicating the success or failure of the color adjustment.
     */
    ResponseType adjust_color(int8_t percentage, uint16_t duration, LightType lightType = AUTO);

    /**
     * Sets the scene flow for the Yeelight device.
     *
     * This function sets the scene flow for the Yeelight device. The scene flow determines the dynamic effect
     * that the light will display, such as a gradual color change or a flashing effect.
     *
     * @param flow The flow object that describes the scene flow.
     * @param lightType The type of light to apply the scene flow to. Defaults to AUTO, which applies the scene flow to all lights.
     * @return The response type indicating the success or failure of the operation.
     */
    ResponseType set_scene_flow(Flow flow, LightType lightType = AUTO);

    void set_adjust(ajust_action action, ajust_prop prop);

    void bg_set_adjust(ajust_action action, ajust_prop prop);

    std::uint16_t get_timeout() const;

    void set_timeout(std::uint16_t timeout);
};

#endif
