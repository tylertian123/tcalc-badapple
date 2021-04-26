#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "gpiopin.h"

GPIOPin statusLED(GPIOA, GPIO_Pin_1);
GPIOPin ctrlLED(GPIOA, GPIO_Pin_2);
GPIOPin shiftLED(GPIOA, GPIO_Pin_3);

int main() {
    sys::initRCC();
    sys::initNVIC();

    // Init LEDs
    statusLED.init(GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    shiftLED.init(GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    ctrlLED.init(GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    statusLED = true;
    shiftLED = false;
    ctrlLED = false;

    while (true) {
        statusLED = !statusLED;
        delay::ms(1000);
    }
}
