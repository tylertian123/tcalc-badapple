#pragma once

#include <stdint.h>

/*
 * Disables interrupts for the rest of the current block.
 */
class NoInterrupt {
public:
    NoInterrupt() : interrupt_PRIMASK(__get_PRIMASK()) {
        __disable_irq();
    }

    ~NoInterrupt() {
        if (!interrupt_PRIMASK) {
            __enable_irq();
        }
    }

private:
    uint32_t interrupt_PRIMASK;
};
