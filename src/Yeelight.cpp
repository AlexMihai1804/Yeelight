#include "Yeelight.h"
#include <WiFiUdp.h>
#include <cJSON.h>

ResponseType Yeelight::checkResponse() {
    unsigned long startTime = millis();
    String line;
    while ((millis() - startTime) < timeout) {
        if (client.available()) {
            line = client.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                cJSON *root = cJSON_Parse(line.c_str());
                if (root) {
                    cJSON *method = cJSON_GetObjectItem(root, "method");
                    cJSON *result = cJSON_GetObjectItem(root, "result");
                    cJSON *error = cJSON_GetObjectItem(root, "error");
                    if (method && strcmp(method->valuestring, "props") == 0) {
                        // TODO: Update properties
                        cJSON_Delete(root);
                        continue;
                    }
                    if (result && cJSON_IsArray(result)) {
                        cJSON *firstItem = cJSON_GetArrayItem(result, 0);
                        if (firstItem && cJSON_IsString(firstItem) && strcmp(firstItem->valuestring, "ok") == 0) {
                            cJSON_Delete(root);
                            return ResponseType::SUCCESS;
                        } else {
                            cJSON_Delete(root);
                            return ResponseType::UNEXPECTED_RESPONSE;
                        }
                    }
                    if (error) {
                        cJSON_Delete(root);
                        return ResponseType::ERROR;
                    }
                    cJSON_Delete(root);
                }
            }
        } else {
            delay(10);
        }
    }
    return ResponseType::TIMEOUT;
}

Yeelight::Yeelight(const uint8_t ip[4], const uint16_t port) : port(port) {
    for (int i = 0; i < 4; i++) {
        this->ip[i] = ip[i];
    }
    refreshSupportedMethods();
    connect();
}

Yeelight::Yeelight(const YeelightDevice &device) : port(device.port), supported_methods(device.supported_methods) {
    for (int i = 0; i < 4; i++) {
        ip[i] = device.ip[i];
    }
    connect();
}

SupportedMethods Yeelight::getSupportedMethods() {
    return supported_methods;
}

void Yeelight::refreshSupportedMethods() {
    WiFiUDP udp;
    IPAddress multicastIP(239, 255, 255, 250);
    unsigned int localUdpPort = 1982;
    const char *ssdpRequest =
            "M-SEARCH * HTTP/1.1\r\n"
            "HOST: 239.255.255.250:1982\r\n"
            "MAN: \"ssdp:discover\"\r\n"
            "ST: wifi_bulb\r\n\r\n";
    if (!udp.begin(localUdpPort)) {
    }
    udp.beginPacket(multicastIP, localUdpPort);
    udp.print(ssdpRequest);
    udp.endPacket();
    unsigned long startTime = millis();
    while (millis() - startTime < timeout) {
        int packetSize = udp.parsePacket();
        if (packetSize) {
            char packetBuffer[1024];
            int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
            if (len > 0) {
                packetBuffer[len] = '\0';
                YeelightDevice device = parseDiscoveryResponse(packetBuffer);
                if (memcmp(device.ip, ip, sizeof(ip)) == 0) {
                    supported_methods = device.supported_methods;
                    udp.stop();
                    break;
                }
            }
        }
    }
}

Yeelight::~Yeelight() {
    client.stop();
}

ResponseType Yeelight::connect() {
    client.connect(ip, port);
    if (client.connected()) {
        return ResponseType::SUCCESS;
    } else {
        return ResponseType::CONNECTION_FAILED;
    }
}

ResponseType Yeelight::send_command(const char *method, const char *params) {
    uint8_t current_retries = 0;
    while (!client.connected() && current_retries < max_retry) {
        connect();
        current_retries++;
        delay(250);
    }
    if (client.connected()) {
        char command[256];
        snprintf(command, sizeof(command), "{\"id\":1,\"method\":\"%s\",\"params\":%s}\r\n", method, params);
        client.print(command);
        return checkResponse();
    } else {
        return ResponseType::CONNECTION_LOST;
    }
}

ResponseType Yeelight::set_power_command(bool power, effect effect, uint16_t duration, mode mode) {
    if (!supported_methods.set_power) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (duration < 30) {
        return ResponseType::INVALID_PARAMS;
    }
    char params[256];
    if (mode == MODE_CURRENT) {
        snprintf(params, sizeof(params), R"(["%s","%s",%d])", power ? "on" : "off",
                 effect == EFFECT_SMOOTH ? "smooth" : "sudden", duration);
    } else {
        snprintf(params, sizeof(params), R"(["%s","%s",%d,%d])", power ? "on" : "off",
                 effect == EFFECT_SMOOTH ? "smooth" : "sudden", duration, mode);
    }
    return send_command("set_power", params);
}

ResponseType Yeelight::toggle_command() {
    return send_command("toggle", "[]");
}

ResponseType Yeelight::set_ct_abx_command(uint16_t ct_value, effect effect, uint16_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"([%d,"%s",%d])", ct_value, effect == EFFECT_SMOOTH ? "smooth" : "sudden",
             duration);
    return send_command("set_ct_abx", params);
}

ResponseType Yeelight::set_rgb_command(uint8_t r, uint8_t g, uint8_t b, effect effect, uint16_t duration) {
    uint32_t rgb = (r << 16) | (g << 8) | b;
    char params[256];
    snprintf(params, sizeof(params), R"([%d,"%s",%d])", rgb, effect == EFFECT_SMOOTH ? "smooth" : "sudden", duration);
    return send_command("set_rgb", params);
}

ResponseType Yeelight::set_hsv_command(uint16_t hue, uint8_t sat, effect effect, uint16_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"([%d,%d,"%s",%d])", hue, sat, effect == EFFECT_SMOOTH ? "smooth" : "sudden",
             duration);
    return send_command("set_hsv", params);
}

ResponseType Yeelight::set_bright_command(uint8_t bright, effect effect, uint16_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"([%d,"%s",%d])", bright, effect == EFFECT_SMOOTH ? "smooth" : "sudden",
             duration);
    return send_command("set_bright", params);
}

ResponseType Yeelight::set_default() {
    return send_command("set_default", "[]");
}

ResponseType Yeelight::start_cf_command(uint8_t count, flow_action action, uint8_t size, flow_expression *flow) {
    char params[1024] = "";
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "[%d,%d,\"", count, action);
    strncat(params, buffer, sizeof(params) - strlen(params) - 1);
    for (int i = 0; i < size - 1; i++) {
        snprintf(buffer, sizeof(buffer), "%d,%d,%d,%d,", flow[i].duration, flow[i].mode, flow[i].value,
                 flow[i].brightness);
        strncat(params, buffer, sizeof(params) - strlen(params) - 1);
    }
    snprintf(buffer, sizeof(buffer), "%d,%d,%d,%d\"]", flow[size - 1].duration, flow[size - 1].mode,
             flow[size - 1].value, flow[size - 1].brightness);
    strncat(params, buffer, sizeof(params) - strlen(params) - 1);
    return send_command("start_cf", params);
}

ResponseType Yeelight::stop_cf_command() {
    return send_command("stop_cf", "[]");
}

ResponseType Yeelight::set_scene_rgb_command(uint8_t r, uint8_t g, uint8_t b, uint8_t bright) {
    char params[256];
    uint32_t rgb = (r << 16) | (g << 8) | b;
    snprintf(params, sizeof(params), R"(["%s",%d,%d])", "color", rgb, bright);
    return send_command("set_scene", params);
}

ResponseType Yeelight::set_scene_hsv_command(uint8_t hue, uint8_t sat, uint8_t bright) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%s",%d,%d,%d])", "hsv", hue, sat, bright);
    return send_command("set_scene", params);
}

ResponseType Yeelight::set_scene_ct_command(uint16_t ct, uint8_t bright) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%s",%d,%d])", "ct", ct, bright);
    return send_command("set_scene", params);
}

ResponseType Yeelight::set_scene_auto_delay_off_command(uint8_t brightness, uint32_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%s",%d,%d])", "auto_delay_off", brightness, duration);
    return send_command("set_scene", params);
}

ResponseType Yeelight::set_scene_cf_command(uint32_t count, flow_action action, uint32_t size, flow_expression *flow) {
    char params[1024] = "";
    char buffer[256];
    snprintf(buffer, sizeof(buffer), R"(["cf",%d,%d,")", count, action);
    strncat(params, buffer, sizeof(params) - strlen(params) - 1);
    for (int i = 0; i < (int) size - 1; i++) {
        snprintf(buffer, sizeof(buffer), "%d,%d,%d,%d,", flow[i].duration, flow[i].mode, flow[i].value,
                 flow[i].brightness);
        strncat(params, buffer, sizeof(params) - strlen(params) - 1);
    }
    snprintf(buffer, sizeof(buffer), "%d,%d,%d,%d\"]", flow[size - 1].duration, flow[size - 1].mode,
             flow[size - 1].value, flow[size - 1].brightness);
    strncat(params, buffer, sizeof(params) - strlen(params) - 1);
    return send_command("set_scene", params);
}

ResponseType Yeelight::cron_add_command(uint32_t time) {
    char params[256];
    snprintf(params, sizeof(params), R"([0,%d])", time);
    return send_command("cron_add", params);
}

ResponseType Yeelight::cron_del_command() {
    return send_command("cron_del", "[]");
}

void Yeelight::set_adjust(ajust_action action, ajust_prop prop) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%s","%s"])",
             action == ADJUST_INCREASE ? "increase" : action == ADJUST_DECREASE ? "decrease" : "circle",
             prop == ADJUST_BRIGHT ? "bright" : prop == ADJUST_CT ? "ct" : "color");
    send_command("set_adjust", params);
}

ResponseType Yeelight::set_name_command(const char *name) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%s"])", name);
    return send_command("set_name", params);
}

ResponseType Yeelight::bg_set_power_command(bool power, effect effect, uint16_t duration, mode mode) {
    char params[256];
    if (mode == MODE_CURRENT) {
        snprintf(params, sizeof(params), R"(["%s","%s",%d])", power ? "on" : "off",
                 effect == EFFECT_SMOOTH ? "smooth" : "sudden", duration);
    } else {
        snprintf(params, sizeof(params), R"(["%s","%s",%d,%d])", power ? "on" : "off",
                 effect == EFFECT_SMOOTH ? "smooth" : "sudden", duration, mode);
    }
    return send_command("bg_set_power", params);
}

ResponseType Yeelight::bg_toggle_command() {
    return send_command("bg_toggle", "[]");
}

ResponseType Yeelight::bg_set_ct_abx_command(uint16_t ct_value, effect effect, uint16_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"([%d,"%s",%d])", ct_value, effect == EFFECT_SMOOTH ? "smooth" : "sudden",
             duration);
    return send_command("bg_set_ct_abx", params);
}

ResponseType Yeelight::bg_set_rgb_command(uint8_t r, uint8_t g, uint8_t b, effect effect, uint16_t duration) {
    uint32_t rgb = (r << 16) | (g << 8) | b;
    char params[256];
    snprintf(params, sizeof(params), R"([%d,"%s",%d])", rgb, effect == EFFECT_SMOOTH ? "smooth" : "sudden", duration);
    return send_command("bg_set_rgb", params);
}

ResponseType Yeelight::bg_set_hsv_command(uint16_t hue, uint8_t sat, effect effect, uint16_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"([%d,%d,"%s",%d])", hue, sat, effect == EFFECT_SMOOTH ? "smooth" : "sudden",
             duration);
    return send_command("bg_set_hsv", params);
}

ResponseType Yeelight::bg_set_bright_command(uint8_t bright, effect effect, uint16_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"([%d,"%s",%d])", bright, effect == EFFECT_SMOOTH ? "smooth" : "sudden",
             duration);
    return send_command("bg_set_bright", params);
}

ResponseType Yeelight::bg_set_default() {
    return send_command("bg_set_default", "[]");
}

ResponseType Yeelight::bg_set_scene_rgb_command(uint8_t r, uint8_t g, uint8_t b, uint8_t bright) {
    char params[256];
    uint32_t rgb = (r << 16) | (g << 8) | b;
    snprintf(params, sizeof(params), R"(["%s",%d,%d])", "color", rgb, bright);
    return send_command("bg_set_scene", params);
}

ResponseType Yeelight::bg_set_scene_hsv_command(uint8_t hue, uint8_t sat, uint8_t bright) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%s",%d,%d,%d])", "hsv", hue, sat, bright);
    return send_command("bg_set_scene", params);
}

ResponseType Yeelight::bg_set_scene_ct_command(uint16_t ct, uint8_t bright) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%s",%d,%d])", "ct", ct, bright);
    return send_command("bg_set_scene", params);
}

ResponseType Yeelight::bg_set_scene_auto_delay_off_command(uint8_t brightness, uint32_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%s",%d,%d])", "auto_delay_off", brightness, duration);
    return send_command("bg_set_scene", params);
}

ResponseType
Yeelight::bg_set_scene_cf_command(uint32_t count, flow_action action, uint32_t size, flow_expression *flow) {
    char params[1024] = "";
    char buffer[256];
    snprintf(buffer, sizeof(buffer), R"(["cf",%d,%d,")", count, action);
    strncat(params, buffer, sizeof(params) - strlen(params) - 1);
    for (int i = 0; i < (int) size - 1; i++) {
        snprintf(buffer, sizeof(buffer), "%d,%d,%d,%d,", flow[i].duration, flow[i].mode, flow[i].value,
                 flow[i].brightness);
        strncat(params, buffer, sizeof(params) - strlen(params) - 1);
    }
    snprintf(buffer, sizeof(buffer), "%d,%d,%d,%d\"]", flow[size - 1].duration, flow[size - 1].mode,
             flow[size - 1].value, flow[size - 1].brightness);
    strncat(params, buffer, sizeof(params) - strlen(params) - 1);
    return send_command("bg_set_scene", params);
}

void Yeelight::bg_set_adjust(ajust_action action, ajust_prop prop) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%s","%s"])",
             action == ADJUST_INCREASE ? "increase" : action == ADJUST_DECREASE ? "decrease" : "circle",
             prop == ADJUST_BRIGHT ? "bright" : prop == ADJUST_CT ? "ct" : "color");
    send_command("bg_set_adjust", params);
}

ResponseType Yeelight::dev_toggle_command() {
    return send_command("dev_toggle", "[]");
}

ResponseType Yeelight::adjust_bright_command(int8_t percentage, uint16_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%d","%d"])", percentage, duration);
    return send_command("adjust_bright", params);
}

ResponseType Yeelight::adjust_ct_command(int8_t percentage, uint16_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%d","%d"])", percentage, duration);
    return send_command("adjust_ct", params);
}

ResponseType Yeelight::adjust_color_command(int8_t percentage, uint16_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%d","%d"])", percentage, duration);
    return send_command("adjust_color", params);
}

ResponseType Yeelight::bg_adjust_bright_command(int8_t percentage, uint16_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%d","%d"])", percentage, duration);
    return send_command("bg_adjust_bright", params);
}

ResponseType Yeelight::bg_adjust_ct_command(int8_t percentage, uint16_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%d","%d"])", percentage, duration);
    return send_command("bg_adjust_ct", params);
}

ResponseType Yeelight::bg_adjust_color_command(int8_t percentage, uint16_t duration) {
    char params[256];
    snprintf(params, sizeof(params), R"(["%d","%d"])", percentage, duration);
    return send_command("bg_adjust_color", params);
}

std::vector<YeelightDevice> Yeelight::discoverYeelightDevices(int waitTimeMs) {
    WiFiUDP udp;
    IPAddress multicastIP(239, 255, 255, 250);
    unsigned int localUdpPort = 1982;
    const char *ssdpRequest =
            "M-SEARCH * HTTP/1.1\r\n"
            "HOST: 239.255.255.250:1982\r\n"
            "MAN: \"ssdp:discover\"\r\n"
            "ST: wifi_bulb\r\n\r\n";

    if (!udp.begin(localUdpPort)) {
        return {};
    }
    std::vector<YeelightDevice> devices;
    udp.beginPacket(multicastIP, localUdpPort);
    udp.print(ssdpRequest);
    udp.endPacket();
    unsigned long startTime = millis();
    while (millis() - startTime < (unsigned long) waitTimeMs) {
        int packetSize = udp.parsePacket();
        if (packetSize) {
            char packetBuffer[1024];
            int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
            if (len > 0) {
                packetBuffer[len] = '\0';
                YeelightDevice device = parseDiscoveryResponse(packetBuffer);
                bool found = false;
                for (YeelightDevice &d: devices) {
                    if (memcmp(d.ip, device.ip, sizeof(device.ip)) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    devices.push_back(device);
                }
            }
        }
    }
    udp.stop();
    return devices;
}

YeelightDevice Yeelight::parseDiscoveryResponse(const char *response) {
    YeelightDevice device;
    memset(&device, 0, sizeof(device));
    const char *location = strstr(response, "Location: yeelight://");
    if (location) {
        location += strlen("Location: yeelight://");
        sscanf(location, "%hhu.%hhu.%hhu.%hhu:%hu",
               &device.ip[0], &device.ip[1], &device.ip[2], &device.ip[3], &device.port);
    }
    const char *model = strstr(response, "\r\nmodel: ");
    if (model) {
        model += strlen("\r\nmodel: ");
        char modelStr[16];
        sscanf(model, "%15s", modelStr);
        device.model = modelStr;
    }
    const char *fw_ver = strstr(response, "\r\nfw_ver: ");
    if (fw_ver) {
        fw_ver += strlen("\r\nfw_ver: ");
        sscanf(fw_ver, "%hu", &device.fw_ver);
    }
    const char *power = strstr(response, "\r\npower: ");
    if (power) {
        power += strlen("\r\npower: ");
        char powerStr[8];
        sscanf(power, "%7s", powerStr);
        device.power = (strcmp(powerStr, "on") == 0);
    }
    const char *bright = strstr(response, "\r\nbright: ");
    if (bright) {
        bright += strlen("\r\nbright: ");
        sscanf(bright, "%hhu", &device.bright);
    }
    const char *ct = strstr(response, "\r\nct: ");
    if (ct) {
        ct += strlen("\r\nct: ");
        sscanf(ct, "%hu", &device.ct);
    }
    const char *rgb = strstr(response, "\r\nrgb: ");
    if (rgb) {
        rgb += strlen("\r\nrgb: ");
        sscanf(rgb, "%u", &device.rgb);
    }
    const char *hue = strstr(response, "\r\nhue: ");
    if (hue) {
        hue += strlen("\r\nhue: ");
        sscanf(hue, "%hhu", &device.hue);
    }
    const char *sat = strstr(response, "\r\nsat: ");
    if (sat) {
        sat += strlen("\r\nsat: ");
        sscanf(sat, "%hhu", &device.sat);
    }
    const char *name = strstr(response, "\r\nname: ");
    if (name) {
        name += strlen("\r\nname: ");
        char nameStr[64];
        sscanf(name, "%63[^\r\n]", nameStr);
        device.name = nameStr;
    }
    const char *support = strstr(response, "\r\nsupport: ");
    if (support) {
        support += strlen("\r\nsupport: ");
        char supportStr[256];
        sscanf(support, "%255[^\r\n]", supportStr);
        std::string supportList = supportStr;
        if (supportList.find("get_prop") != std::string::npos) {
            device.supported_methods.get_prop = true;
        }
        if (supportList.find("set_ct_abx") != std::string::npos) {
            device.supported_methods.set_ct_abx = true;
        }
        if (supportList.find("set_rgb") != std::string::npos) {
            device.supported_methods.set_rgb = true;
        }
        if (supportList.find("set_hsv") != std::string::npos) {
            device.supported_methods.set_hsv = true;
        }
        if (supportList.find("set_bright") != std::string::npos) {
            device.supported_methods.set_bright = true;
        }
        if (supportList.find("set_power") != std::string::npos) {
            device.supported_methods.set_power = true;
        }
        if (supportList.find("toggle") != std::string::npos) {
            device.supported_methods.toggle = true;
        }
        if (supportList.find("set_default") != std::string::npos) {
            device.supported_methods.set_default = true;
        }
        if (supportList.find("start_cf") != std::string::npos) {
            device.supported_methods.start_cf = true;
        }
        if (supportList.find("stop_cf") != std::string::npos) {
            device.supported_methods.stop_cf = true;
        }
        if (supportList.find("set_scene") != std::string::npos) {
            device.supported_methods.set_scene = true;
        }
        if (supportList.find("cron_add") != std::string::npos) {
            device.supported_methods.cron_add = true;
        }
        if (supportList.find("cron_get") != std::string::npos) {
            device.supported_methods.cron_get = true;
        }
        if (supportList.find("cron_del") != std::string::npos) {
            device.supported_methods.cron_del = true;
        }
        if (supportList.find("set_adjust") != std::string::npos) {
            device.supported_methods.set_adjust = true;
        }
        if (supportList.find("set_music") != std::string::npos) {
            device.supported_methods.set_music = true;
        }
        if (supportList.find("set_name") != std::string::npos) {
            device.supported_methods.set_name = true;
        }
        if (supportList.find("bg_set_rgb") != std::string::npos) {
            device.supported_methods.bg_set_rgb = true;
        }
        if (supportList.find("bg_set_hsv") != std::string::npos) {
            device.supported_methods.bg_set_hsv = true;
        }
        if (supportList.find("bg_set_ct_abx") != std::string::npos) {
            device.supported_methods.bg_set_ct_abx = true;
        }
        if (supportList.find("bg_start_cf") != std::string::npos) {
            device.supported_methods.bg_start_cf = true;
        }
        if (supportList.find("bg_stop_cf") != std::string::npos) {
            device.supported_methods.bg_stop_cf = true;
        }
        if (supportList.find("bg_set_scene") != std::string::npos) {
            device.supported_methods.bg_set_scene = true;
        }
        if (supportList.find("bg_set_default") != std::string::npos) {
            device.supported_methods.bg_set_default = true;
        }
        if (supportList.find("bg_set_power") != std::string::npos) {
            device.supported_methods.bg_set_power = true;
        }
        if (supportList.find("bg_set_bright") != std::string::npos) {
            device.supported_methods.bg_set_bright = true;
        }
        if (supportList.find("bg_set_adjust") != std::string::npos) {
            device.supported_methods.bg_set_adjust = true;
        }
        if (supportList.find("bg_toggle") != std::string::npos) {
            device.supported_methods.bg_toggle = true;
        }
        if (supportList.find("dev_toggle") != std::string::npos) {
            device.supported_methods.dev_toggle = true;
        }
        if (supportList.find("adjust_bright") != std::string::npos) {
            device.supported_methods.adjust_bright = true;
        }
        if (supportList.find("adjust_ct") != std::string::npos) {
            device.supported_methods.adjust_ct = true;
        }
        if (supportList.find("adjust_color") != std::string::npos) {
            device.supported_methods.adjust_color = true;
        }
        if (supportList.find("bg_adjust_bright") != std::string::npos) {
            device.supported_methods.bg_adjust_bright = true;
        }
        if (supportList.find("bg_adjust_ct") != std::string::npos) {
            device.supported_methods.bg_adjust_ct = true;
        }
        if (supportList.find("bg_adjust_color") != std::string::npos) {
            device.supported_methods.bg_adjust_color = true;
        }
    }
    return device;
}

ResponseType Yeelight::start_flow(Flow flow, LightType lightType) {
    if (!supported_methods.start_cf && !supported_methods.bg_start_cf) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (flow.get_size() == 0) {
        return ResponseType::INVALID_PARAMS;
    }
    if (flow.get_count() < 0) {
        return ResponseType::INVALID_PARAMS;
    }
    if (lightType == AUTO) {
        if (supported_methods.start_cf && supported_methods.bg_start_cf) {
            ResponseType response = start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(),
                                                     flow.get_flow().data());
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
        } else if (supported_methods.start_cf) {
            return start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
        } else {
            return bg_start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
        }
    } else if (lightType == MAIN_LIGHT) {
        return start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    } else if (lightType == BOTH) {
        ResponseType response = start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(),
                                                 flow.get_flow().data());
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::stop_flow(LightType lightType) {
    if (!supported_methods.stop_cf && !supported_methods.bg_stop_cf) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.stop_cf && supported_methods.bg_stop_cf) {
            ResponseType response = stop_cf_command();
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_stop_cf_command();
        } else if (supported_methods.stop_cf) {
            return stop_cf_command();
        } else {
            return bg_stop_cf_command();
        }
    } else if (lightType == MAIN_LIGHT) {
        return stop_cf_command();
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_stop_cf_command();
    } else if (lightType == BOTH) {
        ResponseType response = stop_cf_command();
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_stop_cf_command();
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::toggle_power(LightType lightType) {
    if (lightType == AUTO) {
        if (supported_methods.toggle && supported_methods.bg_toggle) {
            return dev_toggle_command();
        } else if (supported_methods.toggle) {
            return toggle_command();
        } else if (supported_methods.bg_toggle) {
            return bg_toggle_command();
        } else {
            return ResponseType::METHOD_NOT_SUPPORTED;
        }
    } else if (lightType == MAIN_LIGHT) {
        if (supported_methods.toggle) {
            return toggle_command();
        } else {
            return ResponseType::METHOD_NOT_SUPPORTED;
        }
    } else if (lightType == BACKGROUND_LIGHT) {
        if (supported_methods.bg_toggle) {
            return bg_toggle_command();
        } else {
            return ResponseType::METHOD_NOT_SUPPORTED;
        }
    } else if (lightType == BOTH) {
        if (supported_methods.toggle && supported_methods.bg_toggle) {
            return dev_toggle_command();
        } else {
            return ResponseType::METHOD_NOT_SUPPORTED;
        }
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_power(bool power, effect effect, uint16_t duration, mode mode, LightType lightType) {
    if (!supported_methods.set_power && !supported_methods.bg_set_power) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (duration < 30) {
        return ResponseType::INVALID_PARAMS;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_power && supported_methods.bg_set_power) {
            ResponseType response = set_power_command(power, effect, duration, mode);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_power_command(power, effect, duration, mode);
        } else if (supported_methods.set_power) {
            return set_power_command(power, effect, duration, mode);
        } else {
            return bg_set_power_command(power, effect, duration, mode);
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_power_command(power, effect, duration, mode);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_power_command(power, effect, duration, mode);
    } else if (lightType == BOTH) {
        ResponseType response = set_power_command(power, effect, duration, mode);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_power_command(power, effect, duration, mode);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_power(bool power, LightType lightType) {
    return set_power(power, EFFECT_SMOOTH, 500, MODE_CURRENT, lightType);
}

ResponseType Yeelight::set_power(bool power, effect effect, LightType lightType) {
    return set_power(power, effect, 500, MODE_CURRENT, lightType);
}

ResponseType Yeelight::set_power(bool power, effect effect, uint16_t duration, LightType lightType) {
    return set_power(power, effect, duration, MODE_CURRENT, lightType);
}

ResponseType Yeelight::set_power(bool power, mode mode, LightType lightType) {
    return set_power(power, EFFECT_SMOOTH, 500, mode, lightType);
}

ResponseType Yeelight::set_power(bool power, effect effect, mode mode, LightType lightType) {
    return set_power(power, effect, 500, mode, lightType);
}

ResponseType Yeelight::turn_on(LightType lightType) {
    return set_power(true, EFFECT_SMOOTH, 500, MODE_CURRENT, lightType);
}

ResponseType Yeelight::turn_on(effect effect, LightType lightType) {
    return set_power(true, effect, 500, MODE_CURRENT, lightType);
}

ResponseType Yeelight::turn_on(effect effect, uint16_t duration, LightType lightType) {
    return set_power(true, effect, duration, MODE_CURRENT, lightType);
}

ResponseType Yeelight::turn_on(mode mode, LightType lightType) {
    return set_power(true, EFFECT_SMOOTH, 500, mode, lightType);
}

ResponseType Yeelight::turn_on(effect effect, mode mode, LightType lightType) {
    return set_power(true, effect, 500, mode, lightType);
}

ResponseType Yeelight::turn_on(effect effect, uint16_t duration, mode mode, LightType lightType) {
    return set_power(true, effect, duration, mode, lightType);
}

ResponseType Yeelight::turn_off(LightType lightType) {
    return set_power(false, EFFECT_SMOOTH, 500, MODE_CURRENT, lightType);
}

ResponseType Yeelight::turn_off(effect effect, LightType lightType) {
    return set_power(false, effect, 500, MODE_CURRENT, lightType);
}

ResponseType Yeelight::turn_off(effect effect, uint16_t duration, LightType lightType) {
    return set_power(false, effect, duration, MODE_CURRENT, lightType);
}

ResponseType Yeelight::turn_off(mode mode, LightType lightType) {
    return set_power(false, EFFECT_SMOOTH, 500, mode, lightType);
}

ResponseType Yeelight::turn_off(effect effect, mode mode, LightType lightType) {
    return set_power(false, effect, 500, mode, lightType);
}

ResponseType Yeelight::turn_off(effect effect, uint16_t duration, mode mode, LightType lightType) {
    return set_power(false, effect, duration, mode, lightType);
}

ResponseType Yeelight::set_color_temp(uint16_t ct_value, LightType lightType) {
    return set_color_temp(ct_value, EFFECT_SMOOTH, 500, lightType);
}

ResponseType Yeelight::set_color_temp(uint16_t ct_value, effect effect, uint16_t duration, LightType lightType) {
    if (ct_value < 1700 || ct_value > 6500) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.set_ct_abx && !supported_methods.bg_set_ct_abx) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_ct_abx && supported_methods.bg_set_ct_abx) {
            ResponseType response = set_ct_abx_command(ct_value, effect, duration);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_ct_abx_command(ct_value, effect, duration);
        } else if (supported_methods.set_ct_abx) {
            return set_ct_abx_command(ct_value, effect, duration);
        } else {
            return bg_set_ct_abx_command(ct_value, effect, duration);
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_ct_abx_command(ct_value, effect, duration);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_ct_abx_command(ct_value, effect, duration);
    } else if (lightType == BOTH) {
        ResponseType response = set_ct_abx_command(ct_value, effect, duration);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_ct_abx_command(ct_value, effect, duration);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_color_temp(uint16_t ct_value, effect effect, LightType lightType) {
    return set_color_temp(ct_value, effect, 500, lightType);
}

ResponseType Yeelight::set_color_temp(uint16_t ct_value, uint8_t bright, LightType lightType) {
    if (ct_value < 1700 || ct_value > 6500) {
        return ResponseType::INVALID_PARAMS;
    }
    if (bright < 1 || bright > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            ResponseType response = set_scene_ct_command(ct_value, bright);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_scene_ct_command(ct_value, bright);
        } else if (supported_methods.set_scene) {
            return set_scene_ct_command(ct_value, bright);
        } else {
            return bg_set_scene_ct_command(ct_value, bright);
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_scene_ct_command(ct_value, bright);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_ct_command(ct_value, bright);
    } else if (lightType == BOTH) {
        ResponseType response = set_scene_ct_command(ct_value, bright);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_scene_ct_command(ct_value, bright);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_rgb_color(uint8_t r, uint8_t g, uint8_t b, LightType lightType) {
    return set_rgb_color(r, g, b, EFFECT_SMOOTH, 500, lightType);
}

ResponseType Yeelight::set_rgb_color(uint8_t r, uint8_t g, uint8_t b, effect effect, LightType lightType) {
    return set_rgb_color(r, g, b, effect, 500, lightType);
}

ResponseType
Yeelight::set_rgb_color(uint8_t r, uint8_t g, uint8_t b, effect effect, uint16_t duration, LightType lightType) {
    if (duration < 30) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.set_rgb && !supported_methods.bg_set_rgb) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_rgb && supported_methods.bg_set_rgb) {
            ResponseType response = set_rgb_command(r, g, b, effect, duration);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_rgb_command(r, g, b, effect, duration);
        } else if (supported_methods.set_rgb) {
            return set_rgb_command(r, g, b, effect, duration);
        } else {
            return bg_set_rgb_command(r, g, b, effect, duration);
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_rgb_command(r, g, b, effect, duration);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_rgb_command(r, g, b, effect, duration);
    } else if (lightType == BOTH) {
        ResponseType response = set_rgb_command(r, g, b, effect, duration);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_rgb_command(r, g, b, effect, duration);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_rgb_color(uint8_t r, uint8_t g, uint8_t b, uint8_t bright, LightType lightType) {
    if (bright < 1 || bright > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            ResponseType response = set_scene_rgb_command(r, g, b, bright);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_scene_rgb_command(r, g, b, bright);
        } else if (supported_methods.set_scene) {
            return set_scene_rgb_command(r, g, b, bright);
        } else {
            return bg_set_scene_rgb_command(r, g, b, bright);
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_scene_rgb_command(r, g, b, bright);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_rgb_command(r, g, b, bright);
    } else if (lightType == BOTH) {
        ResponseType response = set_scene_rgb_command(r, g, b, bright);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_scene_rgb_command(r, g, b, bright);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_brightness(uint8_t bright, LightType lightType) {
    return set_brightness(bright, EFFECT_SMOOTH, lightType);
}

ResponseType Yeelight::set_brightness(uint8_t bright, effect effect, LightType lightType) {
    return set_brightness(bright, effect, 500, lightType);
}

ResponseType Yeelight::set_brightness(uint8_t bright, effect effect, uint16_t duration, LightType lightType) {
    if (bright < 1 || bright > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (duration < 30) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.set_bright && !supported_methods.bg_set_bright) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_bright && supported_methods.bg_set_bright) {
            ResponseType response = set_bright_command(bright, effect, duration);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_bright_command(bright, effect, duration);
        } else if (supported_methods.set_bright) {
            return set_bright_command(bright, effect, duration);
        } else {
            return bg_set_bright_command(bright, effect, duration);
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_bright_command(bright, effect, duration);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_bright_command(bright, effect, duration);
    } else if (lightType == BOTH) {
        ResponseType response = set_bright_command(bright, effect, duration);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_bright_command(bright, effect, duration);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_hsv_color(uint16_t hue, uint8_t sat, LightType lightType) {
    return set_hsv_color(hue, sat, EFFECT_SMOOTH, 500, lightType);
}

ResponseType Yeelight::set_hsv_color(uint16_t hue, uint8_t sat, effect effect, LightType lightType) {
    return set_hsv_color(hue, sat, effect, 500, lightType);
}

ResponseType Yeelight::set_hsv_color(uint16_t hue, uint8_t sat, effect effect, uint16_t duration, LightType lightType) {
    if (hue > 359) {
        return ResponseType::INVALID_PARAMS;
    }
    if (sat > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (duration < 30) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.set_hsv && !supported_methods.bg_set_hsv) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_hsv && supported_methods.bg_set_hsv) {
            ResponseType response = set_hsv_command(hue, sat, effect, duration);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_hsv_command(hue, sat, effect, duration);
        } else if (supported_methods.set_hsv) {
            return set_hsv_command(hue, sat, effect, duration);
        } else {
            return bg_set_hsv_command(hue, sat, effect, duration);
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_hsv_command(hue, sat, effect, duration);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_hsv_command(hue, sat, effect, duration);
    } else if (lightType == BOTH) {
        ResponseType response = set_hsv_command(hue, sat, effect, duration);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_hsv_command(hue, sat, effect, duration);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_hsv_color(uint16_t hue, uint8_t sat, uint8_t bright, LightType lightType) {
    if (bright < 1 || bright > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (hue > 359) {
        return ResponseType::INVALID_PARAMS;
    }
    if (sat > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            ResponseType response = set_scene_hsv_command((uint8_t) hue, sat, bright);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_scene_hsv_command((uint8_t) hue, sat, bright);
        } else if (supported_methods.set_scene) {
            return set_scene_hsv_command((uint8_t) hue, sat, bright);
        } else {
            return bg_set_scene_hsv_command((uint8_t) hue, sat, bright);
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_scene_hsv_command((uint8_t) hue, sat, bright);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_hsv_command((uint8_t) hue, sat, bright);
    } else if (lightType == BOTH) {
        ResponseType response = set_scene_hsv_command((uint8_t) hue, sat, bright);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_scene_hsv_command((uint8_t) hue, sat, bright);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_scene_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t bright, LightType lightType) {
    if (bright < 1 || bright > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            ResponseType response = set_scene_rgb_command(r, g, b, bright);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_scene_rgb_command(r, g, b, bright);
        } else if (supported_methods.set_scene) {
            return set_scene_rgb_command(r, g, b, bright);
        } else {
            return bg_set_scene_rgb_command(r, g, b, bright);
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_scene_rgb_command(r, g, b, bright);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_rgb_command(r, g, b, bright);
    } else if (lightType == BOTH) {
        ResponseType response = set_scene_rgb_command(r, g, b, bright);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_scene_rgb_command(r, g, b, bright);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_scene_hsv(uint16_t hue, uint8_t sat, uint8_t bright, LightType lightType) {
    if (bright < 1 || bright > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (hue > 359) {
        return ResponseType::INVALID_PARAMS;
    }
    if (sat > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            ResponseType response = set_scene_hsv_command((uint8_t) hue, sat, bright);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_scene_hsv_command((uint8_t) hue, sat, bright);
        } else if (supported_methods.set_scene) {
            return set_scene_hsv_command((uint8_t) hue, sat, bright);
        } else {
            return bg_set_scene_hsv_command((uint8_t) hue, sat, bright);
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_scene_hsv_command((uint8_t) hue, sat, bright);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_hsv_command((uint8_t) hue, sat, bright);
    } else if (lightType == BOTH) {
        ResponseType response = set_scene_hsv_command((uint8_t) hue, sat, bright);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_scene_hsv_command((uint8_t) hue, sat, bright);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_scene_color_temperature(uint16_t ct, uint8_t bright, LightType lightType) {
    if (bright < 1 || bright > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (ct < 1700 || ct > 6500) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            ResponseType response = set_scene_ct_command(ct, bright);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_scene_ct_command(ct, bright);
        } else if (supported_methods.set_scene) {
            return set_scene_ct_command(ct, bright);
        } else {
            return bg_set_scene_ct_command(ct, bright);
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_scene_ct_command(ct, bright);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_ct_command(ct, bright);
    } else if (lightType == BOTH) {
        ResponseType response = set_scene_ct_command(ct, bright);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_scene_ct_command(ct, bright);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_scene_auto_delay_off(uint8_t brightness, uint32_t duration, LightType lightType) {
    if (brightness < 1 || brightness > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            ResponseType response = set_scene_auto_delay_off_command(brightness, duration);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_scene_auto_delay_off_command(brightness, duration);
        } else if (supported_methods.set_scene) {
            return set_scene_auto_delay_off_command(brightness, duration);
        } else {
            return bg_set_scene_auto_delay_off_command(brightness, duration);
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_scene_auto_delay_off_command(brightness, duration);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_auto_delay_off_command(brightness, duration);
    } else if (lightType == BOTH) {
        ResponseType response = set_scene_auto_delay_off_command(brightness, duration);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_scene_auto_delay_off_command(brightness, duration);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_turn_off_delay(uint32_t duration) {
    if (!supported_methods.cron_add) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    return cron_add_command(duration);
}

ResponseType Yeelight::remove_turn_off_delay() {
    if (!supported_methods.cron_del) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    return cron_del_command();
}

ResponseType Yeelight::set_default_state(LightType lightType) {
    if (!supported_methods.set_default && !supported_methods.bg_set_default) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_default && supported_methods.bg_set_default) {
            ResponseType response = set_default();
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_default();
        } else if (supported_methods.set_default) {
            return set_default();
        } else {
            return bg_set_default();
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_default();
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_default();
    } else if (lightType == BOTH) {
        ResponseType response = set_default();
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_default();
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::set_device_name(const char *name) {
    if (!supported_methods.set_name) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    return set_name_command(name);
}

ResponseType Yeelight::set_device_name(const std::string &name) {
    return set_device_name(name.c_str());
}

ResponseType Yeelight::adjust_brightness(int8_t percentage, LightType lightType) {
    return adjust_brightness(percentage, 500, lightType);
}

ResponseType Yeelight::adjust_brightness(int8_t percentage, uint16_t duration, LightType lightType) {
    if (percentage < -100 || percentage > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (duration < 30) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.set_adjust && !supported_methods.bg_set_adjust) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.adjust_bright && supported_methods.bg_adjust_bright) {
            ResponseType response = adjust_bright_command(percentage, duration);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_adjust_bright_command(percentage, duration);
        } else if (supported_methods.adjust_bright) {
            return adjust_bright_command(percentage, duration);
        } else {
            return bg_adjust_bright_command(percentage, duration);
        }
    } else if (lightType == MAIN_LIGHT) {
        return adjust_bright_command(percentage, duration);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_adjust_bright_command(percentage, duration);
    } else if (lightType == BOTH) {
        ResponseType response = adjust_bright_command(percentage, duration);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_adjust_bright_command(percentage, duration);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::adjust_color_temp(int8_t percentage, LightType lightType) {
    return adjust_color_temp(percentage, 500, lightType);
}

ResponseType Yeelight::adjust_color_temp(int8_t percentage, uint16_t duration, LightType lightType) {
    if (percentage < -100 || percentage > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (duration < 30) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.adjust_ct && !supported_methods.bg_adjust_ct) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.adjust_ct && supported_methods.bg_adjust_ct) {
            ResponseType response = adjust_ct_command(percentage, duration);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_adjust_ct_command(percentage, duration);
        } else if (supported_methods.adjust_ct) {
            return adjust_ct_command(percentage, duration);
        } else {
            return bg_adjust_ct_command(percentage, duration);
        }
    } else if (lightType == MAIN_LIGHT) {
        return adjust_ct_command(percentage, duration);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_adjust_ct_command(percentage, duration);
    } else if (lightType == BOTH) {
        ResponseType response = adjust_ct_command(percentage, duration);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_adjust_ct_command(percentage, duration);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::adjust_color(int8_t percentage, LightType lightType) {
    return adjust_color(percentage, 500, lightType);
}

ResponseType Yeelight::adjust_color(int8_t percentage, uint16_t duration, LightType lightType) {
    if (percentage < -100 || percentage > 100) {
        return ResponseType::INVALID_PARAMS;
    }
    if (duration < 30) {
        return ResponseType::INVALID_PARAMS;
    }
    if (!supported_methods.adjust_color && !supported_methods.bg_adjust_color) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.adjust_color && supported_methods.bg_adjust_color) {
            ResponseType response = adjust_color_command(percentage, duration);
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_adjust_color_command(percentage, duration);
        } else if (supported_methods.adjust_color) {
            return adjust_color_command(percentage, duration);
        } else {
            return bg_adjust_color_command(percentage, duration);
        }
    } else if (lightType == MAIN_LIGHT) {
        return adjust_color_command(percentage, duration);
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_adjust_color_command(percentage, duration);
    } else if (lightType == BOTH) {
        ResponseType response = adjust_color_command(percentage, duration);
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_adjust_color_command(percentage, duration);
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::bg_start_cf_command(uint8_t count, flow_action action, uint8_t size, flow_expression *flow) {
    char params[1024] = "";
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "[%d,%d,\"", count, action);
    strncat(params, buffer, sizeof(params) - strlen(params) - 1);
    for (int i = 0; i < size - 1; i++) {
        snprintf(buffer, sizeof(buffer), "%d,%d,%d,%d,", flow[i].duration, flow[i].mode, flow[i].value,
                 flow[i].brightness);
        strncat(params, buffer, sizeof(params) - strlen(params) - 1);
    }
    snprintf(buffer, sizeof(buffer), "%d,%d,%d,%d\"]", flow[size - 1].duration, flow[size - 1].mode,
             flow[size - 1].value, flow[size - 1].brightness);
    strncat(params, buffer, sizeof(params) - strlen(params) - 1);
    return send_command("bg_start_cf", params);
}

ResponseType Yeelight::bg_stop_cf_command() {
    return send_command("bg_stop_cf", "");
}

ResponseType Yeelight::set_scene_flow(Flow flow, LightType lightType) {
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    if (flow.get_size() == 0) {
        return ResponseType::INVALID_PARAMS;
    }
    if (flow.get_count() < 0) {
        return ResponseType::INVALID_PARAMS;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            ResponseType response = set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(),
                                                         flow.get_flow().data());
            if (response != ResponseType::SUCCESS) {
                return response;
            }
            return bg_set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
        } else if (supported_methods.set_scene) {
            return set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
        } else {
            return bg_set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
        }
    } else if (lightType == MAIN_LIGHT) {
        return set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    } else if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    } else if (lightType == BOTH) {
        ResponseType response = set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(),
                                                     flow.get_flow().data());
        if (response != ResponseType::SUCCESS) {
            return response;
        }
        return bg_set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    }
    return ResponseType::ERROR;
}

ResponseType Yeelight::refreshProperties() {
    if (!supported_methods.get_prop) {
        return ResponseType::METHOD_NOT_SUPPORTED;
    }
    char params[] =
            R"("power","bright","ct","rgb","hue","sat","color_mode","flowing","delayoff","music_on","name","bg_power","bg_flowing","bg_ct","bg_lmode","bg_bright","bg_rgb","bg_hue","bg_sat","nl_br","active_mode")";
    uint8_t current_retries = 0;
    while (!client.connected() && current_retries < max_retry) {
        connect();
        current_retries++;
        delay(250);
    }
    if (!client.connected()) {
        return ResponseType::CONNECTION_LOST;
    }

    char command[512];
    snprintf(command, sizeof(command), "{\"id\":1,\"method\":\"get_prop\",\"params\":[%s]}\r\n", params);
    client.print(command);

    unsigned long startTime = millis();
    String response = "";

    while (millis() - startTime < timeout) {
        while (client.available()) {
            String chunk = client.readString();
            response += chunk;
        }

        // If we found "result" or "error" in response, probably complete
        if (response.indexOf("\"result\"") != -1 || response.indexOf("\"error\"") != -1) {
            break;
        }

        delay(10);
    }

    if (response.length() == 0) {
        return ResponseType::TIMEOUT;
    }

    cJSON *root = cJSON_Parse(response.c_str());
    if (root == nullptr) {
        cJSON_Delete(root);
        return ResponseType::ERROR;
    }

    cJSON *result = cJSON_GetObjectItem(root, "result");
    if (result == nullptr || !cJSON_IsArray(result)) {
        cJSON_Delete(root);
        return ResponseType::ERROR;
    }

    // Parse properties safely
    auto getArrayString = [&](int idx) -> std::string {
        cJSON *item = cJSON_GetArrayItem(result, idx);
        if (item && cJSON_IsString(item)) {
            return std::string(item->valuestring);
        }
        return "";
    };

    std::string power_str = getArrayString(0);
    properties.power = (power_str == "on");

    std::string bright_str = getArrayString(1);
    properties.bright = bright_str.empty() ? 0 : (uint8_t) atoi(bright_str.c_str());

    std::string ct_str = getArrayString(2);
    properties.ct = ct_str.empty() ? 0 : (uint16_t) atoi(ct_str.c_str());

    std::string rgb_str = getArrayString(3);
    properties.rgb = rgb_str.empty() ? 0 : (uint32_t) atoi(rgb_str.c_str());

    std::string hue_str = getArrayString(4);
    properties.hue = hue_str.empty() ? 0 : (uint16_t) atoi(hue_str.c_str());

    std::string sat_str = getArrayString(5);
    properties.sat = sat_str.empty() ? 0 : (uint8_t) atoi(sat_str.c_str());

    std::string color_mode_str = getArrayString(6);
    if (!color_mode_str.empty()) {
        uint8_t color_mode_int = (uint8_t) atoi(color_mode_str.c_str());
        if (color_mode_int == 1) {
            properties.color_mode = COLOR_MODE_RGB;
        } else if (color_mode_int == 2) {
            properties.color_mode = COLOR_MODE_COLOR_TEMPERATURE;
        } else if (color_mode_int == 3) {
            properties.color_mode = COLOR_MODE_HSV;
        } else {
            properties.color_mode = COLOR_MODE_UNKNOWN;
        }
    } else {
        properties.color_mode = COLOR_MODE_UNKNOWN;
    }

    std::string flowing_str = getArrayString(7);
    properties.flowing = (!flowing_str.empty() && flowing_str == "1");

    std::string delayoff_str = getArrayString(8);
    properties.delayoff = delayoff_str.empty() ? 0 : (uint8_t) atoi(delayoff_str.c_str());

    std::string music_on_str = getArrayString(9);
    properties.music_on = (!music_on_str.empty() && music_on_str == "1");

    std::string name_str = getArrayString(10);
    properties.name = name_str;

    std::string bg_power_str = getArrayString(11);
    properties.bg_power = (bg_power_str == "on");

    std::string bg_flowing_str = getArrayString(12);
    properties.bg_flowing = (!bg_flowing_str.empty() && bg_flowing_str == "1");

    std::string bg_ct_str = getArrayString(13);
    properties.bg_ct = bg_ct_str.empty() ? 0 : (uint16_t) atoi(bg_ct_str.c_str());

    std::string bg_lmode_str = getArrayString(14);
    if (!bg_lmode_str.empty()) {
        uint8_t bg_lmode_int = (uint8_t) atoi(bg_lmode_str.c_str());
        if (bg_lmode_int == 1) {
            properties.bg_color_mode = COLOR_MODE_RGB;
        } else if (bg_lmode_int == 2) {
            properties.bg_color_mode = COLOR_MODE_COLOR_TEMPERATURE;
        } else if (bg_lmode_int == 3) {
            properties.bg_color_mode = COLOR_MODE_HSV;
        } else {
            properties.bg_color_mode = COLOR_MODE_UNKNOWN;
        }
    } else {
        properties.bg_color_mode = COLOR_MODE_UNKNOWN;
    }

    std::string bg_bright_str = getArrayString(15);
    properties.bg_bright = bg_bright_str.empty() ? 0 : (uint8_t) atoi(bg_bright_str.c_str());

    std::string bg_rgb_str = getArrayString(16);
    properties.bg_rgb = bg_rgb_str.empty() ? 0 : (uint32_t) atoi(bg_rgb_str.c_str());

    std::string bg_hue_str = getArrayString(17);
    properties.bg_hue = bg_hue_str.empty() ? 0 : (uint16_t) atoi(bg_hue_str.c_str());

    std::string bg_sat_str = getArrayString(18);
    properties.bg_sat = bg_sat_str.empty() ? 0 : (uint8_t) atoi(bg_sat_str.c_str());

    std::string nl_br_str = getArrayString(19);
    properties.nl_br = nl_br_str.empty() ? 0 : (uint8_t) atoi(nl_br_str.c_str());

    std::string active_mode_str = getArrayString(20);
    // active_mode is 0 or 1
    properties.active_mode = (!active_mode_str.empty() && active_mode_str == "1");

    cJSON_Delete(root);
    return ResponseType::SUCCESS;
}

YeelightProperties Yeelight::getProperties() {
    return properties;
}

ResponseType Yeelight::connect(const uint8_t *ip, uint16_t port) {
    if (client.connected()) {
        client.stop();
    }
    for (uint8_t i = 0; i < 4; i++) {
        this->ip[i] = ip[i];
    }
    this->port = port;
    refreshSupportedMethods();
    return connect();
}

ResponseType Yeelight::connect(const YeelightDevice &device) {
    for (int i = 0; i < 4; ++i) {
        this->ip[i] = device.ip[i];
    }
    this->port = device.port;
    supported_methods = device.supported_methods;
    return connect();
}

Yeelight::Yeelight() {
    for (uint8_t &i: ip) {
        i = 0;
    }
    port = 0;
    timeout = 1000;
    max_retry = 5;
    supported_methods = {};
    properties = {};
}

bool Yeelight::is_connected() {
    return client.connected();
}

void Yeelight::set_timeout(uint16_t timeout) {
    this->timeout = timeout;
}

std::uint16_t Yeelight::get_timeout() const {
    return timeout;
}
