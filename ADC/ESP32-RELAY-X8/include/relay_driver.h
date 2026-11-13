#ifndef RELAY_DRIVER_H
#define RELAY_DRIVER_H

#include <Arduino.h>

void relay_driver_init();
void relay_set_channel(int channel, bool state);
void relay_set_all(bool state);
void relay_run_chase_step();

#endif