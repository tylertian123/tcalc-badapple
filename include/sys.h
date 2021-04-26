#pragma once

#include "stm32f10x.h"

// The clock source used, as returned by RCC_GetSYSCLKSource(), according to its documentation

#define SYSCLK_Source_HSI 0x00
#define SYSCLK_Source_HSE 0x04
#define SYSCLK_Source_PLL 0x08

namespace sys {
    void initRCC();
    void initNVIC();
}

