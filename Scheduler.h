#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>

namespace Scheduler {
    typedef void (*command)();

    template <unsigned long wait_ms, command c>
    void repeat() {
        static unsigned long start = millis();
        if (millis() - start >= wait_ms) {
            start += wait_ms;
            c();
        }
    }
}

#endif