#ifndef SOFT_TIMER_H
#define SOFT_TIMER_H

#include <Arduino.h>

// Bộ định thời non-blocking dùng chung cho mọi module
typedef struct {
    uint32_t periodMs; // chu kỳ
    uint32_t lastMs;   // mốc thời gian lần chạy trước
} SoftTimer_t;

// Trả về true mỗi khi hết chu kỳ (an toàn khi millis() tràn số)
static inline bool SoftTimer_Expired(SoftTimer_t* t) {
    uint32_t now = millis();
    if (now - t->lastMs < t->periodMs) return false;
    t->lastMs = now;
    return true;
}

#endif