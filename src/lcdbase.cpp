#include "lcdbase.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "delay.h"

namespace lcd {
    
    const GPIOConfig LCDBase::READ_CONFIG = { GPIO_Mode_IPU, GPIO_Speed_2MHz };
    const GPIOConfig LCDBase::WRITE_CONFIG = { GPIO_Mode_Out_PP, GPIO_Speed_2MHz };
    
    #define LCD_EDELAY() delay::cycles(LCD_ENABLE_DELAY)
    #define INIT_I(x) x.init(GPIO_Mode_IPU, GPIO_Speed_10MHz)
    #define INIT_O(x) x.init(GPIO_Mode_Out_PP, GPIO_Speed_10MHz)
    
    void LCDBase::init_GPIO() {
        // Initialize the pins for output first
        INIT_O(RS);
        INIT_O(RW);
        INIT_O(E);
        
        set_GPIO_mode(WRITE_CONFIG);
        
        // Reset the data pins just in case
        set_data_port(0x00);
        RS = false;
        RW = false;
        E = false;
    }
    void LCDBase::set_GPIO_mode(const GPIOConfig &config) {
        if(!FOUR_WIRE_INTERFACE) {
            D0.init(config);
            D1.init(config);
            D2.init(config);
            D3.init(config);
        }
        D4.init(config);
        D5.init(config);
        D6.init(config);
        D7.init(config);
    }
    
    uint32_t LCDBase::get_timeout() {
        return this->timeout;
    }
    void LCDBase::set_timeout(uint32_t t) {
        timeout = t;
    }
    
    // These functions set and read from the data port
    // If in four wire interface, only the lowest 4 bits will be written
    void LCDBase::set_data_port(uint8_t data) {
        if(!FOUR_WIRE_INTERFACE) {
            D0 = data & 0x01;
            D1 = data & 0x02;
            D2 = data & 0x04;
            D3 = data & 0x08;
            D4 = data & 0x10;
            D5 = data & 0x20;
            D6 = data & 0x40;
            D7 = data & 0x80;
        }
        else {
            data &= 0x0F;
            D4 = data & 0x01;
            D5 = data & 0x02;
            D6 = data & 0x04;
            D7 = data & 0x08;
        }
    }
    // If in four wire interface, only a nibble will be read
    uint8_t LCDBase::read_data_port() {
        set_GPIO_mode(READ_CONFIG);
        uint8_t result = FOUR_WIRE_INTERFACE ? (D7 << 3 | D6 << 2 | D5 << 1 | D4 << 0) : 
                (D0 << 0 | D1 << 1 | D2 << 2 | D3 << 3 | D4 << 4 | D5 << 5 | D6 << 6 | D7 << 7);
        set_GPIO_mode(WRITE_CONFIG);
        return result;
    }
    
    /*
     * RS selects whether a command or data will be sent. High - Data, Low - Command
     * RW selects whether the operation is a read or write operation. High - Read, Low - Write
     * E enables the LCD by generating a pulse
     */
    void LCDBase::wait_busy() {
        {
            NoInterrupt noi;
            D7 = true;
            RS = false;
            RW = true;
            E = true;
            LCD_EDELAY();
        }
        // Initialize to read the busy flag
        INIT_I(D7);
        // Wait until the pin is cleared
        while(D7) {
            {
                NoInterrupt noi;
                E = false;
                LCD_EDELAY();
                E = true;
                
                if(FOUR_WIRE_INTERFACE) {
                    LCD_EDELAY();
                    E = false;
                    LCD_EDELAY();
                    E = true;
                }
            }
        }
        E = false;
        INIT_O(D7);
    }
    
        
    void LCDBase::write_cmd(uint8_t cmd) {
        wait_busy();

        NoInterrupt noi;
        RS = false;
        RW = false;
        
        if(FOUR_WIRE_INTERFACE) {
            set_data_port(cmd >> 4);
            E = true;
            LCD_EDELAY();
            E = false;
            set_data_port(cmd & 0x0F);
            LCD_EDELAY();
            E = true;
            LCD_EDELAY();
            E = false;
        }
        else {
            set_data_port(cmd);
            E = true;
            LCD_EDELAY();
            E = false;
        }
    }
    // The busy flag cannot be checked before initialization, thus delays are used instead of busy flag checking
    void LCDBase::write_cmd_no_wait(uint8_t cmd) {
        NoInterrupt noi;

        RS = false;
        RW = false;
        
        if(FOUR_WIRE_INTERFACE) {
            set_data_port(cmd >> 4);
            E = true;
            LCD_EDELAY();
            E = false;
            set_data_port(cmd & 0x0F);
            LCD_EDELAY();
            E = true;
            LCD_EDELAY();
            E = false;
        }
        else {
            set_data_port(cmd);
            E = true;
            LCD_EDELAY();
            E = false;
        }
    }
    void LCDBase::write_data(uint8_t data) {
        wait_busy();

        NoInterrupt noi;

        RS = true;
        RW = false;
        
        if(FOUR_WIRE_INTERFACE) {
            set_data_port(data >> 4);
            E = true;
            LCD_EDELAY();
            E = false;
            set_data_port(data & 0x0F);
            LCD_EDELAY();
            E = true;
            LCD_EDELAY();
            E = false;
        }
        else {
            set_data_port(data);
            E = true;
            LCD_EDELAY();
            E = false;
        }
    }
    uint8_t LCDBase::read_data() {
        wait_busy();

        NoInterrupt noi;

        RS = true;
        RW = true;
        
        uint8_t out;
        if(FOUR_WIRE_INTERFACE) {
            E = true;
            LCD_EDELAY();
            out = read_data_port() << 4;
            E = false;
            LCD_EDELAY();
            E = true;
            LCD_EDELAY();
            out |= read_data_port() & 0x0F;
            E = false;
        }
        else {
            E = true;
            LCD_EDELAY();
            out = read_data_port();
            E = false;
        }
        return out;
    }
    void LCDBase::write_str(const char *str) {
        for(uint16_t i = 0; str[i] != '\0'; i ++) {
            write_data(str[i]);
        }
    }

    // TODO: Remove for production
    void LCDBase::printf(const char *fmt, ...) {
        // Buffer used to store formatted string
        char buf[LCD_PRINTF_BUFFER_SIZE] = { 0 };
        va_list args;
        va_start(args, fmt);
        // Use vsnprintf to safely format the string and put into the buffer
        vsnprintf(buf, LCD_PRINTF_BUFFER_SIZE, fmt, args);
        write_str(buf);
    }
    
    #undef LCD_EDELAY
    #undef INIT_I
    #undef INIT_O
}
