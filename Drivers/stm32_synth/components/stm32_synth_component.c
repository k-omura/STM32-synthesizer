/*
 * stm32_synth_component.c
 *
 *  Created on: Feb 11, 2024
 *      Author: k-omura
 */

#include "components/stm32_synth_component.h"

#define STM32SYNTH_FILT_SHIFT 3

stm32synth_res_t stm32synth_component_updateDacbuff(stm32synth_config_t *_config, uint32_t _start, uint32_t _end)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	// Pointer
	q15_t *p_in = (&_config->buff.back[_start]);

#ifndef STM32SYNTH_I2S
	uint32_t out_counter = _start;
	uint16_t *p_out = (unt16_t *)(&_config->buff.dac[out_counter]);
#else
	uint32_t out_counter = 2 * _start;
	q15_t *p_out = (q15_t *)(&_config->buff.dac[out_counter]);

	// Pan
	q15_t inL[STM32SYNTH_HALF_NUM_SAMPLING] = {0};
	q15_t inR[STM32SYNTH_HALF_NUM_SAMPLING] = {0};
	q15_t* p_inL = inL;
	q15_t* p_inR = inR;

	arm_scale_q15(p_in, _config->pan.l_scaleFract, _config->pan.l_shift, inL, STM32SYNTH_HALF_NUM_SAMPLING);
	arm_scale_q15(p_in, _config->pan.r_scaleFract, _config->pan.r_shift, inR, STM32SYNTH_HALF_NUM_SAMPLING);

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
stm32synth_res_t stm32synth_component_updateLPF(stm32synth_config_filter_t *_configFilter, int32_t _wow_val)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	float32_t freq = STM32SYNTH_TUNING * powf(2.0f, ((float32_t)((int32_t)_configFilter->para.cutoff_freq_nn.absolute + _wow_val) / 3072.0f) - 5.75f);
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

	arm_biquad_cascade_df1_init_q15(&_configFilter->instance, 1, _configFilter->pCoeffs, _configFilter->pState, STM32SYNTH_FILT_SHIFT);
	_configFilter->state = STM32SYNTH_PARA_NOCHANGE;

	return res;
}

stm32synth_res_t stm32synth_component_updateHPF(stm32synth_config_filter_t *_configFilter, int32_t _wow_val)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	float32_t freq = STM32SYNTH_TUNING * powf(2.0f, ((float32_t)((int32_t)_configFilter->para.cutoff_freq_nn.absolute + _wow_val) / 3072.0f) - 5.75f);
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

	arm_biquad_cascade_df1_init_q15(&_configFilter->instance, 1, _configFilter->pCoeffs, _configFilter->pState, STM32SYNTH_FILT_SHIFT);
	_configFilter->state = STM32SYNTH_PARA_NOCHANGE;

	return res;
}

stm32synth_res_t stm32synth_component_updateLSF(stm32synth_config_filter_t *_configFilter, int32_t _wow_val)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	float32_t freq = STM32SYNTH_TUNING * powf(2.0f, ((float32_t)((int32_t)_configFilter->para.cutoff_freq_nn.absolute + _wow_val) / 3072.0f) - 5.75f);
	if (freq > STM32SYNTH_MAX_FREQ)
	{
		freq = (float32_t)STM32SYNTH_MAX_FREQ;
	}

	float32_t omega_c = freq * (360.0f / (float32_t)STM32SYNTH_SAMPLE_FREQ);
	float32_t sin_omega_c, cos_omega_c;
	arm_sin_cos_f32(omega_c, &sin_omega_c, &cos_omega_c);
	float32_t alfa = sin_omega_c * (0.70710678118654752440084436f); // 0.70710678118654752440084436f is 1/(2 * Q), Q=1/sqrt(2)
	float32_t shelfA = powf(10.0f, (_configFilter->para.q_factor * (6.0f / 40.0f)));
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

	arm_biquad_cascade_df1_init_q15(&_configFilter->instance, 1, _configFilter->pCoeffs, _configFilter->pState, STM32SYNTH_FILT_SHIFT);
	_configFilter->state = STM32SYNTH_PARA_NOCHANGE;

	return res;
}
#else
stm32synth_res_t stm32synth_component_updateLPF(stm32synth_config_filter_t *_configFilter, int32_t _wow_val)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	float32_t freq = STM32SYNTH_TUNING * powf(2.0f, ((float32_t)(_configFilter->para.cutoff_freq_nn.absolute + _wow_val) / 3072.0f) - 5.75f);
	if (freq > STM32SYNTH_MAX_FREQ)
	{
		freq = (float32_t)STM32SYNTH_MAX_FREQ;
	}

	float32_t omega_c = freq * STM32SYNTH_DOUBLE_PI / (float32_t)STM32SYNTH_SAMPLE_FREQ;
	float32_t alfa = arm_sin_f32(omega_c) / _configFilter->para.q_factor;
	float32_t a0 = 1.0f + alfa;
	float32_t cos_omega_c = arm_cos_f32(omega_c);

	_configFilter->pCoeffs[1] = (1.0f - cos_omega_c) / a0;		  // b1
	_configFilter->pCoeffs[0] = _configFilter->pCoeffs[1] / 2.0f; // b0
	_configFilter->pCoeffs[2] = _configFilter->pCoeffs[0];		  // b2
	_configFilter->pCoeffs[3] = (2.0f * cos_omega_c) / a0;		  // a1
	_configFilter->pCoeffs[4] = 1.0f - (2.0f / a0);				  // a2

	arm_biquad_cascade_df2T_init_f32(&_configFilter->instance, 1, _configFilter->pCoeffs, _configFilter->pState);
	_configFilter->state = STM32SYNTH_PARA_NOCHANGE;

	return res;
}

stm32synth_res_t stm32synth_component_updateHPF(stm32synth_config_filter_t *_configFilter, int32_t _wow_val)
{
	stm32synth_res_t res = STM32SYNTH_RES_OK;

	float32_t freq = STM32SYNTH_TUNING * powf(2.0f, ((float32_t)(_configFilter->para.cutoff_freq_nn.absolute + _wow_val) / 3072.0f) - 5.75f);
	if (freq > STM32SYNTH_MAX_FREQ)
	{
		freq = (float32_t)STM32SYNTH_MAX_FREQ;
	}

	float32_t omega_c = freq * STM32SYNTH_DOUBLE_PI / (float32_t)STM32SYNTH_SAMPLE_FREQ;
	float32_t alfa = arm_sin_f32(omega_c) / _configFilter->para.q_factor;
	float32_t a0 = 1.0f + alfa;
	float32_t cos_omega_c = arm_cos_f32(omega_c);

	_configFilter->pCoeffs[1] = -(1.0f + cos_omega_c) / a0;		   // b1
	_configFilter->pCoeffs[0] = -_configFilter->pCoeffs[1] / 2.0f; // b0
	_configFilter->pCoeffs[2] = _configFilter->pCoeffs[0];		   // b2
	_configFilter->pCoeffs[3] = (2.0f * cos_omega_c) / a0;		   // a1
	_configFilter->pCoeffs[4] = 1.0f - (2.0f / a0);				   // a2

	arm_biquad_cascade_df2T_init_f32(&_configFilter->instance, 1, _configFilter->pCoeffs, _configFilter->pState);
	_configFilter->state = STM32SYNTH_PARA_NOCHANGE;

	return res;
}
#endif /* STM32SYNTH_FILT_CMSIS */

stm32synth_res_t stm32synth_component_f32toq15fract(float32_t _scale, q15_t *_scaleFract, int8_t *_shift)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;
    int8_t shift = 0;
    q15_t scaleFract;
    uint16_t shifted = 1;

    while ((float32_t)shifted < _scale)
    {
        shift++;
        shifted <<= 1;
    }
    scaleFract = (int16_t)((_scale / (float32_t)shifted) * (float32_t)STM32SYNTH_Q15_MAX);

    *_shift = shift;
    *_scaleFract = scaleFract;

    return res;
}

stm32synth_res_t stm32synth_component_q15toq15fract(q15_t _scale, q15_t *_scaleFract, int8_t *_shift)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;
    int8_t shift = 0;
    q15_t scaleFract;
    uint16_t shifted = 1;

    while (shifted < _scale)
    {
        shift++;
        shifted <<= 1;
    }
    scaleFract = (int16_t)(_scale * STM32SYNTH_Q15_MAX / shifted);

    *_shift = shift;
    *_scaleFract = scaleFract;

    return res;
}
