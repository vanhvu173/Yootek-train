#include <mode_relay.h>

const int relay_pin[] = {32, 33, 25, 26, 27, 14, 12, 13}; // relay pins
const int num_relays = sizeof(relay_pin) / sizeof(relay_pin[0]); // number of relays

#define RELAY_MODE_OFF LOW
#define RELAY_MODE_ON HIGH

static int current_mode = 0; // current mode
static int blink_speed = 250;
static unsigned long prev_time = 0;

// mode veriables
//mode 2
static bool sync_blink_state = false;
// mode 3
static int chase_direction_index = 0; 
static int chase_direction = 1;
// mode 4
static int fill_step = 0;
static bool filling = true;

static void relay_set_all(bool state) {
  for (int i = 0; i < num_relays; i++) {
    digitalWrite(relay_pin[i], state ? RELAY_MODE_ON : RELAY_MODE_OFF);
  }
}

void relay_init() {
  for (int i = 0; i < num_relays; i++) {
    pinMode(relay_pin[i], OUTPUT);
    digitalWrite(relay_pin[i], RELAY_MODE_OFF);
  }
}

void relay_set_mode(int mode) {
    if(mode <0 || mode >= MODES) {
        return; // Invalid mode, do nothing
    }
    // change mode and reset all mode variables
    current_mode = mode;
    sync_blink_state = false;
    chase_direction_index = 0; 
    chase_direction = 1;
    fill_step = 0;
    filling = true;

    Serial.printf("Mode set to: %d\n", current_mode);
    switch (current_mode) {
        case 0:
            relay_set_all(false);
            Serial.printf("All relays OFF\n");
            break;
        case 1:
            relay_set_all(true);
            Serial.printf("All relays ON\n");
            break;
        case 2:
            Serial.printf("Sync Blink mode (%d ms)\n", blink_speed);
            break;
        case 3:
            Serial.printf("Chase mode (%d ms)\n", blink_speed);
            break;
        case 4:
            Serial.printf("Fill mode (%d ms)\n", blink_speed);
            break;
    }
}

// Get the current mode
int relay_get_mode() {
    return current_mode;
}

// Set the blink speed 
void relay_set_speed(int speedMs) {
    if(speedMs > 20 || speedMs < 5000) {
        blink_speed = speedMs;
        Serial.printf("Blink speed set to: %d ms\n", blink_speed);
    } else {
        Serial.printf("Invalid speed. Must be between 20 and 5000 ms.\n");
    }
}

// Get the blink speed
int relay_get_speed() {
    return blink_speed;
}

// Cycle through the blink speeds: 100ms, 500ms, 1000ms
void relay_cycle_mode() {
    if(blink_speed == 100) blink_speed = 500;
    else if(blink_speed == 500) blink_speed = 1000;
    else blink_speed = 100;
    Serial.printf("Blink speed set to: %d ms\n", blink_speed);
}

void relay_update(){
    unsigned long current_time = millis();
    if(current_time - prev_time >= (unsigned long)blink_speed) {

    prev_time = current_time; // Update the previous time

    // Update mode 2
    if(current_mode == 2) {
        sync_blink_state = !sync_blink_state;
        relay_set_all(sync_blink_state);
    }

    // Update mode 3
    else if(current_mode == 3) {
        relay_set_all(false); // Turn off all relays first
        digitalWrite(relay_pin[chase_direction_index], RELAY_MODE_ON); //only turn on the current relay
        chase_direction_index += chase_direction; //1 -> 8
        if(chase_direction_index >= num_relays) {
            chase_direction_index = num_relays - 1;
            chase_direction = -1; // Change direction 8 -> 1
        } 
        else if(chase_direction_index < 0) {
            chase_direction_index = 0;
            chase_direction = 1; // Change direction 1 -> 8
        }
    }

    // Update mode 4
    else if (current_mode == 4) {
        
            if (filling) { 
                digitalWrite(relay_pin[fill_step], RELAY_MODE_ON); 
                fill_step++;                      
                if (fill_step >= num_relays) {     
                fill_step = num_relays - 1;    
                filling = false;         
            }
        } else {                             
            digitalWrite(relay_pin[fill_step], RELAY_MODE_OFF);
            fill_step--;                      
            if (fill_step < 0) {           
                fill_step = 0;                
                filling = true;            
            }  
        }
        
    }
}
}