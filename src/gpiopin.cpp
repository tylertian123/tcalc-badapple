#include "gpiopin.h"

GPIOPin::GPIOPin() : port(0), pin(0) {
}

GPIOPin::GPIOPin(GPIO_TypeDef *port, uint16_t pin) : port(port), pin(pin) {
}

void GPIOPin::set(const bool &val) {
    GPIO_WriteBit(port, pin, static_cast<BitAction>(val));
}

bool GPIOPin::get(void) const {
    return GPIO_ReadInputDataBit(port, pin);
}

GPIOPin &GPIOPin::operator=(const bool &val) {
    set(val);
    return *this;
}

GPIOPin::operator bool() const {
    return get();
}

uint32_t GPIOPin::get_RCC_perhiph() const {
    if (port == GPIOA) {
        return RCC_APB2Periph_GPIOA;
    }
    else if (port == GPIOB) {
        return RCC_APB2Periph_GPIOB;
    }
    else if (port == GPIOC) {
        return RCC_APB2Periph_GPIOC;
    }
    else if (port == GPIOD) {
        return RCC_APB2Periph_GPIOD;
    }
    else if (port == GPIOE) {
        return RCC_APB2Periph_GPIOE;
    }
    else if (port == GPIOF) {
        return RCC_APB2Periph_GPIOF;
    }
    else {
        return RCC_APB2Periph_GPIOG;
    }
}

void GPIOPin::init(GPIOMode_TypeDef mode, GPIOSpeed_TypeDef speed) {
    RCC_APB2PeriphClockCmd(get_RCC_perhiph(), ENABLE);
    GPIO_InitTypeDef initStruct;
    initStruct.GPIO_Mode = mode;
    initStruct.GPIO_Speed = speed;
    initStruct.GPIO_Pin = pin;
    GPIO_Init(port, &initStruct);
}

void GPIOPin::init(const GPIOConfig &config) {
    init(config.mode, config.speed);
}
