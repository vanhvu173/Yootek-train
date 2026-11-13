#include "button_uart.h"    
#include "mode_relay.h"      

#define BUILTIN_BUTTON 0

static int lastButtonState = HIGH;            
static unsigned long buttonPressTime = 0;     
static bool isButtonPressed = false;          
const unsigned long debounceDelay = 40;       
const unsigned long longPressTime = 1000;     

// Hàm khởi tạo chân nút nhấn
void input_init() {
    pinMode(BUILTIN_BUTTON, INPUT_PULLUP);    
}

// Hàm xử lý thao tác nhấn nhả (Click) và nhấn giữ (Long Press)
void input_handle_button() {
    int reading = digitalRead(BUILTIN_BUTTON); 

    // Phát hiện thời điểm vừa nhấn nút (chuyển từ HIGH sang LOW)
    if (reading == LOW && lastButtonState == HIGH) {
        buttonPressTime = millis();          
        isButtonPressed = true;                
    }

    // Phát hiện thời điểm vừa nhả nút (chuyển từ LOW lên HIGH)
    if (reading == HIGH && lastButtonState == LOW && isButtonPressed) {
        unsigned long pressDuration = millis() - buttonPressTime; 
        
        //  Thay đổi tốc độ nháy nếu nhấn giữ lâu hơn 1 giây, hoặc chuyển mode nếu nhấn nhanh
        if (pressDuration >= longPressTime) {  
            relay_cycle_mode();               
        } else if (pressDuration >= debounceDelay) { 
            int nextMode = (relay_get_mode() + 1) % MODES; // 
            relay_set_mode(nextMode);         
        }
        isButtonPressed = false;              
    }
    lastButtonState = reading;                 // Cập nhật trạng thái nút hiện tại cho vòng lặp kế tiếp
}

// Hàm nhận và phân tích lệnh gửi từ máy tính qua cổng Serial UART
void input_handle_uart() {
    if (Serial.available() > 0) {              
        String cmd = Serial.readStringUntil('\n'); 
        cmd.trim();                            
        cmd.toUpperCase();                     
        if (cmd.startsWith("S ")) {            
            int val = cmd.substring(2).toInt();
            relay_set_speed(val);              
        } else if (cmd.startsWith("MODE ")) {  
            int newMode = cmd.substring(5).toInt(); 
            relay_set_mode(newMode);           
        }
    }
}