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
#ifdef STM32SYNTH_I2S
static q15_t audiobuffer_back[STM32SYNTH_AUDIO_STEREO_NUM][STM32SYNTH_NUM_SAMPLING];
#else
static q15_t audiobuffer_back[STM32SYNTH_NUM_SAMPLING];
#endif

#ifdef STM32SYNTH_REVERB
static q15_t audiobuffer_reverb[STM32SYNTH_REVERB_NUM][STM32SYNTH_HALF_NUM_SAMPLING];
#endif /* STM32SYNTH_REVERB */

/**
 * @brief Init STM32 Synthesizer API
 * @param _dacBuff: Pointer to DAC buffer (must be at least STM32SYNTH_HALF_NUM_SAMPLING in size)
 * @param _cordicHW: Pointer to CORDIC hardware handle (required if STM32SYNTH_SIN_CORDIC is defined)
 * @return STM32SYNTH_RES_OK on success, STM32SYNTH_RES_NG on failure
 */
stm32synth_res_t stm32synth_init(
	uint16_t *_dacBuff
#ifdef STM32SYNTH_SIN_CORDIC
	,
	CORDIC_HandleTypeDef *_cordicHW
#endif /* STM32SYNTH_SIN_CORDIC*/
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

	if (res == STM32SYNTH_RES_NG)
	{
		// Initialize synthesizer configuration
		res = stm32synth_component_initSynthConfig(&config);
	}

	// Chord
	stm32synth_component_initChord();
	// stm32synth_component_testChord();

	// CORDIC
#ifdef STM32SYNTH_SIN_CORDIC
	stm32synth_component_initCORDIC(_cordicHW);
#endif /* STM32SYNTH_SIN_CORDIC */

	return res;
}

/**
 * @brief Handle main loop for STM32 Synthesizer API (process MIDI input buffer and update synthesizer state)
 * @return STM32SYNTH_RES_OK on success, STM32SYNTH_RES_NG on failure
 */
stm32synth_res_t stm32synth_hundleloop()
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	res = stm32synth_midi_buff2input();

	return res;
}

/**
 * @brief Handle DAC DMA complete interrupt (update DAC buffer with new audio data for the second half of the buffer)
 * @return STM32SYNTH_RES_OK on success, STM32SYNTH_RES_NG on failure
 */
stm32synth_res_t stm32synth_dacDmaCmplt_hundle()
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	stm32synth_component_updateDacbuff(&config, STM32SYNTH_HALF_NUM_SAMPLING, STM32SYNTH_NUM_SAMPLING);
	stm32synth_component_updateBuff(&config, STM32SYNTH_UPDATE_LATTER);

	return res;
}

/**
 * @brief Handle DAC DMA half complete interrupt (update DAC buffer with new audio data for the first half of the buffer)
 * @return STM32SYNTH_RES_OK on success, STM32SYNTH_RES_NG on failure
 */
stm32synth_res_t stm32synth_dacDmaHalfCmplt_hundle()
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	stm32synth_component_updateDacbuff(&config, 0, STM32SYNTH_HALF_NUM_SAMPLING);
	stm32synth_component_updateBuff(&config, STM32SYNTH_UPDATE_FORMER);

	return res;
}

/**
 * @brief Get current synthesizer configuration (copy internal config to provided buffer)
 * @param _configBufftoCopy: Pointer to buffer where current config will be copied (must be at least sizeof(stm32synth_config_t) in size)
 * @return STM32SYNTH_RES_OK on success, STM32SYNTH_RES_NG on failure
 */
stm32synth_res_t stm32synth_getConfig(stm32synth_config_t *_configBufftoCopy)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	memcpy(_configBufftoCopy, &config, sizeof(stm32synth_config_t));

	return res;
}

/**
 * @brief Set new synthesizer configuration (copy provided config to internal config and update any necessary components)
 * @param _configBuff: Pointer to new config to set (must be at least sizeof(stm32synth_config_t) in size)
 * @return STM32SYNTH_RES_OK on success, STM32SYNTH_RES_NG on failure
 */
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

		if (config.filter_master.para.type == STM32SYNTH_FILTERTYPE_LSF)
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

/**
 * @brief Set MIDI program for a specific channel (update internal config and any necessary components for the given channel and program)
 * @param _ch: MIDI channel to set program for (0-15)
 * @param _program: MIDI program number to set (0-127)
 * @return STM32SYNTH_RES_OK on success, STM32SYNTH_RES_NG on failure
 */
stm32synth_res_t stm32synth_program(uint8_t _ch, uint8_t _program)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	res = stm32synth_program_set(&config, _ch, _program);

	return res;
}

/**
 * @brief Set multi-channel preset (update internal config and any necessary components based on the given preset)
 * @param _preset: Multi-channel preset to set (STM32SYNTH_MULTICH_NONE, STM32SYNTH_MULTICH_ALLCHON, etc.)
 * @return STM32SYNTH_RES_OK on success, STM32SYNTH_RES_NG on failure
 */
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

/**
 * @brief Handle MIDI input (process incoming MIDI message and update synthesizer state accordingly)
 * @param _rcvdMidi: Pointer to received MIDI message (must be at least 3 bytes in size for a complete MIDI message)
 * @return STM32SYNTH_RES_OK on success, STM32SYNTH_RES_NG on failure
 */
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
