/*
  Copyright (C) 2017 LeRoy Miller.

  Linknode ESP8266 4 Port Wifi enable relay, using MQTT

  Publish to:
  Host: test.mosquitto.org
  Topic: relayin
  Commands are 1 to 4 (1, 2, 3, 4) - These will toggle the relay

  Subscribe to:
  Host: test.mosquitto.org
  Topic: relayout
  Shows the status of relay

  This is a experiment using MQTT. Currently using a public broker,
  setting can be changed below. And no security, use at your own risk
  Libraries required:
  ESP8266Wifi, ESP8266WebServer, WiFiManager PubSubClient

  License:
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses>

*/

/*
   Nov 3, 2017 - Added status update.
                 Added unique MQTT client ID.
                 Moved Ack Updates to void updateStatus()

   Things that could be improved:
   Relay just listen for messages addressed to this module.
   Change status, and relay information (relayout) to use json strings

*/

/*
 * Please install the following libraries in
 * Arduino IDE -> Tools -> Manage Libraries
 * Or download from https://github.com/esp8266/Arduino/tree/master/libraries
 * and extract to library directory specified by File -> Preference -> Sketchbook location
 * - ESP8266
 * - PubSubClient
 * - ESP8266HTTPClient  // download from github and extract to library directory
 * - ESP8266httpUpdate  // download from github and extract to library directory
 */
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <time.h>

/* WIFI */

static char wifi_ssid[32] = {0};

static char *wifi_get_ssid(void) {
    return wifi_ssid;
}

static void wifi_set_ssid(char *ssid) {
    memset(wifi_ssid, 0, sizeof(wifi_ssid));
    snprintf(wifi_ssid, sizeof(wifi_ssid), ssid);
}

static void wifi_init_param_ssid(void) {
    char ssid[32] = {0};
    prop_load_wifi_ssid(ssid, sizeof(ssid));
    wifi_set_ssid(ssid);
}

static char wifi_password[32] = {0};

static char *wifi_get_password(void) {
    return wifi_password;
}

static void wifi_set_password(char *password) {
    memset(wifi_password, 0, sizeof(wifi_password));
    snprintf(wifi_password, sizeof(wifi_password), password);
}

static void wifi_init_param_password(void) {
    char password[32] = {0};
    prop_load_wifi_password(password, sizeof(password));
    wifi_set_password(password);
}

static void wifi_init_params(void) {
    wifi_init_param_ssid();
    wifi_init_param_password();
}

static void wifiManagerSaveConfigCallback() {
    Serial.printf("[WiFiManager] SaveConfigCallback called\n");
    Serial.printf("[WiFiManager] ssid: %s, password: %s\n", WiFi.SSID().c_str(), WiFi.psk().c_str());
    config_set_wifi_ssid((char *)(WiFi.SSID().c_str()));
    config_set_wifi_password((char *)(WiFi.psk().c_str()));
    system_reboot();
}

static void setup_wifi_manager(void) {
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    wifiManager.setSaveConfigCallback(wifiManagerSaveConfigCallback);
    char ap_ssid[32] = {0};
    snprintf(ap_ssid, sizeof(ap_ssid), "ESP-%s", system_get_deviceid());
    wifiManager.autoConnect(ap_ssid);
}

int setup_wifi() {
    wifi_init_params();
    
    Serial.printf("Connecting to WIFI %s/%s ...", wifi_get_ssid(), wifi_get_password());
    WiFi.begin(wifi_get_ssid(), wifi_get_password());

    time_t timeout = time(NULL) + 30;
    while(WL_CONNECTED != WiFi.status()) {
        if(time(NULL) > timeout) {
            setup_wifi_manager();
            return 0;
        }
        
        delay(500);
        Serial.printf(".");
    }
    Serial.println();

    Serial.printf("Connected, IP Address: ");
    Serial.println(WiFi.localIP());

    return 0;
}

