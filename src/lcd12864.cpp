#include "lcd12864.h"
#include <string.h>
#include "util.h"
#include "delay.h"

namespace lcd {
    
    void LCD12864::clear() {
        write_cmd(Command::CLEAR);
    }
    void LCD12864::home() {
        write_cmd(Command::HOME);
    }
            
    void LCD12864::init() {
        if(!FOUR_WIRE_INTERFACE) {
            delay::ms(15);
            write_cmd_no_wait(Command::NORMAL_CMD_8BIT);
            delay::ms(5);
            write_cmd_no_wait(Command::NORMAL_CMD_8BIT);
            delay::ms(5);
        }
        else {
            delay::ms(15);
            write_cmd_no_wait(Command::NORMAL_CMD_4BIT);
            delay::ms(5);
            write_cmd_no_wait(Command::NORMAL_CMD_4BIT);
            delay::ms(5);
        }
        
        write_cmd(Command::ENTRY_CURSOR_SHIFT_RIGHT);
        write_cmd(Command::CLEAR);
        write_cmd(Command::DISPLAY_ON_CURSOR_OFF);
    }
    
    void LCD12864::set_cursor(uint8_t row, uint8_t col) {
        // If using extended command set, first set to use basic, write the address and change back
        if(is_using_extended()) {
            use_basic();
            switch(row){
            case 0: col += 0x80; break;
            case 1: col += 0x90; break;
            case 2: col += 0x88; break;
            case 3: col += 0x98; break;
            default: break;
            }
            // Make the first bit 1 and second bit 0 to match the command requirements
            col |= 0x80; // 1000 0000
            col &= 0xBF; // 1011 1111
            write_cmd(col);
            use_extended();
        }
        else {
            switch(row){
            case 0: col += 0x80; break;
            case 1: col += 0x90; break;
            case 2: col += 0x88; break;
            case 3: col += 0x98; break;
            default: break;
            }
            // Make the first bit 1 and second bit 0 to match the command requirements
            col |= 0x80; // 1000 0000
            col &= 0xBF; // 1011 1111
            write_cmd(col);
        }
    }
    
    bool LCD12864::is_using_extended() {
        return extended;
    }
    void LCD12864::use_extended() {
        if(extended) 
            return;
        write_cmd(FOUR_WIRE_INTERFACE ? Command::EXT_CMD_4BIT : Command::EXT_CMD_8BIT);
        extended = true;
    }
    void LCD12864::use_basic() {
        if(!extended)
            return;
        write_cmd(FOUR_WIRE_INTERFACE ? Command::NORMAL_CMD_4BIT : Command::NORMAL_CMD_8BIT);
        extended = false;
    }
    
    bool LCD12864::is_drawing() {
        return drawing;
    }
    void LCD12864::start_draw() {
        if(!is_using_extended()) {
            use_extended();
        }
        write_cmd(FOUR_WIRE_INTERFACE ? Command::EXT_GRAPHICS_ON_4BIT : Command::EXT_GRAPHICS_ON_8BIT);
        drawing = true;
    }
    void LCD12864::end_draw() {
        write_cmd(FOUR_WIRE_INTERFACE ? Command::EXT_GRAPHICS_OFF_4BIT : Command::EXT_GRAPHICS_OFF_8BIT);
        drawing = false;
    }
    
    void LCD12864::clear_drawing() {
        if(!is_drawing()) {
            return;
        }
        
        for(uint8_t row = 0; row < 32; row ++) {
            for(uint8_t col = 0; col < 16; col ++) {
                // The row gets written first
                // There are 32 rows (bottom 32 are just extensions of the top 32)
                // And then the column gets written (16 pixels)
                NoInterrupt noi;
                write_cmd(0x80 | row);
                write_cmd(0x80 | col);
                write_data(0x00);
                write_data(0x00);
            }
        }
        memset(display_buf, 0, sizeof(display_buf));
        memset(draw_buf, 0, sizeof(draw_buf));
    }
    
    // This function takes the drawing buffer, compares it with the display buffer and writes any necessary bytes.
    void LCD12864::update_drawing() {
        if (!is_drawing()) {
            return;
        }
        for (uint8_t row = 0; row < 32; row ++) {
            bool run = false;
            for (uint8_t col = 0; col < 16; col ++) {
                uint16_t data = draw_buf[row][col];
                // Compare drawBuf with dispBuf
                if (display_buf[row][col] != data) {
                    // Update the display buffer
                    display_buf[row][col] = data;
                    NoInterrupt noi;
                    if (!run) {
                        run = true;
                        write_cmd(0x80 | row);
                        write_cmd(0x80 | col);
                    }
                    // Write higher order byte first
                    write_data(display_buf[row][col] >> 8);
                    write_data(display_buf[row][col] & 0x00FF);
                }
                else {
                    run = false;
                }
            }
        }
    }
}
