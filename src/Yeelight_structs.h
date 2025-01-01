#ifndef YEELIGHTARDUINO_YEELIGHT_STRUCTS_H
#define YEELIGHTARDUINO_YEELIGHT_STRUCTS_H

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief Struct representing a flow expression for controlling Yeelight devices.
 *
 * A flow expression consists of a duration, flow mode, value, and brightness.
 * The duration specifies the total time in milliseconds for the flow to complete.
 * The flow mode determines the type of flow to be executed.
 * The value is a parameter specific to the flow mode.
 * The brightness specifies the brightness level for the flow.
 */
struct flow_expression
{
    uint32_t duration; /**< Duration of the flow expression in milliseconds */
    flow_mode mode;    /**< Flow mode */
    uint32_t value;    /**< Value specific to the flow mode */
    uint8_t brightness; /**< Brightness level for the flow */
};
/**
 * @brief Struct representing the supported methods of the Yeelight device.
 */
struct SupportedMethods
{
    bool get_prop;         /**< Get property method */
    bool set_ct_abx;       /**< Set color temperature method */
    bool set_rgb;          /**< Set RGB color method */
    bool set_hsv;          /**< Set HSV color method */
    bool set_bright;       /**< Set brightness method */
    bool set_power;        /**< Set power state method */
    bool toggle;           /**< Toggle power state method */
    bool set_default;      /**< Set default state method */
    bool start_cf;         /**< Start color flow method */
    bool stop_cf;          /**< Stop color flow method */
    bool set_scene;        /**< Set scene method */
    bool cron_add;         /**< Add cron job method */
    bool cron_get;         /**< Get cron job method */
    bool cron_del;         /**< Delete cron job method */
    bool set_adjust;       /**< Set adjust method */
    bool set_music;        /**< Set music mode method */
    bool set_name;         /**< Set device name method */
    bool bg_set_rgb;       /**< Set background RGB color method */
    bool bg_set_hsv;       /**< Set background HSV color method */
    bool bg_set_ct_abx;    /**< Set background color temperature method */
    bool bg_start_cf;      /**< Start background color flow method */
    bool bg_stop_cf;       /**< Stop background color flow method */
    bool bg_set_scene;     /**< Set background scene method */
    bool bg_set_default;   /**< Set background default state method */
    bool bg_set_power;     /**< Set background power state method */
    bool bg_set_bright;    /**< Set background brightness method */
    bool bg_set_adjust;    /**< Set background adjust method */
    bool bg_toggle;        /**< Toggle background power state method */
    bool dev_toggle;       /**< Toggle device power state method */
    bool adjust_bright;    /**< Adjust brightness method */
    bool adjust_ct;        /**< Adjust color temperature method */
    bool adjust_color;     /**< Adjust color method */
    bool bg_adjust_bright; /**< Adjust background brightness method */
    bool bg_adjust_ct;     /**< Adjust background color temperature method */
    bool bg_adjust_color;  /**< Adjust background color method */
};
/**
 * @brief Struct representing a Yeelight device.
 */
struct YeelightDevice
{
    uint8_t ip[4]{};                      /**< IP address of the device */
    uint16_t port{};                      /**< Port number of the device */
    std::string model;                  /**< Model of the device */
    uint16_t fw_ver{};                    /**< Firmware version of the device */
    bool power{};                         /**< Power state of the device */
    uint8_t bright{};                     /**< Brightness level of the device */
    uint16_t ct{};                        /**< Color temperature of the device */
    uint32_t rgb{};                       /**< RGB color value of the device */
    uint8_t hue{};                        /**< Hue value of the device */
    uint8_t sat{};                        /**< Saturation value of the device */
    std::string name;                   /**< Name of the device */
    SupportedMethods supported_methods{}; /**< Supported methods of the device */
};

/**
 * @brief Struct representing the properties of a Yeelight device.
 */
struct YeelightProperties
{
    bool power;                           /**< Power state of the device */
    uint8_t bright;                       /**< Brightness level of the device */
    uint16_t ct;                          /**< Color temperature of the device */
    uint32_t rgb;                         /**< RGB color value of the device */
    uint16_t hue;                         /**< Hue value of the device */
    uint8_t sat;                          /**< Saturation value of the device */
    Color_mode color_mode;                /**< Color mode of the device */
    bool flowing;                         /**< Flowing state of the device */
    uint8_t delayoff;                     /**< Delay off time in minutes */
    bool music_on;                        /**< Music mode state of the device */
    std::string name;                     /**< Name of the device */
    bool bg_power;                        /**< Background power state of the device */
    bool bg_flowing;                      /**< Background flowing state of the device */
    uint16_t bg_ct;                       /**< Background color temperature of the device */
    Color_mode bg_color_mode;             /**< Background color mode of the device */
    uint8_t bg_bright;                    /**< Background brightness level of the device */
    uint32_t bg_rgb;                      /**< Background RGB color value of the device */
    uint16_t bg_hue;                      /**< Background hue value of the device */
    uint8_t bg_sat;                       /**< Background saturation value of the device */
    uint8_t nl_br;                        /**< Night light brightness level of the device */
    bool active_mode;                     /**< Active mode state of the device */
};

#endif
