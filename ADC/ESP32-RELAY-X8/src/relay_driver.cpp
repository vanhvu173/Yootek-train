#include "relay_driver.h"

const int NUM_RELAYS = 8;
const int relayPins[NUM_RELAYS] = {32, 33, 25, 26, 27, 14, 12, 13};

#define RELAY_ON   HIGH
#define RELAY_OFF  LOW

static int chaseIndex = 0;

void relay_driver_init() {
    for (int i = 0; i < NUM_RELAYS; i++) {
        pinMode(relayPins[i], OUTPUT);
        digitalWrite(relayPins[i], RELAY_OFF);
    }
}

void relay_set_channel(int channel, bool state) {
    if (channel >= 1 && channel <= NUM_RELAYS) {
        digitalWrite(relayPins[channel - 1], state ? RELAY_ON : RELAY_OFF);
    }
}

void relay_set_all(bool state) {
    for (int i = 0; i < NUM_RELAYS; i++) {
        digitalWrite(relayPins[i], state ? RELAY_ON : RELAY_OFF);
    }
}

void relay_run_chase_step() {
    relay_set_all(false);
    digitalWrite(relayPins[chaseIndex], RELAY_ON);
    chaseIndex = (chaseIndex + 1) % NUM_RELAYS;
}