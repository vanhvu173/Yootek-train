#include <Arduino.h>         
#include "mode_relay.h"      
#include "button_uart.h"     


void setup() {
    Serial.begin(115200);    
    relay_init();            
    input_init();      


}


void loop() {
    input_handle_uart();    
    input_handle_button();   
    relay_update();       
}