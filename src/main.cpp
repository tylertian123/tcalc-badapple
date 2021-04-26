#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "gpiopin.h"
#include "lcd12864.h"

GPIOPin RS(GPIOC, GPIO_Pin_10), RW(GPIOC, GPIO_Pin_11), E(GPIOC, GPIO_Pin_12), D7(GPIOC, GPIO_Pin_9),
        D6(GPIOC, GPIO_Pin_8), D5(GPIOC, GPIO_Pin_7), D4(GPIOC, GPIO_Pin_6), D3(GPIOB, GPIO_Pin_15),
        D2(GPIOB, GPIO_Pin_14), D1(GPIOB, GPIO_Pin_13), D0(GPIOB, GPIO_Pin_12);
lcd::LCD12864 display(RS, RW, E, D0, D1, D2, D3, D4, D5, D6, D7);

int main() {
    sys::init_NVIC();

    display.init();
    display.start_draw();
    display.clear_drawing();

    display.draw_buf[0][0] = 0b11110000'10101010;
    display.draw_buf[0][1] = 0b11111111'11111111;

    display.update_drawing();
    
    while (true) {}
}
