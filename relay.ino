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

/* Relay */
/*
 * GPIO12 0 S4-OFF D10-OFF
 * GPIO12 1 S4-ON  D10-ON
 * GPIO13 0 S5-OFF D8-OFF
 * GPIO13 1 S5-ON  D8-ON
 * GPIO14 0 S3-OFF D4-OFF
 * GPIO14 1 S3-ON  D4-ON
 * GPIO16 0 S2-OFF D3-OFF
 * GPIO16 1 S2-ON  D3-ON
 */
#define PIN_R1 12
#define PIN_R2 13
#define PIN_R3 14
#define PIN_R4 16

#define RID_GLB 0   /* rid for board level configuration */
#define RID_MIN 1
#define RID_MAX 4
#define RID_NUM (RID_MAX - RID_MIN + 1)

unsigned int pins[RID_NUM] = {PIN_R1, PIN_R2, PIN_R3, PIN_R4};
#define PIN(rid) pins[(rid)-1]
boolean toggles[RID_NUM] = {false, false, false, false};
#define TOGGLE(rid) toggles[(rid)-1]

#define INV_PULSE_LENGTH 0
unsigned int pulse_length[RID_NUM+1] = {0};
#define PULSE_LENGTH(rid) pulse_length[rid]
#define PULSE_LENGTH_GLB pulse_length[RID_GLB]

#define INV_PULSE_INTERVAL 0
unsigned int pulse_interval[RID_NUM+1] = {0};
#define PULSE_INTERVAL(rid) pulse_interval[rid]
#define PULSE_INTERVAL_GLB pulse_interval[RID_GLB]

#define INV_CENTS_PER_PULSE 0
unsigned int cents_per_pulse[RID_NUM+1] = {0};
#define CENTS_PER_PULSE(rid) cents_per_pulse[rid]
#define CENTS_PER_PULSE_GLB cents_per_pulse[RID_GLB]

int relay_get_rid_min(void) {
    return RID_MIN;
}

int relay_get_rid_max(void) {
    return RID_MAX;
}

boolean is_rid_glb(unsigned int rid) {
    return RID_GLB == rid;
}

boolean is_rid_valid(unsigned int rid) {
    if(rid >= RID_MIN && rid <= RID_MAX) {
        return true;
    }

    return false;
}

boolean relay_get_toggle_status(unsigned int rid) {
    if(!is_rid_valid(rid)) {
        return false;
    }
    return TOGGLE(rid);
}

void relay_toggle(unsigned int rid) {
    digitalWrite(PIN(rid), !TOGGLE(rid));
    TOGGLE(rid) = !TOGGLE(rid);
}

unsigned int relay_get_pulse_length(unsigned int rid) {
    if(is_rid_glb(rid)) {
        return PULSE_LENGTH_GLB;
    }
    
    if(!is_rid_valid(rid)) {
        Serial.printf("Invalid rid %d", rid);
        return INV_PULSE_LENGTH;
    }

    return PULSE_LENGTH(rid);
}

unsigned int relay_get_pulse_length_glb(void) {
    return relay_get_pulse_length(RID_GLB);
}

void relay_set_pulse_length(unsigned int rid, unsigned int value) {
    if(is_rid_glb(rid)) {
        PULSE_LENGTH_GLB = value;
        return;
    }

    if(!is_rid_valid(rid)) {
        Serial.printf("Invalid rid %d", rid);
        return;
    }

    PULSE_LENGTH(rid) = value;
    return;
}

void relay_set_pulse_length_glb(unsigned int value) {
    relay_set_pulse_length(RID_GLB, value);

    unsigned int rid;
    for(rid = RID_MIN; rid <= RID_MAX; rid++) {
        if(INV_PULSE_LENGTH == PULSE_LENGTH(rid)) {
            PULSE_LENGTH(rid) = value;
        }
    }
}

unsigned int relay_get_pulse_interval(unsigned int rid) {
    if(is_rid_glb(rid)) {
        return PULSE_INTERVAL_GLB;
    }
    
    if(!is_rid_valid(rid)) {
        Serial.printf("Invalid rid %d", rid);
        return INV_PULSE_INTERVAL;
    }

    return PULSE_INTERVAL(rid);
}

unsigned int relay_get_pulse_interval_glb(void) {
    return relay_get_pulse_interval(RID_GLB);
}

void relay_set_pulse_interval(unsigned int rid, unsigned int value) {
    if(is_rid_glb(rid)) {
        PULSE_INTERVAL_GLB = value;
        return;
    }

    if(!is_rid_valid(rid)) {
        Serial.printf("Invalid rid %d", rid);
        return;
    }

    PULSE_INTERVAL(rid) = value;
    return;
}

void relay_set_pulse_interval_glb(unsigned int value) {
    relay_set_pulse_interval(RID_GLB, value);

    unsigned int rid;
    for(rid = RID_MIN; rid <= RID_MAX; rid++) {
        if(INV_PULSE_INTERVAL == PULSE_INTERVAL(rid)) {
            PULSE_INTERVAL(rid) = value;
        }
    }
}

unsigned int relay_get_cents_per_pulse(unsigned int rid) {
    if(is_rid_glb(rid)) {
        return CENTS_PER_PULSE_GLB;
    }
    
    if(!is_rid_valid(rid)) {
        Serial.printf("Invalid rid %d", rid);
        return INV_CENTS_PER_PULSE;
    }

    return CENTS_PER_PULSE(rid);
}

unsigned int relay_get_cents_per_pulse_glb(void) {
    return relay_get_cents_per_pulse(RID_GLB);
}

void relay_set_cents_per_pulse(unsigned int rid, unsigned int value) {
    if(is_rid_glb(rid)) {
        CENTS_PER_PULSE_GLB = value;
        return;
    }

    if(!is_rid_valid(rid)) {
        Serial.printf("Invalid rid %d", rid);
        return;
    }

    CENTS_PER_PULSE(rid) = value;
    return;
}

void relay_set_cents_per_pulse_glb(unsigned int value) {
    relay_set_cents_per_pulse(RID_GLB, value);

    unsigned int rid;
    for(rid = RID_MIN; rid <= RID_MAX; rid++) {
        if(INV_CENTS_PER_PULSE == CENTS_PER_PULSE(rid)) {
            CENTS_PER_PULSE(rid) = value;
        }
    }
}

static void relay_init_param_pulse_length(void) {
    int value = prop_load_pulse_length_glb();
    relay_set_pulse_length_glb(value);

    unsigned int rid;
    for(rid = RID_MIN; rid <= RID_MAX; rid++) {
        relay_set_pulse_length(rid, INV_PULSE_LENGTH);
    }
}

static void relay_init_param_pulse_interval(void) {
    int value = prop_load_pulse_interval_glb();
    relay_set_pulse_interval_glb(value);

    unsigned int rid;
    for(rid = RID_MIN; rid <= RID_MAX; rid++) {
        relay_set_pulse_interval(rid, INV_PULSE_INTERVAL);
    }
}

static void relay_init_param_cents_per_pulse(void) {
    int value = prop_load_cents_per_pulse_glb();
    relay_set_cents_per_pulse_glb(value);

    unsigned int rid;
    for(rid = RID_MIN; rid <= RID_MAX; rid++) {
        relay_set_cents_per_pulse(rid, INV_CENTS_PER_PULSE);
    }
}

static void relay_init_params(void) {
    relay_init_param_pulse_length();
    relay_init_param_pulse_interval();
    relay_init_param_cents_per_pulse();

    Serial.printf("RID #%d, pulse_length: %d, pulse_interval: %d, cents_per_pulse: %d\n",
        RID_GLB, relay_get_pulse_length_glb(), relay_get_pulse_interval_glb(),
        relay_get_cents_per_pulse_glb());
    
    unsigned int rid;
    for(rid = RID_MIN; rid <= RID_MAX; rid++) {
        Serial.printf("RID #%d, pulse_length: %d, pulse_interval: %d, cents_per_pulse: %d\n",
            rid, relay_get_pulse_length(rid), relay_get_pulse_interval(rid),
            relay_get_cents_per_pulse(rid));
    }
}

static void setup_pinmode(void) {
    unsigned int rid;
    for(rid = RID_MIN; rid <= RID_MAX; rid++) {
        pinMode(PIN(rid), OUTPUT);
    }
}

void setup_relay(void) {
    relay_init_params();
    setup_pinmode();
}

void relay_start(unsigned int rid, unsigned int credit) {
    if(!is_rid_valid(rid)) {
        Serial.printf("Invalid rid %d\n", rid);
        return;
    }

    unsigned int pulse_length = relay_get_pulse_length(rid);
    pulse_length = (INV_PULSE_LENGTH == pulse_length) ?
        relay_get_pulse_length_glb() : pulse_length;
    unsigned int pulse_interval = relay_get_pulse_interval(rid);
    pulse_interval = (INV_PULSE_INTERVAL == pulse_interval) ?
        relay_get_pulse_interval_glb() : pulse_interval;
    unsigned int cents_per_pulse = relay_get_cents_per_pulse(rid);
    cents_per_pulse = (INV_CENTS_PER_PULSE == cents_per_pulse) ?
        relay_get_cents_per_pulse_glb() : cents_per_pulse;

    /* in case of divide by zero error */
    if(0 == cents_per_pulse) {
        Serial.printf("Invalid cents_per_pulse %d\n", cents_per_pulse);
        return;
    }
    
    int pulse_times = credit / cents_per_pulse;
    if(0 != (credit%cents_per_pulse)) {
        pulse_times++;
    }
    
    Serial.printf("Relay Starting... pulse_length = %d, pulse_interval = %d, cents_per_pulse = %d, credit = %d, pulse_times = %d\n",
        pulse_length, pulse_interval, cents_per_pulse, credit, pulse_times);

    int i;
    for(i = 0; i < pulse_times; i++) {
        relay_toggle(rid);
        delay(pulse_length);
        relay_toggle(rid);
        delay(pulse_interval);
    }
}

