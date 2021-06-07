#include <stdint.h>
#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "gpiopin.h"
#include "lcd12864.h"
#include "decoder.h"

GPIOPin RS(GPIOC, GPIO_Pin_10), RW(GPIOC, GPIO_Pin_11), E(GPIOC, GPIO_Pin_12), D7(GPIOC, GPIO_Pin_9),
        D6(GPIOC, GPIO_Pin_8), D5(GPIOC, GPIO_Pin_7), D4(GPIOC, GPIO_Pin_6), D3(GPIOB, GPIO_Pin_15),
        D2(GPIOB, GPIO_Pin_14), D1(GPIOB, GPIO_Pin_13), D0(GPIOB, GPIO_Pin_12);
lcd::LCD12864 display(RS, RW, E, D0, D1, D2, D3, D4, D5, D6, D7);
GPIOPin green(GPIOA, GPIO_Pin_1);
GPIOPin yellow(GPIOA, GPIO_Pin_2);
GPIOPin red(GPIOA, GPIO_Pin_3);

constexpr uint8_t FPS = 12;
constexpr uint16_t FRAME_INTERVAL = 1000 / FPS;

VideoDecoder decoder;

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

// Interrupt handler for TIM3 (frame timer)
extern "C" void TIM3_IRQHandler() {
    // Check and clear interrupt pending bit
    if (TIM_GetITStatus(TIM3, TIM_IT_Update)) {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        
        if (decoder.read_frame(display.draw_buf)) {
            display.update_drawing();
        }
        else {
            TIM_Cmd(TIM3, DISABLE);
        }
    }
}

int main() {
    sys::init_NVIC();

    green.init(GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    red.init(GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    yellow.init(GPIO_Mode_Out_PP, GPIO_Speed_2MHz);

    display.init();
    display.start_draw();
    display.clear_drawing();

    init_frame_timer();

    TIM_Cmd(TIM3, ENABLE);
    
    while (true) {}
}
