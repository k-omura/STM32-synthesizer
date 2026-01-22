/*
 * stm32_synth_lfo.c
 *
 *  Created on: Feb 12, 2024
 *      Author: k-omura
 */

#include "stm32_synth.h"
#include "components/stm32_synth_component.h"

stm32synth_res_t stm32synth_component_lfo(stm32synth_config_lfo_t *_configLfo)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;
    _configLfo->val = 0;

    if ((_configLfo->amp == 0) || (_configLfo->freq == 0))
    {
        return res;
    }

    switch (_configLfo->type)
    {
    case STM32SYNTH_LFO_SQU:
    {
        float32_t dutyHighRad = _configLfo->para.duty * STM32SYNTH_DOUBLE_PI;

        if (_configLfo->rad < dutyHighRad)
        {
            _configLfo->val = _configLfo->amp;
        }
        else
        {
            _configLfo->val = -_configLfo->amp;
        }
        break;
    }
    case STM32SYNTH_LFO_TRI:
    {
        float32_t double_amp = 2.0f * _configLfo->amp;
        float32_t peakPoint = STM32SYNTH_DOUBLE_PI * _configLfo->para.peak;

        float32_t doubel_amp_pi_up = double_amp / peakPoint;
        float32_t doubel_amp_pi_down = double_amp / (STM32SYNTH_DOUBLE_PI - peakPoint);

        if (_configLfo->rad < peakPoint)
        {
            _configLfo->val += doubel_amp_pi_up * _configLfo->rad - _configLfo->amp;
        }
        else
        {
            _configLfo->val -= doubel_amp_pi_down * (_configLfo->rad - peakPoint) - _configLfo->amp;
        }

        //_configLfo->val = _configLfo->amp * ((-_configLfo->rad / STM32SYNTH_M_PIF32) + 1.0f);
        break;
    }
    case STM32SYNTH_LFO_SIN:
    {
        _configLfo->val = _configLfo->amp * arm_sin_f32(_configLfo->rad);
        break;
    }
    }

    // refresh omega
    float32_t delta_omega = _configLfo->freq * STM32SYNTH_DOUBLE_PI / (float32_t)STM32SYNTH_SAMPLE_FREQ;
    _configLfo->rad = _configLfo->rad + ((float32_t)STM32SYNTH_HALF_NUM_SAMPLING * delta_omega);
    if (STM32SYNTH_DOUBLE_PI < _configLfo->rad)
    {
        _configLfo->rad -= STM32SYNTH_DOUBLE_PI;
    }

    return res;
}