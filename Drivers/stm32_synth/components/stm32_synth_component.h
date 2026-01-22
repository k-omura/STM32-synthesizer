/*
 * stm32_synth_component.h
 *
 *  Created on: Feb 11, 2024
 *      Author: k-omura
 */

#ifndef STM32_SYNTH_COMPONENTS_STM32_SYNTH_COMPONENT_H_
#define STM32_SYNTH_COMPONENTS_STM32_SYNTH_COMPONENT_H_

#include "stm32_synth.h"

// define

#define STM32SYNTH_NOW_PREV_CNT 2
//- user start
//- user end
// define end

// enum

typedef enum
{
    STM32SYNTH_ADSRSTATE_WAIT = 0,
    STM32SYNTH_ADSRSTATE_ATTACK,
    STM32SYNTH_ADSRSTATE_DECAY,
    STM32SYNTH_ADSRSTATE_SUSTAIN,
    STM32SYNTH_ADSRSTATE_RELEASE,
    STM32SYNTH_ADSRSTATE_DONE,
} stm32synth_chord_adsrstate_t;

typedef enum
{
    STM32SYNTH_UPDATE_FORMER = 0,
    STM32SYNTH_UPDATE_LATTER,
} stm32synth_update_half_t;
//- user start
//- user end
// enum end

// struct
typedef struct
{
    q15_t rad[STM32SYNTH_WAVEFORM_NUM_PERCHORD];
    q15_t presample[STM32SYNTH_PRE_SAMPLE];

    float32_t velocity;
    uint16_t noteNum[STM32SYNTH_NOW_PREV_CNT];
    uint8_t channel;

    struct Adsr
    {
        int32_t count;
        stm32synth_chord_adsrstate_t state;
    } adsr;

    struct Envelope
    {
        uint32_t volume;
        uint32_t freq;
        uint32_t cutoff;
    } envelope;

    stm32synth_config_filter_t lpf, hpf;

    // user add here start
    // user add here end
} stm32synth_chord_t;

//- user start
//- user end
// struct end

// function
stm32synth_res_t stm32synth_component_updateDacbuff(stm32synth_config_t *_config, uint32_t _start, uint32_t _end);

stm32synth_res_t stm32synth_component_initChord();
stm32synth_res_t stm32synth_component_testChord(stm32synth_config_t *_config);
stm32synth_res_t stm32synth_component_noteonChord(stm32synth_config_t *_config, stm32synth_chord_t *_inpurChord);
stm32synth_res_t stm32synth_component_noteoffChord(stm32synth_config_t *_config, stm32synth_chord_t *_inpurChord);
stm32synth_res_t stm32synth_component_disableChord(stm32synth_chord_t *_configChord);
stm32synth_res_t stm32synth_component_noteoffAll();
stm32synth_res_t stm32synth_component_disableChordAll();

stm32synth_res_t stm32synth_component_updateBuff(stm32synth_config_t *_config, stm32synth_update_half_t _half);
stm32synth_res_t stm32synth_component_updateLPF(stm32synth_config_filter_t *_configFilter, int32_t _wow_val);
stm32synth_res_t stm32synth_component_updateHPF(stm32synth_config_filter_t *_configFilter, int32_t _wow_val);
stm32synth_res_t stm32synth_component_updateLSF(stm32synth_config_filter_t *_configFilter, int32_t _wow_val);

stm32synth_res_t stm32synth_component_lfo(stm32synth_config_lfo_t *_configLfo);

stm32synth_res_t stm32synth_component_f32toq15fract(float32_t _scale, q15_t *_scaleFract, int8_t *_shift);
stm32synth_res_t stm32synth_component_q15toq15fract(q15_t _scale, q15_t *_scaleFract, int8_t *_shift);

#ifndef STM32SYNTH_SIN_LUT
stm32synth_res_t stm32synth_component_initCORDIC(CORDIC_HandleTypeDef *_cordicHW);
#endif /* STM32SYNTH_SIN_LUT */

#ifdef STM32SYNTH_DRUM_TESTMODE
stm32synth_res_t stm32synth_drum_change_to_test(uint8_t _midiCC, uint8_t _val);
stm32synth_res_t stm32synth_drum_init();
#endif /* STM32SYNTH_DRUM_TESTMODE */

//- user start
//- user end
// function end

#endif /* STM32_SYNTH_COMPONENTS_STM32_SYNTH_COMPONENT_H_ */
