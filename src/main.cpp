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
GPIOPin green(GPIOA, GPIO_Pin_1);
GPIOPin yellow(GPIOA, GPIO_Pin_2);
GPIOPin red(GPIOA, GPIO_Pin_3);

extern "C" const uint8_t viddata[];
extern "C" const uint32_t viddata_size;

constexpr uint8_t FPS = 10;
constexpr uint16_t FRAME_INTERVAL = 1000 / FPS;

void init_frame_timer() {
    // Set up timer
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_TimeBaseInitTypeDef initStruct = {
        // 72MHz / 36,000 = 2kHz
        .TIM_Prescaler = 36000 - 1,
        .TIM_CounterMode = TIM_CounterMode_Up,
        // Each tick is half a millisecond, so double the frame interval
        .TIM_Period = FRAME_INTERVAL * 2 - 1,
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_RepetitionCounter = 0,
    };
    TIM_TimeBaseInit(TIM3, &initStruct);
    // Set up interrupts
    NVIC_InitTypeDef nvicInit = {
        .NVIC_IRQChannel = TIM3_IRQn,
        .NVIC_IRQChannelPreemptionPriority = 2,
        .NVIC_IRQChannelSubPriority = 1,
        .NVIC_IRQChannelCmd = ENABLE,
    };
    NVIC_Init(&nvicInit);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
}


constexpr uint32_t FRAME_COUNT = 200;
uint32_t frame_number = 0;
void draw_frame(const uint8_t*);


volatile uint32_t elapsed = 0;
// Interrupt handler for TIM3 (frame timer)
extern "C" void TIM3_IRQHandler() {
    // Check and clear interrupt pending bit
    if (TIM_GetITStatus(TIM3, TIM_IT_Update)) {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        draw_frame(viddata + frame_number++ * 128 / 8 * 64);
        display.update_drawing();
        if (frame_number >= FRAME_COUNT) {
            TIM_Cmd(TIM3, DISABLE);
            TIM_Cmd(TIM4, DISABLE);

            display.clear_drawing();
            display.use_basic();
            display.home();
            display.printf("Expected %d", FRAME_INTERVAL * (FRAME_COUNT - 1));
            display.set_cursor(1, 0);
            display.printf("Actual %d", elapsed);
        }
    }
}


// Interrupt handler for TIM4
extern "C" void TIM4_IRQHandler() {
    // Check and clear interrupt pending bit
    if (TIM_GetITStatus(TIM4, TIM_IT_Update)) {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
        elapsed++;
    }
}

void draw_frame(const uint8_t *data) {
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
                display.draw_buf[lcd_row][lcd_col] = data[row * (128 / 8) + col] << 8;
            }
            else {
                display.draw_buf[lcd_row][lcd_col] |= data[row * (128 / 8) + col];
            }
        }
    }
}

int main() {
    sys::init_NVIC();

    green.init(GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    red.init(GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    yellow.init(GPIO_Mode_Out_PP, GPIO_Speed_2MHz);

    // Assert number of frames is correct
    if (viddata_size != FRAME_COUNT * 128 / 8 * 64) {
        red = true;
        while (true);
    }

    display.init();
    display.start_draw();
    display.clear_drawing();

    init_frame_timer();

    // Set up timer
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    TIM_TimeBaseInitTypeDef initStruct = {
        // 72MHz / 36000 = 2kHz
        .TIM_Prescaler = 36000 - 1,
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = 1 * 2 - 1,
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_RepetitionCounter = 0,
    };
    TIM_TimeBaseInit(TIM4, &initStruct);
    // Set up interrupts
    NVIC_InitTypeDef nvicInit = {
        .NVIC_IRQChannel = TIM4_IRQn,
        .NVIC_IRQChannelPreemptionPriority = 0,
        .NVIC_IRQChannelSubPriority = 0,
        .NVIC_IRQChannelCmd = ENABLE,
    };
    NVIC_Init(&nvicInit);
    //NVIC_SetPriority(TIM4_IRQn, 0);
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

    TIM_Cmd(TIM3, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
    
    while (true) {}
}
