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
#include <ESP8266HTTPClient.h>  /* from github */
#include <ESP8266httpUpdate.h>  /* from github */

void upgrade_started() {
  Serial.println("[UPGRADE] HTTP update process started");
}

void upgrade_finished() {
  Serial.println("[UPGRADE] HTTP update process finished");
}

void upgrade_progress(int cur, int total) {
  Serial.printf("[UPGRADE] HTTP update process at %d of %d bytes...\n", cur, total);
}

void upgrade_error(int err) {
  Serial.printf("[UPGRADE] HTTP update fatal error code %d\n", err);
}


/* ESP8266httpUpdate callback */
void setup_upgrade(void) {
    ESPhttpUpdate.onStart(upgrade_started);
    ESPhttpUpdate.onEnd(upgrade_finished);
    ESPhttpUpdate.onProgress(upgrade_progress);
    ESPhttpUpdate.onError(upgrade_error);
}

void upgrade(char *url) {
    t_httpUpdate_return ret = ESPhttpUpdate.update(url);
    if(HTTP_UPDATE_OK != ret) {
        Serial.printf("Update failed, result = %d, %s\n",
            ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        return;
    }

    Serial.println("Update success");
    
    system_reboot();
}

