/**
 * @file Yeelight.h
 * @brief Header file for the Yeelight class, providing an interface for controlling and interacting with Yeelight devices.
 *
 * This header defines the Yeelight class, which enables network discovery, connection handling,
 * and command-based control of various Yeelight device functionalities, such as power state,
 * brightness, color, scenes, and more. It also declares the relevant structures and enums
 * used by the Yeelight class. (The actual definitions of those structures, enums, and the Flow
 * class are found in other files and are not of interest here.)
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
#include <freertos/semphr.h>
/**
 * @class Yeelight
 * @brief The main class for discovering, connecting, and controlling Yeelight devices.
 *
 * This class provides methods to discover Yeelight devices on the network, establish TCP connections,
 * execute commands (e.g., set power, change color, etc.), and manage device properties. It supports both
 * the main light and background light (for devices that have multiple light channels). In addition,
 * it can enable and disable music mode, which allows sending commands over a custom TCP channel.
 */
class Yeelight {
private:
    //---------------------------------------------------------------------------------------------------------
    // PRIVATE VARIABLES
    //---------------------------------------------------------------------------------------------------------

    /**
     * @brief The IP address of the Yeelight device.
     */
    uint8_t ip[4]{};

    /**
     * @brief The port number for connecting to the Yeelight device.
     */
    uint16_t port;

    /**
     * @brief A bitmask or structured object containing the supported methods for the device.
     */
    SupportedMethods supported_methods;

    /**
     * @brief The communication timeout in milliseconds.
     */
    uint16_t timeout;

    /**
     * @brief The maximum number of command retries if a command fails.
     */
    uint8_t max_retry;

    /**
     * @brief A structure holding the properties of the Yeelight device (like power state, color, etc.).
     */
    YeelightProperties properties;

    /**
     * @brief A map that tracks response IDs and their corresponding response types.
     */
    std::map<uint16_t, ResponseType> responses;

    /**
     * @brief The identifier for the current command/response.
     */
    uint16_t response_id;

    /**
     * @brief The primary Asynchronous TCP client used to send commands to the device.
     */
    AsyncClient *client = nullptr;

    /**
     * @brief The secondary Asynchronous TCP client used for music mode commands.
     */
    AsyncClient *music_client = nullptr;

    /**
     * @brief A buffer that holds partial responses when reading from the device.
     */
    std::string partialResponse;

    /**
     * @brief Indicates whether music mode is enabled (true) or disabled (false).
     */
    bool music_mode;

    /** @brief Indicates manual closure of the main client. */
    bool closingManually = false;
    /** @brief Flag set once supported methods have been fetched. */
    bool refreshedMethods = false;
    /** @brief Host IP for music‐mode server. */
    uint8_t music_host[4]{};
    /** @brief Port for music‐mode communication. */
    uint16_t music_port{};
    /** @brief Number of retries when connecting music client. */
    uint8_t music_retry_count{};
    /** @brief Maximum allowed music‐mode retries. */
    const uint8_t max_music_retries = 5;
    /** @brief Internal flag when switching back to normal mode. */
    bool is_switching_to_normal = false;
    /** @brief Mutex protecting the static devices map. */
    static SemaphoreHandle_t devices_mutex;
    /** @brief Indicates whether a connection attempt is in progress. */
    bool connecting = false;
    /** @brief Semaphore used for music‐mode handshake. */
    SemaphoreHandle_t music_sem = nullptr;


    /**
     * @brief The static server instance for handling inbound music mode connections.
     */
    static AsyncServer *music_mode_server;

    /**
     * @brief A static map associating client connections with Yeelight instances.
     */
    static std::map<uint32_t, Yeelight *> devices;

    //---------------------------------------------------------------------------------------------------------
    // PRIVATE METHODS
    //---------------------------------------------------------------------------------------------------------
    /**
     * @brief Called when the main TCP client successfully connects.
     * @param c Pointer to the connected client.
     */
    static void onMainClientConnect(AsyncClient *c);

    /**
     * @brief Handles errors on the main TCP client.
     * @param c Pointer to the client reporting the error.
     * @param error The error code.
     */
    void onMainClientError(AsyncClient *c, int8_t error);

    /**
     * @brief Safely inserts or updates this instance in the static devices map.
     * @param ip32 The 32‑bit representation of the device IP address.
     */
    void safeInsertDevice(uint32_t ip32);

    /**
     * @brief Called when a new music‑mode client connection is established.
     * @param c Pointer to the newly connected music‑mode client.
     */
    void onMusicConnect(AsyncClient *c);

    /**
     * @brief Timer callback to delete an AsyncClient after a short delay.
     * @param xTimer The FreeRTOS timer handle.
     */
    static void deleteClientCallback(TimerHandle_t xTimer);

    /**
     * @brief Schedules deferred deletion of a client to avoid race conditions.
     * @param c The AsyncClient instance to delete.
     */
    static void scheduleDeleteClient(AsyncClient *c);

    /**
     * @brief Callback triggered when the main client is disconnected.
     * @param c A pointer to the disconnected client.
     */
    void onMainClientDisconnect(const AsyncClient *c);

    /**
     * @brief Callback triggered when the music-mode client is disconnected.
     * @param c A pointer to the disconnected client.
     */
    void onMusicDisconnect(const AsyncClient *c);

    /**
     * @brief Callback for handling a new inbound client connection in music mode.
     * @param arg An argument pointer (unused).
     * @param client A pointer to the newly connected client.
     */
    static void handleNewClient(void *arg, AsyncClient *client);

    /**
     * @brief Callback that processes incoming data from the Yeelight device.
     * @param c A pointer to the client that received data.
     * @param data A pointer to the received data.
     * @param len The length of the received data.
     */
    void onData(AsyncClient *c, const void *data, size_t len);

    /**
     * @brief Parses a single discovery response and converts it into a YeelightDevice object.
     * @param response The raw discovery response string.
     * @return A YeelightDevice constructed from the provided response.
     */
    static YeelightDevice parseDiscoveryResponse(const char *response);

    /**
     * @brief Creates the TCP server for handling music mode. If already created, does nothing.
     * @return True if the server is created or already running, false on failure.
     */
    static bool createMusicModeServer();

    /**
     * @brief Sends the internal `set_music` command to enable or disable music mode.
     * @param power If true, enable music mode; if false, disable it.
     * @param host (Optional) The IP address for the music mode server (defaults to the Yeelight itself).
     * @param port (Optional) The port for music mode commands (defaults to 55443).
     * @return The response type indicating success or failure.
     */
    ResponseType set_music_command(bool power, const uint8_t *host = nullptr, uint16_t port = 55443);

    /**
     * @brief Sends a `start_cf` command to start a color flow.
     * @param count The number of times the flow will run (0 for infinite).
     * @param action The action to execute after the flow completes.
     * @param size The size of the flow expression array.
     * @param flow A pointer to the flow expression array.
     * @return The response type indicating success or failure.
     */
    ResponseType start_cf_command(uint8_t count, flow_action action, uint8_t size, const flow_expression *flow);

    /**
     * @brief Sends a `stop_cf` command to stop a running color flow.
     * @return The response type indicating success or failure.
     */
    ResponseType stop_cf_command();

    /**
     * @brief Establishes or re-establishes a connection to the Yeelight device.
     * @return The response type indicating success or failure.
     */
    ResponseType connect();

    /**
     * @brief Sends a generic JSON-formatted command to the Yeelight device.
     * @param method The method name to call on the device.
     * @param params A cJSON object containing the command parameters.
     * @return The response type indicating success or failure.
     */
    ResponseType send_command(const char *method, cJSON *params);

    /**
     * @brief Sends a `bg_set_power` command to control the background light's power state.
     * @param power True to turn on, false to turn off.
     * @param effect The transition effect (smooth or sudden).
     * @param duration The duration of the transition in milliseconds.
     * @param mode The operating mode for the device.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_set_power_command(bool power, effect effect = EFFECT_SMOOTH,
                                      uint16_t duration = 500, mode mode = MODE_CURRENT);

    /**
     * @brief Sends a `set_power` command to control the main light's power state.
     * @param power True to turn on, false to turn off.
     * @param effect The transition effect (smooth or sudden).
     * @param duration The duration of the transition in milliseconds.
     * @param mode The operating mode for the device.
     * @return The response type indicating success or failure.
     */
    ResponseType set_power_command(bool power, effect effect = EFFECT_SMOOTH,
                                   uint16_t duration = 500, mode mode = MODE_CURRENT);

    /**
     * @brief Sends a `bg_toggle` command to toggle the background light's power state.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_toggle_command();

    /**
     * @brief Sends a `toggle` command to toggle the main light's power state.
     * @return The response type indicating success or failure.
     */
    ResponseType toggle_command();

    /**
     * @brief Sends a `dev_toggle` command to toggle power across all light channels.
     * @return The response type indicating success or failure.
     */
    ResponseType dev_toggle_command();

    /**
     * @brief Checks the command response for a specific response ID.
     * @param id The response ID to check.
     * @return The response type indicating success, failure, or timeout.
     */
    ResponseType checkResponse(uint16_t id);

    /**
     * @brief Sends a `set_ct_abx` command to set the main light's color temperature.
     * @param ct_value The color temperature to set.
     * @param effect The transition effect (smooth or sudden).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType set_ct_abx_command(uint16_t ct_value, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * @brief Sends a `set_scene_ct` command to set a CT-based scene on the main light.
     * @param ct The color temperature.
     * @param bright The brightness level (0-100).
     * @return The response type indicating success or failure.
     */
    ResponseType set_scene_ct_command(uint16_t ct, uint8_t bright);

    /**
     * @brief Sends a `bg_set_ct_abx` command to set the background light's color temperature.
     * @param ct_value The color temperature.
     * @param effect The transition effect (smooth or sudden).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_set_ct_abx_command(uint16_t ct_value, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * @brief Sends a `bg_set_scene_ct` command to set a CT-based scene on the background light.
     * @param ct The color temperature.
     * @param bright The brightness level (0-100).
     * @return The response type indicating success or failure.
     */
    ResponseType bg_set_scene_ct_command(uint16_t ct, uint8_t bright);

    /**
     * @brief Sends a `set_scene_rgb` command to set an RGB-based scene on the main light.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param bright Brightness level (0-100).
     * @return The response type indicating success or failure.
     */
    ResponseType set_scene_rgb_command(uint8_t r, uint8_t g, uint8_t b, uint8_t bright);

    /**
     * @brief Sends a `bg_set_rgb` command to set the background light's color using RGB components.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param effect The transition effect (smooth or sudden).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_set_rgb_command(uint8_t r, uint8_t g, uint8_t b, effect effect = EFFECT_SMOOTH,
                                    uint16_t duration = 500);

    /**
     * @brief Sends a `bg_set_scene_rgb` command to set an RGB-based scene on the background light.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param bright Brightness level (0-100).
     * @return The response type indicating success or failure.
     */
    ResponseType bg_set_scene_rgb_command(uint8_t r, uint8_t g, uint8_t b, uint8_t bright);

    /**
     * @brief Sends a `set_rgb` command to set the main light's color using RGB components.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param effect The transition effect (smooth or sudden).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType set_rgb_command(uint8_t r, uint8_t g, uint8_t b, effect effect = EFFECT_SMOOTH,
                                 uint16_t duration = 500);

    /**
     * @brief Sends a `set_bright` command to set the main light's brightness.
     * @param bright The brightness level (0-100).
     * @param effect The transition effect (smooth or sudden).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType set_bright_command(uint8_t bright, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * @brief Sends a `bg_set_bright` command to set the background light's brightness.
     * @param bright The brightness level (0-100).
     * @param effect The transition effect (smooth or sudden).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_set_bright_command(uint8_t bright, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * @brief Sends a `set_scene_hsv` command to set an HSV-based scene on the main light.
     * @param hue The hue value (0-65535).
     * @param sat The saturation value (0-100).
     * @param bright The brightness level (0-100).
     * @return The response type indicating success or failure.
     */
    ResponseType set_scene_hsv_command(uint8_t hue, uint8_t sat, uint8_t bright);

    /**
     * @brief Sends a `set_hsv` command to set the main light's color using HSV components.
     * @param hue The hue value (0-65535).
     * @param sat The saturation value (0-100).
     * @param effect The transition effect (smooth or sudden).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType set_hsv_command(uint16_t hue, uint8_t sat, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * @brief Sends a `bg_set_hsv` command to set the background light's color using HSV components.
     * @param hue The hue value (0-65535).
     * @param sat The saturation value (0-100).
     * @param effect The transition effect (smooth or sudden).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_set_hsv_command(uint16_t hue, uint8_t sat, effect effect = EFFECT_SMOOTH, uint16_t duration = 500);

    /**
     * @brief Sends a `bg_set_scene_hsv` command to set an HSV-based scene on the background light.
     * @param hue The hue value (0-65535).
     * @param sat The saturation value (0-100).
     * @param bright The brightness level (0-100).
     * @return The response type indicating success or failure.
     */
    ResponseType bg_set_scene_hsv_command(uint8_t hue, uint8_t sat, uint8_t bright);

    /**
     * @brief Sends a `set_scene_auto_delay_off` command to schedule a turn-off after a specified time.
     * @param brightness The brightness level to use while the light is on.
     * @param duration The delay in milliseconds before turning off.
     * @return The response type indicating success or failure.
     */
    ResponseType set_scene_auto_delay_off_command(uint8_t brightness, uint32_t duration);

    /**
     * @brief Sends a `bg_set_scene_auto_delay_off` command to schedule a turn-off for the background light.
     * @param brightness The brightness level to use while the background light is on.
     * @param duration The delay in milliseconds before turning off.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_set_scene_auto_delay_off_command(uint8_t brightness, uint32_t duration);

    /**
     * @brief Sends a `cron_add` command to configure a timer for turning off.
     * @param time The time in minutes after which the device will turn off.
     * @return The response type indicating success or failure.
     */
    ResponseType cron_add_command(uint32_t time);

    /**
     * @brief Sends a `cron_del` command to remove any existing timer.
     * @return The response type indicating success or failure.
     */
    ResponseType cron_del_command();

    /**
     * @brief Sends a `set_default` command to set the current state as the default for the main light.
     * @return The response type indicating success or failure.
     */
    ResponseType set_default();

    /**
     * @brief Sends a `bg_set_default` command to set the current state as the default for the background light.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_set_default();

    /**
     * @brief Sends a `set_name` command to assign a custom name to the device.
     * @param name The new name for the device.
     * @return The response type indicating success or failure.
     */
    ResponseType set_name_command(const char *name);

    /**
     * @brief Sends an `adjust_bright` command to increase or decrease brightness.
     * @param percentage The brightness change in percent (negative to decrease).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType adjust_bright_command(int8_t percentage, uint16_t duration);

    /**
     * @brief Sends an `adjust_ct` command to increase or decrease color temperature.
     * @param percentage The color temperature change in percent (negative to decrease).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType adjust_ct_command(int8_t percentage, uint16_t duration);

    /**
     * @brief Sends an `adjust_color` command to adjust the color by a percentage.
     * @param percentage The color change in percent (negative to shift in opposite direction).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType adjust_color_command(int8_t percentage, uint16_t duration);

    /**
     * @brief Sends a `bg_adjust_bright` command to increase or decrease the background light brightness.
     * @param percentage The brightness change in percent (negative to decrease).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_adjust_bright_command(int8_t percentage, uint16_t duration);

    /**
     * @brief Sends a `bg_adjust_ct` command to adjust the background light's color temperature.
     * @param percentage The color temperature change in percent (negative to decrease).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_adjust_ct_command(int8_t percentage, uint16_t duration);

    /**
     * @brief Sends a `bg_adjust_color` command to adjust the background light's color by a percentage.
     * @param percentage The color change in percent (negative to shift in opposite direction).
     * @param duration The duration of the transition in milliseconds.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_adjust_color_command(int8_t percentage, uint16_t duration);

    /**
     * @brief Sends a `bg_start_cf` command to start a background color flow.
     * @param count The number of times the flow will run (0 for infinite).
     * @param action The action to execute after the flow completes.
     * @param size The size of the flow expression array.
     * @param flow A pointer to the flow expression array.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_start_cf_command(uint8_t count, flow_action action, uint8_t size, const flow_expression *flow);

    /**
     * @brief Sends a `bg_stop_cf` command to stop any running background color flow.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_stop_cf_command();

    /**
     * @brief Sends a `set_scene_cf` command to set a color flow scene on the main light.
     * @param count The number of times the flow will run (0 for infinite).
     * @param action The action to execute after the flow completes.
     * @param size The size of the flow expression array.
     * @param flow A pointer to the flow expression array.
     * @return The response type indicating success or failure.
     */
    ResponseType set_scene_cf_command(uint32_t count, flow_action action, uint32_t size, const flow_expression *flow);

    /**
     * @brief Sends a `bg_set_scene_cf` command to set a color flow scene on the background light.
     * @param count The number of times the flow will run (0 for infinite).
     * @param action The action to execute after the flow completes.
     * @param size The size of the flow expression array.
     * @param flow A pointer to the flow expression array.
     * @return The response type indicating success or failure.
     */
    ResponseType bg_set_scene_cf_command(uint32_t count, flow_action action, uint32_t size,
                                         const flow_expression *flow);

public:
    //---------------------------------------------------------------------------------------------------------
    // PUBLIC METHODS
    //---------------------------------------------------------------------------------------------------------

    //
    // 1) DISCOVERY
    //

    /**
     * @brief Scans the network to discover available Yeelight devices.
     * @param waitTimeMs The duration in milliseconds to wait for discovery responses (default 5000).
     * @return A vector of discovered YeelightDevice objects.
     */
    static std::vector<YeelightDevice> discoverYeelightDevices(int waitTimeMs = 5000);

    //
    // 2) CONSTRUCTORS AND DESTRUCTOR
    //

    /**
     * @brief Constructs a Yeelight object with a specified IP address and port.
     * @param ip The IP address as an array of 4 bytes.
     * @param port The port number (default 55443).
     */
    explicit Yeelight(const uint8_t ip[4], uint16_t port = 55443);

    /**
     * @brief Constructs a Yeelight object using a YeelightDevice structure.
     * @param device A YeelightDevice holding device connection details.
     */
    explicit Yeelight(const YeelightDevice &device);

    /**
     * @brief Default constructor for Yeelight.
     *
     * Creates a Yeelight instance without assigning an IP or port. They must be set before connecting.
     */
    Yeelight();

    /**
     * @brief Destructor. Closes any open connections and frees resources.
     */
    ~Yeelight();

    //
    // 3) CONNECTION METHODS
    //

    /**
     * @brief Connects to a Yeelight device given by IP and port.
     * @param ip The IP address as an array of 4 bytes.
     * @param port The port number (default 55443).
     * @return The response type indicating success or failure.
     */
    ResponseType connect(const uint8_t ip[4], uint16_t port = 55443);

    /**
     * @brief Connects to a Yeelight device based on a YeelightDevice structure.
     * @param device The YeelightDevice to connect to.
     * @return The response type indicating success or failure.
     */
    ResponseType connect(const YeelightDevice &device);

    /**
     * @brief Checks if the main TCP client is currently connected to the device.
     * @return True if connected, otherwise false.
     */
    bool is_connected() const;

    /**
     * @brief Checks if the music mode TCP client is currently connected.
     * @return True if music mode connection is established, otherwise false.
     */
    bool is_connected_music() const;

    //
    // 4) DEVICE CAPABILITIES AND PROPERTIES
    //

    /**
     * @brief Retrieves the device's supported methods, if discovered.
     * @return A SupportedMethods bitmask/structure describing available commands.
     */
    SupportedMethods getSupportedMethods() const;

    /**
     * @brief Refreshes the known supported methods from the device.
     *
     * Sends a request to the device to update the supported methods bitmask/structure.
     */
    void refreshSupportedMethods();

    /**
     * @brief Fetches the latest properties (power state, color, etc.) from the device.
     * @return The response type indicating success or failure.
     */
    ResponseType refreshProperties();

    /**
     * @brief Gets the most recently retrieved properties of the device.
     * @return A YeelightProperties structure containing the device's state.
     */
    YeelightProperties getProperties();

    //
    // 5) POWER CONTROL
    //

    /**
     * @brief Sets the power state of the device (on/off).
     * @param power True to turn on, false to turn off.
     * @param lightType The light channel to target (main, background, or auto-detect).
     * @return The response type indicating success or failure.
     */
    ResponseType set_power(bool power, LightType lightType = AUTO);

    /**
     * @brief Sets the power state of the device (on/off) with a specific effect.
     * @param power True to turn on, false to turn off.
     * @param effect The transition effect (smooth or sudden).
     * @param lightType The light channel to target (main, background, or auto-detect).
     * @return The response type indicating success or failure.
     */
    ResponseType set_power(bool power, effect effect, LightType lightType = AUTO);

    /**
     * @brief Sets the power state of the device (on/off) with a specific effect and duration.
     * @param power True to turn on, false to turn off.
     * @param effect The transition effect (smooth or sudden).
     * @param duration The duration of the transition in milliseconds.
     * @param lightType The light channel to target (main, background, or auto-detect).
     * @return The response type indicating success or failure.
     */
    ResponseType set_power(bool power, effect effect, uint16_t duration, LightType lightType = AUTO);

    /**
     * @brief Sets the power state of the device (on/off) with a specified mode.
     * @param power True to turn on, false to turn off.
     * @param mode The mode to apply (e.g., color mode, CT mode).
     * @param lightType The light channel to target (main, background, or auto-detect).
     * @return The response type indicating success or failure.
     */
    ResponseType set_power(bool power, mode mode, LightType lightType = AUTO);

    /**
     * @brief Sets the power state of the device (on/off) with a specific effect and mode.
     * @param power True to turn on, false to turn off.
     * @param effect The transition effect (smooth or sudden).
     * @param mode The mode to apply (e.g., color mode, CT mode).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_power(bool power, effect effect, mode mode, LightType lightType = AUTO);

    /**
     * @brief Sets the power state of the device (on/off) with effect, duration, and mode.
     * @param power True to turn on, false to turn off.
     * @param effect The transition effect (smooth or sudden).
     * @param duration The duration of the transition in milliseconds.
     * @param mode The mode to apply (e.g., color mode, CT mode).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_power(bool power, effect effect, uint16_t duration, mode mode, LightType lightType = AUTO);

    /**
     * @brief Toggles the power state of the device.
     * @param lightType The light channel to toggle (main, background, or auto-detect).
     * @return The response type indicating success or failure.
     */
    ResponseType toggle_power(LightType lightType = AUTO);

    /**
     * @brief Turns on the device (main or background light).
     * @param lightType The light channel to turn on (main, background, or auto-detect).
     * @return The response type indicating success or failure.
     */
    ResponseType turn_on(LightType lightType = AUTO);

    /**
     * @brief Turns on the device (main or background light) with a specific effect.
     * @param effect The transition effect (smooth or sudden).
     * @param lightType The light channel to turn on.
     * @return The response type indicating success or failure.
     */
    ResponseType turn_on(effect effect, LightType lightType = AUTO);

    /**
     * @brief Turns on the device (main or background) with a specific effect and duration.
     * @param effect The transition effect.
     * @param duration The duration in milliseconds.
     * @param lightType The light channel to turn on.
     * @return The response type indicating success or failure.
     */
    ResponseType turn_on(effect effect, uint16_t duration, LightType lightType = AUTO);

    /**
     * @brief Turns on the device (main or background) with a specified mode.
     * @param mode The mode to apply (e.g., color mode, CT mode).
     * @param lightType The light channel to turn on.
     * @return The response type indicating success or failure.
     */
    ResponseType turn_on(mode mode, LightType lightType = AUTO);

    /**
     * @brief Turns on the device (main or background) with a specified effect and mode.
     * @param effect The transition effect.
     * @param mode The mode to apply.
     * @param lightType The light channel to turn on.
     * @return The response type indicating success or failure.
     */
    ResponseType turn_on(effect effect, mode mode, LightType lightType = AUTO);

    /**
     * @brief Turns on the device (main or background) with effect, duration, and mode.
     * @param effect The transition effect.
     * @param duration The duration in milliseconds.
     * @param mode The mode to apply.
     * @param lightType The light channel to turn on.
     * @return The response type indicating success or failure.
     */
    ResponseType turn_on(effect effect, uint16_t duration, mode mode, LightType lightType = AUTO);

    /**
     * @brief Turns off the device (main or background light).
     * @param lightType The light channel to turn off.
     * @return The response type indicating success or failure.
     */
    ResponseType turn_off(LightType lightType = AUTO);

    /**
     * @brief Turns off the device with a specified effect.
     * @param effect The transition effect.
     * @param lightType The light channel to turn off.
     * @return The response type indicating success or failure.
     */
    ResponseType turn_off(effect effect, LightType lightType = AUTO);

    /**
     * @brief Turns off the device with a specified effect and duration.
     * @param effect The transition effect.
     * @param duration The duration of the transition in milliseconds.
     * @param lightType The light channel to turn off.
     * @return The response type indicating success or failure.
     */
    ResponseType turn_off(effect effect, uint16_t duration, LightType lightType = AUTO);

    /**
     * @brief Turns off the device with a specified mode.
     * @param mode The mode to apply (e.g., color mode, CT mode).
     * @param lightType The light channel to turn off.
     * @return The response type indicating success or failure.
     */
    ResponseType turn_off(mode mode, LightType lightType = AUTO);

    /**
     * @brief Turns off the device with a specified effect and mode.
     * @param effect The transition effect.
     * @param mode The mode to apply (e.g., color mode, CT mode).
     * @param lightType The light channel to turn off.
     * @return The response type indicating success or failure.
     */
    ResponseType turn_off(effect effect, mode mode, LightType lightType = AUTO);

    /**
     * @brief Turns off the device with effect, duration, and mode.
     * @param effect The transition effect.
     * @param duration The duration of the transition in milliseconds.
     * @param mode The mode to apply.
     * @param lightType The light channel to turn off.
     * @return The response type indicating success or failure.
     */
    ResponseType turn_off(effect effect, uint16_t duration, mode mode, LightType lightType = AUTO);

    //
    // 6) COLOR AND BRIGHTNESS
    //

    /**
     * @brief Sets the color temperature for the device.
     * @param ct_value The color temperature value.
     * @param lightType The light channel (main, background, or auto-detect).
     * @return The response type indicating success or failure.
     */
    ResponseType set_color_temp(uint16_t ct_value, LightType lightType = AUTO);

    /**
     * @brief Sets the color temperature with a specified effect.
     * @param ct_value The color temperature value.
     * @param effect The transition effect.
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_color_temp(uint16_t ct_value, effect effect, LightType lightType = AUTO);

    /**
     * @brief Sets the color temperature with effect and duration.
     * @param ct_value The color temperature value.
     * @param effect The transition effect.
     * @param duration The duration in milliseconds.
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_color_temp(uint16_t ct_value, effect effect, uint16_t duration, LightType lightType = AUTO);

    /**
     * @brief Sets both color temperature and brightness at once.
     * @param ct_value The color temperature value.
     * @param bright The brightness level (0-100).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_color_temp(uint16_t ct_value, uint8_t bright, LightType lightType = AUTO);

    /**
     * @brief Sets the brightness of the device.
     * @param bright The brightness level (0-100).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_brightness(uint8_t bright, LightType lightType = AUTO);

    /**
     * @brief Sets the brightness with a specified effect.
     * @param bright The brightness level (0-100).
     * @param effect The transition effect.
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_brightness(uint8_t bright, effect effect, LightType lightType = AUTO);

    /**
     * @brief Sets the brightness with effect and duration.
     * @param bright The brightness level (0-100).
     * @param effect The transition effect.
     * @param duration The duration in milliseconds.
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_brightness(uint8_t bright, effect effect, uint16_t duration, LightType lightType = AUTO);

    /**
     * @brief Sets the color using RGB components.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_rgb_color(uint8_t r, uint8_t g, uint8_t b, LightType lightType = AUTO);

    /**
     * @brief Sets the color using RGB components with a specified effect.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param effect The transition effect.
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_rgb_color(uint8_t r, uint8_t g, uint8_t b, effect effect, LightType lightType = AUTO);

    /**
     * @brief Sets the color using RGB components with effect and duration.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param effect The transition effect.
     * @param duration The duration in milliseconds.
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_rgb_color(uint8_t r, uint8_t g, uint8_t b, effect effect, uint16_t duration,
                               LightType lightType = AUTO);

    /**
     * @brief Sets the color using RGB components and brightness.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param bright The brightness level (0-100).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_rgb_color(uint8_t r, uint8_t g, uint8_t b, uint8_t bright, LightType lightType = AUTO);

    /**
     * @brief Sets the color using HSV components.
     * @param hue The hue value (0-65535).
     * @param sat The saturation value (0-100).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_hsv_color(uint16_t hue, uint8_t sat, LightType lightType = AUTO);

    /**
     * @brief Sets the color using HSV components with a specified effect.
     * @param hue The hue value (0-65535).
     * @param sat The saturation value (0-100).
     * @param effect The transition effect.
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_hsv_color(uint16_t hue, uint8_t sat, effect effect, LightType lightType = AUTO);

    /**
     * @brief Sets the color using HSV components with effect and duration.
     * @param hue The hue value (0-65535).
     * @param sat The saturation value (0-100).
     * @param effect The transition effect.
     * @param duration The duration in milliseconds.
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_hsv_color(uint16_t hue, uint8_t sat, effect effect, uint16_t duration,
                               LightType lightType = AUTO);

    /**
     * @brief Sets the color using HSV components and brightness.
     * @param hue The hue value (0-65535).
     * @param sat The saturation value (0-100).
     * @param bright The brightness level (0-100).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_hsv_color(uint16_t hue, uint8_t sat, uint8_t bright, LightType lightType = AUTO);

    //
    // 7) SCENES AND FLOWS
    //

    /**
     * @brief Sets an RGB-based scene.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @param bright Brightness level (0-100).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_scene_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t bright, LightType lightType = AUTO);

    /**
     * @brief Sets an HSV-based scene.
     * @param hue The hue value (0-65535).
     * @param sat The saturation value (0-100).
     * @param bright The brightness level (0-100).
     * @param lightType The light channel to target. Defaults to AUTO.
     * @return The response type indicating success or failure.
     */
    ResponseType set_scene_hsv(uint16_t hue, uint8_t sat, uint8_t bright, LightType lightType = AUTO);

    /**
     * @brief Sets a CT-based scene.
     * @param ct The color temperature.
     * @param bright The brightness level (0-100).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_scene_color_temperature(uint16_t ct, uint8_t bright, LightType lightType = AUTO);

    /**
     * @brief Sets an auto delay off scene.
     * @param brightness The brightness level (0-100).
     * @param duration The duration before the light turns off (milliseconds).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_scene_auto_delay_off(uint8_t brightness, uint32_t duration, LightType lightType = AUTO);

    /**
     * @brief Starts a color flow effect.
     * @param flow A Flow object specifying the steps and timings of the effect.
     * @param lightType The light channel to target (main, background, or auto-detect).
     * @return The response type indicating success or failure.
     */
    ResponseType start_flow(const Flow& flow, LightType lightType = AUTO);

    /**
     * @brief Stops any currently running color flow effect.
     * @param lightType The light channel to target (main, background, or auto-detect).
     * @return The response type indicating success or failure.
     */
    ResponseType stop_flow(LightType lightType = AUTO);

    /**
     * @brief Sets a color flow scene without manually constructing the JSON command.
     * @param flow The Flow object describing the scene flow steps.
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType set_scene_flow(const Flow& flow, LightType lightType = AUTO);

    //
    // 8) TIMERS AND DEFAULT STATES
    //

    /**
     * @brief Sets a turn-off delay using cron_add.
     * @param duration The duration in minutes before the device turns off.
     * @return The response type indicating success or failure.
     */
    ResponseType set_turn_off_delay(uint32_t duration);

    /**
     * @brief Removes any existing turn-off delay using cron_del.
     * @return The response type indicating success or failure.
     */
    ResponseType remove_turn_off_delay();

    /**
     * @brief Sets the current state as the default for main or background light.
     * @param lightType The light channel to apply this to.
     * @return The response type indicating success or failure.
     */
    ResponseType set_default_state(LightType lightType = AUTO);

    //
    // 9) DEVICE NAMING
    //

    /**
     * @brief Assigns a name to the Yeelight device.
     * @param name The new name for the device.
     * @return The response type indicating success or failure.
     */
    ResponseType set_device_name(const char *name);

    /**
     * @brief Assigns a name to the Yeelight device, using a std::string.
     * @param name The new name for the device.
     * @return The response type indicating success or failure.
     */
    ResponseType set_device_name(const std::string &name);

    //
    // 10) MUSIC MODE
    //

    /**
     * @brief Enables or disables music mode.
     * @param enabled True to enable, false to disable.
     * @return The response type indicating success or failure.
     */
    ResponseType set_music_mode(bool enabled);

    /**
     * @brief Enables music mode on the device.
     * @return The response type indicating success or failure.
     */
    ResponseType enable_music_mode();

    /**
     * @brief Disables music mode on the device.
     * @return The response type indicating success or failure.
     */
    ResponseType disable_music_mode();

    //
    // 11) ADJUSTMENTS (BRIGHTNESS, COLOR TEMP, COLOR)
    //

    /**
     * @brief Adjusts the brightness by a specified percentage.
     * @param percentage The amount (in percent) to adjust brightness (negative to decrease).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType adjust_brightness(int8_t percentage, LightType lightType = AUTO);

    /**
     * @brief Adjusts brightness by a specified percentage over a given duration.
     * @param percentage The amount (in percent) to adjust brightness (negative to decrease).
     * @param duration The duration in milliseconds.
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType adjust_brightness(int8_t percentage, uint16_t duration, LightType lightType = AUTO);

    /**
     * @brief Adjusts the color temperature by a specified percentage.
     * @param percentage The percentage to change color temperature (negative to decrease).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType adjust_color_temp(int8_t percentage, LightType lightType = AUTO);

    /**
     * @brief Adjusts the color temperature by a specified percentage over a given duration.
     * @param percentage The percentage to change color temperature (negative to decrease).
     * @param duration The duration in milliseconds.
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType adjust_color_temp(int8_t percentage, uint16_t duration, LightType lightType = AUTO);

    /**
     * @brief Adjusts the color by a specified percentage.
     * @param percentage The percentage to adjust the color (negative to shift in opposite direction).
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType adjust_color(int8_t percentage, LightType lightType = AUTO);

    /**
     * @brief Adjusts the color by a specified percentage over a given duration.
     * @param percentage The percentage to adjust the color (negative to shift in opposite direction).
     * @param duration The duration in milliseconds.
     * @param lightType The light channel to target.
     * @return The response type indicating success or failure.
     */
    ResponseType adjust_color(int8_t percentage, uint16_t duration, LightType lightType = AUTO);

    /**
     * @brief Sends a main-light-specific adjust action, used internally by the library.
     * @param action The action to perform (increase, decrease).
     * @param prop The property to be adjusted (bright, ct, color).
     */
    void set_adjust(adjust_action action, adjust_prop prop);

    /**
     * @brief Sends a BG-specific adjust action, used internally by the library.
     * @param action The action to perform (increase, decrease).
     * @param prop The property to be adjusted (bright, ct, color).
     */
    void bg_set_adjust(adjust_action action, adjust_prop prop);

    //
    // 12) TIMEOUT SETTINGS
    //

    /**
     * @brief Gets the current timeout value used for command responses.
     * @return The timeout in milliseconds.
     */
    std::uint16_t get_timeout() const;

    /**
     * @brief Sets the timeout value used for command responses.
     * @param timeout The new timeout in milliseconds.
     */
    void set_timeout(std::uint16_t timeout);
};

#endif
