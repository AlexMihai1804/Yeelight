#include "Yeelight.h"
#include <cJSON.h>
#include <WiFi.h>
#include <WiFiUdp.h>
std::map<uint32_t, Yeelight *> Yeelight::devices;
AsyncServer *Yeelight::music_mode_server = nullptr;
SemaphoreHandle_t Yeelight::devices_mutex = nullptr;
static SemaphoreHandle_t responses_mutex = nullptr;

static inline void lockResponses() {
    xSemaphoreTake(responses_mutex, portMAX_DELAY);
}

static inline void unlockResponses() {
    xSemaphoreGive(responses_mutex);
}

Yeelight::Yeelight() : port(0), supported_methods(), timeout(7000), max_retry(5), properties(), response_id(1),
                       music_mode(false), connecting(false) {
    memset(ip, 0, sizeof(ip));
    if (!devices_mutex) {
        devices_mutex = xSemaphoreCreateMutex();
        if (!devices_mutex) Serial.println("Failed to create devices mutex");
    }
    if (!responses_mutex) {
        responses_mutex = xSemaphoreCreateMutex();
        if (!responses_mutex) Serial.println("Failed to create responses mutex");
    }
    music_sem = xSemaphoreCreateBinary();
}

Yeelight::Yeelight(const uint8_t ip[4], const uint16_t port) : port(port), supported_methods(), timeout(7000),
                                                               max_retry(3), response_id(1), music_mode(false),
                                                               connecting(false) {
    memcpy(this->ip, ip, 4);
    if (!devices_mutex) {
        devices_mutex = xSemaphoreCreateMutex();
        if (!devices_mutex) Serial.println("Failed to create devices mutex");
    }
    if (!responses_mutex) {
        responses_mutex = xSemaphoreCreateMutex();
        if (!responses_mutex) Serial.println("Failed to create responses mutex");
    }
    music_sem = xSemaphoreCreateBinary();
    const uint32_t ip32 = ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];
    if (xSemaphoreTake(devices_mutex, portMAX_DELAY) == pdTRUE) {
        devices[ip32] = this;
        xSemaphoreGive(devices_mutex);
    }
    uint8_t current_retries = 0;
    while (!refreshedMethods && current_retries < max_retry) {
        refreshSupportedMethods();
        ++current_retries;
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    connect();
}

Yeelight::Yeelight(const YeelightDevice &device) : port(device.port), supported_methods(device.supported_methods),
                                                   timeout(7000), max_retry(3), response_id(1), music_mode(false),
                                                   connecting(false) {
    memcpy(ip, device.ip, 4);
    if (!devices_mutex) {
        devices_mutex = xSemaphoreCreateMutex();
        if (!devices_mutex) Serial.println("Failed to create devices mutex");
    }
    if (!responses_mutex) {
        responses_mutex = xSemaphoreCreateMutex();
        if (!responses_mutex) Serial.println("Failed to create responses mutex");
    }
    music_sem = xSemaphoreCreateBinary();
    const uint32_t ip32 = device.ip[0] << 24 | device.ip[1] << 16 | device.ip[2] << 8 | device.ip[3];
    if (xSemaphoreTake(devices_mutex, portMAX_DELAY) == pdTRUE) {
        devices[ip32] = this;
        xSemaphoreGive(devices_mutex);
    }
    connect();
}

Yeelight::~Yeelight() {
    if (client) {
        closingManually = true;
        client->close();
        scheduleDeleteClient(client);
        client = nullptr;
    }
    if (music_client) {
        music_client->close();
        scheduleDeleteClient(music_client);
        music_client = nullptr;
    }
    if (music_mode_server) {
        music_mode_server->end();
        delete music_mode_server;
        music_mode_server = nullptr;
    }
    const uint32_t ip32 = ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];
    if (xSemaphoreTake(devices_mutex, portMAX_DELAY) == pdTRUE) {
        auto it = devices.find(ip32);
        if (it != devices.end() && it->second == this) {
            devices.erase(it);
        }
        xSemaphoreGive(devices_mutex);
    }
    if (music_sem) {
        vSemaphoreDelete(music_sem);
    }
}

ResponseType Yeelight::checkResponse(const uint16_t id) {
    const uint32_t start = millis();
    while (millis() - start < timeout) {
        lockResponses();
        auto it = responses.find(id);
        if (it != responses.end()) {
            ResponseType r = it->second;
            responses.erase(it);
            unlockResponses();
            return r;
        }
        unlockResponses();
        if (!is_connected()) {
            return CONNECTION_LOST;
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    return TIMEOUT;
}

inline void Yeelight::safeInsertDevice(uint32_t ip32) {
    if (xSemaphoreTake(devices_mutex, portMAX_DELAY) == pdTRUE) {
        auto old = devices.find(ip32);
        if (old != devices.end() && old->second != this) {
            devices.erase(old);
        }
        devices[ip32] = this;
        xSemaphoreGive(devices_mutex);
    }
}

ResponseType Yeelight::connect() {
    if (connecting) {
        return IN_PROGRESS;
    }
    connecting = true;
    if (client) {
        closingManually = true;
        client->close();
        scheduleDeleteClient(client);
        client = nullptr;
    }
    client = new AsyncClient();
    if (!client) {
        connecting = false;
        return ERROR;
    }
    client->onConnect([](void *arg, AsyncClient *c) {
        auto *that = static_cast<Yeelight *>(arg);
        that->connecting = false;
        that->onMainClientConnect(c);
    }, this);
    client->onDisconnect([](void *arg, const AsyncClient *c) {
        auto *that = static_cast<Yeelight *>(arg);
        that->onMainClientDisconnect(c);
    }, this);
    client->onError([](void *arg, AsyncClient *c, int8_t error) {
        auto *that = static_cast<Yeelight *>(arg);
        that->onMainClientError(c, error);
    }, this);
    client->onData([](void *arg, AsyncClient *c, const void *d, size_t l) {
        static_cast<Yeelight *>(arg)->onData(c, d, l);
    }, this);
    IPAddress devIP(ip[0], ip[1], ip[2], ip[3]);
    if (!client->connect(devIP, port)) {
        delete client;
        client = nullptr;
        connecting = false;
        return CONNECTION_FAILED;
    }
    return SUCCESS;
}

ResponseType Yeelight::send_command(const char *method, cJSON *params) {
    if (music_mode && is_connected_music()) {
        cJSON *root = cJSON_CreateObject();
        if (!root) {
            cJSON_Delete(params);
            return ERROR;
        }
        cJSON_AddNumberToObject(root, "id", response_id++);
        cJSON_AddStringToObject(root, "method", method);
        cJSON_AddItemToObject(root, "params", params);
        char *command = cJSON_PrintUnformatted(root);
        if (!command) {
            cJSON_Delete(root);
            return ERROR;
        }
        music_client->write(command, strlen(command));
        music_client->write("\r\n", 2);
        cJSON_Delete(root);
        free(command);
        return SUCCESS;
    }
    if (!is_connected()) {
        connect();
    }
    uint32_t waited = 0;
    while (!is_connected() && waited < timeout) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        waited += 10;
    }
    if (!is_connected()) {
        cJSON_Delete(params);
        return CONNECTION_LOST;
    }
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        cJSON_Delete(params);
        return ERROR;
    }
    cJSON_AddNumberToObject(root, "id", response_id++);
    cJSON_AddStringToObject(root, "method", method);
    cJSON_AddItemToObject(root, "params", params);
    char *command = cJSON_PrintUnformatted(root);
    if (!command) {
        cJSON_Delete(root);
        return ERROR;
    }
    client->write(command, strlen(command));
    client->write("\r\n", 2);
    cJSON_Delete(root);
    free(command);
    return checkResponse(response_id - 1);
}

ResponseType Yeelight::enable_music_mode() {
    if (!supported_methods.set_music) {
        return METHOD_NOT_SUPPORTED;
    }
    if (!client || !is_connected()) {
        connect();
    }
    createMusicModeServer();
    IPAddress loc = WiFi.localIP();
    music_host[0] = loc[0];
    music_host[1] = loc[1];
    music_host[2] = loc[2];
    music_host[3] = loc[3];
    music_port = 55443;
    if (xSemaphoreTake(music_sem, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return CONNECTION_FAILED;
    }
    return set_music_command(true, music_host, music_port);
}

void Yeelight::onMusicConnect(AsyncClient *c) {
    music_mode = true;
    music_retry_count = 0;
    if (client) {
        client->close();
        scheduleDeleteClient(client);
        client = nullptr;
    }
    xSemaphoreGive(music_sem);
    Serial.println("Music mode connected successfully.");
}

void Yeelight::onMainClientConnect(AsyncClient *c) {
    Serial.println("Main socket connected");
}

void Yeelight::onMainClientError(AsyncClient *c, int8_t error) {
    Serial.printf("AsyncClient error %d\n", error);
    connecting = false;
}

ResponseType Yeelight::connect(const uint8_t *ip, const uint16_t port) {
    if (is_connected()) {
        client->close();
        scheduleDeleteClient(client);
        client = nullptr;
    }
    for (uint8_t i = 0; i < 4; i++) {
        this->ip[i] = ip[i];
    }
    const uint32_t ip32 = ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];
    if (xSemaphoreTake(Yeelight::devices_mutex, portMAX_DELAY) == pdTRUE) {
        devices[ip32] = this;
        xSemaphoreGive(Yeelight::devices_mutex);
    }
    this->port = port;
    uint8_t current_retries = 0;
    while (!refreshedMethods && current_retries < max_retry) {
        refreshSupportedMethods();
        current_retries++;
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    return connect();
}

ResponseType Yeelight::connect(const YeelightDevice &device) {
    for (int i = 0; i < 4; ++i) {
        this->ip[i] = device.ip[i];
    }
    const uint32_t ip32 = device.ip[0] << 24 | device.ip[1] << 16 | device.ip[2] << 8 | device.ip[3];
    if (xSemaphoreTake(Yeelight::devices_mutex, portMAX_DELAY) == pdTRUE) {
        devices[ip32] = this;
        xSemaphoreGive(Yeelight::devices_mutex);
    }
    this->port = device.port;
    supported_methods = device.supported_methods;
    return connect();
}

bool Yeelight::is_connected() const {
    return client && client->connected();
}

bool Yeelight::is_connected_music() const {
    return music_client && music_client->connected();
}

bool Yeelight::createMusicModeServer() {
    if (music_mode_server) {
        return false;
    }
    music_mode_server = new AsyncServer(55443);
    music_mode_server->onClient(handleNewClient, nullptr);
    music_mode_server->begin();
    return true;
}

void Yeelight::handleNewClient(void *arg, AsyncClient *client) {
    if (!client) return;
    IPAddress remoteIP = client->remoteIP();
    const uint32_t remoteIP32 = remoteIP[0] << 24 | remoteIP[1] << 16 | remoteIP[2] << 8 | remoteIP[3];
    Yeelight *y = nullptr;
    if (xSemaphoreTake(Yeelight::devices_mutex, portMAX_DELAY) == pdTRUE) {
        auto it = devices.find(remoteIP32);
        if (it != devices.end()) {
            y = it->second;
            Serial.printf("Client from IP %u.%u.%u.%u associated with Yeelight instance %p\n",
                          remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3], y);
        } else {
            Serial.printf("No Yeelight instance found for IP %u.%u.%u.%u\n",
                          remoteIP[0], remoteIP[1], remoteIP[2], remoteIP[3]);
        }
        xSemaphoreGive(Yeelight::devices_mutex);
    }
    if (!y) {
        Serial.println("Unassociated client. Closing connection.");
        client->close();
        return;
    }
    client->onConnect([](void *arg2, AsyncClient *c) {
        auto *that = static_cast<Yeelight *>(arg2);
        that->onMusicConnect(c);
    }, y);
    client->onDisconnect([](void *arg2, const AsyncClient *c) {
        auto *that = static_cast<Yeelight *>(arg2);
        that->onMusicDisconnect(c);
    }, y);
    client->onData([](void *arg2, AsyncClient *c, const void *data, size_t len) {
        auto *that = static_cast<Yeelight *>(arg2);
        that->onData(c, data, len);
    }, y);
    if (y->music_client && y->music_client->connected()) {
        Serial.println("Existing music_client detected. Closing it before assigning a new one.");
        y->music_client->close();
    }
    y->music_client = client;
    y->music_mode = true;
    y->client->close();
}

void Yeelight::deleteClientCallback(const TimerHandle_t xTimer) {
    const auto *c = static_cast<AsyncClient *>(pvTimerGetTimerID(xTimer));
    delete c;
    xTimerDelete(xTimer, 0);
}

void Yeelight::scheduleDeleteClient(AsyncClient *c) {
    if (!c) return;
    const TimerHandle_t delTimer = xTimerCreate(
        "delAsyncClient",
        pdMS_TO_TICKS(10),
        pdFALSE,
        c,
        deleteClientCallback
    );
    if (delTimer != nullptr) {
        xTimerStart(delTimer, 0);
    } else {
        delete c;
    }
}

void Yeelight::onData(AsyncClient *c, const void *data, const size_t len) {
    const auto chunk = static_cast<const char *>(data);
    partialResponse.append(chunk, len);
    size_t pos = partialResponse.find('\n');
    while (pos != std::string::npos) {
        std::string line = partialResponse.substr(0, pos);
        partialResponse.erase(0, pos + 1);
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }
        if (!line.empty()) {
            cJSON *root = cJSON_Parse(line.c_str());
            if (root) {
                if (cJSON_GetObjectItem(root, "id")) {
                    uint16_t id = cJSON_GetObjectItem(root, "id")->valueint;
                    if (cJSON_GetObjectItem(root, "result")) {
                        const cJSON *result_array = cJSON_GetObjectItem(root, "result");
                        if (result_array == nullptr) {
                            responses[id] = UNEXPECTED_RESPONSE;
                            cJSON_Delete(root);
                            continue;
                        }
                        if (!cJSON_IsArray(result_array)) {
                            responses[id] = UNEXPECTED_RESPONSE;
                            cJSON_Delete(root);
                            continue;
                        }
                        const cJSON *firstItem = cJSON_GetArrayItem(result_array, 0);
                        if (firstItem && cJSON_IsString(firstItem) && strcmp(firstItem->valuestring, "ok") == 0) {
                            responses[id] = SUCCESS;
                        } else {
                            if (cJSON_GetArraySize(result_array) < 21) {
                                cJSON_Delete(root);
                                responses[id] = UNEXPECTED_RESPONSE;
                                continue;
                            }
                            const cJSON *item = cJSON_GetArrayItem(result_array, 0);
                            if (cJSON_IsString(item)) {
                                properties.power = strcmp(item->valuestring, "on") == 0;
                            }
                            item = cJSON_GetArrayItem(result_array, 1);
                            if (cJSON_IsNumber(item)) {
                                properties.bright = static_cast<uint8_t>(item->valuedouble);
                            } else if (cJSON_IsString(item)) {
                                properties.bright = static_cast<uint8_t>(atoi(item->valuestring));
                            }
                            item = cJSON_GetArrayItem(result_array, 2);
                            if (cJSON_IsNumber(item)) {
                                properties.ct = static_cast<uint16_t>(item->valuedouble);
                            } else if (cJSON_IsString(item)) {
                                properties.ct = static_cast<uint16_t>(atoi(item->valuestring));
                            }
                            item = cJSON_GetArrayItem(result_array, 3);
                            if (cJSON_IsNumber(item)) {
                                properties.rgb = static_cast<uint32_t>(item->valuedouble);
                            } else if (cJSON_IsString(item)) {
                                properties.rgb = static_cast<uint32_t>(atoi(item->valuestring));
                            }
                            item = cJSON_GetArrayItem(result_array, 4);
                            if (cJSON_IsNumber(item)) {
                                properties.hue = static_cast<uint16_t>(item->valuedouble);
                            } else if (cJSON_IsString(item)) {
                                properties.hue = static_cast<uint16_t>(atoi(item->valuestring));
                            }
                            item = cJSON_GetArrayItem(result_array, 5);
                            if (cJSON_IsNumber(item)) {
                                properties.sat = static_cast<uint8_t>(item->valuedouble);
                            } else if (cJSON_IsString(item)) {
                                properties.sat = static_cast<uint8_t>(atoi(item->valuestring));
                            }
                            item = cJSON_GetArrayItem(result_array, 6);
                            if (cJSON_IsNumber(item)) {
                                const auto cm = static_cast<uint8_t>(item->valuedouble);
                                if (cm == 1) {
                                    properties.color_mode = COLOR_MODE_RGB;
                                } else if (cm == 2) {
                                    properties.color_mode = COLOR_MODE_COLOR_TEMPERATURE;
                                } else if (cm == 3) {
                                    properties.color_mode = COLOR_MODE_HSV;
                                } else {
                                    properties.color_mode = COLOR_MODE_UNKNOWN;
                                }
                            } else if (cJSON_IsString(item)) {
                                const auto cm = static_cast<uint8_t>(atoi(item->valuestring));
                                if (cm == 1) {
                                    properties.color_mode = COLOR_MODE_RGB;
                                } else if (cm == 2) {
                                    properties.color_mode = COLOR_MODE_COLOR_TEMPERATURE;
                                } else if (cm == 3) {
                                    properties.color_mode = COLOR_MODE_HSV;
                                } else {
                                    properties.color_mode = COLOR_MODE_UNKNOWN;
                                }
                            }
                            item = cJSON_GetArrayItem(result_array, 7);
                            if (cJSON_IsNumber(item)) {
                                properties.flowing = static_cast<int>(item->valuedouble) == 1;
                            } else if (cJSON_IsString(item)) {
                                properties.flowing = atoi(item->valuestring) == 1;
                            }
                            item = cJSON_GetArrayItem(result_array, 8);
                            if (cJSON_IsNumber(item)) {
                                properties.delayoff = static_cast<uint8_t>(item->valuedouble);
                            } else if (cJSON_IsString(item)) {
                                properties.delayoff = static_cast<uint8_t>(atoi(item->valuestring));
                            }
                            item = cJSON_GetArrayItem(result_array, 9);
                            if (cJSON_IsNumber(item)) {
                                properties.music_on = static_cast<int>(item->valuedouble) == 1;
                            } else if (cJSON_IsString(item)) {
                                properties.music_on = atoi(item->valuestring) == 1;
                            }
                            item = cJSON_GetArrayItem(result_array, 10);
                            if (cJSON_IsString(item)) {
                                properties.name = String(item->valuestring).c_str();
                            }
                            item = cJSON_GetArrayItem(result_array, 11);
                            if (cJSON_IsString(item)) {
                                properties.bg_power = strcmp(item->valuestring, "on") == 0;
                            }
                            item = cJSON_GetArrayItem(result_array, 12);
                            if (cJSON_IsNumber(item)) {
                                properties.bg_flowing = static_cast<int>(item->valuedouble) == 1;
                            } else if (cJSON_IsString(item)) {
                                properties.bg_flowing = atoi(item->valuestring) == 1;
                            }
                            item = cJSON_GetArrayItem(result_array, 13);
                            if (cJSON_IsNumber(item)) {
                                properties.bg_ct = static_cast<uint16_t>(item->valuedouble);
                            } else if (cJSON_IsString(item)) {
                                properties.bg_ct = static_cast<uint16_t>(atoi(item->valuestring));
                            }
                            item = cJSON_GetArrayItem(result_array, 14);
                            if (cJSON_IsNumber(item)) {
                                const auto bg_lmode_int = static_cast<uint8_t>(item->valuedouble);
                                if (bg_lmode_int == 1) {
                                    properties.bg_color_mode = COLOR_MODE_RGB;
                                } else if (bg_lmode_int == 2) {
                                    properties.bg_color_mode = COLOR_MODE_COLOR_TEMPERATURE;
                                } else if (bg_lmode_int == 3) {
                                    properties.bg_color_mode = COLOR_MODE_HSV;
                                } else {
                                    properties.bg_color_mode = COLOR_MODE_UNKNOWN;
                                }
                            } else if (cJSON_IsString(item)) {
                                const auto bg_lmode_int = static_cast<uint8_t>(atoi(item->valuestring));
                                if (bg_lmode_int == 1) {
                                    properties.bg_color_mode = COLOR_MODE_RGB;
                                } else if (bg_lmode_int == 2) {
                                    properties.bg_color_mode = COLOR_MODE_COLOR_TEMPERATURE;
                                } else if (bg_lmode_int == 3) {
                                    properties.bg_color_mode = COLOR_MODE_HSV;
                                } else {
                                    properties.bg_color_mode = COLOR_MODE_UNKNOWN;
                                }
                            }
                            item = cJSON_GetArrayItem(result_array, 15);
                            if (cJSON_IsNumber(item)) {
                                properties.bg_bright = static_cast<uint8_t>(item->valuedouble);
                            } else if (cJSON_IsString(item)) {
                                properties.bg_bright = static_cast<uint8_t>(atoi(item->valuestring));
                            }
                            item = cJSON_GetArrayItem(result_array, 16);
                            if (cJSON_IsNumber(item)) {
                                properties.bg_rgb = static_cast<uint32_t>(item->valuedouble);
                            } else if (cJSON_IsString(item)) {
                                properties.bg_rgb = static_cast<uint32_t>(atoi(item->valuestring));
                            }
                            item = cJSON_GetArrayItem(result_array, 17);
                            if (cJSON_IsNumber(item)) {
                                properties.bg_hue = static_cast<uint16_t>(item->valuedouble);
                            } else if (cJSON_IsString(item)) {
                                properties.bg_hue = static_cast<uint16_t>(atoi(item->valuestring));
                            }
                            item = cJSON_GetArrayItem(result_array, 18);
                            if (cJSON_IsNumber(item)) {
                                properties.bg_sat = static_cast<uint8_t>(item->valuedouble);
                            } else if (cJSON_IsString(item)) {
                                properties.bg_sat = static_cast<uint8_t>(atoi(item->valuestring));
                            }
                            item = cJSON_GetArrayItem(result_array, 19);
                            if (cJSON_IsNumber(item)) {
                                properties.nl_br = static_cast<uint8_t>(item->valuedouble);
                            } else if (cJSON_IsString(item)) {
                                properties.nl_br = static_cast<uint8_t>(atoi(item->valuestring));
                            }
                            item = cJSON_GetArrayItem(result_array, 20);
                            if (cJSON_IsNumber(item)) {
                                properties.active_mode = static_cast<int>(item->valuedouble) == 1;
                            } else if (cJSON_IsString(item)) {
                                properties.active_mode = atoi(item->valuestring) == 1;
                            }
                            responses[id] = SUCCESS;
                        }
                    } else if (cJSON_GetObjectItem(root, "error")) {
                        responses[id] = ERROR;
                    }
                } else if (cJSON_GetObjectItem(root, "method")) {
                    const char *method = cJSON_GetObjectItem(root, "method")->valuestring;
                    if (strcmp(method, "props") == 0) {
                        const cJSON *params = cJSON_GetObjectItem(root, "params");
                        if (!params || !cJSON_IsObject(params)) {
                            return;
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "power");
                            if (item && cJSON_IsString(item)) {
                                properties.power = strcmp(item->valuestring, "on") == 0;
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "bright");
                            if (item && cJSON_IsString(item)) {
                                properties.bright = static_cast<uint8_t>(atoi(item->valuestring));
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "ct");
                            if (item && cJSON_IsString(item)) {
                                properties.ct = static_cast<uint16_t>(atoi(item->valuestring));
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "rgb");
                            if (item && cJSON_IsString(item)) {
                                properties.rgb = static_cast<uint32_t>(atoi(item->valuestring));
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "hue");
                            if (item && cJSON_IsString(item)) {
                                properties.hue = static_cast<uint16_t>(atoi(item->valuestring));
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "sat");
                            if (item && cJSON_IsString(item)) {
                                properties.sat = static_cast<uint8_t>(atoi(item->valuestring));
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "color_mode");
                            if (item && cJSON_IsString(item)) {
                                const int cm = atoi(item->valuestring);
                                switch (cm) {
                                    case 1: properties.color_mode = COLOR_MODE_RGB;
                                        break;
                                    case 2: properties.color_mode = COLOR_MODE_COLOR_TEMPERATURE;
                                        break;
                                    case 3: properties.color_mode = COLOR_MODE_HSV;
                                        break;
                                    default: properties.color_mode = COLOR_MODE_UNKNOWN;
                                        break;
                                }
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "flowing");
                            if (item && cJSON_IsString(item)) {
                                properties.flowing = atoi(item->valuestring) == 1;
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "delayoff");
                            if (item && cJSON_IsString(item)) {
                                properties.delayoff = static_cast<uint8_t>(atoi(item->valuestring));
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "flow_params");
                            if (item && cJSON_IsString(item)) {
                                //properties.flow_params = item->valuestring;
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "music_on");
                            if (item && cJSON_IsString(item)) {
                                properties.music_on = atoi(item->valuestring) == 1;
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "name");
                            if (item && cJSON_IsString(item)) {
                                properties.name = item->valuestring;
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "bg_power");
                            if (item && cJSON_IsString(item)) {
                                properties.bg_power = strcmp(item->valuestring, "on") == 0;
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "bg_flowing");
                            if (item && cJSON_IsString(item)) {
                                properties.bg_flowing = atoi(item->valuestring) == 1;
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "bg_flow_params");
                            if (item && cJSON_IsString(item)) {
                                //properties.bg_flow_params = item->valuestring;
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "bg_ct");
                            if (item && cJSON_IsString(item)) {
                                properties.bg_ct = static_cast<uint16_t>(atoi(item->valuestring));
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "bg_lmode");
                            if (item && cJSON_IsString(item)) {
                                const int cm = atoi(item->valuestring);
                                switch (cm) {
                                    case 1: properties.bg_color_mode = COLOR_MODE_RGB;
                                        break;
                                    case 2: properties.bg_color_mode = COLOR_MODE_COLOR_TEMPERATURE;
                                        break;
                                    case 3: properties.bg_color_mode = COLOR_MODE_HSV;
                                        break;
                                    default: properties.bg_color_mode = COLOR_MODE_UNKNOWN;
                                        break;
                                }
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "bg_bright");
                            if (item && cJSON_IsString(item)) {
                                properties.bg_bright = static_cast<uint8_t>(atoi(item->valuestring));
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "bg_rgb");
                            if (item && cJSON_IsString(item)) {
                                properties.bg_rgb = static_cast<uint32_t>(atoi(item->valuestring));
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "bg_hue");
                            if (item && cJSON_IsString(item)) {
                                properties.bg_hue = static_cast<uint16_t>(atoi(item->valuestring));
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "bg_sat");
                            if (item && cJSON_IsString(item)) {
                                properties.bg_sat = static_cast<uint8_t>(atoi(item->valuestring));
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "nl_br");
                            if (item && cJSON_IsString(item)) {
                                properties.nl_br = static_cast<uint8_t>(atoi(item->valuestring));
                            }
                        } {
                            const cJSON *item = cJSON_GetObjectItem(params, "active_mode");
                            if (item && cJSON_IsString(item)) {
                                properties.active_mode = atoi(item->valuestring) == 1;
                            }
                        }
                    }
                }
                cJSON_Delete(root);
            }
        }
        pos = partialResponse.find('\n');
    }
}

void Yeelight::onMainClientDisconnect(const AsyncClient *c) {
    if (client == c) {
        client = nullptr;
        scheduleDeleteClient(const_cast<AsyncClient *>(c));
    }
}

void Yeelight::onMusicDisconnect(const AsyncClient *c) {
    if (music_client == c) {
        music_client = nullptr;
        scheduleDeleteClient(const_cast<AsyncClient *>(c));
    }
    music_mode = false;
    Serial.println("Music mode disconnected.");
}

SupportedMethods Yeelight::getSupportedMethods() const {
    return supported_methods;
}

void Yeelight::refreshSupportedMethods() {
    WiFiUDP udp;
    const IPAddress multicastIP(239, 255, 255, 250);
    constexpr unsigned int localUdpPort = 1982;
    const auto ssdpRequest =
            "M-SEARCH * HTTP/1.1\r\n"
            "HOST: 239.255.255.250:1982\r\n"
            "MAN: \"ssdp:discover\"\r\n"
            "ST: wifi_bulb\r\n\r\n";
    if (!udp.begin(localUdpPort)) {
    }
    udp.beginPacket(multicastIP, localUdpPort);
    udp.print(ssdpRequest);
    udp.endPacket();
    const unsigned long startTime = millis();
    while (millis() - startTime < timeout) {
        const int packetSize = udp.parsePacket();
        if (packetSize) {
            char packetBuffer[1024];
            const int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
            if (len > 0) {
                packetBuffer[len] = '\0';
                const YeelightDevice device = parseDiscoveryResponse(packetBuffer);
                if (memcmp(device.ip, ip, sizeof(ip)) == 0) {
                    Serial.println(device.supported_methods.set_music);
                    supported_methods = device.supported_methods;
                    refreshedMethods = true;
                    udp.stop();
                    break;
                }
            }
        }
    }
}

ResponseType Yeelight::set_power_command(const bool power, const effect effect, const uint16_t duration,
                                         const mode mode) {
    if (!supported_methods.set_power) {
        return METHOD_NOT_SUPPORTED;
    }
    if (duration < 30) {
        return INVALID_PARAMS;
    }
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString(power ? "on" : "off"));
    cJSON_AddItemToArray(params, cJSON_CreateString(effect == EFFECT_SMOOTH ? "smooth" : "sudden"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    if (mode != MODE_CURRENT) {
        cJSON_AddItemToArray(params, cJSON_CreateNumber(mode));
    }
    return send_command("set_power", params);
}

ResponseType Yeelight::toggle_command() {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    return send_command("toggle", params);
}

ResponseType Yeelight::set_ct_abx_command(const uint16_t ct_value, const effect effect, const uint16_t duration) {
    if (!supported_methods.set_ct_abx) {
        return METHOD_NOT_SUPPORTED;
    }
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(ct_value));
    cJSON_AddItemToArray(params, cJSON_CreateString(effect == EFFECT_SMOOTH ? "smooth" : "sudden"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("set_ct_abx", params);
}

ResponseType Yeelight::set_rgb_command(const uint8_t r, const uint8_t g, const uint8_t b, const effect effect,
                                       const uint16_t duration) {
    const uint32_t rgb = r << 16 | g << 8 | b;
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(rgb));
    cJSON_AddItemToArray(params, cJSON_CreateString(effect == EFFECT_SMOOTH ? "smooth" : "sudden"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("set_rgb", params);
}

ResponseType Yeelight::set_hsv_command(const uint16_t hue, const uint8_t sat, const effect effect,
                                       const uint16_t duration) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(hue));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(sat));
    cJSON_AddItemToArray(params, cJSON_CreateString(effect == EFFECT_SMOOTH ? "smooth" : "sudden"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("set_hsv", params);
}

ResponseType Yeelight::set_bright_command(const uint8_t bright, const effect effect, const uint16_t duration) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(bright));
    cJSON_AddItemToArray(params, cJSON_CreateString(effect == EFFECT_SMOOTH ? "smooth" : "sudden"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("set_bright", params);
}

ResponseType Yeelight::set_default() {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    return send_command("set_default", params);
}

ResponseType Yeelight::start_cf_command(const uint8_t count, const flow_action action, const uint8_t size,
                                        const flow_expression *flow) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(count));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(action));
    std::string flowExpression;
    for (int i = 0; i < size; i++) {
        flowExpression += std::to_string(flow[i].duration) + "," + std::to_string(flow[i].mode) + "," +
                std::to_string(flow[i].value) + "," + std::to_string(flow[i].brightness) + ",";
    }
    flowExpression.pop_back();
    cJSON_AddItemToArray(params, cJSON_CreateString(flowExpression.c_str()));
    return send_command("start_cf", params);
}

ResponseType Yeelight::stop_cf_command() {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    return send_command("stop_cf", params);
}

ResponseType Yeelight::set_scene_rgb_command(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t bright) {
    const uint32_t rgb = r << 16 | g << 8 | b;
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString("color"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(rgb));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(bright));
    return send_command("set_scene", params);
}

ResponseType Yeelight::set_scene_hsv_command(const uint8_t hue, const uint8_t sat, const uint8_t bright) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString("hsv"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(hue));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(sat));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(bright));
    return send_command("set_scene", params);
}

ResponseType Yeelight::set_scene_ct_command(const uint16_t ct, const uint8_t bright) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString("ct"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(ct));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(bright));
    return send_command("set_scene", params);
}

ResponseType Yeelight::set_scene_auto_delay_off_command(const uint8_t brightness, const uint32_t duration) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString("auto_delay_off"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(brightness));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("set_scene", params);
}

ResponseType Yeelight::set_scene_cf_command(const uint32_t count, const flow_action action, const uint32_t size,
                                            const flow_expression *flow) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString("cf"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(count));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(action));
    std::string flowExpression;
    for (int i = 0; i < size; i++) {
        flowExpression += std::to_string(flow[i].duration) + "," + std::to_string(flow[i].mode) + "," +
                std::to_string(flow[i].value) + "," + std::to_string(flow[i].brightness) + ",";
    }
    flowExpression.pop_back();
    cJSON_AddItemToArray(params, cJSON_CreateString(flowExpression.c_str()));
    return send_command("set_scene", params);
}

ResponseType Yeelight::cron_add_command(const uint32_t time) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(0));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(time));
    return send_command("cron_add", params);
}

ResponseType Yeelight::cron_del_command() {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(0));
    return send_command("cron_del", params);
}

void Yeelight::set_adjust(const adjust_action action, const adjust_prop prop) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return;
    }
    if (action == ADJUST_INCREASE) {
        cJSON_AddItemToArray(params, cJSON_CreateString("increase"));
    } else if (action == ADJUST_DECREASE) {
        cJSON_AddItemToArray(params, cJSON_CreateString("decrease"));
    } else {
        cJSON_AddItemToArray(params, cJSON_CreateString("circle"));
    }
    if (prop == ADJUST_BRIGHT) {
        cJSON_AddItemToArray(params, cJSON_CreateString("bright"));
    } else if (prop == ADJUST_CT) {
        cJSON_AddItemToArray(params, cJSON_CreateString("ct"));
    } else {
        cJSON_AddItemToArray(params, cJSON_CreateString("color"));
    }
    send_command("set_adjust", params);
}

ResponseType Yeelight::set_name_command(const char *name) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString(name));
    return send_command("set_name", params);
}

ResponseType Yeelight::bg_set_power_command(const bool power, const effect effect, const uint16_t duration,
                                            const mode mode) {
    if (!supported_methods.bg_set_power) {
        return METHOD_NOT_SUPPORTED;
    }
    if (duration < 30) {
        return INVALID_PARAMS;
    }
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString(power ? "on" : "off"));
    cJSON_AddItemToArray(params, cJSON_CreateString(effect == EFFECT_SMOOTH ? "smooth" : "sudden"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    if (mode != MODE_CURRENT) {
        cJSON_AddItemToArray(params, cJSON_CreateNumber(mode));
    }
    return send_command("bg_set_power", params);
}

ResponseType Yeelight::bg_toggle_command() {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    return send_command("bg_toggle", params);
}

ResponseType Yeelight::bg_set_ct_abx_command(const uint16_t ct_value, const effect effect, const uint16_t duration) {
    if (!supported_methods.bg_set_ct_abx) {
        return METHOD_NOT_SUPPORTED;
    }
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(ct_value));
    cJSON_AddItemToArray(params, cJSON_CreateString(effect == EFFECT_SMOOTH ? "smooth" : "sudden"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("bg_set_ct_abx", params);
}

ResponseType Yeelight::bg_set_rgb_command(const uint8_t r, const uint8_t g, const uint8_t b, const effect effect,
                                          const uint16_t duration) {
    const uint32_t rgb = r << 16 | g << 8 | b;
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(rgb));
    cJSON_AddItemToArray(params, cJSON_CreateString(effect == EFFECT_SMOOTH ? "smooth" : "sudden"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("bg_set_rgb", params);
}

ResponseType Yeelight::bg_set_hsv_command(const uint16_t hue, const uint8_t sat, const effect effect,
                                          const uint16_t duration) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(hue));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(sat));
    cJSON_AddItemToArray(params, cJSON_CreateString(effect == EFFECT_SMOOTH ? "smooth" : "sudden"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("bg_set_hsv", params);
}

ResponseType Yeelight::bg_set_bright_command(const uint8_t bright, const effect effect, const uint16_t duration) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(bright));
    cJSON_AddItemToArray(params, cJSON_CreateString(effect == EFFECT_SMOOTH ? "smooth" : "sudden"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("bg_set_bright", params);
}

ResponseType Yeelight::bg_set_default() {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    return send_command("bg_set_default", params);
}

ResponseType
Yeelight::bg_set_scene_rgb_command(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t bright) {
    const uint32_t rgb = r << 16 | g << 8 | b;
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString("color"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(rgb));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(bright));
    return send_command("bg_set_scene", params);
}

ResponseType Yeelight::bg_set_scene_hsv_command(const uint8_t hue, const uint8_t sat, const uint8_t bright) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString("hsv"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(hue));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(sat));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(bright));
    return send_command("bg_set_scene", params);
}

ResponseType Yeelight::bg_set_scene_ct_command(const uint16_t ct, const uint8_t bright) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString("ct"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(ct));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(bright));
    return send_command("bg_set_scene", params);
}

ResponseType Yeelight::bg_set_scene_auto_delay_off_command(const uint8_t brightness, const uint32_t duration) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString("auto_delay_off"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(brightness));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("bg_set_scene", params);
}

ResponseType Yeelight::bg_set_scene_cf_command(const uint32_t count, const flow_action action, const uint32_t size,
                                               const flow_expression *flow) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString("cf"));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(count));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(action));
    std::string flowExpression;
    for (int i = 0; i < size; i++) {
        flowExpression += std::to_string(flow[i].duration) + "," + std::to_string(flow[i].mode) + "," +
                std::to_string(flow[i].value) + "," + std::to_string(flow[i].brightness) + ",";
    }
    flowExpression.pop_back();
    cJSON_AddItemToArray(params, cJSON_CreateString(flowExpression.c_str()));
    return send_command("bg_set_scene", params);
}

void Yeelight::bg_set_adjust(const adjust_action action, const adjust_prop prop) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return;
    }
    if (action == ADJUST_INCREASE) {
        cJSON_AddItemToArray(params, cJSON_CreateString("increase"));
    } else if (action == ADJUST_DECREASE) {
        cJSON_AddItemToArray(params, cJSON_CreateString("decrease"));
    } else {
        cJSON_AddItemToArray(params, cJSON_CreateString("circle"));
    }
    if (prop == ADJUST_BRIGHT) {
        cJSON_AddItemToArray(params, cJSON_CreateString("bright"));
    } else if (prop == ADJUST_CT) {
        cJSON_AddItemToArray(params, cJSON_CreateString("ct"));
    } else {
        cJSON_AddItemToArray(params, cJSON_CreateString("color"));
    }
    send_command("bg_set_adjust", params);
}

ResponseType Yeelight::dev_toggle_command() {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    return send_command("dev_toggle", params);
}

ResponseType Yeelight::adjust_bright_command(const int8_t percentage, const uint16_t duration) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(percentage));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("adjust_bright", params);
}

ResponseType Yeelight::adjust_ct_command(const int8_t percentage, const uint16_t duration) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(percentage));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("adjust_ct", params);
}

ResponseType Yeelight::adjust_color_command(const int8_t percentage, const uint16_t duration) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(percentage));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("adjust_color", params);
}

ResponseType Yeelight::bg_adjust_bright_command(const int8_t percentage, const uint16_t duration) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(percentage));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("bg_adjust_bright", params);
}

ResponseType Yeelight::bg_adjust_ct_command(const int8_t percentage, const uint16_t duration) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(percentage));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("bg_adjust_ct", params);
}

ResponseType Yeelight::bg_adjust_color_command(const int8_t percentage, const uint16_t duration) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(percentage));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(duration));
    return send_command("bg_adjust_color", params);
}

std::vector<YeelightDevice> Yeelight::discoverYeelightDevices(int waitTimeMs) {
    WiFiUDP udp;
    IPAddress multicastIP(239, 255, 255, 250);
    unsigned int localUdpPort = 1982;
    auto ssdpRequest =
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
    while (millis() - startTime < static_cast<unsigned long>(waitTimeMs)) {
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
        device.power = strcmp(powerStr, "on") == 0;
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
        char supportStr[512];
        sscanf(support, "%511[^\r\n]", supportStr);
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

ResponseType Yeelight::set_music_command(const bool power, const uint8_t *host, const uint16_t port) {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateNumber(power));
    const std::string hostStr = std::to_string(host[0]) + "." + std::to_string(host[1]) + "." + std::to_string(host[2])
                                + "." + std::to_string(host[3]);
    cJSON_AddItemToArray(params, cJSON_CreateString(hostStr.c_str()));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(port));
    return send_command("set_music", params);
}

ResponseType Yeelight::start_flow(Flow flow, const LightType lightType) {
    if (!supported_methods.start_cf && !supported_methods.bg_start_cf) {
        return METHOD_NOT_SUPPORTED;
    }
    if (flow.get_size() == 0) {
        return INVALID_PARAMS;
    }
    if (flow.get_count() < 0) {
        return INVALID_PARAMS;
    }
    if (lightType == AUTO) {
        if (supported_methods.start_cf && supported_methods.bg_start_cf) {
            const ResponseType response = start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(),
                                                           flow.get_flow().data());
            if (response != SUCCESS) {
                return response;
            }
            return bg_start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
        }
        if (supported_methods.start_cf) {
            return start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
        }
        return bg_start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    }
    if (lightType == MAIN_LIGHT) {
        return start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    }
    if (lightType == BOTH) {
        const ResponseType response = start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(),
                                                       flow.get_flow().data());
        if (response != SUCCESS) {
            return response;
        }
        return bg_start_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    }
    return ERROR;
}

ResponseType Yeelight::stop_flow(const LightType lightType) {
    if (!supported_methods.stop_cf && !supported_methods.bg_stop_cf) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.stop_cf && supported_methods.bg_stop_cf) {
            const ResponseType response = stop_cf_command();
            if (response != SUCCESS) {
                return response;
            }
            return bg_stop_cf_command();
        }
        if (supported_methods.stop_cf) {
            return stop_cf_command();
        }
        return bg_stop_cf_command();
    }
    if (lightType == MAIN_LIGHT) {
        return stop_cf_command();
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_stop_cf_command();
    }
    if (lightType == BOTH) {
        const ResponseType response = stop_cf_command();
        if (response != SUCCESS) {
            return response;
        }
        return bg_stop_cf_command();
    }
    return ERROR;
}

ResponseType Yeelight::toggle_power(const LightType lightType) {
    if (lightType == AUTO) {
        if (supported_methods.toggle && supported_methods.bg_toggle) {
            return dev_toggle_command();
        }
        if (supported_methods.toggle) {
            return toggle_command();
        }
        if (supported_methods.bg_toggle) {
            return bg_toggle_command();
        }
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == MAIN_LIGHT) {
        if (supported_methods.toggle) {
            return toggle_command();
        }
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == BACKGROUND_LIGHT) {
        if (supported_methods.bg_toggle) {
            return bg_toggle_command();
        }
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == BOTH) {
        if (supported_methods.toggle && supported_methods.bg_toggle) {
            return dev_toggle_command();
        }
        return METHOD_NOT_SUPPORTED;
    }
    return ERROR;
}

ResponseType Yeelight::set_power(const bool power, const effect effect, const uint16_t duration, const mode mode,
                                 const LightType lightType) {
    if (!supported_methods.set_power && !supported_methods.bg_set_power) {
        return METHOD_NOT_SUPPORTED;
    }
    if (duration < 30) {
        return INVALID_PARAMS;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_power && supported_methods.bg_set_power) {
            const ResponseType response = set_power_command(power, effect, duration, mode);
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_power_command(power, effect, duration, mode);
        }
        if (supported_methods.set_power) {
            return set_power_command(power, effect, duration, mode);
        }
        return bg_set_power_command(power, effect, duration, mode);
    }
    if (lightType == MAIN_LIGHT) {
        return set_power_command(power, effect, duration, mode);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_power_command(power, effect, duration, mode);
    }
    if (lightType == BOTH) {
        const ResponseType response = set_power_command(power, effect, duration, mode);
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_power_command(power, effect, duration, mode);
    }
    return ERROR;
}

ResponseType Yeelight::set_power(const bool power, const LightType lightType) {
    return set_power(power, EFFECT_SMOOTH, 500, MODE_CURRENT, lightType);
}

ResponseType Yeelight::set_power(const bool power, const effect effect, const LightType lightType) {
    return set_power(power, effect, 500, MODE_CURRENT, lightType);
}

ResponseType Yeelight::set_power(const bool power, const effect effect, const uint16_t duration,
                                 const LightType lightType) {
    return set_power(power, effect, duration, MODE_CURRENT, lightType);
}

ResponseType Yeelight::set_power(const bool power, const mode mode, const LightType lightType) {
    return set_power(power, EFFECT_SMOOTH, 500, mode, lightType);
}

ResponseType Yeelight::set_power(const bool power, const effect effect, const mode mode, const LightType lightType) {
    return set_power(power, effect, 500, mode, lightType);
}

ResponseType Yeelight::turn_on(const LightType lightType) {
    return set_power(true, EFFECT_SMOOTH, 500, MODE_CURRENT, lightType);
}

ResponseType Yeelight::turn_on(const effect effect, const LightType lightType) {
    return set_power(true, effect, 500, MODE_CURRENT, lightType);
}

ResponseType Yeelight::turn_on(const effect effect, const uint16_t duration, const LightType lightType) {
    return set_power(true, effect, duration, MODE_CURRENT, lightType);
}

ResponseType Yeelight::turn_on(const mode mode, const LightType lightType) {
    return set_power(true, EFFECT_SMOOTH, 500, mode, lightType);
}

ResponseType Yeelight::turn_on(const effect effect, const mode mode, const LightType lightType) {
    return set_power(true, effect, 500, mode, lightType);
}

ResponseType Yeelight::turn_on(const effect effect, const uint16_t duration, const mode mode,
                               const LightType lightType) {
    return set_power(true, effect, duration, mode, lightType);
}

ResponseType Yeelight::turn_off(const LightType lightType) {
    return set_power(false, EFFECT_SMOOTH, 500, MODE_CURRENT, lightType);
}

ResponseType Yeelight::turn_off(const effect effect, const LightType lightType) {
    return set_power(false, effect, 500, MODE_CURRENT, lightType);
}

ResponseType Yeelight::turn_off(const effect effect, const uint16_t duration, const LightType lightType) {
    return set_power(false, effect, duration, MODE_CURRENT, lightType);
}

ResponseType Yeelight::turn_off(const mode mode, const LightType lightType) {
    return set_power(false, EFFECT_SMOOTH, 500, mode, lightType);
}

ResponseType Yeelight::turn_off(const effect effect, const mode mode, const LightType lightType) {
    return set_power(false, effect, 500, mode, lightType);
}

ResponseType Yeelight::turn_off(const effect effect, const uint16_t duration, const mode mode,
                                const LightType lightType) {
    return set_power(false, effect, duration, mode, lightType);
}

ResponseType Yeelight::set_color_temp(const uint16_t ct_value, const LightType lightType) {
    return set_color_temp(ct_value, EFFECT_SMOOTH, 500, lightType);
}

ResponseType Yeelight::set_color_temp(const uint16_t ct_value, const effect effect, const uint16_t duration,
                                      const LightType lightType) {
    if (ct_value < 1700 || ct_value > 6500) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.set_ct_abx && !supported_methods.bg_set_ct_abx) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_ct_abx && supported_methods.bg_set_ct_abx) {
            const ResponseType response = set_ct_abx_command(ct_value, effect, duration);
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_ct_abx_command(ct_value, effect, duration);
        }
        if (supported_methods.set_ct_abx) {
            return set_ct_abx_command(ct_value, effect, duration);
        }
        return bg_set_ct_abx_command(ct_value, effect, duration);
    }
    if (lightType == MAIN_LIGHT) {
        return set_ct_abx_command(ct_value, effect, duration);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_ct_abx_command(ct_value, effect, duration);
    }
    if (lightType == BOTH) {
        const ResponseType response = set_ct_abx_command(ct_value, effect, duration);
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_ct_abx_command(ct_value, effect, duration);
    }
    return ERROR;
}

ResponseType Yeelight::set_color_temp(const uint16_t ct_value, const effect effect, const LightType lightType) {
    return set_color_temp(ct_value, effect, 500, lightType);
}

ResponseType Yeelight::set_color_temp(const uint16_t ct_value, const uint8_t bright, const LightType lightType) {
    if (ct_value < 1700 || ct_value > 6500) {
        return INVALID_PARAMS;
    }
    if (bright < 1 || bright > 100) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            const ResponseType response = set_scene_ct_command(ct_value, bright);
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_scene_ct_command(ct_value, bright);
        }
        if (supported_methods.set_scene) {
            return set_scene_ct_command(ct_value, bright);
        }
        return bg_set_scene_ct_command(ct_value, bright);
    }
    if (lightType == MAIN_LIGHT) {
        return set_scene_ct_command(ct_value, bright);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_ct_command(ct_value, bright);
    }
    if (lightType == BOTH) {
        const ResponseType response = set_scene_ct_command(ct_value, bright);
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_scene_ct_command(ct_value, bright);
    }
    return ERROR;
}

ResponseType Yeelight::set_rgb_color(const uint8_t r, const uint8_t g, const uint8_t b, const LightType lightType) {
    return set_rgb_color(r, g, b, EFFECT_SMOOTH, 500, lightType);
}

ResponseType Yeelight::set_rgb_color(const uint8_t r, const uint8_t g, const uint8_t b, const effect effect,
                                     const LightType lightType) {
    return set_rgb_color(r, g, b, effect, 500, lightType);
}

ResponseType Yeelight::set_rgb_color(const uint8_t r, const uint8_t g, const uint8_t b, const effect effect,
                                     const uint16_t duration, const LightType lightType) {
    if (duration < 30) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.set_rgb && !supported_methods.bg_set_rgb) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_rgb && supported_methods.bg_set_rgb) {
            const ResponseType response = set_rgb_command(r, g, b, effect, duration);
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_rgb_command(r, g, b, effect, duration);
        }
        if (supported_methods.set_rgb) {
            return set_rgb_command(r, g, b, effect, duration);
        }
        return bg_set_rgb_command(r, g, b, effect, duration);
    }
    if (lightType == MAIN_LIGHT) {
        return set_rgb_command(r, g, b, effect, duration);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_rgb_command(r, g, b, effect, duration);
    }
    if (lightType == BOTH) {
        const ResponseType response = set_rgb_command(r, g, b, effect, duration);
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_rgb_command(r, g, b, effect, duration);
    }
    return ERROR;
}

ResponseType Yeelight::set_rgb_color(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t bright,
                                     const LightType lightType) {
    if (bright < 1 || bright > 100) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            const ResponseType response = set_scene_rgb_command(r, g, b, bright);
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_scene_rgb_command(r, g, b, bright);
        }
        if (supported_methods.set_scene) {
            return set_scene_rgb_command(r, g, b, bright);
        }
        return bg_set_scene_rgb_command(r, g, b, bright);
    }
    if (lightType == MAIN_LIGHT) {
        return set_scene_rgb_command(r, g, b, bright);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_rgb_command(r, g, b, bright);
    }
    if (lightType == BOTH) {
        const ResponseType response = set_scene_rgb_command(r, g, b, bright);
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_scene_rgb_command(r, g, b, bright);
    }
    return ERROR;
}

ResponseType Yeelight::set_brightness(const uint8_t bright, const LightType lightType) {
    return set_brightness(bright, EFFECT_SMOOTH, lightType);
}

ResponseType Yeelight::set_brightness(const uint8_t bright, const effect effect, const LightType lightType) {
    return set_brightness(bright, effect, 500, lightType);
}

ResponseType Yeelight::set_brightness(const uint8_t bright, const effect effect, const uint16_t duration,
                                      const LightType lightType) {
    if (bright < 1 || bright > 100) {
        return INVALID_PARAMS;
    }
    if (duration < 30) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.set_bright && !supported_methods.bg_set_bright) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_bright && supported_methods.bg_set_bright) {
            const ResponseType response = set_bright_command(bright, effect, duration);
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_bright_command(bright, effect, duration);
        }
        if (supported_methods.set_bright) {
            return set_bright_command(bright, effect, duration);
        }
        return bg_set_bright_command(bright, effect, duration);
    }
    if (lightType == MAIN_LIGHT) {
        return set_bright_command(bright, effect, duration);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_bright_command(bright, effect, duration);
    }
    if (lightType == BOTH) {
        const ResponseType response = set_bright_command(bright, effect, duration);
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_bright_command(bright, effect, duration);
    }
    return ERROR;
}

ResponseType Yeelight::set_hsv_color(const uint16_t hue, const uint8_t sat, const LightType lightType) {
    return set_hsv_color(hue, sat, EFFECT_SMOOTH, 500, lightType);
}

ResponseType Yeelight::set_hsv_color(const uint16_t hue, const uint8_t sat, const effect effect,
                                     const LightType lightType) {
    return set_hsv_color(hue, sat, effect, 500, lightType);
}

ResponseType Yeelight::set_hsv_color(const uint16_t hue, const uint8_t sat, const effect effect,
                                     const uint16_t duration, const LightType lightType) {
    if (hue > 359) {
        return INVALID_PARAMS;
    }
    if (sat > 100) {
        return INVALID_PARAMS;
    }
    if (duration < 30) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.set_hsv && !supported_methods.bg_set_hsv) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_hsv && supported_methods.bg_set_hsv) {
            const ResponseType response = set_hsv_command(hue, sat, effect, duration);
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_hsv_command(hue, sat, effect, duration);
        }
        if (supported_methods.set_hsv) {
            return set_hsv_command(hue, sat, effect, duration);
        }
        return bg_set_hsv_command(hue, sat, effect, duration);
    }
    if (lightType == MAIN_LIGHT) {
        return set_hsv_command(hue, sat, effect, duration);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_hsv_command(hue, sat, effect, duration);
    }
    if (lightType == BOTH) {
        const ResponseType response = set_hsv_command(hue, sat, effect, duration);
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_hsv_command(hue, sat, effect, duration);
    }
    return ERROR;
}

ResponseType Yeelight::set_hsv_color(const uint16_t hue, const uint8_t sat, const uint8_t bright,
                                     const LightType lightType) {
    if (bright < 1 || bright > 100) {
        return INVALID_PARAMS;
    }
    if (hue > 359) {
        return INVALID_PARAMS;
    }
    if (sat > 100) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            const ResponseType response = set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
        }
        if (supported_methods.set_scene) {
            return set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
        }
        return bg_set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
    }
    if (lightType == MAIN_LIGHT) {
        return set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
    }
    if (lightType == BOTH) {
        const ResponseType response = set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
    }
    return ERROR;
}

ResponseType Yeelight::set_scene_rgb(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t bright,
                                     const LightType lightType) {
    if (bright < 1 || bright > 100) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            const ResponseType response = set_scene_rgb_command(r, g, b, bright);
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_scene_rgb_command(r, g, b, bright);
        }
        if (supported_methods.set_scene) {
            return set_scene_rgb_command(r, g, b, bright);
        }
        return bg_set_scene_rgb_command(r, g, b, bright);
    }
    if (lightType == MAIN_LIGHT) {
        return set_scene_rgb_command(r, g, b, bright);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_rgb_command(r, g, b, bright);
    }
    if (lightType == BOTH) {
        const ResponseType response = set_scene_rgb_command(r, g, b, bright);
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_scene_rgb_command(r, g, b, bright);
    }
    return ERROR;
}

ResponseType Yeelight::set_scene_hsv(const uint16_t hue, const uint8_t sat, const uint8_t bright,
                                     const LightType lightType) {
    if (bright < 1 || bright > 100) {
        return INVALID_PARAMS;
    }
    if (hue > 359) {
        return INVALID_PARAMS;
    }
    if (sat > 100) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            const ResponseType response = set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
        }
        if (supported_methods.set_scene) {
            return set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
        }
        return bg_set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
    }
    if (lightType == MAIN_LIGHT) {
        return set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
    }
    if (lightType == BOTH) {
        const ResponseType response = set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_scene_hsv_command(static_cast<uint8_t>(hue), sat, bright);
    }
    return ERROR;
}

ResponseType Yeelight::set_scene_color_temperature(const uint16_t ct, const uint8_t bright, const LightType lightType) {
    if (bright < 1 || bright > 100) {
        return INVALID_PARAMS;
    }
    if (ct < 1700 || ct > 6500) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            const ResponseType response = set_scene_ct_command(ct, bright);
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_scene_ct_command(ct, bright);
        }
        if (supported_methods.set_scene) {
            return set_scene_ct_command(ct, bright);
        }
        return bg_set_scene_ct_command(ct, bright);
    }
    if (lightType == MAIN_LIGHT) {
        return set_scene_ct_command(ct, bright);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_ct_command(ct, bright);
    }
    if (lightType == BOTH) {
        const ResponseType response = set_scene_ct_command(ct, bright);
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_scene_ct_command(ct, bright);
    }
    return ERROR;
}

ResponseType Yeelight::set_scene_auto_delay_off(const uint8_t brightness, const uint32_t duration,
                                                const LightType lightType) {
    if (brightness < 1 || brightness > 100) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            const ResponseType response = set_scene_auto_delay_off_command(brightness, duration);
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_scene_auto_delay_off_command(brightness, duration);
        }
        if (supported_methods.set_scene) {
            return set_scene_auto_delay_off_command(brightness, duration);
        }
        return bg_set_scene_auto_delay_off_command(brightness, duration);
    }
    if (lightType == MAIN_LIGHT) {
        return set_scene_auto_delay_off_command(brightness, duration);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_auto_delay_off_command(brightness, duration);
    }
    if (lightType == BOTH) {
        const ResponseType response = set_scene_auto_delay_off_command(brightness, duration);
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_scene_auto_delay_off_command(brightness, duration);
    }
    return ERROR;
}

ResponseType Yeelight::set_turn_off_delay(const uint32_t duration) {
    if (!supported_methods.cron_add) {
        return METHOD_NOT_SUPPORTED;
    }
    return cron_add_command(duration);
}

ResponseType Yeelight::remove_turn_off_delay() {
    if (!supported_methods.cron_del) {
        return METHOD_NOT_SUPPORTED;
    }
    return cron_del_command();
}

ResponseType Yeelight::set_default_state(const LightType lightType) {
    if (!supported_methods.set_default && !supported_methods.bg_set_default) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_default && supported_methods.bg_set_default) {
            const ResponseType response = set_default();
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_default();
        }
        if (supported_methods.set_default) {
            return set_default();
        }
        return bg_set_default();
    }
    if (lightType == MAIN_LIGHT) {
        return set_default();
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_default();
    }
    if (lightType == BOTH) {
        const ResponseType response = set_default();
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_default();
    }
    return ERROR;
}

ResponseType Yeelight::set_device_name(const char *name) {
    if (!supported_methods.set_name) {
        return METHOD_NOT_SUPPORTED;
    }
    return set_name_command(name);
}

ResponseType Yeelight::set_device_name(const std::string &name) {
    return set_device_name(name.c_str());
}

ResponseType Yeelight::set_music_mode(const bool enabled) {
    if (!supported_methods.set_music) {
        return METHOD_NOT_SUPPORTED;
    }
    if (enabled) {
        if (!client || !is_connected()) {
            connect();
        }
        createMusicModeServer();
        IPAddress loc = WiFi.localIP();
        music_host[0] = loc[0];
        music_host[1] = loc[1];
        music_host[2] = loc[2];
        music_host[3] = loc[3];
        music_port = 55443;
        music_retry_count = 0;
        const auto resp = set_music_command(true, music_host, music_port);
        if (resp != SUCCESS) {
            return resp;
        }
        return SUCCESS;
    }
    const auto resp = set_music_command(false);
    if (resp != SUCCESS) {
        return resp;
    }
    if (music_client) {
        music_client->close();
        delete music_client;
        music_client = nullptr;
    }
    connect();
    return SUCCESS;
}

ResponseType Yeelight::disable_music_mode() {
    return set_music_mode(false);
}

ResponseType Yeelight::adjust_brightness(const int8_t percentage, const LightType lightType) {
    return adjust_brightness(percentage, 500, lightType);
}

ResponseType Yeelight::adjust_brightness(const int8_t percentage, const uint16_t duration, const LightType lightType) {
    if (percentage < -100 || percentage > 100) {
        return INVALID_PARAMS;
    }
    if (duration < 30) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.set_adjust && !supported_methods.bg_set_adjust) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.adjust_bright && supported_methods.bg_adjust_bright) {
            const ResponseType response = adjust_bright_command(percentage, duration);
            if (response != SUCCESS) {
                return response;
            }
            return bg_adjust_bright_command(percentage, duration);
        }
        if (supported_methods.adjust_bright) {
            return adjust_bright_command(percentage, duration);
        }
        return bg_adjust_bright_command(percentage, duration);
    }
    if (lightType == MAIN_LIGHT) {
        return adjust_bright_command(percentage, duration);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_adjust_bright_command(percentage, duration);
    }
    if (lightType == BOTH) {
        const ResponseType response = adjust_bright_command(percentage, duration);
        if (response != SUCCESS) {
            return response;
        }
        return bg_adjust_bright_command(percentage, duration);
    }
    return ERROR;
}

ResponseType Yeelight::adjust_color_temp(const int8_t percentage, const LightType lightType) {
    return adjust_color_temp(percentage, 500, lightType);
}

ResponseType Yeelight::adjust_color_temp(const int8_t percentage, const uint16_t duration, const LightType lightType) {
    if (percentage < -100 || percentage > 100) {
        return INVALID_PARAMS;
    }
    if (duration < 30) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.adjust_ct && !supported_methods.bg_adjust_ct) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.adjust_ct && supported_methods.bg_adjust_ct) {
            const ResponseType response = adjust_ct_command(percentage, duration);
            if (response != SUCCESS) {
                return response;
            }
            return bg_adjust_ct_command(percentage, duration);
        }
        if (supported_methods.adjust_ct) {
            return adjust_ct_command(percentage, duration);
        }
        return bg_adjust_ct_command(percentage, duration);
    }
    if (lightType == MAIN_LIGHT) {
        return adjust_ct_command(percentage, duration);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_adjust_ct_command(percentage, duration);
    }
    if (lightType == BOTH) {
        const ResponseType response = adjust_ct_command(percentage, duration);
        if (response != SUCCESS) {
            return response;
        }
        return bg_adjust_ct_command(percentage, duration);
    }
    return ERROR;
}

ResponseType Yeelight::adjust_color(const int8_t percentage, const LightType lightType) {
    return adjust_color(percentage, 500, lightType);
}

ResponseType Yeelight::adjust_color(const int8_t percentage, const uint16_t duration, const LightType lightType) {
    if (percentage < -100 || percentage > 100) {
        return INVALID_PARAMS;
    }
    if (duration < 30) {
        return INVALID_PARAMS;
    }
    if (!supported_methods.adjust_color && !supported_methods.bg_adjust_color) {
        return METHOD_NOT_SUPPORTED;
    }
    if (lightType == AUTO) {
        if (supported_methods.adjust_color && supported_methods.bg_adjust_color) {
            const ResponseType response = adjust_color_command(percentage, duration);
            if (response != SUCCESS) {
                return response;
            }
            return bg_adjust_color_command(percentage, duration);
        }
        if (supported_methods.adjust_color) {
            return adjust_color_command(percentage, duration);
        }
        return bg_adjust_color_command(percentage, duration);
    }
    if (lightType == MAIN_LIGHT) {
        return adjust_color_command(percentage, duration);
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_adjust_color_command(percentage, duration);
    }
    if (lightType == BOTH) {
        const ResponseType response = adjust_color_command(percentage, duration);
        if (response != SUCCESS) {
            return response;
        }
        return bg_adjust_color_command(percentage, duration);
    }
    return ERROR;
}

ResponseType Yeelight::bg_start_cf_command(const uint8_t count, const flow_action action, const uint8_t size,
                                           const flow_expression *flow) {
    cJSON *params = cJSON_CreateArray();
    cJSON_AddItemToArray(params, cJSON_CreateNumber(count));
    cJSON_AddItemToArray(params, cJSON_CreateNumber(action));
    std::string flow_str;
    for (uint8_t i = 0; i < size; i++) {
        flow_str += std::to_string(flow[i].duration) + "," + std::to_string(flow[i].mode) + "," +
                std::to_string(flow[i].value) + "," + std::to_string(flow[i].brightness) + ",";
    }
    flow_str.pop_back();
    cJSON_AddItemToArray(params, cJSON_CreateString(flow_str.c_str()));
    return send_command("bg_start_cf", params);
}

ResponseType Yeelight::bg_stop_cf_command() {
    cJSON *params = cJSON_CreateArray();
    if (params == nullptr) {
        return ERROR;
    }
    return send_command("bg_stop_cf", params);
}

ResponseType Yeelight::set_scene_flow(Flow flow, const LightType lightType) {
    if (!supported_methods.set_scene && !supported_methods.bg_set_scene) {
        return METHOD_NOT_SUPPORTED;
    }
    if (flow.get_size() == 0) {
        return INVALID_PARAMS;
    }
    if (flow.get_count() < 0) {
        return INVALID_PARAMS;
    }
    if (lightType == AUTO) {
        if (supported_methods.set_scene && supported_methods.bg_set_scene) {
            const ResponseType response = set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(),
                                                               flow.get_flow().data());
            if (response != SUCCESS) {
                return response;
            }
            return bg_set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
        }
        if (supported_methods.set_scene) {
            return set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
        }
        return bg_set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    }
    if (lightType == MAIN_LIGHT) {
        return set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    }
    if (lightType == BACKGROUND_LIGHT) {
        return bg_set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    }
    if (lightType == BOTH) {
        const ResponseType response = set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(),
                                                           flow.get_flow().data());
        if (response != SUCCESS) {
            return response;
        }
        return bg_set_scene_cf_command(flow.get_count(), flow.getAction(), flow.get_size(), flow.get_flow().data());
    }
    return ERROR;
}

ResponseType Yeelight::refreshProperties() {
    if (!supported_methods.get_prop) {
        return METHOD_NOT_SUPPORTED;
    }
    cJSON *params = cJSON_CreateArray();
    if (!params) {
        cJSON_Delete(params);
        return ERROR;
    }
    cJSON_AddItemToArray(params, cJSON_CreateString("power"));
    cJSON_AddItemToArray(params, cJSON_CreateString("bright"));
    cJSON_AddItemToArray(params, cJSON_CreateString("ct"));
    cJSON_AddItemToArray(params, cJSON_CreateString("rgb"));
    cJSON_AddItemToArray(params, cJSON_CreateString("hue"));
    cJSON_AddItemToArray(params, cJSON_CreateString("sat"));
    cJSON_AddItemToArray(params, cJSON_CreateString("color_mode"));
    cJSON_AddItemToArray(params, cJSON_CreateString("flowing"));
    cJSON_AddItemToArray(params, cJSON_CreateString("delayoff"));
    cJSON_AddItemToArray(params, cJSON_CreateString("music_on"));
    cJSON_AddItemToArray(params, cJSON_CreateString("name"));
    cJSON_AddItemToArray(params, cJSON_CreateString("bg_power"));
    cJSON_AddItemToArray(params, cJSON_CreateString("bg_flowing"));
    cJSON_AddItemToArray(params, cJSON_CreateString("bg_ct"));
    cJSON_AddItemToArray(params, cJSON_CreateString("bg_lmode"));
    cJSON_AddItemToArray(params, cJSON_CreateString("bg_bright"));
    cJSON_AddItemToArray(params, cJSON_CreateString("bg_rgb"));
    cJSON_AddItemToArray(params, cJSON_CreateString("bg_hue"));
    cJSON_AddItemToArray(params, cJSON_CreateString("bg_sat"));
    cJSON_AddItemToArray(params, cJSON_CreateString("nl_br"));
    cJSON_AddItemToArray(params, cJSON_CreateString("active_mode"));
    return send_command("get_prop", params);
}

YeelightProperties Yeelight::getProperties() {
    return properties;
}

void Yeelight::set_timeout(const uint16_t timeout) {
    this->timeout = timeout;
}

std::uint16_t Yeelight::get_timeout() const {
    return timeout;
}
