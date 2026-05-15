/**
 * @file stm32_synth_fastmath.h
 * @brief High-speed math approximation functions for STM32 synthesizer
 * @author Generated for STM32-synthesizer project
 *
 * This header provides fast approximations for pow(2,x) and pow(10,x)
 * optimized for real-time audio synthesis on embedded systems.
 */

// Enable fast math approximations

#ifndef STM32_SYNTH_FASTMATH_H
#define STM32_SYNTH_FASTMATH_H

#include "stm32_synth.h"

float32_t fast_exp2f(float32_t x);
float32_t fast_exp10f(float32_t x);

#endif /* STM32_SYNTH_FASTMATH_H */