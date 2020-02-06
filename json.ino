#include <cJSON.h>  /* from github */
#include "config.h"

char *json_get_value(char *data, char *key, char *value, unsigned int value_len) {
    if(NULL == data) {
        Serial.println("Invalid data");
        return NULL;
    }

    if(NULL == key) {
        Serial.println("Invalid key");
        return NULL;
    }

    if(NULL == value || value_len <= 0) {
        Serial.println("Invalid value or value_len");
        return NULL;
    }

    cJSON *json = cJSON_Parse(data);
    if(NULL == json) {
        const char *err_ptr = cJSON_GetErrorPtr();
        if(NULL != err_ptr) {
            Serial.printf("cJSON parse failed, result = %s\n", err_ptr);
        } else {
            Serial.printf("cJSON parse failed\n");
        }
        return NULL;
    }

    cJSON *json_value = cJSON_GetObjectItemCaseSensitive(json, key);
    if(NULL == json_value) {
        Serial.printf("cJSON GetObjectItem failed, key = %s\n", key);
        cJSON_Delete(json);
        return NULL;
    }

    if(strlen(json_value->valuestring) + 1 > value_len) {
        Serial.printf("Value buffer too small, size = %d, len = %d\n",
-            value_len, strlen(json_value->valuestring) + 1);
        cJSON_Delete(json);
        return NULL;
    }

    snprintf(value, value_len, json_value->valuestring);

    cJSON_Delete(json);

    return value;
}

/*
 * set the value of specific key in json string
 * data: the original json string as input
 * key: key
 * value: new value of the key
 * buf: the buffer to write back the modified json string
 * buf_len: the write-back buffer length
 * return value:
 * <0: Failure
 *  0: Success & Changed
 *  1: Success & Unchanged
 */
int json_set_value(char *data, char *key, char *value, char *buf, unsigned int buf_len) {
    if(NULL == data) {
        Serial.println("Invalid data");
        return -1;
    }

    if(NULL == key) {
        Serial.println("Invalid key");
        return -1;
    }
    
    if(NULL == value) {
        Serial.println("Invalid value");
        return -1;
    }

    if(NULL == buf || buf_len <= 0) {
        Serial.println("Invalid buf or buf_len");
        return -1;
    }
    
    cJSON *json = cJSON_Parse(data);
    if(NULL == json) {
        const char *err_ptr = cJSON_GetErrorPtr();
        if(NULL != err_ptr) {
            Serial.printf("cJSON parse failed, result = %s\n", err_ptr);
        } else {
            Serial.printf("cJSON parse failed\n");
        }
        return -2;
    }

    cJSON *json_value = cJSON_GetObjectItemCaseSensitive(json, key);
    if(NULL != json_value) {
        if(0 == strcmp(value, json_value->valuestring)) {
            Serial.printf("The new value %s is the same with old value %s for key %s\n",
                value, json_value->valuestring, key);
            cJSON_Delete(json);
            return 1;
        }    
        cJSON_DeleteItemFromObjectCaseSensitive(json, key);
    }

    if(NULL == cJSON_AddStringToObject(json, key, value)) {
        Serial.printf("cJSON AddStringToObject failed, result = %s", cJSON_GetErrorPtr());
        cJSON_Delete(json);
        return -3;
    }

    if(!cJSON_PrintPreallocated(json, buf, buf_len, false)) {
        Serial.printf("cJSON export failed, result = %s\n", cJSON_GetErrorPtr());
        cJSON_Delete(json);
        return -4;
    }

    cJSON_Delete(json);

    return 0;
}

int json_init(char *data, unsigned int data_len) {
    cJSON *json = cJSON_CreateObject();
    if(NULL == json) {
        Serial.printf("cJSON create object failed\n");
        return -1;
    }

    if(!cJSON_PrintPreallocated(json, data, data_len, false)) {
        const char *err_ptr = cJSON_GetErrorPtr();
        if(NULL != err_ptr) {
            Serial.printf("cJSON print failed, result = %s\n", err_ptr);
        } else {
            Serial.printf("cJSON print failed, result = %s\n");
        }
        cJSON_Delete(json);
        return -2;
    }

    return 0;
}

char *json_get_wifi_ssid(char *json, char *ssid, unsigned int ssid_len) {
    return json_get_value(json, KEY_WIFI_SSID, ssid, ssid_len);
}

char *json_get_wifi_password(char *json, char *password, unsigned int password_len) {
    return json_get_value(json, KEY_WIFI_PASSWORD, password, password_len);
}

