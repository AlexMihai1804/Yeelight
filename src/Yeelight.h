/**
* @file Yeelight.h
 * @brief Header file for the Yeelight class, providing methods to control and monitor Yeelight smart bulbs.
 *
 * This file contains the declarations of the Yeelight class and its member functions, designed to communicate
 * with Yeelight devices using TCP and SSDP-based discovery. Various methods allow the user to control
 * power, color, brightness, schedules, and more, including support for background light operations
 * on devices that offer dual-light capabilities.
 */

#ifndef YEELIGHTARDUINO_YEELIGHT_H
#define YEELIGHTARDUINO_YEELIGHT_H
#include <Arduino.h>
#include <AsyncTCP.h>
#include <cJSON.h>
#include <Flow.h>
#include <map>
#include <Yeelight_enums.h>
#include <Yeelight_structs.h>
/**
 * @class Yeelight
 * @brief A class providing a wide range of operations for controlling Yeelight smart bulbs.
 *
 * This class manages discovery, connection, command sending, property retrieval,
 * and advanced features such as flow effects and music mode. It can handle both the
 * main light and background light on bulbs that support dual-light functionality.
 */
class Yeelight {
private:
    /**
     * @brief Stores references to Yeelight instances indexed by their IP address.
     *
     * Key: A 32-bit integer constructed from the IPv4 address (e.g. A.B.C.D -> (A<<24)|(B<<16)|(C<<8)|D).
     * Value: Pointer to the corresponding Yeelight instance.
     */
    static std::map<uint32_t, Yeelight *> devices;

    /**
     * @brief Pointer to the server used for Music Mode connections.
     *
     * Music mode allows controlling the bulb via a dedicated TCP connection for faster,
     * real-time changes without closing the main control socket.
     */
    static AsyncServer *music_mode_server;

    /**
     * @brief Internal TCP client used for communication with the main light or single-light Yeelight devices.
     */
    AsyncClient *client;

    /**
     * @brief Internal TCP client used for Music Mode communication.
     */
    AsyncClient *music_client;

    /**
     * @brief Indicates whether the device is currently in music mode (true) or not (false).
     */
    bool music_mode;

    /**
     * @brief Indicates if the connection is being closed intentionally (true) or due to an error (false).
     */
    bool closingManually;

    /**
     * @brief The IP address of the Yeelight device.
     */
    uint8_t ip[4];

    /**
     * @brief The port number of the Yeelight device (commonly 55443).
     */
    uint16_t port;

    /**
     * @brief Tracks the supported methods (features) that the Yeelight device reports.
     *
     * This is populated automatically when a device is discovered or refreshed.
     * The structure that holds these booleans is defined elsewhere.
     */
    SupportedMethods supported_methods;

    /**
     * @brief The timeout (in milliseconds) for waiting on responses from the Yeelight device.
     */
    uint16_t timeout;

    /**
     * @brief The maximum number of retries allowed when attempting to reconnect to a device.
     */
    uint8_t max_retry;

    /**
     * @brief Holds the device properties obtained from the Yeelight device (power, brightness, color, etc.).
     *
     * This structure is updated whenever certain commands (e.g. get_prop) are called
     * or certain responses (like notification events) are received.
     */
    YeelightProperties properties;

    /**
     * @brief Holds the ID for the next command to be sent. Used to correlate responses to specific commands.
     */
    uint16_t response_id;

    /**
     * @brief Stores the partial response data when reading from the socket before a newline is encountered.
     */
    std::string partialResponse;

    /**
     * @brief Caches the responses from the device, keyed by the command ID.
     */
    std::map<uint16_t, ResponseType> responses;

    /**
     * @brief Checks if a response with a given ID has arrived within the specified timeout.
     *
     * @param id The command ID to check for.
     * @return ResponseType The status of the response (SUCCESS, TIMEOUT, ERROR, etc.).
     */
    ResponseType checkResponse(uint16_t id);

    /**
     * @brief Refreshes the list of supported methods for the current Yeelight device by sending an M-SEARCH SSDP request.
     *
     * This function updates supported_methods to reflect the capabilities announced by the device.
     */
    void refreshSupportedMethods();

    /**
     * @brief Callback to handle incoming data from the main or music mode client.
     *
     * @param c The AsyncClient pointer associated with the connection.
     * @param data Pointer to the data buffer received.
     * @param len The length of the data in bytes.
     */
    void onData(AsyncClient *c, const void *data, size_t len);

    /**
     * @brief Parses the device information from an SSDP response packet string.
     *
     * @param response A null-terminated string containing the response from the Yeelight device.
     * @return YeelightDevice A struct containing the parsed device information.
     */
    static YeelightDevice parseDiscoveryResponse(const char *response);

    /**
     * @brief Called when the main control client is disconnected.
     *
     * If the disconnection is unintentional and not in music mode, this method initiates a reconnection.
     *
     * @param c A pointer to the AsyncClient that disconnected.
     */
    void onMainClientDisconnect(const AsyncClient *c);

    /**
     * @brief Called when the music mode client is disconnected.
     *
     * Disables music mode and attempts to reconnect via the main client.
     *
     * @param c A pointer to the AsyncClient that disconnected.
     */
    void onMusicDisconnect(const AsyncClient *c);

    /**
     * @brief Accepts an incoming connection for music mode.
     *
     * This is called by the static server instance (music_mode_server) whenever a
     * device tries to connect to the designated music mode port (55443 by default).
     *
     * @param arg Unused parameter (passed by the AsyncServer).
     * @param client Pointer to the newly accepted AsyncClient.
     */
    static void handleNewClient(void *arg, AsyncClient *client);

    /**
     * @brief Creates the TCP server that listens for music mode connections.
     *
     * @return true If the server was successfully created.
     * @return false If the server was already created or failed to create.
     */
    bool createMusicModeServer();

    /**
     * @brief Connects the class's main TCP client to the Yeelight device using the current IP and port.
     *
     * @return ResponseType The result of the connection attempt (SUCCESS, CONNECTION_FAILED, etc.).
     */
    ResponseType connect();

    /**
     * @brief Sends a command (with parameters in JSON) to the Yeelight device.
     *
     * If music mode is active, the command is routed through the music_client instead of the main client.
     *
     * @param method The name of the Yeelight command to send (e.g., "set_power", "set_rgb", etc.).
     * @param params A cJSON object containing the parameters to pass to the command.
     * @return ResponseType The result of sending the command (SUCCESS, ERROR, TIMEOUT, etc.).
     */
    ResponseType send_command(const char *method, cJSON *params);

    /**
     * @brief Issues a "set_power" command to the Yeelight device.
     *
     * @param power Whether the light should be turned on (true) or off (false).
     * @param effect Transition effect (smooth or sudden).
     * @param duration Duration of the transition effect in milliseconds (>=30 for smooth transitions).
     * @param mode The operating mode (e.g., color, CT, HSV).
     * @return ResponseType Result of the command.
     */
    ResponseType set_power_command(bool power, effect effect, uint16_t duration, mode mode);

    /**
     * @brief Issues a "toggle" command to the Yeelight device, toggling its current power state.
     *
     * @return ResponseType Result of the command.
     */
    ResponseType toggle_command();

    /**
     * @brief Issues a "set_ct_abx" command to set the light's color temperature (main light).
     *
     * @param ct_value The desired color temperature in Kelvin (1700K - 6500K).
     * @param effect Transition effect.
     * @param duration Transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType set_ct_abx_command(uint16_t ct_value, effect effect, uint16_t duration);

    /**
     * @brief Issues a "set_rgb" command to set the light's color using an RGB value (main light).
     *
     * @param r The red component (0-255).
     * @param g The green component (0-255).
     * @param b The blue component (0-255).
     * @param effect Transition effect.
     * @param duration Transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType set_rgb_command(uint8_t r, uint8_t g, uint8_t b, effect effect, uint16_t duration);

    /**
     * @brief Issues a "set_hsv" command to set the light's color using HSV (main light).
     *
     * @param hue The hue component (0-359).
     * @param sat The saturation component (0-100).
     * @param effect Transition effect.
     * @param duration Transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType set_hsv_command(uint16_t hue, uint8_t sat, effect effect, uint16_t duration);

    /**
     * @brief Issues a "set_bright" command to set the brightness of the main light.
     *
     * @param bright The brightness level (1-100).
     * @param effect Transition effect.
     * @param duration Transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType set_bright_command(uint8_t bright, effect effect, uint16_t duration);

    /**
     * @brief Issues a "set_default" command, setting the current state as the default power-on state for the main light.
     *
     * @return ResponseType Result of the command.
     */
    ResponseType set_default();

    /**
     * @brief Issues a "start_cf" (Color Flow) command to begin a color flow effect on the main light.
     *
     * @param count The number of flow transitions to run.
     * @param action The action after flow stops (stay, turn off, etc.).
     * @param size The number of flow expressions.
     * @param flow An array of flow_expression structures describing each transition.
     * @return ResponseType Result of the command.
     */
    ResponseType start_cf_command(uint8_t count, flow_action action, uint8_t size, const flow_expression *flow);

    /**
     * @brief Issues a "stop_cf" command to stop any active color flow on the main light.
     *
     * @return ResponseType Result of the command.
     */
    ResponseType stop_cf_command();

    /**
     * @brief Issues a "set_scene" command to set a color-based scene on the main light using an RGB value.
     *
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param bright Brightness level (1-100).
     * @return ResponseType Result of the command.
     */
    ResponseType set_scene_rgb_command(uint8_t r, uint8_t g, uint8_t b, uint8_t bright);

    /**
     * @brief Issues a "set_scene" command to set an HSV-based scene on the main light.
     *
     * @param hue The hue component (0-359).
     * @param sat The saturation component (0-100).
     * @param bright The brightness level (1-100).
     * @return ResponseType Result of the command.
     */
    ResponseType set_scene_hsv_command(uint8_t hue, uint8_t sat, uint8_t bright);

    /**
     * @brief Issues a "set_scene" command to set a color temperature-based scene on the main light.
     *
     * @param ct The color temperature (1700K - 6500K).
     * @param bright Brightness level (1-100).
     * @return ResponseType Result of the command.
     */
    ResponseType set_scene_ct_command(uint16_t ct, uint8_t bright);

    /**
     * @brief Issues a "set_scene" command to enable auto-delay-off, turning off after a specified duration.
     *
     * @param brightness Brightness level (1-100).
     * @param duration Delay in seconds before automatically turning off.
     * @return ResponseType Result of the command.
     */
    ResponseType set_scene_auto_delay_off_command(uint8_t brightness, uint32_t duration);

    /**
     * @brief Issues a "set_scene" command to run a color flow (Flow) scene on the main light.
     *
     * @param count The number of flow transitions.
     * @param action The action after the flow stops.
     * @param size The number of expressions in the flow.
     * @param flow The flow_expression array describing the transitions.
     * @return ResponseType Result of the command.
     */
    ResponseType set_scene_cf_command(uint32_t count, flow_action action, uint32_t size, const flow_expression *flow);

    /**
     * @brief Issues a "cron_add" command to add a countdown timer to turn off the light.
     *
     * @param time The time in minutes after which the light will turn off automatically.
     * @return ResponseType Result of the command.
     */
    ResponseType cron_add_command(uint32_t time);

    /**
     * @brief Issues a "cron_del" command to remove the existing countdown timer.
     *
     * @return ResponseType Result of the command.
     */
    ResponseType cron_del_command();

    /**
     * @brief Issues a "set_adjust" command to adjust a property (brightness, color temperature, or color).
     *
     * This is a convenience function used internally to build the parameters;
     * for adjusting the main light only.
     *
     * @param action The type of adjustment (increase, decrease, circle).
     * @param prop The property to adjust (bright, ct, color).
     */
    void set_adjust(ajust_action action, ajust_prop prop);

    /**
     * @brief Issues a "set_name" command to set a custom name for the device.
     *
     * @param name The new name to assign to the device.
     * @return ResponseType Result of the command.
     */
    ResponseType set_name_command(const char *name);

    /**
     * @brief Issues a "bg_set_power" command to control power state for the background light (if supported).
     *
     * @param power True to turn on, false to turn off.
     * @param effect Transition effect (smooth or sudden).
     * @param duration Transition duration in ms.
     * @param mode Light mode (e.g., color, ct, hsv).
     * @return ResponseType Result of the command.
     */
    ResponseType bg_set_power_command(bool power, effect effect, uint16_t duration, mode mode);

    /**
     * @brief Issues a "bg_toggle" command to toggle the background light power state.
     *
     * @return ResponseType Result of the command.
     */
    ResponseType bg_toggle_command();

    /**
     * @brief Issues a "bg_set_ct_abx" command for the background light.
     *
     * @param ct_value The color temperature (1700K - 6500K).
     * @param effect Transition effect (smooth or sudden).
     * @param duration Transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType bg_set_ct_abx_command(uint16_t ct_value, effect effect, uint16_t duration);

    /**
     * @brief Issues a "bg_set_rgb" command for the background light.
     *
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param effect Transition effect.
     * @param duration Transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType bg_set_rgb_command(uint8_t r, uint8_t g, uint8_t b, effect effect, uint16_t duration);

    /**
     * @brief Issues a "bg_set_hsv" command for the background light.
     *
     * @param hue The hue component (0-359).
     * @param sat The saturation component (0-100).
     * @param effect Transition effect.
     * @param duration Transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType bg_set_hsv_command(uint16_t hue, uint8_t sat, effect effect, uint16_t duration);

    /**
     * @brief Issues a "bg_set_bright" command to control brightness of the background light.
     *
     * @param bright Brightness level (1-100).
     * @param effect Transition effect.
     * @param duration Transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType bg_set_bright_command(uint8_t bright, effect effect, uint16_t duration);

    /**
     * @brief Issues a "bg_set_default" command to set the current background light state as the default.
     *
     * @return ResponseType Result of the command.
     */
    ResponseType bg_set_default();

    /**
     * @brief Issues a "bg_set_scene" command for the background light with an RGB-based scene.
     *
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param bright Brightness level (1-100).
     * @return ResponseType Result of the command.
     */
    ResponseType bg_set_scene_rgb_command(uint8_t r, uint8_t g, uint8_t b, uint8_t bright);

    /**
     * @brief Issues a "bg_set_scene" command for the background light with an HSV-based scene.
     *
     * @param hue The hue component (0-359).
     * @param sat The saturation component (0-100).
     * @param bright Brightness level (1-100).
     * @return ResponseType Result of the command.
     */
    ResponseType bg_set_scene_hsv_command(uint8_t hue, uint8_t sat, uint8_t bright);

    /**
     * @brief Issues a "bg_set_scene" command for the background light with a color temperature-based scene.
     *
     * @param ct The color temperature (1700K - 6500K).
     * @param bright Brightness level (1-100).
     * @return ResponseType Result of the command.
     */
    ResponseType bg_set_scene_ct_command(uint16_t ct, uint8_t bright);

    /**
     * @brief Issues a "bg_set_scene" command enabling auto-delay-off for the background light.
     *
     * @param brightness Brightness level (1-100).
     * @param duration Delay in seconds before auto off.
     * @return ResponseType Result of the command.
     */
    ResponseType bg_set_scene_auto_delay_off_command(uint8_t brightness, uint32_t duration);

    /**
     * @brief Issues a "bg_set_scene" command to start a color flow on the background light.
     *
     * @param count The number of flow transitions.
     * @param action The action after flow stops.
     * @param size The number of expressions.
     * @param flow The array of flow_expression data.
     * @return ResponseType Result of the command.
     */
    ResponseType bg_set_scene_cf_command(uint32_t count, flow_action action, uint32_t size,
                                         const flow_expression *flow);

    /**
     * @brief Issues a "bg_set_adjust" command to adjust a property on the background light (bright, ct, or color).
     *
     * @param action The adjustment action (increase, decrease, or circle).
     * @param prop The property to adjust (bright, ct, or color).
     */
    void bg_set_adjust(ajust_action action, ajust_prop prop);

    /**
     * @brief Issues a "dev_toggle" command, toggling both the main and background lights at once (if supported).
     *
     * @return ResponseType Result of the command.
     */
    ResponseType dev_toggle_command();

    /**
     * @brief Issues an "adjust_bright" command to adjust brightness relative to the current level (main light).
     *
     * @param percentage The percentage to adjust (-100 to 100).
     * @param duration The transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType adjust_bright_command(int8_t percentage, uint16_t duration);

    /**
     * @brief Issues an "adjust_ct" command to adjust the color temperature by a percentage (main light).
     *
     * @param percentage The percentage to adjust (-100 to 100).
     * @param duration Transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType adjust_ct_command(int8_t percentage, uint16_t duration);

    /**
     * @brief Issues an "adjust_color" command to adjust the color by a relative amount (main light).
     *
     * @param percentage The percentage to adjust (-100 to 100).
     * @param duration Transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType adjust_color_command(int8_t percentage, uint16_t duration);

    /**
     * @brief Issues a "bg_adjust_bright" command to adjust background light brightness by a percentage.
     *
     * @param percentage The percentage to adjust (-100 to 100).
     * @param duration Transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType bg_adjust_bright_command(int8_t percentage, uint16_t duration);

    /**
     * @brief Issues a "bg_adjust_ct" command to adjust background light color temperature by a percentage.
     *
     * @param percentage The percentage to adjust (-100 to 100).
     * @param duration Transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType bg_adjust_ct_command(int8_t percentage, uint16_t duration);

    /**
     * @brief Issues a "bg_adjust_color" command to adjust the background light color by a relative amount.
     *
     * @param percentage The percentage to adjust (-100 to 100).
     * @param duration Transition duration in ms.
     * @return ResponseType Result of the command.
     */
    ResponseType bg_adjust_color_command(int8_t percentage, uint16_t duration);

    /**
     * @brief Issues a "bg_start_cf" command to start a color flow on the background light.
     *
     * @param count Number of flow transitions.
     * @param action Action after flow stops.
     * @param size Number of flow expressions.
     * @param flow The flow_expression data describing each step.
     * @return ResponseType Result of the command.
     */
    ResponseType bg_start_cf_command(uint8_t count, flow_action action, uint8_t size, const flow_expression *flow);

    /**
     * @brief Issues a "bg_stop_cf" command to stop a color flow on the background light.
     *
     * @return ResponseType Result of the command.
     */
    ResponseType bg_stop_cf_command();

public:
    /**
     * @brief Default constructor for the Yeelight class (usually not used directly).
     *
     * Initializes default values for IP (0.0.0.0), port, timeouts, retries, etc.
     */
    Yeelight();

    /**
     * @brief Constructs a Yeelight object with a given IP address and port.
     *
     * Automatically refreshes supported methods, then attempts a connection.
     *
     * @param ip A 4-byte array representing the IP address (e.g. [192, 168, 1, 100]).
     * @param port The device's listening port (commonly 55443).
     */
    Yeelight(const uint8_t ip[4], uint16_t port);

    /**
     * @brief Constructs a Yeelight object from a discovered YeelightDevice structure.
     *
     * Automatically attempts to connect using the provided device information.
     *
     * @param device A YeelightDevice struct holding IP, port, model info, and supported methods.
     */
    explicit Yeelight(const YeelightDevice &device);

    /**
     * @brief Destructor for the Yeelight class.
     *
     * Cleans up the main client, music client, and music mode server if active.
     * Also removes itself from the global devices map.
     */
    ~Yeelight();

    /**
     * @brief Returns the set of supported methods retrieved from the device.
     *
     * @return SupportedMethods A structure containing boolean flags of the device's capabilities.
     */
    SupportedMethods getSupportedMethods() const;

    /**
     * @brief Discovers available Yeelight devices in the local network by sending an M-SEARCH request.
     *
     * @param waitTimeMs The time (in milliseconds) to wait for SSDP responses.
     * @return std::vector<YeelightDevice> A vector of discovered device information structures.
     */
    static std::vector<YeelightDevice> discoverYeelightDevices(int waitTimeMs);

    /**
     * @brief Connects to a Yeelight device given an IP and port, refreshing the supported methods first.
     *
     * @param ip A 4-byte array of the device's IP address.
     * @param port The device's port.
     * @return ResponseType The result of the connection attempt.
     */
    ResponseType connect(const uint8_t *ip, uint16_t port);

    /**
     * @brief Connects to a Yeelight device using a YeelightDevice structure (commonly from discovery).
     *
     * @param device The YeelightDevice containing IP, port, and supported methods.
     * @return ResponseType The result of the connection attempt.
     */
    ResponseType connect(const YeelightDevice &device);

    /**
     * @brief Checks if the main client is currently connected to the Yeelight device.
     *
     * @return true If the main client is connected.
     * @return false Otherwise.
     */
    bool is_connected() const;

    /**
     * @brief Checks if the music mode client is currently connected.
     *
     * @return true If the music mode client is connected.
     * @return false Otherwise.
     */
    bool is_connected_music() const;

    /**
     * @brief Sets the maximum time to wait for a response from the device.
     *
     * @param timeout Timeout in milliseconds.
     */
    void set_timeout(uint16_t timeout);

    /**
     * @brief Retrieves the currently configured timeout.
     *
     * @return std::uint16_t The timeout in milliseconds.
     */
    std::uint16_t get_timeout() const;

    /**
     * @brief Issues a "start_cf" or "bg_start_cf" command depending on the LightType,
     *        controlling color flow on main light, background light, or both.
     *
     * @param flow A Flow object describing the color transitions.
     * @param lightType Which light (main, background, or both) should be targeted.
     * @return ResponseType Result of the command.
     */
    ResponseType start_flow(Flow flow, LightType lightType = AUTO);

    /**
     * @brief Issues a "stop_cf" or "bg_stop_cf" command to stop color flow(s) depending on the LightType.
     *
     * @param lightType Specifies which light(s) to stop flow on (main, background, or both).
     * @return ResponseType Result of stopping the flow.
     */
    ResponseType stop_flow(LightType lightType = AUTO);

    /**
     * @brief Toggles power on main light, background light, or both based on LightType.
     *
     * @param lightType The target light or lights (main, background, both, or auto).
     * @return ResponseType Result of the command.
     */
    ResponseType toggle_power(LightType lightType = AUTO);

    /**
     * @brief Sets power on/off for main light, background light, or both, with transition effect and duration.
     *
     * @param power True to turn on, false to turn off.
     * @param effect Transition effect.
     * @param duration Duration in ms (>=30 for smooth).
     * @param mode The mode to set (e.g., color, ct, hsv).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_power(bool power, effect effect, uint16_t duration, mode mode, LightType lightType = AUTO);

    /**
     * @brief Convenience overload to set power with default effect, duration, and mode.
     *
     * @param power True to turn on, false to turn off.
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_power(bool power, LightType lightType);

    /**
     * @brief Convenience overload to set power with a specified effect and default duration, mode.
     *
     * @param power True to turn on, false to turn off.
     * @param effect Transition effect.
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_power(bool power, effect effect, LightType lightType);

    /**
     * @brief Convenience overload to set power with a specified effect, duration, and default mode.
     *
     * @param power True to turn on, false to turn off.
     * @param effect Transition effect.
     * @param duration Duration in ms.
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_power(bool power, effect effect, uint16_t duration, LightType lightType);

    /**
     * @brief Convenience overload to set power with a specified mode, default effect, and duration.
     *
     * @param power True to turn on, false to turn off.
     * @param mode The mode (color, ct, hsv).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_power(bool power, mode mode, LightType lightType);

    /**
     * @brief Convenience overload to set power with specified effect, mode, and default duration.
     *
     * @param power True to turn on, false to turn off.
     * @param effect Transition effect.
     * @param mode The mode (color, ct, hsv).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_power(bool power, effect effect, mode mode, LightType lightType);

    /**
     * @brief Turns on the light (main, background, or both) using default smooth effect and 500 ms duration.
     *
     * @param lightType Which light(s) to turn on.
     * @return ResponseType Result of the command.
     */
    ResponseType turn_on(LightType lightType = AUTO);

    /**
     * @brief Turns on the light with a specified transition effect, using default duration (500 ms).
     *
     * @param effect The transition effect (smooth or sudden).
     * @param lightType Which light(s) to turn on.
     * @return ResponseType Result of the command.
     */
    ResponseType turn_on(effect effect, LightType lightType);

    /**
     * @brief Turns on the light with a specified transition effect and duration.
     *
     * @param effect Transition effect.
     * @param duration Duration in ms.
     * @param lightType Which light(s) to turn on.
     * @return ResponseType Result of the command.
     */
    ResponseType turn_on(effect effect, uint16_t duration, LightType lightType);

    /**
     * @brief Turns on the light with a specified mode (color, ct, hsv), default effect, and duration.
     *
     * @param mode Light mode.
     * @param lightType Which light(s) to turn on.
     * @return ResponseType Result of the command.
     */
    ResponseType turn_on(mode mode, LightType lightType);

    /**
     * @brief Turns on the light with specified effect, mode, and default duration.
     *
     * @param effect Transition effect.
     * @param mode Light mode.
     * @param lightType Which light(s) to turn on.
     * @return ResponseType Result of the command.
     */
    ResponseType turn_on(effect effect, mode mode, LightType lightType);

    /**
     * @brief Turns on the light with specified effect, duration, and mode.
     *
     * @param effect Transition effect.
     * @param duration Duration in ms.
     * @param mode Light mode.
     * @param lightType Which light(s) to turn on.
     * @return ResponseType Result of the command.
     */
    ResponseType turn_on(effect effect, uint16_t duration, mode mode, LightType lightType);

    /**
     * @brief Turns off the light (main, background, or both) using default smooth effect and 500 ms duration.
     *
     * @param lightType Which light(s) to turn off.
     * @return ResponseType Result of the command.
     */
    ResponseType turn_off(LightType lightType = AUTO);

    /**
     * @brief Turns off the light with a specified transition effect, using default duration.
     *
     * @param effect The transition effect (smooth or sudden).
     * @param lightType Which light(s) to turn off.
     * @return ResponseType Result of the command.
     */
    ResponseType turn_off(effect effect, LightType lightType);

    /**
     * @brief Turns off the light with a specified transition effect and duration.
     *
     * @param effect Transition effect.
     * @param duration Duration in ms.
     * @param lightType Which light(s) to turn off.
     * @return ResponseType Result of the command.
     */
    ResponseType turn_off(effect effect, uint16_t duration, LightType lightType);

    /**
     * @brief Turns off the light with a specified mode, using default smooth effect and 500 ms duration.
     *
     * @param mode The mode (color, ct, hsv).
     * @param lightType Which light(s) to turn off.
     * @return ResponseType Result of the command.
     */
    ResponseType turn_off(mode mode, LightType lightType);

    /**
     * @brief Turns off the light with specified effect, mode, and default duration.
     *
     * @param effect Transition effect.
     * @param mode The mode (color, ct, hsv).
     * @param lightType Which light(s) to turn off.
     * @return ResponseType Result of the command.
     */
    ResponseType turn_off(effect effect, mode mode, LightType lightType);

    /**
     * @brief Turns off the light with specified effect, duration, and mode.
     *
     * @param effect Transition effect.
     * @param duration Duration in ms.
     * @param mode The mode (color, ct, hsv).
     * @param lightType Which light(s) to turn off.
     * @return ResponseType Result of the command.
     */
    ResponseType turn_off(effect effect, uint16_t duration, mode mode, LightType lightType);

    /**
     * @brief Sets the color temperature for the specified light(s).
     *
     * @param ct_value The color temperature in Kelvin (1700-6500).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_color_temp(uint16_t ct_value, LightType lightType);

    /**
     * @brief Sets the color temperature with a specified transition effect and duration.
     *
     * @param ct_value The color temperature (1700-6500).
     * @param effect Transition effect.
     * @param duration Transition duration in ms.
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_color_temp(uint16_t ct_value, effect effect, uint16_t duration, LightType lightType);

    /**
    * @brief Sets the color temperature with a specified transition effect and duration.
    *
    * @param ct_value The color temperature (1700-6500).
    * @param effect Transition effect.
    * @param lightType Which light(s) to control.
    * @return ResponseType Result of the command.
    */
    ResponseType set_color_temp(uint16_t ct_value, effect effect, LightType lightType);

    /**
     * @brief Sets the color temperature and brightness simultaneously by sending a "set_scene" command.
     *
     * @param ct_value The desired color temperature (1700-6500).
     * @param bright The brightness level (1-100).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_color_temp(uint16_t ct_value, uint8_t bright, LightType lightType);

    /**
     * @brief Sets the main or background light color using an RGB value.
     *
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_rgb_color(uint8_t r, uint8_t g, uint8_t b, LightType lightType);

    /**
     * @brief Sets the RGB color with a specified transition effect, using default duration.
     *
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param effect Transition effect (smooth or sudden).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_rgb_color(uint8_t r, uint8_t g, uint8_t b, effect effect, LightType lightType);

    /**
     * @brief Sets the RGB color with a specified effect and duration.
     *
     * @param r Red (0-255).
     * @param g Green (0-255).
     * @param b Blue (0-255).
     * @param effect Transition effect.
     * @param duration Transition duration in ms.
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_rgb_color(uint8_t r, uint8_t g, uint8_t b, effect effect, uint16_t duration, LightType lightType);

    /**
     * @brief Sets an RGB color and brightness using a "set_scene" command (bypassing transitions).
     *
     * @param r Red (0-255).
     * @param g Green (0-255).
     * @param b Blue (0-255).
     * @param bright Brightness (1-100).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_rgb_color(uint8_t r, uint8_t g, uint8_t b, uint8_t bright, LightType lightType);

    /**
     * @brief Sets brightness for the main, background, or both lights.
     *
     * @param bright Brightness (1-100).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_brightness(uint8_t bright, LightType lightType);

    /**
     * @brief Sets brightness with a specified transition effect (default 500 ms).
     *
     * @param bright Brightness (1-100).
     * @param effect Transition effect (smooth or sudden).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_brightness(uint8_t bright, effect effect, LightType lightType);

    /**
     * @brief Sets brightness with a specified effect and duration.
     *
     * @param bright Brightness (1-100).
     * @param effect Transition effect.
     * @param duration Transition duration in ms.
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_brightness(uint8_t bright, effect effect, uint16_t duration, LightType lightType);

    /**
     * @brief Sets an HSV color for the specified light(s).
     *
     * @param hue The hue (0-359).
     * @param sat The saturation (0-100).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_hsv_color(uint16_t hue, uint8_t sat, LightType lightType);

    /**
     * @brief Sets an HSV color with a specified transition effect (default 500 ms).
     *
     * @param hue Hue (0-359).
     * @param sat Saturation (0-100).
     * @param effect Transition effect.
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_hsv_color(uint16_t hue, uint8_t sat, effect effect, LightType lightType);

    /**
     * @brief Sets an HSV color with a specified effect and duration.
     *
     * @param hue Hue (0-359).
     * @param sat Saturation (0-100).
     * @param effect Transition effect.
     * @param duration Transition duration in ms.
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_hsv_color(uint16_t hue, uint8_t sat, effect effect, uint16_t duration, LightType lightType);

    /**
     * @brief Sets an HSV color and brightness using a "set_scene" command (bypassing transitions).
     *
     * @param hue Hue (0-359).
     * @param sat Saturation (0-100).
     * @param bright Brightness (1-100).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_hsv_color(uint16_t hue, uint8_t sat, uint8_t bright, LightType lightType);

    /**
     * @brief Sets a scene with an RGB-based color on the main or background light (or both).
     *
     * @param r Red (0-255).
     * @param g Green (0-255).
     * @param b Blue (0-255).
     * @param bright Brightness (1-100).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_scene_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t bright, LightType lightType);

    /**
     * @brief Sets a scene with an HSV-based color on the main or background light (or both).
     *
     * @param hue Hue (0-359).
     * @param sat Saturation (0-100).
     * @param bright Brightness (1-100).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_scene_hsv(uint16_t hue, uint8_t sat, uint8_t bright, LightType lightType);

    /**
     * @brief Sets a scene based on color temperature on the main or background light (or both).
     *
     * @param ct Color temperature (1700-6500).
     * @param bright Brightness (1-100).
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_scene_color_temperature(uint16_t ct, uint8_t bright, LightType lightType);

    /**
     * @brief Sets a scene with auto-delay-off on the main or background light (or both).
     *
     * @param brightness Brightness (1-100).
     * @param duration Delay in seconds.
     * @param lightType Which light(s) to control.
     * @return ResponseType Result of the command.
     */
    ResponseType set_scene_auto_delay_off(uint8_t brightness, uint32_t duration, LightType lightType);

    /**
     * @brief Sets a turn-off delay using the "cron_add" command.
     *
     * @param duration Time in minutes after which the light will turn off automatically.
     * @return ResponseType Result of the command.
     */
    ResponseType set_turn_off_delay(uint32_t duration);

    /**
     * @brief Removes the previously configured turn-off delay using "cron_del".
     *
     * @return ResponseType Result of the command.
     */
    ResponseType remove_turn_off_delay();

    /**
     * @brief Sets the current state as the default power-on state for main, background, or both lights.
     *
     * @param lightType The target light(s).
     * @return ResponseType Result of the command.
     */
    ResponseType set_default_state(LightType lightType);

    /**
     * @brief Sets the device name by issuing the "set_name" command (only if supported).
     *
     * @param name The new name for the device.
     * @return ResponseType Result of the command.
     */
    ResponseType set_device_name(const char *name);

    /**
     * @brief Sets the device name by issuing the "set_name" command (std::string overload).
     *
     * @param name The new name for the device.
     * @return ResponseType Result of the command.
     */
    ResponseType set_device_name(const std::string &name);

    /**
     * @brief Enables or disables music mode on the device if supported.
     *
     * @param enabled True to enable, false to disable.
     * @return ResponseType Result of the command (SUCCESS, METHOD_NOT_SUPPORTED, etc.).
     */
    ResponseType set_music_mode(bool enabled);

    /**
     * @brief Convenience method to enable music mode (calls set_music_mode(true)).
     *
     * @return ResponseType Result of the command.
     */
    ResponseType enable_music_mode();

    /**
     * @brief Convenience method to disable music mode (calls set_music_mode(false)).
     *
     * @return ResponseType Result of the command.
     */
    ResponseType disable_music_mode();

    /**
     * @brief Adjusts brightness by a certain percentage (main or background or both),
     *        using the appropriate "adjust_bright" command.
     *
     * @param percentage The relative brightness change (-100 to 100).
     * @param lightType Which light(s) to adjust.
     * @return ResponseType Result of the command.
     */
    ResponseType adjust_brightness(int8_t percentage, LightType lightType);

    /**
     * @brief Adjusts brightness by a certain percentage with a specified duration.
     *
     * @param percentage The relative brightness change (-100 to 100).
     * @param duration The transition duration in ms.
     * @param lightType Which light(s) to adjust.
     * @return ResponseType Result of the command.
     */
    ResponseType adjust_brightness(int8_t percentage, uint16_t duration, LightType lightType);

    /**
     * @brief Adjusts color temperature by a certain percentage (main or background or both).
     *
     * @param percentage The relative temperature change (-100 to 100).
     * @param lightType Which light(s) to adjust.
     * @return ResponseType Result of the command.
     */
    ResponseType adjust_color_temp(int8_t percentage, LightType lightType);

    /**
     * @brief Adjusts color temperature by a certain percentage with a specified duration.
     *
     * @param percentage The relative temperature change (-100 to 100).
     * @param duration Transition duration in ms.
     * @param lightType Which light(s) to adjust.
     * @return ResponseType Result of the command.
     */
    ResponseType adjust_color_temp(int8_t percentage, uint16_t duration, LightType lightType);

    /**
     * @brief Adjusts the color by a certain percentage (main or background or both).
     *
     * @param percentage The relative color change (-100 to 100).
     * @param lightType Which light(s) to adjust.
     * @return ResponseType Result of the command.
     */
    ResponseType adjust_color(int8_t percentage, LightType lightType);

    /**
     * @brief Adjusts the color by a certain percentage with a specified duration.
     *
     * @param percentage The relative color change (-100 to 100).
     * @param duration Transition duration in ms.
     * @param lightType Which light(s) to adjust.
     * @return ResponseType Result of the command.
     */
    ResponseType adjust_color(int8_t percentage, uint16_t duration, LightType lightType);

    /**
     * @brief Issues a "set_scene" command to start a flow on the main or background light (or both).
     *
     * @param flow A Flow object describing the sequence of transitions.
     * @param lightType Which light(s) to target.
     * @return ResponseType Result of the command.
     */
    ResponseType set_scene_flow(Flow flow, LightType lightType = AUTO);

    /**
     * @brief Issues a "get_prop" command to refresh the current properties from the Yeelight device.
     *
     * @return ResponseType Result of the command.
     */
    ResponseType refreshProperties();

    /**
     * @brief Retrieves the cached properties of the device (updated upon certain commands or notifications).
     *
     * @return YeelightProperties A copy of the current property structure.
     */
    YeelightProperties getProperties();

    /**
     * @brief Issues a "set_music" command to start or stop music mode, specifying IP and port if starting.
     *
     * Typically used internally, prefer using set_music_mode().
     *
     * @param power True to enable, false to disable.
     * @param host A pointer to a 4-byte array representing the server IP (default: WiFi.localIP()).
     * @param port The port on which the server is listening for music mode.
     * @return ResponseType Result of the command.
     */
    ResponseType set_music_command(bool power, const uint8_t *host = nullptr, uint16_t port = 55443);
};

#endif
