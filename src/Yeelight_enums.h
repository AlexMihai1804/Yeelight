#ifndef YEELIGHTARDUINO_YEELIGHT_ENUMS_H
#define YEELIGHTARDUINO_YEELIGHT_ENUMS_H
/**
 * @brief Enumeration of effects for controlling the transition effect of the Yeelight device.
 */
enum effect {
    EFFECT_SMOOTH,  /**< Smooth transition effect */
    EFFECT_SUDDEN   /**< Sudden transition effect */
};
/**
 * @brief Enumeration of modes for controlling the Yeelight device.
 */
enum mode {
    MODE_CURRENT,           /**< Current mode */
    MODE_CT,                /**< Color temperature mode */
    MODE_RGB,               /**< RGB mode */
    MODE_HSV,               /**< HSV mode */
    MODE_COLOR_FLOW,        /**< Color flow mode */
    MODE_NIGHT_LIGHT        /**< Night light mode */
};
/**
 * @brief Enumeration of flow modes for controlling the flow expression of the Yeelight device.
 */
enum flow_mode {
    FLOW_COLOR = 1,                 /**< Color flow mode */
    FLOW_COLOR_TEMPERATURE = 2,     /**< Color temperature flow mode */
    FLOW_SLEEP = 7                  /**< Sleep flow mode */
};
/**
 * @brief Enumeration of flow actions for controlling the flow expression of the Yeelight device.
 */
enum flow_action {
    FLOW_RECOVER = 0,   /**< Recover flow action */
    FLOW_STAY = 1,      /**< Stay flow action */
    FLOW_OFF = 2        /**< Turn off flow action */
};
/**
 * @brief Enumeration of adjust actions for adjusting properties of the Yeelight device.
 */
enum ajust_action {
    ADJUST_INCREASE,    /**< Increase adjust action */
    ADJUST_DECREASE,    /**< Decrease adjust action */
    ADJUST_CIRCLE       /**< Circle adjust action */
};
/**
 * @brief Enumeration of adjust properties for adjusting properties of the Yeelight device.
 */
enum ajust_prop {
    ADJUST_BRIGHT,      /**< Brightness adjust property */
    ADJUST_CT,          /**< Color temperature adjust property */
    ADJUST_COLOR        /**< Color adjust property */
};
/**
 * @brief Enumeration of response types for Yeelight commands.
 */
enum ResponseType {
    SUCCESS,                /**< Success response */
    DEVICE_NOT_FOUND,       /**< Device not found response */
    METHOD_NOT_SUPPORTED,   /**< Method not supported response */
    INVALID_PARAMS,         /**< Invalid parameters response */
    ERROR,                  /**< Error response */
    UNEXPECTED_RESPONSE,    /**< Unexpected response */
    TIMEOUT,                /**< Timeout response */
    CONNECTION_FAILED,      /**< Connection failed response */
    CONNECTION_LOST         /**< Connection lost response */
};
/**
 * @brief Enumeration of light types for controlling Yeelight devices.
 */
enum LightType {
    MAIN_LIGHT,         /**< Main light */
    BACKGROUND_LIGHT,   /**< Background light */
    BOTH,               /**< Both main light and background light */
    AUTO                /**< Auto light type */
};
#endif
