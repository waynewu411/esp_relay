#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <cJSON.h>

/* MQTT */
static WiFiClient wifiClient;
static PubSubClient mqttc(wifiClient);

static char *mqtt_get_server(void) {
    return "mqtt.server.com";
}

static int mqtt_get_port(void) {
    return 6883;
}

static char *mqtt_get_username(void) {
    return "mqtt_user";
}

static char *mqtt_get_password(void) {
    return "mqtt_password";
}

void setup_mqtt(void) {
    mqttc.setServer(mqtt_get_server(), mqtt_get_port());
    mqttc.setCallback(callback);
}

static void mqtt_print_message(char *topic, byte *content, unsigned int content_len) {
    Serial.printf("%s ", topic);
    for (int i = 0; i < content_len; i++) {
        Serial.print((char)content[i]);
    }
    Serial.println();
}

static void mqtt_publish(char *topic, char *content) {
    mqttc.publish(topic, (byte *)content, strlen(content));
    Serial.printf("<- %s %s\n", topic, content);
}

static void mqtt_act_toggle(char *message) {
    char rid_str[32] = {0};
    if(NULL == json_get_value(message, "rid", rid_str, sizeof(rid_str))) {
        Serial.println("Fail to get rid");
        return;
    }

    unsigned int rid = atoi(rid_str);
    if(!is_rid_valid(rid)) {
        Serial.printf("Invalid rid %d\n", rid);
        return;
    }

    relay_toggle(rid);
}

/* /MBS/RELAY/PUBLIC/STATUS/{IMEI} */
#define MQTT_TOPIC_FMT_STATUS "/MBS/RELAY/PUBLIC/STATUS/%s"

#define MQTT_CONTENT_FMT_STATUS_ECHO "{\"status\":\"ok\"}"
static void mqtt_act_status_echo(char *message) {
    char topic[64] = {0};
    snprintf(topic, sizeof(topic), MQTT_TOPIC_FMT_STATUS, system_get_deviceid());

    char content[32] = {0};
    snprintf(content, sizeof(content), "%s", MQTT_CONTENT_FMT_STATUS_ECHO);

    mqtt_publish(topic, content);
}

#define MQTT_CONTENT_FMT_STATUS "{\"rid\":\"%d\",\"toggle\":\"%d\",\"pulse_length\":\"%d\",\"pulse_interval\":\"%d\",\"cents_per_pulse\":\"%d\"}"
#define MQTT_CONTENT_FMT_STATUS_GLB "{\"rid\":\"0\",\"pulse_length\":\"%d\",\"pulse_interval\":\"%d\",\"cents_per_pulse\":\"%d\"}"
static char *mqtt_act_status_get_content(unsigned int rid, char *content, unsigned int content_len) {
    if(is_rid_valid(rid)) {
        snprintf(content, content_len, MQTT_CONTENT_FMT_STATUS,
            rid, relay_get_toggle_status(rid), relay_get_pulse_length(rid),
            relay_get_pulse_interval(rid), relay_get_cents_per_pulse(rid));
        return content;
    }

    if(is_rid_glb(rid)) {
        snprintf(content, content_len, MQTT_CONTENT_FMT_STATUS_GLB, relay_get_pulse_length_glb(),
            relay_get_pulse_interval_glb(), relay_get_cents_per_pulse_glb());
        return content;
    }
    
    return NULL;
}

static void mqtt_act_status_relay(char *message) {
    char topic[64] = {0};
    snprintf(topic, sizeof(topic), MQTT_TOPIC_FMT_STATUS, system_get_deviceid());
    
    char rid_str[16] = {0};
    if(NULL == json_get_value(message, "rid", rid_str, sizeof(rid_str))) {
        Serial.printf("Invalid rid\n");
        return;
    }
    
    unsigned int rid = atoi(rid_str);
    char content[256] = {0};
    if(NULL != mqtt_act_status_get_content(rid, content, sizeof(content))) {
        mqtt_publish(topic, content);
    }
}

#define MQTT_CONTENT_FMT_STATUS_WIFI "{\"wifi_ssid\":\"%s\",\"wifi_password\":\"%s\"}"
static void mqtt_act_status_wifi(char *message) {
    char topic[64] = {0};
    snprintf(topic, sizeof(topic), MQTT_TOPIC_FMT_STATUS, system_get_deviceid());

    char content[128] = {0};
    snprintf(content, sizeof(content), MQTT_CONTENT_FMT_STATUS_WIFI,
        wifi_get_ssid(), wifi_get_password());

    mqtt_publish(topic, content);
}

static void mqtt_act_status(char *message) {
    char module[16] = {0};
    if(NULL == json_get_value(message, "module", module, sizeof(module)) ||
        strlen(module) <= 0) {
        mqtt_act_status_echo(message);
        return;
    }

    if(0 == strcmp(module, "relay")) {
        mqtt_act_status_relay(message);
        return;
    }

    if(0 == strcmp(module, "wifi")) {
        mqtt_act_status_wifi(message);
        return;
    }
}

static void mqtt_act_upgrade(char *message) {
    char url[128] = {0};
    if(NULL == json_get_value(message, "firmware_url", url, sizeof(url)) ||
        strlen(url) <= 0) {
        Serial.println("Fail to get firmware url");
        return;
    }

    upgrade(url);
}

static void mqtt_act_reboot(char *message) {
    system_reboot();
}

static boolean mqtt_act_config_wifi_ssid(char *message) {
    char config[256] = {0};
    if(NULL == json_get_wifi_ssid(message, config, sizeof(config)) ||
        strlen(config) <= 0) {
        return false;
    }

    return config_set_wifi_ssid(config);
}

static boolean mqtt_act_config_wifi_password(char *message) {
    char config[256] = {0};
    if(NULL == json_get_wifi_password(message, config, sizeof(config)) ||
        strlen(config) <= 0) {
        return false;
    }

    return config_set_wifi_password(config);
}

static boolean mqtt_act_config_relay(char *message) {
    cJSON *json = cJSON_Parse(message);
    if(NULL == json) {
        const char *err_ptr = cJSON_GetErrorPtr();
        if(NULL != err_ptr) {
            Serial.printf("cJSON parse failed, result = %s\n", err_ptr);
        } else {
            Serial.printf("cJSON parse failed\n");
        }
        return false;
    }

    const cJSON *relays = NULL, *relay = NULL;
    relays = cJSON_GetObjectItemCaseSensitive(json, "relays");
    if(NULL == relays) {
        Serial.printf("cJSON GetObjectItem failed, key = %s\n", "relays");
        cJSON_Delete(json);
        return false;
    }
    
    cJSON_ArrayForEach(relay, relays) {
        cJSON *json_rid = cJSON_GetObjectItemCaseSensitive(relay, "rid");
        if(NULL == json_rid || !cJSON_IsString(json_rid) || strlen(json_rid->valuestring) <= 0) {
            Serial.printf("Invalid rid\n");
            cJSON_Delete(json);
            return false;
        }
        unsigned int rid = atoi(json_rid->valuestring);
        if(!is_rid_glb(rid) && !is_rid_valid(rid)) {
            Serial.printf("Invalid rid %d", rid);
            continue;
        }
        Serial.printf("rid = %d\n", rid);
        
        cJSON *json_pulse_length = cJSON_GetObjectItemCaseSensitive(relay, KEY_PULSE_LENGTH);
        if(NULL != json_pulse_length && cJSON_IsString(json_pulse_length) && strlen(json_pulse_length->valuestring) > 0) {
            unsigned int pulse_length = atoi(json_pulse_length->valuestring);
            Serial.printf("pulse_length = %d\n", pulse_length);
            if(is_rid_glb(rid)) {
                config_set_pulse_length_glb(pulse_length);
            } else {
                relay_set_pulse_length(rid, pulse_length);
            }
        }
        
        cJSON *json_pulse_interval = cJSON_GetObjectItemCaseSensitive(relay, KEY_PULSE_INTERVAL);
        if(NULL != json_pulse_interval && cJSON_IsString(json_pulse_interval) && strlen(json_pulse_interval->valuestring) > 0) {
            unsigned int pulse_interval = atoi(json_pulse_interval->valuestring);
            Serial.printf("pulse_interval = %d\n", pulse_interval);
            if(is_rid_glb(rid)) {
                config_set_pulse_interval_glb(pulse_interval);
            } else {
                relay_set_pulse_interval(rid, pulse_interval);
            }
        }
        
        cJSON *json_cents_per_pulse = cJSON_GetObjectItemCaseSensitive(relay, KEY_CENTS_PER_PULSE);
        if(NULL != json_cents_per_pulse && cJSON_IsString(json_cents_per_pulse) && strlen(json_cents_per_pulse->valuestring) > 0) {
            unsigned int cents_per_pulse = atoi(json_cents_per_pulse->valuestring);
            Serial.printf("cents_per_pulse = %d\n", cents_per_pulse);
            if(is_rid_glb(rid)) {
                config_set_cents_per_pulse_glb(cents_per_pulse);
            } else {
                relay_set_cents_per_pulse(rid, cents_per_pulse);
            }
        }

        Serial.printf("\n");
    }

    cJSON_Delete(json);

    return false;
}

static void mqtt_act_config(char *message) {
    boolean need_reboot = false;

    if(mqtt_act_config_wifi_ssid(message)) {
        need_reboot = true;
    }
    
    if(mqtt_act_config_wifi_password(message)) {
        need_reboot = true;
    }

    if(mqtt_act_config_relay(message)) {
        need_reboot = true;
    }

    if(need_reboot) {
        Serial.println("Need reboot on configuration change via mqtt");
        system_reboot();
    }
}

static void mqtt_act_reset(char *message) {
    system_reset();
}

static void mqtt_act_relay(char *message) {
    char rid_str[16] = {0};
    if(NULL == json_get_value(message, "rid", rid_str, sizeof(rid_str)) ||
        strlen(rid_str) <= 0) {
        Serial.printf("Invalid rid\n");
        return;
    }
    unsigned int rid = atoi(rid_str);

    char credit_str[16] = {0};
    if(NULL == json_get_value(message, "credit", credit_str, sizeof(credit_str)) ||
        strlen(credit_str) <= 0) {
        Serial.printf("Invalid credit\n");
        return;
    }
    unsigned int credit = atoi(credit_str);

    Serial.printf("rid = %d, credit = %d\n", rid, credit);
    relay_start(rid, credit);
}

#define MQTT_ACT_TOGGLE "toggle"
#define MQTT_ACT_STATUS "status"
#define MQTT_ACT_UPGRADE "upgrade"
#define MQTT_ACT_REBOOT "reboot"
#define MQTT_ACT_CONFIG "config"
#define MQTT_ACT_RESET "reset"
#define MQTT_ACT_RELAY "relay"
static void callback(char* topic, byte* payload, unsigned int payload_len) {
    /*
     * Notice: PubSubClient inserts the latest message content
     * before the content of previous message in payload buffer
     * instead of clear payload buffer before copy the latest message into it.
     * This means:
     * 1. the old message content will reside after the new message in the payload buffer.
     * 2. the payload_len is where the latest message content ends
     * We just copy the payload based on payload_len to a new content buffer so that
     * this problem won't lead to parsing mulfunction.
     */
    char content[512] = {0};
    if(payload_len+1 > sizeof(content)) {
        Serial.printf("MQTT payload overflow, payload_len = %d, content buffer size = %d\n",
            payload_len, sizeof(content));
        return;
    }
    memmove(content, payload, payload_len);
    Serial.printf("-> %s %s\n", topic, content);

    char act[32] = {0};
    if(NULL == json_get_value((char *)payload, "action", act, sizeof(act))) {
        Serial.println("Fail to get action");
        return;
    }

    if(0 == strcmp(act, MQTT_ACT_TOGGLE)) {
        mqtt_act_toggle(content);
        return;
    } else if(0 == strcmp(act, MQTT_ACT_STATUS)) {
        mqtt_act_status(content);
        return;
    } else if(0 == strcmp(act, MQTT_ACT_UPGRADE)) {
        mqtt_act_upgrade(content);
        return;
    } else if(0 == strcmp(act, MQTT_ACT_REBOOT)) {
        mqtt_act_reboot(content);
        return;
    } else if(0 == strcmp(act, MQTT_ACT_CONFIG)) {
        mqtt_act_config(content);
        return;
    } else if(0 == strcmp(act, MQTT_ACT_RESET)) {
        mqtt_act_reset(content);
        return;
    } else if(0 == strcmp(act, MQTT_ACT_RELAY)) {
        mqtt_act_relay(content);
        return;
    }

    Serial.println("Invalid action");
}

/* /MBS/RELAY/PUBLIC/REGISTER/{IMEI} */
#define MQTT_TOPIC_FMT_REGISTER "/MBS/RELAY/PUBLIC/REGISTER/%s"
#define MQTT_CONTENT_FMT_REGISTER "{\"action\":\"register\",\"imei\":\"%s\",\"model\":\"%s\",\"firmware_version\":\"%s\"}"
static void mqtt_register(void) {
    char topic[64] = {0};
    snprintf(topic, sizeof(topic), MQTT_TOPIC_FMT_REGISTER, system_get_deviceid());

    char content[128] = {0};
    snprintf(content, sizeof(content), MQTT_CONTENT_FMT_REGISTER,
        system_get_deviceid(), system_get_model_no(), system_get_firmware_version());

    mqtt_publish(topic, content);
}

/* /MBS/RELAY/{IMEI} */
#define MQTT_TOPIC_FMT_SUBSCRIBE "/MBS/RELAY/%s"
static void mqtt_subscribe(void) {
    char topic[64] = {0};
    snprintf(topic, sizeof(topic), MQTT_TOPIC_FMT_SUBSCRIBE, system_get_deviceid());
    mqttc.subscribe(topic);
}

static void mqtt_reconnect() {
    // Loop until we're reconnected
    Serial.printf("Connecting to MQTT Server %s:%d ...\n",
        mqtt_get_server(), mqtt_get_port());
    
    if (!mqttc.connect(system_get_deviceid(), mqtt_get_username(), mqtt_get_password())) {
        Serial.printf("Connect failed, result = %d\n", mqttc.state());
        Serial.printf("Try again in 5 seconds\n");
        delay(5000);
        return;
    }
    Serial.println("connected");
    
    mqtt_register();
    
    // ... and resubscribe
    mqtt_subscribe();
}

void mqtt_loop(void) {
    if (!mqttc.connected()) {
        mqtt_reconnect();
    }
    mqttc.loop();
}

