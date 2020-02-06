#include <FS.h>
#include "config.h"

#define DTU_PROPERTIES_FILE "/dtu.properties"

static char * prop_load_file(char *buf, unsigned int buf_len) {
    if(!SPIFFS.exists(DTU_PROPERTIES_FILE)) {
        Serial.printf("%s not exist\n", DTU_PROPERTIES_FILE);
        return NULL;
    }
    
    File f = SPIFFS.open(DTU_PROPERTIES_FILE, "r");
    if(NULL == f) {
        Serial.printf("Open %s for read failed\n", DTU_PROPERTIES_FILE);
        return NULL;
    }
    Serial.printf("Open %s for read ok\n", DTU_PROPERTIES_FILE);

    if(0 == f.size()) {
        Serial.printf("%s is empty", DTU_PROPERTIES_FILE);
        return NULL;
    }

    char *ptr = buf;
    unsigned int size = buf_len;
    while(f.available()) {
        int n = f.readBytes(ptr, size);
        ptr += n;
        size -= n;
    }
    f.close();

    Serial.printf("%s Content(%d): %s\n", DTU_PROPERTIES_FILE, strlen(buf), buf);

    return buf;
}

static int prop_save_file(char *buf, unsigned int len) {
    File f = SPIFFS.open(DTU_PROPERTIES_FILE, "w");
    if(NULL == f) {
        Serial.printf("Open %s for write failed\n", DTU_PROPERTIES_FILE);
        return -1;
    }
    Serial.printf("Open %s for write ok\n", DTU_PROPERTIES_FILE);

    return f.write(buf, len);
}

void prop_delete_file(void) {
    SPIFFS.remove(DTU_PROPERTIES_FILE);
}

static char * prop_load_value(char *key, char *value, unsigned int value_len, char *default_value) {
    char data[256] = {0};
    
    if(NULL == prop_load_file(data, sizeof(data))) {
        Serial.printf("Load %s failed, use default value \"%s\" for key \"%s\"\n",
            DTU_PROPERTIES_FILE, default_value, key);
        snprintf(value, value_len, default_value);
        return value;
    }

    if(NULL == json_get_value(data, key, value, value_len)) {
        Serial.printf("Get value for key \"%s\" failed, use default value \"%s\"\n",
            key, default_value);
        snprintf(value, value_len, default_value);
        return value;
    }

    Serial.printf("Loaded value for key \"%s\": \"%s\"\n", key, value);
    
    return value;
}

static int prop_save_value(char *key, char *value) {
    char data[256] = {0};
    
    if(NULL == prop_load_file(data, sizeof(data))) {
        json_init(data, sizeof(data));
    }

    char new_data[256] = {0};
    int ret = json_set_value(data, key, value, new_data, sizeof(new_data));
    if(1 == ret) {  /* no change on value */
        return 0;
    }
    if(0 == ret) {
        return prop_save_file(new_data, strlen(new_data));
    }

    return ret;
}

#define DEF_WIFI_SSID "circumvend"

void prop_load_wifi_ssid(char *ssid, unsigned int ssid_len) {
    prop_load_value(KEY_WIFI_SSID, ssid, ssid_len, DEF_WIFI_SSID);
}

void prop_save_wifi_ssid(char *ssid) {
    prop_save_value(KEY_WIFI_SSID, ssid);
}

#define DEF_WIFI_PASSWORD "circumvend19731"

void prop_load_wifi_password(char *password, unsigned int password_len) {
    prop_load_value(KEY_WIFI_PASSWORD, password, password_len, DEF_WIFI_PASSWORD);
}

void prop_save_wifi_password(char *password) {
    prop_save_value(KEY_WIFI_PASSWORD, password);
}

#define DEF_PULSE_LENGTH 40 /* 40ms */

unsigned int prop_load_pulse_length_glb(void) {
    char value[32] = {0};
    char def_value[32] = {0};
    snprintf(def_value, sizeof(def_value), "%d", DEF_PULSE_LENGTH);
    prop_load_value(KEY_PULSE_LENGTH, value, sizeof(value), def_value);
    return atoi(value);
}

void prop_save_pulse_length_glb(unsigned int value) {
    char value_str[32] = {0};
    snprintf(value_str, sizeof(value_str), "%d", value);
    prop_save_value(KEY_PULSE_LENGTH, value_str);
}

#define DEF_PULSE_INTERVAL 40   /* 40ms */

unsigned int prop_load_pulse_interval_glb(void) {
    char value[32] = {0};
    char def_value[32] = {0};
    snprintf(def_value, sizeof(def_value), "%d", DEF_PULSE_INTERVAL);
    prop_load_value(KEY_PULSE_INTERVAL, value, sizeof(value), def_value);
    return atoi(value);
}

void prop_save_pulse_interval_glb(unsigned int value) {
    char value_str[32] = {0};
    snprintf(value_str, sizeof(value_str), "%d", value);
    prop_save_value(KEY_PULSE_INTERVAL, value_str);
}

#define DEF_CENTS_PER_PULSE 100 /* 100cents */

unsigned int prop_load_cents_per_pulse_glb(void) {
    char value[32] = {0};
    char def_value[32] = {0};
    snprintf(def_value, sizeof(def_value), "%d", DEF_CENTS_PER_PULSE);
    prop_load_value(KEY_CENTS_PER_PULSE, value, sizeof(value), def_value);
    return atoi(value);
}

void prop_save_cents_per_pulse_glb(unsigned int value) {
    char value_str[32] = {0};
    snprintf(value_str, sizeof(value_str), "%d", value);
    prop_save_value(KEY_CENTS_PER_PULSE, value_str);
}

