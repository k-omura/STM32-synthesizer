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

    // LFO
    stm32synth_config_lfo_t initLfo;
    initLfo.amp = 64;
    initLfo.freq = 0;
    initLfo.type = STM32SYNTH_LFO_SIN;
    initLfo.para.duty = 0.5f;

    // init
    for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
    {
        if (cc == STM32SYNTH_MIDINN_DRUMCH)
        {
            _config->volume[STM32SYNTH_MIDINN_DRUMCH] = 0.5f;
            _config->pitch[STM32SYNTH_MIDINN_DRUMCH] = 0;
            _config->distortion[STM32SYNTH_MIDINN_DRUMCH] = 128;

            _config->waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_0].sin_level = 1.8f;
            _config->waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_0].tri_level = 1.8f;
            _config->waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_0].tri_peak = 0.35f;
            _config->waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_0].squ_level = 2.0f;
            _config->waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_0].squ_duty = 0.35f;
            _config->waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_1].sin_level = 0.0f;
            _config->waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_1].tri_level = 0.0f;
            _config->waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_1].tri_peak = 0.35f;
            _config->waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_1].squ_level = 1.8f;
            _config->waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_1].squ_duty = 0.15f;

            _config->adsr[STM32SYNTH_MIDINN_DRUMCH].attack_level = 3.5f;
            _config->adsr[STM32SYNTH_MIDINN_DRUMCH].attack_ms = 0;
            _config->adsr[STM32SYNTH_MIDINN_DRUMCH].decay_ms = 0;
            _config->adsr[STM32SYNTH_MIDINN_DRUMCH].release_ms = 300;

            _config->envelope[STM32SYNTH_MIDINN_DRUMCH].freq.finish_value = -(3 << 8);
            _config->envelope[STM32SYNTH_MIDINN_DRUMCH].freq.time_ms = 500;
            _config->envelope[STM32SYNTH_MIDINN_DRUMCH].volume.finish_value = -0x40;
            _config->envelope[STM32SYNTH_MIDINN_DRUMCH].volume.time_ms = 600;

            //_config->phonic[STM32SYNTH_MIDINN_DRUMCH] = STM32SYNTH_PHONIC_POLY;
            _config->phonic[STM32SYNTH_MIDINN_DRUMCH] = STM32SYNTH_PHONIC_POLY_SUSTON;
            _config->distortion[STM32SYNTH_MIDINN_DRUMCH] = 1;
        }
        else
        {
            _config->volume[cc] = 0.0f;
            _config->pitch[cc] = 0;
            _config->distortion[cc] = 128;

            _config->waveform[cc][STM32SYNTH_WAVEFORM_0].sin_level = 0.0f;
            _config->waveform[cc][STM32SYNTH_WAVEFORM_0].squ_level = 0.0f;
            _config->waveform[cc][STM32SYNTH_WAVEFORM_0].squ_duty = 0.5f;
            _config->waveform[cc][STM32SYNTH_WAVEFORM_0].tri_level = 0.0f;
            _config->waveform[cc][STM32SYNTH_WAVEFORM_0].tri_peak = 0.5f;
            _config->waveform[cc][STM32SYNTH_WAVEFORM_0].pitch = 0;
            _config->waveform[cc][STM32SYNTH_WAVEFORM_1].sin_level = 0.0f;
            _config->waveform[cc][STM32SYNTH_WAVEFORM_1].squ_level = 0.0f;
            _config->waveform[cc][STM32SYNTH_WAVEFORM_1].squ_duty = 0.5f;
            _config->waveform[cc][STM32SYNTH_WAVEFORM_1].tri_level = 0.0f;
            _config->waveform[cc][STM32SYNTH_WAVEFORM_1].tri_peak = 0.5f;
            _config->waveform[cc][STM32SYNTH_WAVEFORM_1].pitch = 0;

            _config->adsr[cc].attack_level = 0.0f;
            _config->adsr[cc].attack_ms = 0;
            _config->adsr[cc].decay_ms = 0;
            _config->adsr[cc].release_ms = 500;

#ifdef STM32SYNTH_FILTER
#ifdef STM32SYNTH_CHORDFILTER
            _config->filter[cc].q_factor = 0.8f;
            //_config->lpf[cc].cutoff_freq_nn.relative = 127 - 0x40;
			_config->filter[cc].cutoff_freq_nn.absolute = 127 << 8;
#endif
#endif

            _config->tre[cc] = initLfo;
            _config->vib[cc] = initLfo;

#ifdef STM32SYNTH_FILTER
#ifdef STM32SYNTH_CHORDFILTER
            _config->wow[cc] = initLfo;
#endif
#endif /* STM32SYNTH_FILTER */

            _config->envelope[cc].freq.finish_value = 64;
            _config->envelope[cc].freq.time_ms = 0;

            _config->phonic[cc] = STM32SYNTH_PHONIC_POLY_SUSTON;
            _config->distortion[cc] = 128;
        }
    }

    // Pan
    _config->pan.l_level = 1.0f;
    _config->pan.r_level = 1.0f;
    stm32synth_component_f32toq15fract(_config->pan.l_level, &_config->pan.l_scaleFract, &_config->pan.l_shift);
    stm32synth_component_f32toq15fract(_config->pan.r_level, &_config->pan.r_scaleFract, &_config->pan.r_shift);

#ifdef STM32SYNTH_FILTER
    // LPF
    _config->filter_master.para.q_factor = 0.8f;
    _config->filter_master.para.cutoff_freq_nn.absolute = STM32SYNTH_MAX_FREQ_NOTE;

		if(_config->filter_master.para.type == STM32SYNTH_FILTERTYPE_LSF)
		{
		    stm32synth_component_updateLSF(&_config->filter_master, 0);
		}
		else
		{
		    stm32synth_component_updateLPF(&_config->filter_master, 0);
		}
#endif /* STM32SYNTH_FILTER */

    _config->tre_master = initLfo;
    _config->vib_master = initLfo;
#ifdef STM32SYNTH_FILTER
    _config->wow_master = initLfo;
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
