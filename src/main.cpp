#include <stdint.h>
#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "gpiopin.h"
#include "lcd12864.h"

GPIOPin RS(GPIOC, GPIO_Pin_10), RW(GPIOC, GPIO_Pin_11), E(GPIOC, GPIO_Pin_12), D7(GPIOC, GPIO_Pin_9),
        D6(GPIOC, GPIO_Pin_8), D5(GPIOC, GPIO_Pin_7), D4(GPIOC, GPIO_Pin_6), D3(GPIOB, GPIO_Pin_15),
        D2(GPIOB, GPIO_Pin_14), D1(GPIOB, GPIO_Pin_13), D0(GPIOB, GPIO_Pin_12);
lcd::LCD12864 display(RS, RW, E, D0, D1, D2, D3, D4, D5, D6, D7);

extern "C" const uint8_t viddata[];
extern "C" const uint32_t viddata_size;

int main() {
    sys::init_NVIC();

    display.init();
    display.start_draw();
    display.clear_drawing();
    for (uint8_t row = 0; row < 64; row ++) {
        for (uint8_t col = 0; col < 128 / 8; col ++) {
            uint8_t lcd_row = row;
            uint8_t lcd_col = col;
            if (row >= 32) {
                lcd_row -= 32;
                lcd_col += 128 / 8;
            }
            lcd_col /= 2;
            if (col % 2 == 0) {
                display.draw_buf[lcd_row][lcd_col] = viddata[row * (128 / 8) + col] << 8;
            }
            else {
                display.draw_buf[lcd_row][lcd_col] |= viddata[row * (128 / 8) + col];
            }
        }
    }
    display.update_drawing();
    
    while (true) {}
}
