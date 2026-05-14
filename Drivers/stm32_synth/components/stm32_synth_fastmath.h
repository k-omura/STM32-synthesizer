/**
 * @file stm32_synth_fastmath.h
 * @brief High-speed math approximation functions for STM32 synthesizer
 * @author Generated for STM32-synthesizer project
 *
 * This header provides fast approximations for pow(2,x) and pow(10,x)
 * optimized for real-time audio synthesis on embedded systems.
 */

// Enable fast math approximations (can be disabled for testing)
// #define STM32SYNTH_FASTMATH_ENABLE

#ifndef STM32_SYNTH_FASTMATH_H
#define STM32_SYNTH_FASTMATH_H

#include "stm32_synth.h"

#ifdef STM32SYNTH_FASTMATH_ENABLE

float32_t fast_pow2_bit(float32_t x);
float32_t fast_pow10_poly(float32_t x);

#endif /* STM32SYNTH_FASTMATH_ENABLE */

#endif /* STM32_SYNTH_FASTMATH_H */