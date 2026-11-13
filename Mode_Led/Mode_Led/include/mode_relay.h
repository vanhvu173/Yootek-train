#ifndef mode_relay_h
#define mode_relay_h

#include <Arduino.h>

const int MODES = 5; // 5 modes in led mode

void relay_init();
void relay_set_mode(int mode);
int relay_get_mode();
void relay_set_speed(int speedMs);
int relay_get_speed();
void relay_cycle_mode();
void relay_update();

#endif