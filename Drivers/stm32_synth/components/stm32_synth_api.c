/*
 * stm32_synth_api.c
 *
 *  Created on: Feb 10, 2024
 *      Author: k-omura
 */

#include <string.h>

#include "stm32_synth.h"
#include "components/stm32_synth_component.h"
#include "components/stm32_synth_midi.h"
#include "components/stm32_synth_flashconfig.h"
#include "components/stm32_synth_program.h"
#include "components/stm32_synth_multiplech.h"

static stm32synth_config_t config;
static q15_t audiobuffer_back[STM32SYNTH_NUM_SAMPLING];
static q15_t audiobuffer_presample[STM32SYNTH_PRE_SAMPLE];

#ifdef STM32SYNTH_REVERB
static q15_t audiobuffer_reverb[STM32SYNTH_REVERB_NUM][STM32SYNTH_HALF_NUM_SAMPLING];
#endif /* STM32SYNTH_REVERB */

stm32synth_res_t stm32synth_init(
	uint16_t *_dacBuff
#ifndef STM32SYNTH_SIN_LUT
	, CORDIC_HandleTypeDef *_cordicHW
#endif /* STM32SYNTH_SIN_LUT*/
	)
{
	stm32synth_res_t res = STM32SYNTH_RES_NG;

	// Load Saved Config.
#ifdef SYNTH_USE_FLASH_CONFIG
	res = stm32_synth_read_config_sector(&config);
#endif

	// Buff
	config.buff.dac = _dacBuff;
	config.buff.back = audiobuffer_back;
#ifdef STM32SYNTH_REVERB
	config.buff.reverb = audiobuffer_reverb;
#endif /* STM32SYNTH_REVERB */
	config.buff.presample = audiobuffer_presample;

	if (res == STM32SYNTH_RES_NG)
	{
		// Parameter
		for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
		{
			config.volume[cc] = 0.5f;
			config.pitch[cc] = 0;
			config.distortion[cc] = 128;
		}

		// waveform
		for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
		{
			config.waveform[cc][STM32SYNTH_WAVEFORM_0].sin_level = 0.0f;
			config.waveform[cc][STM32SYNTH_WAVEFORM_0].tri_level = 0.0f;
			config.waveform[cc][STM32SYNTH_WAVEFORM_0].tri_peak = 0.5f;
			config.waveform[cc][STM32SYNTH_WAVEFORM_0].squ_level = 0.0f;
			config.waveform[cc][STM32SYNTH_WAVEFORM_0].squ_duty = 0.5f;
		}
		config.waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_0].sin_level = 1.8f;
		config.waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_0].tri_level = 1.8f;
		config.waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_0].tri_peak = 0.35f;
		config.waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_0].squ_level = 2.0f;
		config.waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_0].squ_duty = 0.35f;
		config.waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_1].sin_level = 0.0f;
		config.waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_1].tri_level = 0.0f;
		config.waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_1].tri_peak = 0.35f;
		config.waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_1].squ_level = 1.8f;
		config.waveform[STM32SYNTH_MIDINN_DRUMCH][STM32SYNTH_WAVEFORM_1].squ_duty = 0.15f;

		// ADSR
		for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
		{
			config.adsr[cc].attack_level = 2.0f;
			config.adsr[cc].attack_ms = 100;
			config.adsr[cc].decay_ms = 150;
			config.adsr[cc].release_ms = 500;
		}
		config.adsr[STM32SYNTH_MIDINN_DRUMCH].attack_level = 3.5f;
		config.adsr[STM32SYNTH_MIDINN_DRUMCH].attack_ms = 0;
		config.adsr[STM32SYNTH_MIDINN_DRUMCH].decay_ms = 0;
		config.adsr[STM32SYNTH_MIDINN_DRUMCH].release_ms = 300;

		// Envelope
		for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
		{
			config.envelope[cc].freq.finish_value = 64;
			config.envelope[cc].freq.time_ms = 0;
		}
		config.envelope[STM32SYNTH_MIDINN_DRUMCH].freq.finish_value = -(3 << 8);
		config.envelope[STM32SYNTH_MIDINN_DRUMCH].freq.time_ms = 500;
		config.envelope[STM32SYNTH_MIDINN_DRUMCH].volume.finish_value = -0x40;
		config.envelope[STM32SYNTH_MIDINN_DRUMCH].volume.time_ms = 600;

#ifdef STM32SYNTH_FILTER
#ifdef STM32SYNTH_CHORDFILTER
		// LPF
		for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
		{
			//config.lpf[cc].cutoff_freq_nn.relative = 127 - 0x40;
			config.filter[cc].type = STM32SYNTH_FILTERTYPE_LPF;
			config.filter[cc].cutoff_freq_nn.absolute = 127 << 8;
			config.filter[cc].q_factor = 0.8f;
		}
#endif
#endif /* STM32SYNTH_FILTER */

		// LFO
		stm32synth_config_lfo_t initLfo;
		initLfo.amp = 64;
		initLfo.freq = 0;
		initLfo.type = STM32SYNTH_LFO_SIN;
		initLfo.para.duty = 0.5f;
		for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
		{
			config.tre[cc] = initLfo;
			config.vib[cc] = initLfo;
#ifdef STM32SYNTH_FILTER
#ifdef STM32SYNTH_CHORDFILTER
			config.wow[cc] = initLfo;
#endif
#endif /* STM32SYNTH_FILTER */
		}

		// Phonic
		for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
		{
			config.phonic[cc] = STM32SYNTH_PHONIC_POLY_SUSTON;
		}
		config.phonic[STM32SYNTH_MIDINN_DRUMCH] = STM32SYNTH_PHONIC_POLY_SUSTON;

		// Distortion
		for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
		{
			config.distortion[cc] = 128;
		}
		config.distortion[STM32SYNTH_MIDINN_DRUMCH] = 1;

		// Master LFO
		config.tre_master = initLfo;
		config.vib_master = initLfo;
#ifdef STM32SYNTH_FILTER
		config.wow_master = initLfo;
#endif /* STM32SYNTH_FILTER */

		// Master Filter
#ifdef STM32SYNTH_FILTER
		config.filter_master.para.type = STM32SYNTH_FILTERTYPE_LSF;
		config.filter_master.para.q_factor = 1.0f;
		config.filter_master.para.cutoff_freq_nn.absolute = (78 << 8);

		if(config.filter_master.para.type == STM32SYNTH_FILTERTYPE_LSF)
		{
			stm32synth_component_updateLSF(&config.filter_master, 0);
		}
		else
		{
			stm32synth_component_updateLPF(&config.filter_master, 0);
		}
#endif /* STM32SYNTH_FILTER */

		// Pan
		config.pan.l_level = 1.0f;
		config.pan.r_level = 1.0f;
		stm32synth_component_f32toq15fract(config.pan.l_level, &config.pan.l_scaleFract, &config.pan.l_shift);
    	stm32synth_component_f32toq15fract(config.pan.r_level, &config.pan.r_scaleFract, &config.pan.r_shift);

		// Reverb
#ifdef STM32SYNTH_REVERB
		config.reverb.level = 64.0f * 0.8f / 127.0f;
        stm32synth_component_f32toq15fract(config.reverb.level, &config.reverb.scaleFract, &config.reverb.shift);
#endif /* STM32SYNTH_REVERB */

		// Multi Channel
		config.ch0NoteMirror = 0;

		res = STM32SYNTH_RES_OK;
	}

	// Chord
	stm32synth_component_initChord();
	// stm32synth_component_testChord();

	// CORDIC
#ifndef STM32SYNTH_SIN_LUT
	stm32synth_component_initCORDIC(_cordicHW);
#endif /* STM32SYNTH_SIN_LUT */

	return res;
}

// If use input midi buff, use this hundle.
// Others don't need to use this function.
stm32synth_res_t stm32synth_hundleloop()
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	res = stm32synth_midi_buff2input();

	return res;
}

stm32synth_res_t stm32synth_dacDmaCmplt_hundle()
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	stm32synth_component_updateDacbuff(&config, STM32SYNTH_HALF_NUM_SAMPLING, STM32SYNTH_NUM_SAMPLING);
	stm32synth_component_updateBuff(&config, STM32SYNTH_UPDATE_LATTER);

	return res;
}

stm32synth_res_t stm32synth_dacDmaHalfCmplt_hundle()
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	stm32synth_component_updateDacbuff(&config, 0, STM32SYNTH_HALF_NUM_SAMPLING);
	stm32synth_component_updateBuff(&config, STM32SYNTH_UPDATE_FORMER);

	return res;
}

stm32synth_res_t stm32synth_getConfig(stm32synth_config_t *_configBufftoCopy)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	memcpy(_configBufftoCopy, &config, sizeof(stm32synth_config_t));

	return res;
}

stm32synth_res_t stm32synth_setConfig(stm32synth_config_t *_configBuff)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	memcpy(&config, _configBuff, sizeof(stm32synth_config_t));

#ifdef STM32SYNTH_FILTER
	if (config.filter_master.state == STM32SYNTH_PARA_CHANGE)
	{
		config.filter_master.state = STM32SYNTH_PARA_NOCHANGE;
		if (config.filter_master.para.cutoff_freq_nn.absolute > (127 << 8))
		{
			config.filter_master.para.cutoff_freq_nn.absolute = (127 << 8);
		}

		if(config.filter_master.para.type == STM32SYNTH_FILTERTYPE_LSF)
		{
			stm32synth_component_updateLSF(&config.filter_master, 0);
		}
		else
		{
			stm32synth_component_updateLPF(&config.filter_master, 0);
		}
	}
#endif /* STM32SYNTH_FILTER */

	return res;
}

stm32synth_res_t stm32synth_program(uint8_t _ch, uint8_t _program)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

    res = stm32synth_program_set(&config, _ch, _program);

	return res;
}

stm32synth_res_t stm32synth_multich(stm32synth_multich_t _preset)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	switch (_preset)
	{
	case STM32SYNTH_MULTICH_NONE:
	default:
		res = stm32synth_multich_clear(&config);
		break;
	case STM32SYNTH_MULTICH_ALLCHON:
		res = stm32synth_multich_allchon(&config);
		break;
	}

	return res;
}

stm32synth_res_t stm32synth_inputMIDI(uint8_t *_rcvdMidi)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	res = stm32synth_component_inputmidi(&config, _rcvdMidi);

	return res;
}

stm32synth_res_t stm32synth_midi_writebuff(uint8_t *_midi)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	res = stm32synth_component_midi_writebuff(_midi);

	return res;
}

stm32synth_res_t stm32synth_midi_readbuff(uint8_t *_midi)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	res = stm32synth_component_midi_readbuff(_midi);

	return res;
}

stm32synth_res_t stm32synth_midi_buff2input()
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	res = stm32synth_component_midi_buff2input(&config);

	return res;
}
