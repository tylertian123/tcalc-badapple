#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "gpiopin.h"

GPIOPin statusLED(GPIOA, GPIO_Pin_1);

int main() {
    sys::initNVIC();

    // Init LEDs
    statusLED.init(GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
    statusLED = true;

    while (true) {
        delay::ms(1000);
        statusLED = !statusLED;
    }
}
