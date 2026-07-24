/*
 * stm32_synth_component.c
 *
 *  Created on: Feb 11, 2024
 *      Author: k-omura
 */

#include "components/stm32_synth_component.h"
#include "components/stm32_synth_fastmath.h"
#include "components/stm32_synth_midi.h"

/**
 * @def STM32SYNTH_FILT_SHIFT
 * @brief Filter coefficient shift factor for Q15 fixed-point arithmetic
 *
 * Used to scale filter coefficients for the biquad cascade filter implementation.
 * A value of 3 allows for efficient bit-shifting operations.
 */
#define STM32SYNTH_FILT_SHIFT 3

/**
 * @brief Update DAC buffer with audio samples from the back buffer
 *
 * This function converts audio samples from the back buffer to the DAC output buffer.
 * It supports both mono and I2S stereo configurations. In I2S mode, it copies the
 * interleaved left/right back buffers directly into the DAC output buffer.
 *
 * @param[in] _config Pointer to the synthesizer configuration structure
 * @param[in] _start Starting sample index for the update
 * @param[in] _end Ending sample index for the update
 *
 * @return stm32synth_res_t Result code indicating success or failure
 *
 * @note The function processes samples in groups of 4 for improved efficiency
 */
stm32synth_res_t stm32synth_component_updateDacbuff(stm32synth_config_t *_config, uint32_t _start, uint32_t _end)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

#ifndef STM32SYNTH_I2S
	// Pointer
	q15_t *p_in = (&_config->buff.back[_start]);
	uint32_t out_counter = _start;
	uint16_t *p_out = (uint16_t *)(&_config->buff.dac[out_counter]);
#else
	uint32_t out_counter = 2 * _start;
	q15_t *p_out = (q15_t *)(&_config->buff.dac[out_counter]);
	q15_t *p_inL = &(_config->buff.back[0][_start]);
	q15_t *p_inR = &(_config->buff.back[1][_start]);

#endif /* STM32SYNTH_I2S */

	for (uint32_t t = 0; t < STM32SYNTH_HALF_NUM_SAMPLING_BY_4; t++)
	{
#ifndef STM32SYNTH_I2S
		*p_out++ = (uint16_t)(*p_in++);
		*p_out++ = (uint16_t)(*p_in++);
		*p_out++ = (uint16_t)(*p_in++);
		*p_out++ = (uint16_t)(*p_in++);
#else
		*p_out++ = *p_inL++;
		*p_out++ = *p_inR++;

		*p_out++ = *p_inL++;
		*p_out++ = *p_inR++;

		*p_out++ = *p_inL++;
		*p_out++ = *p_inR++;

		*p_out++ = *p_inL++;
		*p_out++ = *p_inR++;
#endif /* STM32SYNTH_I2S */
	}

	return res;
}

#ifdef STM32SYNTH_FILT_CMSIS

/**
 * @brief Update low-pass filter (LPF) coefficients based on cutoff frequency
 *
 * Calculates and updates the biquad filter coefficients for a low-pass filter.
 * The cutoff frequency is specified in note numbers and can be modulated by a wow value.
 * Uses Q15 fixed-point arithmetic for efficient embedded processing.
 *
 * @param[in,out] _configFilter Pointer to the filter configuration structure
 * @param[in] _wow_val Modulation value in semitones (applied to cutoff frequency)
 *
 * @return stm32synth_res_t Result code indicating success or failure
 *
 * @note Frequency is clamped to STM32SYNTH_MAX_FREQ to prevent aliasing
 * @see stm32synth_component_updateHPF
 * @see stm32synth_component_updateLSF
 */
stm32synth_res_t stm32synth_component_updateLPF(stm32synth_config_filter_t *_configFilter, int32_t _wow_val)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	float32_t freq = STM32SYNTH_TUNING * stm32synth_fast_exp2f(((float32_t)((int32_t)_configFilter->para.cutoff_freq_nn.absolute + _wow_val) / 3072.0f) - 5.75f);
	if (freq > STM32SYNTH_MAX_FREQ)
	{
		freq = (float32_t)STM32SYNTH_MAX_FREQ;
	}

	float32_t omega_c = freq * (360.0f / (float32_t)STM32SYNTH_SAMPLE_FREQ);
	float32_t sin_omega_c, cos_omega_c;
	arm_sin_cos_f32(omega_c, &sin_omega_c, &cos_omega_c);
	float32_t alfa = sin_omega_c / (2.0f * _configFilter->para.q_factor);
	float32_t a0 = 1.0f + alfa;

	_configFilter->pCoeffs[2] = (int16_t)(((1.0f - cos_omega_c) / a0) * (float32_t)(STM32SYNTH_Q15_MAX >> STM32SYNTH_FILT_SHIFT));	 // b1
	_configFilter->pCoeffs[1] = 0;																									 // 0
	_configFilter->pCoeffs[0] = _configFilter->pCoeffs[2] >> 1;																		 // b0
	_configFilter->pCoeffs[3] = _configFilter->pCoeffs[0];																			 // b2
	_configFilter->pCoeffs[4] = -(int16_t)(((-2.0f * cos_omega_c) / a0) * (float32_t)(STM32SYNTH_Q15_MAX >> STM32SYNTH_FILT_SHIFT)); // a1
	_configFilter->pCoeffs[5] = -(int16_t)(((1.0f - alfa) / a0) * (float32_t)(STM32SYNTH_Q15_MAX >> STM32SYNTH_FILT_SHIFT));		 // a2

	// Keep pState continuous across blocks; only refresh coeff/pointer metadata.
	_configFilter->instance.numStages = 1;
	_configFilter->instance.pCoeffs = _configFilter->pCoeffs;
	_configFilter->instance.pState = _configFilter->pState;
	_configFilter->instance.postShift = STM32SYNTH_FILT_SHIFT;
	_configFilter->state = STM32SYNTH_PARA_NOCHANGE;

	return res;
}

/**
 * @brief Update high-pass filter (HPF) coefficients based on cutoff frequency
 *
 * Calculates and updates the biquad filter coefficients for a high-pass filter.
 * The cutoff frequency is specified in note numbers and can be modulated by a wow value.
 * Uses Q15 fixed-point arithmetic for efficient embedded processing.
 *
 * @param[in,out] _configFilter Pointer to the filter configuration structure
 * @param[in] _wow_val Modulation value in semitones (applied to cutoff frequency)
 *
 * @return stm32synth_res_t Result code indicating success or failure
 *
 * @note Frequency is clamped to STM32SYNTH_MAX_FREQ to prevent aliasing
 * @see stm32synth_component_updateLPF
 * @see stm32synth_component_updateLSF
 */
stm32synth_res_t stm32synth_component_updateHPF(stm32synth_config_filter_t *_configFilter, int32_t _wow_val)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	float32_t freq = STM32SYNTH_TUNING * stm32synth_fast_exp2f(((float32_t)((int32_t)_configFilter->para.cutoff_freq_nn.absolute + _wow_val) / 3072.0f) - 5.75f);
	if (freq > STM32SYNTH_MAX_FREQ)
	{
		freq = (float32_t)STM32SYNTH_MAX_FREQ;
	}

	float32_t omega_c = freq * (360.0f / (float32_t)STM32SYNTH_SAMPLE_FREQ);
	float32_t sin_omega_c, cos_omega_c;
	arm_sin_cos_f32(omega_c, &sin_omega_c, &cos_omega_c);
	float32_t alfa = sin_omega_c / (2.0f * _configFilter->para.q_factor);
	float32_t a0 = 1.0f + alfa;

	_configFilter->pCoeffs[2] = (int16_t)((-(1.0f + cos_omega_c) / a0) * (float32_t)(STM32SYNTH_Q15_MAX >> STM32SYNTH_FILT_SHIFT));	 // b1
	_configFilter->pCoeffs[1] = 0;																									 // 0
	_configFilter->pCoeffs[0] = -(_configFilter->pCoeffs[2] >> 1);																	 // b0
	_configFilter->pCoeffs[3] = _configFilter->pCoeffs[0];																			 // b2
	_configFilter->pCoeffs[4] = -(int16_t)(((-2.0f * cos_omega_c) / a0) * (float32_t)(STM32SYNTH_Q15_MAX >> STM32SYNTH_FILT_SHIFT)); // a1
	_configFilter->pCoeffs[5] = -(int16_t)(((1.0f - alfa) / a0) * (float32_t)(STM32SYNTH_Q15_MAX >> STM32SYNTH_FILT_SHIFT));		 // a2

	// Keep pState continuous across blocks; only refresh coeff/pointer metadata.
	_configFilter->instance.numStages = 1;
	_configFilter->instance.pCoeffs = _configFilter->pCoeffs;
	_configFilter->instance.pState = _configFilter->pState;
	_configFilter->instance.postShift = STM32SYNTH_FILT_SHIFT;
	_configFilter->state = STM32SYNTH_PARA_NOCHANGE;

	return res;
}

/**
 * @brief Update low-shelf filter (LSF) coefficients based on frequency and gain
 *
 * Calculates and updates the biquad filter coefficients for a low-shelf filter.
 * The shelf frequency is specified in note numbers and can be modulated by a wow value.
 * The Q factor parameter controls the gain boost/cut. Uses Q15 fixed-point arithmetic
 * for efficient embedded processing.
 *
 * @param[in,out] _configFilter Pointer to the filter configuration structure
 * @param[in] _wow_val Modulation value in semitones (applied to shelf frequency)
 *
 * @return stm32synth_res_t Result code indicating success or failure
 *
 * @note Frequency is clamped to STM32SYNTH_MAX_FREQ to prevent aliasing
 * @note The Q factor is interpreted as gain in dB (6dB per unit)
 * @see stm32synth_component_updateLPF
 * @see stm32synth_component_updateHPF
 */
stm32synth_res_t stm32synth_component_updateLSF(stm32synth_config_filter_t *_configFilter, int32_t _wow_val)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	float32_t freq = STM32SYNTH_TUNING * stm32synth_fast_exp2f(((float32_t)((int32_t)_configFilter->para.cutoff_freq_nn.absolute + _wow_val) / 3072.0f) - 5.75f);
	if (freq > STM32SYNTH_MAX_FREQ)
	{
		freq = (float32_t)STM32SYNTH_MAX_FREQ;
	}

	float32_t omega_c = freq * (360.0f / (float32_t)STM32SYNTH_SAMPLE_FREQ);
	float32_t sin_omega_c, cos_omega_c;
	arm_sin_cos_f32(omega_c, &sin_omega_c, &cos_omega_c);
	float32_t alfa = sin_omega_c * (0.70710678118654752440084436f); // 0.70710678118654752440084436f is 1/(2 * Q), Q=1/sqrt(2)
	float32_t shelfA = stm32synth_fast_exp10f((_configFilter->para.q_factor * (6.0f / 40.0f)));
	float32_t beta;
	arm_sqrt_f32(shelfA, &beta);
	beta = 2.0f * beta * alfa;

	float32_t shelfAp1 = shelfA + 1.0f;
	float32_t shelfAm1 = shelfA - 1.0f;
	float32_t shelfAp1Cos = shelfAp1 * cos_omega_c;
	float32_t shelfAm1Cos = shelfAm1 * cos_omega_c;
	float32_t a0 = shelfAp1 + shelfAm1 * cos_omega_c + beta;

	_configFilter->pCoeffs[0] = (int16_t)(((shelfA * (shelfAp1 - shelfAm1Cos + beta)) / a0) * (float32_t)(STM32SYNTH_Q15_MAX >> STM32SYNTH_FILT_SHIFT)); // b0
	_configFilter->pCoeffs[1] = 0;																														 // 0
	_configFilter->pCoeffs[2] = (int16_t)(((2.0f * shelfA * (shelfAm1 - shelfAp1Cos)) / a0) * (float32_t)(STM32SYNTH_Q15_MAX >> STM32SYNTH_FILT_SHIFT)); // b1
	_configFilter->pCoeffs[3] = (int16_t)(((shelfA * (shelfAp1 - shelfAm1Cos - beta)) / a0) * (float32_t)(STM32SYNTH_Q15_MAX >> STM32SYNTH_FILT_SHIFT)); // b2
	_configFilter->pCoeffs[4] = (int16_t)(((2.0f * (shelfAm1 + shelfAp1Cos)) / a0) * (float32_t)(STM32SYNTH_Q15_MAX >> STM32SYNTH_FILT_SHIFT));			 // a1
	_configFilter->pCoeffs[5] = -(int16_t)(((shelfAp1 + shelfAm1Cos - beta) / a0) * (float32_t)(STM32SYNTH_Q15_MAX >> STM32SYNTH_FILT_SHIFT));			 // a2

	// Keep pState continuous across blocks; only refresh coeff/pointer metadata.
	_configFilter->instance.numStages = 1;
	_configFilter->instance.pCoeffs = _configFilter->pCoeffs;
	_configFilter->instance.pState = _configFilter->pState;
	_configFilter->instance.postShift = STM32SYNTH_FILT_SHIFT;
	_configFilter->state = STM32SYNTH_PARA_NOCHANGE;

	return res;
}
#else
#endif /* STM32SYNTH_FILT_CMSIS */

/**
 * @brief Convert a float32_t scale factor to a Q15 fractional scale and shift for efficient multiplication
 *
 * Decomposes a floating-point scale factor into a Q15 fractional component and a shift value.
 * This allows efficient multiplication using ARM's arm_scale_q15() function without loss of precision.
 * The scale factor is represented as: scale = scaleFract * 2^shift
 *
 * @param[in] _scale Input scale factor as float32_t
 * @param[out] _scaleFract Pointer to store the Q15 fractional part (range: -32768 to 32767)
 * @param[out] _shift Pointer to store the bit shift value (positive for scaling up)
 *
 * @return stm32synth_res_t Result code (STM32SYNTH_RES_OK on success)
 *
 * @see arm_scale_q15
 */
stm32synth_res_t stm32synth_component_f32toq15fract(float32_t _scale, q15_t *_scaleFract, int8_t *_shift)
{

	uint32_t bits;
	memcpy(&bits, &_scale, sizeof(bits));
	int32_t exp = (int32_t)((bits >> 23) & 0xFFu) - 127;

	int8_t shift;
	float32_t shifted;

	if (exp < 0)
	{
		// _scale < 1.0f: 2^0 = 1 already satisfies shifted >= _scale
		shift = 0;
		shifted = 1.0f;
	}
	else if ((bits & 0x7FFFFFu) == 0u)
	{
		// _scale is exactly 2^exp (mantissa == 0): shifted == _scale
		shift = (int8_t)exp;
		shifted = _scale;
	}
	else
	{
		// 2^exp < _scale < 2^(exp+1): need the next power of 2
		shift = (int8_t)(exp + 1);
		shifted = (float32_t)(1u << (uint32_t)(exp + 1));
	}

	*_scaleFract = (q15_t)((_scale / shifted) * (float32_t)STM32SYNTH_Q15_MAX);
	*_shift = shift;

	return STM32SYNTH_RES_OK;
}

/**
 * @brief Initialize default synthesizer configuration
 *
 * Sets up all default parameter values for the synthesizer including volume, pitch,
 * waveforms, ADSR envelopes, filters, LFOs, and other settings.
 *
 * @param[out] _config Pointer to the synthesizer configuration structure to initialize
 *
 * @return stm32synth_res_t Result code (STM32SYNTH_RES_OK on success)
 */
stm32synth_res_t stm32synth_component_initAllSynthConfig(stm32synth_config_t *_config)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	// Parameter
	for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
	{
		_config->volume[cc] = 0.5f;
		_config->pitch[cc] = 0;
		_config->distortion[cc] = 128;
	}

	// waveform
	for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
	{
		_config->waveform[cc][STM32SYNTH_WAVEFORM_0].sin_level = 0.0f;
		_config->waveform[cc][STM32SYNTH_WAVEFORM_0].tri_level = 0.0f;
		_config->waveform[cc][STM32SYNTH_WAVEFORM_0].tri_peak = 0.5f;
		_config->waveform[cc][STM32SYNTH_WAVEFORM_0].squ_level = 0.0f;
		_config->waveform[cc][STM32SYNTH_WAVEFORM_0].squ_duty = 0.5f;
	}

	// ADSR
	for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
	{
		_config->adsr[cc].attack_level = 2.0f;
		_config->adsr[cc].attack_ms = 100;
		_config->adsr[cc].decay_ms = 150;
		_config->adsr[cc].release_ms = 500;
	}

	// Envelope
	for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
	{
		_config->envelope[cc].freq.finish_value = 64;
		_config->envelope[cc].freq.time_ms = 0;
	}

#ifdef STM32SYNTH_FILTER
#ifdef STM32SYNTH_CHORDFILTER
	// LPF
	for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
	{
		_config->filter[cc].type = STM32SYNTH_FILTERTYPE_LPF;
		_config->filter[cc].cutoff_freq_nn.absolute = 127 << 8;
		_config->filter[cc].q_factor = 0.8f;
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
		_config->tre[cc] = initLfo;
		_config->vib[cc] = initLfo;
#ifdef STM32SYNTH_FILTER
#ifdef STM32SYNTH_CHORDFILTER
		_config->wow[cc] = initLfo;
#endif
#endif /* STM32SYNTH_FILTER */
	}

	// Phonic
	for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
	{
		_config->phonic[cc] = STM32SYNTH_PHONIC_POLY_SUSTON;
	}

	// Distortion
	for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
	{
		_config->distortion[cc] = 128;
	}

	// Drum channel has dedicated defaults.
	stm32synth_component_initDrum(_config);

	// Master LFO
	_config->tre_master = initLfo;
	_config->vib_master = initLfo;
#ifdef STM32SYNTH_FILTER
	_config->wow_master = initLfo;
#endif /* STM32SYNTH_FILTER */

	// Master Filter
#ifdef STM32SYNTH_FILTER
	_config->filter_master.para.type = STM32SYNTH_FILTERTYPE_LSF;
	_config->filter_master.para.q_factor = 1.0f;
	_config->filter_master.para.cutoff_freq_nn.absolute = (127 << 8); // Disable filter by default

	if (_config->filter_master.para.type == STM32SYNTH_FILTERTYPE_LSF)
	{
		stm32synth_component_updateLSF(&_config->filter_master, 0);
	}
	else
	{
		stm32synth_component_updateLPF(&_config->filter_master, 0);
	}
#endif /* STM32SYNTH_FILTER */

	// Pan
	for (uint8_t cc = 0; cc < STM32SYNTH_CHANNEL_NUMBER; cc++)
	{
		_config->pan[cc].l_level = 1.0f;
		_config->pan[cc].r_level = 1.0f;
	}

	// Reverb
#ifdef STM32SYNTH_REVERB
	_config->reverb.level = 64.0f * 0.8f / 127.0f;
	stm32synth_component_f32toq15fract(_config->reverb.level, &_config->reverb.scaleFract, &_config->reverb.shift);
#endif /* STM32SYNTH_REVERB */

	// Multi Channel
	_config->ch0NoteMirror = 0;

	return res;
}

/**
 * @brief Initialize drum-channel specific defaults
 *
 * Applies dedicated defaults for the drum channel including waveform, phonic,
 * ADSR, envelope, and distortion parameters.
 *
 * @param[out] _config Pointer to the synthesizer configuration structure
 *
 * @return stm32synth_res_t Result code (STM32SYNTH_RES_OK on success)
 */
stm32synth_res_t stm32synth_component_initDrum(stm32synth_config_t *_config)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	// waveform
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

	// Phonic
	_config->phonic[STM32SYNTH_MIDINN_DRUMCH] = STM32SYNTH_PHONIC_POLY_SUSTON;

	// ADSR
	_config->adsr[STM32SYNTH_MIDINN_DRUMCH].attack_level = 3.5f;
	_config->adsr[STM32SYNTH_MIDINN_DRUMCH].attack_ms = 0;
	_config->adsr[STM32SYNTH_MIDINN_DRUMCH].decay_ms = 0;
	_config->adsr[STM32SYNTH_MIDINN_DRUMCH].release_ms = 300;

	// Envelope
	_config->envelope[STM32SYNTH_MIDINN_DRUMCH].freq.finish_value = -(3 << 8);
	_config->envelope[STM32SYNTH_MIDINN_DRUMCH].freq.time_ms = 500;
	_config->envelope[STM32SYNTH_MIDINN_DRUMCH].volume.finish_value = -0x40;
	_config->envelope[STM32SYNTH_MIDINN_DRUMCH].volume.time_ms = 600;

	// Distortion
	_config->distortion[STM32SYNTH_MIDINN_DRUMCH] = 1;

	return res;
}
