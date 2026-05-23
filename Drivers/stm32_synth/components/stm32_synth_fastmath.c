/**
 * @file stm32_synth_fastmath.c
 * @brief Implementation of high-speed math approximations for STM32 synthesizer
 */

#include "stm32_synth_fastmath.h"

// support functions
static inline float32_t stm32synth_fast_floorf(float32_t x);

/**
 * @brief Fast approximation of powf(2, x)
 * @param x Exponent value
 * @return Approximation of 2^x
 */
float32_t stm32synth_fast_exp2f(float32_t _x)
{
    const float32_t c1 = 0.6931471806f;   // ln(2)
    const float32_t c2 = 0.2402265070f;   // ln(2)^2 / 2
    const float32_t c3 = 0.5550341547e-1; // ln(2)^3 / 6

    // Clip out-of-range values
    if (_x > 127.0f)
    {
        return 10000.0f;
    }

    if (_x < -126.0f)
    {
        return 0.0f;
    }

    // Split into integer and fractional parts
    float32_t i = stm32synth_fast_floorf(_x);
    float32_t f = _x - i;

    // Approximate the fractional exponent with a polynomial
    float32_t poly = c3 * f;
    poly = (poly + c2) * f;
    poly = (poly + c1) * f;
    poly = poly + 1.0f; // 2^f ≈ 1.0 + c1*f + c2*f^2 + c3*f^3

    // Combine integer part into floating-point exponent bits
    // Using IEEE 754 single-precision layout
    union
    {
        float32_t f;
        uint32_t i;
    } result;

    result.i = (uint32_t)((i + 127.0f) * (1 << 23));
    result.f *= poly;

    return result.f;
}

/**
 * @brief Fast approximation of powf(10, x)
 * @param _x Exponent value
 * @return Approximation of 10^x
 */
float32_t stm32synth_fast_exp10f(float32_t _x)
{
    const float32_t LOG2_10 = 3.3219280948873626f; // log2(10)
    return stm32synth_fast_exp2f(_x * LOG2_10);
}

/**
 * @brief Fast approximation of roundf
 * @param _x Value to round
 * @return Rounded value
 */
float32_t stm32synth_fast_roundf(float32_t _x)
{
    return (_x >= 0.0f) ? stm32synth_fast_floorf(_x + 0.5f) : stm32synth_fast_floorf(_x - 0.5f);
}

/**
 * @brief Fast approximation of floorf for positive and negative values
 *
 * @param _x Value to floor
 * @return float32_t
 */
static inline float32_t stm32synth_fast_floorf(float32_t _x)
{
    int32_t i = (int32_t)_x;
    // If _x is negative and not an integer, we need to subtract 1 to get the correct floor value
    return (float32_t)(i - (_x < i));
}