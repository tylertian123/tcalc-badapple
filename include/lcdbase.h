#pragma once

#include "gpiopin.h"
#include "stm32f10x.h"

#define LCD_ENABLE_DELAY 10        // Cycles
#define LCD_PRINTF_BUFFER_SIZE 128 // Characters

namespace lcd {

    class LCDBase {
    public:
        LCDBase(GPIOPin RS, GPIOPin RW, GPIOPin E, GPIOPin D0, GPIOPin D1, GPIOPin D2, GPIOPin D3, GPIOPin D4,
                GPIOPin D5, GPIOPin D6, GPIOPin D7, uint32_t timeout = 1000000)
                : RS(RS), RW(RW), E(E), D0(D0), D1(D1), D2(D2), D3(D3), D4(D4), D5(D5), D6(D6), D7(D7),
                  timeout(timeout), FOUR_WIRE_INTERFACE(false) {
            init_GPIO();
        }
        LCDBase(GPIOPin RS, GPIOPin RW, GPIOPin E, GPIOPin D4, GPIOPin D5, GPIOPin D6, GPIOPin D7,
                uint32_t timeout = 1000000)
                : RS(RS), RW(RW), E(E), D4(D4), D5(D5), D6(D6), D7(D7), timeout(timeout), FOUR_WIRE_INTERFACE(true) {
            init_GPIO();
        }

        virtual void init() = 0;
        virtual void set_cursor(uint8_t, uint8_t) = 0;
        virtual void clear() = 0;
        virtual void home() = 0;

        virtual uint32_t get_timeout();
        virtual void set_timeout(uint32_t);

        virtual void write_cmd(uint8_t);
        virtual void write_data(uint8_t);
        virtual uint8_t read_data();
        virtual void write_str(const char *);
        virtual void printf(const char *, ...);

    protected:
        GPIOPin RS, RW, E;
        GPIOPin D0, D1, D2, D3, D4, D5, D6, D7;
        uint32_t timeout;

        virtual void wait_busy();

        virtual void write_cmd_no_wait(uint8_t);

        virtual void set_data_port(uint8_t);
        virtual uint8_t read_data_port();

        virtual void set_GPIO_mode(const GPIOConfig &);

        const bool FOUR_WIRE_INTERFACE;

        static const GPIOConfig READ_CONFIG;
        static const GPIOConfig WRITE_CONFIG;

    private:
        void init_GPIO();
    };
} // namespace lcd

