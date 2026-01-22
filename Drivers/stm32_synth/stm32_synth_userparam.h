/*
 * stm32_synth_userparam.h
 *
 *  Created on: Nov 22, 2024
 *      Author: k-omura
 */

#ifndef STM32_SYNTH_USERPARAM_H_
#define STM32_SYNTH_USERPARAM_H_

// Build option. Comment out to enable.
#define STM32SYNTH_I2S              // if I2S(or I2S like stereo buffer) enable here
#define STM32SYNTH_FILTER           // enable here to apply low pass filter to whole audio
#define STM32SYNTH_CHORDFILTER      // enable here to apply low pass filter for chord
#define STM32SYNTH_FILT_CMSIS       // if STM32 has FMAC HW disable here (not implemented yet)
#define STM32SYNTH_SIN_LUT          // if STM32 has CORDIC HW disable here
#define STM32SYNTH_REVERB           // enable here to apply reverb
// #define SYNTH_USE_FLASH_CONFIG   // If you use flash to write/read config, enable below.
#define STM32SYNTH_DRUM_TESTMODE    // enable here to test drum sound

// Parametor
#define STM32SYNTH_TUNING 442.0f    // A4 freq. (Hz)

// Change to suit your environment
#ifdef STM32SYNTH_I2S
#define STM32SYNTH_AMP_MAX 0xffff    // 16bit dac
#define STM32SYNTH_AMP_MAX_BIT 15    // 16bit dac
#define STM32SYNTH_SAMPLE_FREQ 32003 // Sampling freq. (Hz)
#else
#define STM32SYNTH_AMP_MAX 0xfff     // 12bit dac
#define STM32SYNTH_AMP_MAX_BIT 11    // 12bit dac
#define STM32SYNTH_SAMPLE_FREQ 40000 // Sampling freq. (Hz)
#endif /* STM32SYNTH_I2S */

#endif /* STM32_SYNTH_USERPARAM_H_ */
