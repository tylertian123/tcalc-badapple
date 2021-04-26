#pragma once

#include "stm32f10x.h"

/*
 * A class that represents a GPIO pin
 */
struct GPIOConfig {
	GPIOMode_TypeDef mode;
	GPIOSpeed_TypeDef speed;
};

class GPIOPin {
public:
	GPIO_TypeDef *port;
	uint16_t pin;

	GPIOPin(GPIO_TypeDef *port, uint16_t pin);
	GPIOPin();
	
	void set(const bool &val);
	bool get(void) const;

	GPIOPin& operator=(const bool&);
	
	operator bool() const;
		
	uint32_t get_RCC_perhiph() const;
	
	void init(GPIOMode_TypeDef, GPIOSpeed_TypeDef);
	void init(const GPIOConfig&);
};