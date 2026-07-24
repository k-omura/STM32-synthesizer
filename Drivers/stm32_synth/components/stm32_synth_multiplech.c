/*
 * stm32_synth_multiplech.c
 *
 *  Created on: May 6, 2024
 *      Author: k-omura
 */

#include "stm32_synth.h"
#include "components/stm32_synth_component.h"
#include "components/stm32_synth_midi.h"
#include "components/stm32_synth_multiplech.h"

stm32synth_res_t stm32synth_multich_clear(stm32synth_config_t *_config)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    // Initialize default configuration
    res = stm32synth_component_initAllSynthConfig(_config);

#ifdef STM32SYNTH_FILTER
    // Master filter: override cutoff frequency for multich_clear
    _config->filter_master.para.cutoff_freq_nn.absolute = STM32SYNTH_MAX_FREQ_NOTE;

    if (_config->filter_master.para.type == STM32SYNTH_FILTERTYPE_LSF)
    {
        stm32synth_component_updateLSF(&_config->filter_master, 0);
    }
    else
    {
        stm32synth_component_updateLPF(&_config->filter_master, 0);
    }
#endif /* STM32SYNTH_FILTER */

    // Multi Channel
    _config->ch0NoteMirror = 0b0000000000000001;

    return res;
}

stm32synth_res_t stm32synth_multich_allchon(stm32synth_config_t *_config)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    // clear
    stm32synth_multich_clear(_config);

    _config->ch0NoteMirror = 0b0000000000000001;

    for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
    {
        if (cc != STM32SYNTH_MIDINN_DRUMCH)
        {
            // Vol&Pitch
            _config->volume[cc] = 1.0f;
            _config->pitch[cc] = 0;

            // waveform
            //_config->waveform[cc].sin_level = 1.0f;
            _config->waveform[cc][STM32SYNTH_WAVEFORM_0].tri_level = 0.8f;
            _config->waveform[cc][STM32SYNTH_WAVEFORM_0].tri_peak = 0.1f;
            //_config->waveform[cc].squ_level = 0.1f;
            //_config->waveform[cc].squ_duty = 0.5f;

            // ADSR
            _config->adsr[cc].attack_level = 1.0f;
            _config->adsr[cc].attack_ms = 50;
            _config->adsr[cc].decay_ms = 20;
            _config->adsr[cc].release_ms = 100;

            _config->phonic[cc] = STM32SYNTH_PHONIC_POLY_SUSTON;
        }
    }

    return res;
}
