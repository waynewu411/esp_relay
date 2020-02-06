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
#include <FS.h>
#include <ESP8266WiFi.h>

static void system_setup_serial(void) {
    Serial.begin(115200);
    delay(10);
}

#define MODEL_NO "LinkNode-R4"
#define FIRMWARE_VERSION "1.00.01L"

char *system_get_model_no(void) {
    return MODEL_NO;
}

char *system_get_firmware_version(void) {
    return FIRMWARE_VERSION;
}

/* system level deviceid */
static char deviceid[32] = {0};

void system_setup_deviceid(void) {
    byte mac[6] = {0};
    WiFi.macAddress(mac);
    snprintf(deviceid, sizeof(deviceid), "%02X%02X%02X%02X%02X%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.printf("Device ID: %s\n", deviceid);
}

char *system_get_deviceid(void) {
    return deviceid;
}

void system_display_startup_info(void) {
    Serial.printf("Model No: %s\n", system_get_model_no());
    Serial.printf("Firmware Version: %s\n", system_get_firmware_version());
}

void setup_spiffs(void) {
    if(SPIFFS.begin()) {
        Serial.println("SPIFFS initialize ok");
        return;
    }

    Serial.println("SPIFFS initialize failed");
}

void setup_flash(void) {
    Serial.printf("Checking Flash...\n");
    
    uint32_t realSize = ESP.getFlashChipRealSize();
    uint32_t ideSize = ESP.getFlashChipSize();
    FlashMode_t ideMode = ESP.getFlashChipMode();

    Serial.printf("Flash real id:   %08X\n", ESP.getFlashChipId());
    Serial.printf("Flash real size: %u bytes\n", realSize);

    Serial.printf("Flash ide  size: %u bytes\n", ideSize);
    Serial.printf("Flash ide speed: %u Hz\n", ESP.getFlashChipSpeed());
    Serial.printf("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));

    if (ideSize != realSize) {
        Serial.printf("Flash Chip configuration wrong!\n");
    } else {
        Serial.printf("Flash Chip configuration ok.\n");
    }
}

void setup_system(void) {
    system_setup_serial();

    system_setup_deviceid();

    system_display_startup_info();

    setup_spiffs();
}

void system_reboot(void) {
    Serial.println("Rebooting...");
    ESP.restart();
}

void system_reset(void) {
    prop_delete_file();
    system_reboot();
}

