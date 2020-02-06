#include "config.h"

boolean config_set_wifi_ssid(char *ssid) {
    if(0 == strcmp(ssid, wifi_get_ssid())) {
        return false;
    }

    prop_save_wifi_ssid(ssid);
    wifi_set_ssid(ssid);

    return true;
}

boolean config_set_wifi_password(char *password) {
    if(0 == strcmp(password, wifi_get_password())) {
        return false;
    }

    prop_save_wifi_password(password);
    wifi_set_password(password);

    return true;
}

boolean config_set_pulse_length_glb(unsigned int value) {
    if(value == relay_get_pulse_length_glb()) {
        return false;
    }

    prop_save_pulse_length_glb(value);
    relay_set_pulse_length_glb(value);

    return false;
}

boolean config_set_pulse_interval_glb(unsigned int value) {
    if(value == relay_get_pulse_interval_glb()) {
        return false;
    }

    prop_save_pulse_interval_glb(value);
    relay_set_pulse_interval_glb(value);

    return false;
}

boolean config_set_cents_per_pulse_glb(unsigned int value) {
    if(value == relay_get_cents_per_pulse_glb()) {
        return false;
    }

    prop_save_cents_per_pulse_glb(value);
    relay_set_cents_per_pulse_glb(value);

    return false;
}

