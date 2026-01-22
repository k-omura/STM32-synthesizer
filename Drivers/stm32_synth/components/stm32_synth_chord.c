/*
 * stm32_synth_chord.c
 *
 *  Created on: Feb 11, 2024
 *      Author: k-omura
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#include "stm32_synth.h"
#include "components/stm32_synth_component.h"
#include "components/stm32_synth_midi.h"

//
#define STM32SYNTH_COMPONENT_CHORD_DISABLEDNN 0xFFFF

#define STM32SYNTH_CHORD_BASE_AMP_SHIFT 7
#define STM32SYNTH_CHORD_BASE_AMP (STM32SYNTH_GND_LEVEL >> STM32SYNTH_CHORD_BASE_AMP_SHIFT)
#define STM32SYNTH_CHORD_DISTORTION_SHIFT (STM32SYNTH_AMP_MAX_BIT - STM32SYNTH_CHORD_BASE_AMP_SHIFT - 7) // 127->7bit

#define STM32SYNTH_CHORD_DRUM_TYPE_SIN   (0b00000001)
#define STM32SYNTH_CHORD_DRUM_TYPE_SQU   (0b00000010)
#define STM32SYNTH_CHORD_DRUM_TYPE_TRI   (0b00000100)
#define STM32SYNTH_CHORD_DRUM_TYPE_NOISE (0b00001000)

// private functions
stm32synth_res_t stm32synth_chord_updateChord(stm32synth_config_t *_config, stm32synth_chord_t *_configChord, q15_t *_pbuff, stm32synth_update_half_t _half);
stm32synth_res_t stm32synth_chord_adsrCurve(stm32synth_config_t *_config, stm32synth_chord_t *_configChord, float32_t *_amp);
stm32synth_res_t stm32synth_chord_envelope(stm32synth_config_envelopec_t *_envelopec, uint32_t *_envelopeCount, int16_t *_outval);

stm32synth_res_t stm32synth_chord_makerad(stm32synth_config_t *_config, stm32synth_chord_t *_configChord, uint16_t _nn, q15_t *_radBuff, stm32synth_waveformnum_t _wnum);
stm32synth_res_t stm32synth_chord_addsine(stm32synth_config_t *_config, stm32synth_chord_t *_configChord, q15_t *_chordBuff, q15_t *_radBuff, stm32synth_waveformnum_t _wnum);
stm32synth_res_t stm32synth_chord_addsque(stm32synth_config_t *_config, stm32synth_chord_t *_configChord, q15_t *_chordBuff, q15_t *_radBuff, stm32synth_waveformnum_t _wnum);
stm32synth_res_t stm32synth_chord_addtrgl(stm32synth_config_t *_config, stm32synth_chord_t *_configChord, q15_t *_chordBuff, q15_t *_radBuff, stm32synth_waveformnum_t _wnum);
stm32synth_res_t stm32synth_chord_addnoise(stm32synth_config_t *_config, float32_t _amp ,q15_t *_chordBuff);

// private variables
static stm32synth_chord_t chords[STM32SYNTH_MAX_CHORD];

#ifndef STM32SYNTH_DRUM_TESTMODE
static const stm32synth_config_drum_t drumConfigList[STM32SYNTH_DRUMCHORD_NUMBER];
#else
static stm32synth_config_drum_t drumConfigList[STM32SYNTH_DRUMCHORD_NUMBER];
#endif /* STM32SYNTH_DRUM_TESTMODE */

#ifndef STM32SYNTH_SIN_LUT
// CORDIC instance, config
static CORDIC_HandleTypeDef *cordicHW;
#else
// sine look up table
static const q15_t sine_lut[256];
#endif /* STM32SYNTH_SIN_LUT */

stm32synth_res_t stm32synth_component_testChord(stm32synth_config_t *_config)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    stm32synth_chord_t testChord;

    // test para
    testChord.channel = 0;
    testChord.noteNum[0] = 72 << 8;
    testChord.velocity = 10;

    stm32synth_component_noteonChord(_config, &testChord);

    return res;
}

stm32synth_res_t stm32synth_component_initChord()
{
    stm32synth_res_t res = STM32SYNTH_RES_NG;

    for (uint8_t cc = 0; cc < STM32SYNTH_MAX_CHORD; cc++)
    {
        chords[cc].rad[STM32SYNTH_WAVEFORM_0] = 0;
        chords[cc].rad[STM32SYNTH_WAVEFORM_1] = 0;
        chords[cc].noteNum[0] = STM32SYNTH_COMPONENT_CHORD_DISABLEDNN;
        chords[cc].adsr.count = 0;
        chords[cc].adsr.state = STM32SYNTH_ADSRSTATE_WAIT;
        chords[cc].envelope.volume = 0;
        chords[cc].envelope.freq = 0;
#ifdef STM32SYNTH_FILTER
        chords[cc].envelope.cutoff = 0;

        if(chords[cc].channel != STM32SYNTH_MIDINN_DRUMCH)
        {
            chords[cc].lpf.para.q_factor = 0.8f;
            chords[cc].hpf.para.q_factor = 0.8f;
            chords[cc].lpf.para.cutoff_freq_nn.absolute = 127 << 8;
            chords[cc].hpf.para.cutoff_freq_nn.absolute = 127 << 8;
            stm32synth_component_updateLPF(&chords[cc].lpf, 0);
            stm32synth_component_updateHPF(&chords[cc].hpf, 0);
        }
#endif
    }

#ifdef STM32SYNTH_DRUM_TESTMODE
    stm32synth_drum_init();
#endif /* STM32SYNTH_DRUM_TESTMODE */

    return res;
}

stm32synth_res_t stm32synth_component_noteonChord(stm32synth_config_t *_config, stm32synth_chord_t *_inpurChord)
{
    stm32synth_res_t res = STM32SYNTH_RES_NG;

    for (uint8_t cc = 0; cc < STM32SYNTH_MAX_CHORD; cc++)
    {
        if (
            ((chords[cc].channel == _inpurChord->channel) && (chords[cc].noteNum[0] == _inpurChord->noteNum[0])) || // comment out if computational resources are sufficient.
            (chords[cc].adsr.state == STM32SYNTH_ADSRSTATE_WAIT) ||
            (_config->phonic[_inpurChord->channel] == STM32SYNTH_PHONIC_MONO))
        {
            chords[cc].channel = _inpurChord->channel;
            chords[cc].noteNum[0] = _inpurChord->noteNum[0];
            chords[cc].noteNum[1] = 0;
            chords[cc].velocity = _inpurChord->velocity;

            chords[cc].adsr.state = STM32SYNTH_ADSRSTATE_ATTACK;
            chords[cc].adsr.count = 0;
            chords[cc].rad[STM32SYNTH_WAVEFORM_0] = 0;
            chords[cc].rad[STM32SYNTH_WAVEFORM_1] = 0;
            chords[cc].envelope.volume = 0;
            chords[cc].envelope.freq = 0;
#ifdef STM32SYNTH_FILTER
            chords[cc].envelope.cutoff = 0;
#endif

            if (chords[cc].channel == STM32SYNTH_MIDINN_DRUMCH)
            {
                // filter
                uint8_t drumConfigIndex = (chords[cc].noteNum[0] >> 8);
                drumConfigIndex = (drumConfigIndex < STM32SYNTH_MIDINN_LOWESTDRUM) ? (STM32SYNTH_MIDINN_LOWESTDRUM) : (drumConfigIndex);
                drumConfigIndex = (STM32SYNTH_MIDINN_HIGHESTDRUM < drumConfigIndex) ? (STM32SYNTH_MIDINN_HIGHESTDRUM) : (drumConfigIndex);
                drumConfigIndex -= STM32SYNTH_MIDINN_LOWESTDRUM;
                chords[cc].noteNum[1] = drumConfigIndex;

                chords[cc].lpf.para.q_factor = drumConfigList[drumConfigIndex].lpfq;
                chords[cc].hpf.para.q_factor = drumConfigList[drumConfigIndex].hpfq;
                chords[cc].lpf.para.cutoff_freq_nn.absolute = (uint16_t)drumConfigList[drumConfigIndex].lpffc_nn << 8;
                chords[cc].hpf.para.cutoff_freq_nn.absolute = (uint16_t)drumConfigList[drumConfigIndex].hpffc_nn << 8;

                // update
                if (drumConfigList[drumConfigIndex].lpffc_nn != STM32SYNTH_MIDINN_FILTER_DISABLE)
                {
                    stm32synth_component_updateLPF(&chords[cc].lpf, 0);
                }

                if (drumConfigList[drumConfigIndex].hpffc_nn != STM32SYNTH_MIDINN_FILTER_DISABLE)
                {
                    stm32synth_component_updateHPF(&chords[cc].hpf, 0);
                }
            }
#ifdef STM32SYNTH_FILTER
#ifdef STM32SYNTH_CHORDFILTER
            else
            {
                // update filter
                if(_config->filter[_inpurChord->channel].type == STM32SYNTH_FILTERTYPE_HPF)
                {
                    chords[cc].hpf.para.q_factor = _config->filter[_inpurChord->channel].q_factor;
                    // chords[cc].hpf.para.cutoff_freq_nn.absolute = _inpurChord->noteNum[0] + (_config->filter[_inpurChord->channel].cutoff_freq_nn.relative << 6);
                    chords[cc].hpf.para.cutoff_freq_nn.absolute = _config->filter[_inpurChord->channel].cutoff_freq_nn.absolute;
                    stm32synth_component_updateHPF(&chords[cc].hpf, 0);
                }
                else
                {
                    chords[cc].lpf.para.q_factor = _config->filter[_inpurChord->channel].q_factor;
                    // chords[cc].lpf.para.cutoff_freq_nn.absolute = _inpurChord->noteNum[0] + (_config->lpf[_inpurChord->channel].cutoff_freq_nn.relative << 6);
                    chords[cc].lpf.para.cutoff_freq_nn.absolute = _config->filter[_inpurChord->channel].cutoff_freq_nn.absolute;
                    stm32synth_component_updateLPF(&chords[cc].lpf, 0);
                }

                arm_fill_q15(0, chords[cc].presample, STM32SYNTH_PRE_SAMPLE);
            }
#endif /* STM32SYNTH_CHORDFILTER */
#endif /* STM32SYNTH_FILTER */

            res = STM32SYNTH_RES_OK;
            break;
        }
    }

    return res;
}

stm32synth_res_t stm32synth_component_noteoffChord(stm32synth_config_t *_config, stm32synth_chord_t *_inpurChord)
{
    stm32synth_res_t res = STM32SYNTH_RES_NG;

    for (uint8_t cc = 0; cc < STM32SYNTH_MAX_CHORD; cc++)
    {
        if (
            ((chords[cc].channel == _inpurChord->channel) && (chords[cc].noteNum[0] == _inpurChord->noteNum[0]) && (chords[cc].adsr.state < STM32SYNTH_ADSRSTATE_RELEASE)))
        {
            chords[cc].adsr.state = STM32SYNTH_ADSRSTATE_RELEASE;
            chords[cc].adsr.count = 0;

            res = STM32SYNTH_RES_OK;
            // break; //To delete with the next noteoff command if the noteoff command is not detected
        }
    }

    return res;
}

stm32synth_res_t stm32synth_component_noteoffAll()
{
    stm32synth_res_t res = STM32SYNTH_RES_NG;

    for (uint8_t cc = 0; cc < STM32SYNTH_MAX_CHORD; cc++)
    {
        chords[cc].adsr.state = STM32SYNTH_ADSRSTATE_RELEASE;
        chords[cc].adsr.count = 0;

        res = STM32SYNTH_RES_OK;
    }

    return res;
}

stm32synth_res_t stm32synth_component_disableChordAll()
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    for (uint8_t cc = 0; cc < STM32SYNTH_MAX_CHORD; cc++)
    {
        chords[cc].noteNum[0] = STM32SYNTH_COMPONENT_CHORD_DISABLEDNN;
        chords[cc].channel = 0;
        chords[cc].velocity = 0;
        chords[cc].rad[STM32SYNTH_WAVEFORM_0] = 0;
        chords[cc].rad[STM32SYNTH_WAVEFORM_1] = 0;
        chords[cc].adsr.count = 0;
        chords[cc].adsr.state = STM32SYNTH_ADSRSTATE_WAIT;
        chords[cc].envelope.volume = 0;
        chords[cc].envelope.freq = 0;

#ifdef STM32SYNTH_FILTER
        chords[cc].envelope.cutoff = 0;
#endif

        res = STM32SYNTH_RES_OK;
    }

    return res;
}

stm32synth_res_t stm32synth_component_disableChord(stm32synth_chord_t *_configChord)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    _configChord->noteNum[0] = STM32SYNTH_COMPONENT_CHORD_DISABLEDNN;
    _configChord->channel = 0;
    _configChord->velocity = 0;
    _configChord->rad[STM32SYNTH_WAVEFORM_0] = 0;
    _configChord->rad[STM32SYNTH_WAVEFORM_1] = 0;
    _configChord->adsr.count = 0;
    _configChord->adsr.state = STM32SYNTH_ADSRSTATE_WAIT;
    _configChord->envelope.volume = 0;
    _configChord->envelope.freq = 0;
#ifdef STM32SYNTH_FILTER
    _configChord->envelope.cutoff = 0;
#endif

    return res;
}

stm32synth_res_t stm32synth_component_updateBuff(stm32synth_config_t *_config, stm32synth_update_half_t _half)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;
    q15_t chordsBuff[STM32SYNTH_SAMPLE_FORFILT] = {0};
    uint16_t lfoUpdateStatus = 0;
    int8_t shift;
    q15_t scaleFract;

    // DAC back buffer pointer
    q15_t *mbuff_p = (_half == STM32SYNTH_UPDATE_LATTER) ? (_config->buff.back + STM32SYNTH_HALF_NUM_SAMPLING) : (_config->buff.back);

    // tremolo
    stm32synth_component_lfo(&_config->tre_master);

    // vivlato
    stm32synth_component_lfo(&_config->vib_master);

#ifdef STM32SYNTH_FILTER
    // wow
    stm32synth_component_lfo(&_config->wow_master);
#endif /* STM32SYNTH_FILTER */

    // update Chord
    for (uint8_t cc = 0; cc < STM32SYNTH_MAX_CHORD; cc++)
    {
        if (((chords[cc].noteNum[0] >> 8) > 127) || (chords[cc].velocity == 0))
        {
            continue;
        }

        if ((lfoUpdateStatus & (0x0001 << chords[cc].channel)) == 0)
        {
            // tremolo
            stm32synth_component_lfo(&_config->tre[chords[cc].channel]);

            // vivlato
            stm32synth_component_lfo(&_config->vib[chords[cc].channel]);

#ifdef STM32SYNTH_FILTER
#ifdef STM32SYNTH_CHORDFILTER
            // wow
            stm32synth_component_lfo(&_config->wow[chords[cc].channel]);
#endif
#endif /* STM32SYNTH_FILTER */

            lfoUpdateStatus |= (0x0001 << chords[cc].channel);
        }

        stm32synth_chord_updateChord(_config, &chords[cc], (chordsBuff + STM32SYNTH_PRE_SAMPLE), _half);
    }

#ifdef STM32SYNTH_FILTER
    if (_config->filter_master.para.cutoff_freq_nn.absolute < (127 << 8))
    {
		// Filter update
		if(_config->filter_master.para.type == STM32SYNTH_FILTERTYPE_LSF)
		{
		    stm32synth_component_updateLSF(&_config->filter_master, (int32_t)_config->wow_master.val);
		}
		else
		{
		    stm32synth_component_updateLPF(&_config->filter_master, (int32_t)_config->wow_master.val);
		}

		// pre-sample for filter
		arm_copy_q15(_config->buff.presample, chordsBuff, STM32SYNTH_PRE_SAMPLE);

		// prepare pre-sample for filter (for next cycle)
		arm_copy_q15((chordsBuff + STM32SYNTH_HALF_NUM_SAMPLING), _config->buff.presample, STM32SYNTH_PRE_SAMPLE);

		// input filter
#ifdef STM32SYNTH_FILT_CMSIS
    	arm_biquad_cascade_df1_fast_q15(&_config->filter_master.instance, chordsBuff, chordsBuff, STM32SYNTH_SAMPLE_FORFILT);
#endif /* STM32SYNTH_FILT_CMSIS */
    }
#endif /* STM32SYNTH_FILTER */

    // Master volume & Tremolo
    stm32synth_component_f32toq15fract((1 + _config->tre_master.val), &scaleFract, &shift);
#ifndef STM32SYNTH_I2S
    arm_scale_q15((chordsBuff + STM32SYNTH_PRE_SAMPLE), scaleFract, shift, (chordsBuff + STM32SYNTH_PRE_SAMPLE), STM32SYNTH_HALF_NUM_SAMPLING);
#else
    arm_scale_q15((chordsBuff + STM32SYNTH_PRE_SAMPLE), scaleFract, shift, mbuff_p, STM32SYNTH_HALF_NUM_SAMPLING);
#endif /* STM32SYNTH_I2S */

    // Reverb
#ifdef STM32SYNTH_REVERB
    if (_config->reverb.level > 0)
    {   
        arm_add_q15(mbuff_p, _config->buff.reverb[0], mbuff_p, STM32SYNTH_HALF_NUM_SAMPLING);

        arm_copy_q15(_config->buff.reverb[1], _config->buff.reverb[0], ((STM32SYNTH_REVERB_NUM - 1) * STM32SYNTH_HALF_NUM_SAMPLING));

        arm_scale_q15(mbuff_p, _config->reverb.scaleFract, _config->reverb.shift, _config->buff.reverb[(STM32SYNTH_REVERB_NUM - 1)], STM32SYNTH_HALF_NUM_SAMPLING);
    }
#endif /* STM32SYNTH_REVERB */

#ifndef STM32SYNTH_I2S
    // offset audio
    arm_offset_f32(mbuff_p, STM32SYNTH_GND_LEVEL, mbuff_p, STM32SYNTH_HALF_NUM_SAMPLING);
#endif /* STM32SYNTH_I2S */

    return res;
}

#ifndef STM32SYNTH_SIN_LUT
stm32synth_res_t stm32synth_component_initCORDIC(CORDIC_HandleTypeDef *_cordicHW)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;
    cordicHW = _cordicHW;

    CORDIC_ConfigTypeDef cordicConfig;

    cordicConfig.Function = CORDIC_FUNCTION_SINE;
    cordicConfig.Precision = CORDIC_PRECISION_2CYCLES;
    cordicConfig.Scale = CORDIC_SCALE_0;
    cordicConfig.NbWrite = CORDIC_NBWRITE_1;
    cordicConfig.NbRead = CORDIC_NBREAD_1;
    cordicConfig.InSize = CORDIC_OUTSIZE_16BITS;
    cordicConfig.OutSize = CORDIC_OUTSIZE_16BITS;

    HAL_CORDIC_Configure(cordicHW, &cordicConfig);

    return res;
}
#endif /* STM32SYNTH_SIN_LUT */

//--------------------------------------------------------------------------------------------------------
stm32synth_res_t stm32synth_chord_updateChord(stm32synth_config_t *_config, stm32synth_chord_t *_configChord, q15_t *_pbuff, stm32synth_update_half_t _half)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    if (_configChord->noteNum[0] > (127 << 8))
    {
        res = STM32SYNTH_RES_NG;
        return res;
    }

    q15_t chordBuff[STM32SYNTH_SAMPLE_FORFILT] = {0};
    q15_t radBuff[STM32SYNTH_HALF_NUM_SAMPLING] = {0};
    float32_t amp = _config->volume[_configChord->channel] * _configChord->velocity;
    int8_t shift;
    q15_t scaleFract;
    int16_t distortion16;

    // adsr
    float32_t adsr_amp = 0;
    stm32synth_chord_adsrCurve(_config, _configChord, &adsr_amp);
    amp *= adsr_amp;
    amp *= 1 + _config->tre[_configChord->channel].val;

    // envelope
    int16_t envelope_volume, envelope_freq_nn;
    stm32synth_chord_envelope(&(_config->envelope[_configChord->channel].volume), &(_configChord->envelope.volume), &envelope_volume);
    if(_configChord->channel == STM32SYNTH_MIDINN_DRUMCH)
    {
        stm32synth_config_envelopec_t freqenv;

        freqenv.time_ms = _config->envelope[_configChord->channel].freq.time_ms;

#ifdef STM32SYNTH_DRUM_TESTMODE
        if((_configChord->noteNum[0] >> 8) == STM32SYNTH_MIDINN_LOWESTDRUM)
        {
            freqenv.finish_value = _config->envelope[_configChord->channel].freq.finish_value;
        }
        else
        {
#endif /* STM32SYNTH_DRUM_TESTMODE */
            freqenv.finish_value = drumConfigList[_configChord->noteNum[1]].freqenv_diff;
#ifdef STM32SYNTH_DRUM_TESTMODE
        }
#endif /* STM32SYNTH_DRUM_TESTMODE */

        stm32synth_chord_envelope(&freqenv, &(_configChord->envelope.freq), &envelope_freq_nn);
    }
    else
    {
        stm32synth_chord_envelope(&(_config->envelope[_configChord->channel].freq), &(_configChord->envelope.freq), &envelope_freq_nn);
    }
    amp *= 1 + envelope_volume / 64.0f;

#ifdef STM32SYNTH_FILTER
#ifdef STM32SYNTH_CHORDFILTER
    int16_t envelope_cutoff;
    stm32synth_chord_envelope(&(_config->envelope[_configChord->channel].cutoff), &(_configChord->envelope.cutoff), &envelope_cutoff);
#endif
#endif /* STM32SYNTH_CHORDFILTER */

    // Convert frequency to phase
    if (_configChord->channel == STM32SYNTH_MIDINN_DRUMCH)
    {   
        uint8_t drumConfigIndex = _configChord->noteNum[1];
        uint16_t drum_nn = drumConfigList[drumConfigIndex].nn;

        if (drum_nn && (drumConfigList[drumConfigIndex].waveformType & 0b00000111))
        {
            for (uint8_t ww = 0; ww < STM32SYNTH_WAVEFORM_NUM_PERCHORD; ww++)
            {
                if ((ww == 1) && ((drumConfigList[drumConfigIndex].waveformType & STM32SYNTH_CHORD_DRUM_TYPE_SQU) == 0))
                {
                    continue;
                }

                uint16_t nn = (drum_nn << 8);
                if(ww == 1)
                {
#ifdef STM32SYNTH_DRUM_TESTMODE
                    if((_configChord->noteNum[0] >> 8) == STM32SYNTH_MIDINN_LOWESTDRUM)
                    {
                        nn += _config->waveform[_configChord->channel][ww].pitch;
                    }
                    else
                    {
#endif /* STM32SYNTH_DRUM_TESTMODE */
                        nn += drumConfigList[drumConfigIndex].pitch2;
#ifdef STM32SYNTH_DRUM_TESTMODE
                    }
#endif /* STM32SYNTH_DRUM_TESTMODE */
                }

                if(envelope_freq_nn < 0)
                {
                    int16_t inv_envelope_freq_nn = -envelope_freq_nn;
                    nn = (nn > inv_envelope_freq_nn) ? (nn - inv_envelope_freq_nn) : (0);
                }
                else
                {
                    nn = nn + envelope_freq_nn;
                    nn = (nn > STM32SYNTH_MAX_FREQ_NOTE) ? STM32SYNTH_MAX_FREQ_NOTE : nn;
                }

                stm32synth_chord_makerad(_config, _configChord, nn, radBuff, ww);

                if(drumConfigList[drumConfigIndex].waveformType & STM32SYNTH_CHORD_DRUM_TYPE_SIN)
                {
                    stm32synth_chord_addsine(_config, _configChord, chordBuff + STM32SYNTH_PRE_SAMPLE, radBuff, ww);
                }
                if(drumConfigList[drumConfigIndex].waveformType & STM32SYNTH_CHORD_DRUM_TYPE_TRI)
                {
                    stm32synth_chord_addtrgl(_config, _configChord, chordBuff + STM32SYNTH_PRE_SAMPLE, radBuff, ww);
                }
                if(drumConfigList[drumConfigIndex].waveformType & STM32SYNTH_CHORD_DRUM_TYPE_SQU)
                {
                    stm32synth_chord_addsque(_config, _configChord, chordBuff + STM32SYNTH_PRE_SAMPLE, radBuff, ww);
                }
            }
        }

        if (drumConfigList[drumConfigIndex].waveformType & STM32SYNTH_CHORD_DRUM_TYPE_NOISE)
        {
            stm32synth_chord_addnoise(_config, drumConfigList[drumConfigIndex].wn_amp, chordBuff);
        }

        // input filter
        if (drumConfigList[drumConfigIndex].lpffc_nn != STM32SYNTH_MIDINN_FILTER_DISABLE)
        {
#ifdef STM32SYNTH_FILT_CMSIS
            arm_biquad_cascade_df1_fast_q15(&_configChord->lpf.instance, chordBuff, chordBuff, STM32SYNTH_SAMPLE_FORFILT); // LPF
#endif /* STM32SYNTH_FILT_CMSIS */
        }

        if (drumConfigList[drumConfigIndex].hpffc_nn != STM32SYNTH_MIDINN_FILTER_DISABLE)
        {
#ifdef STM32SYNTH_FILT_CMSIS
            arm_biquad_cascade_df1_fast_q15(&_configChord->hpf.instance, chordBuff, chordBuff, STM32SYNTH_SAMPLE_FORFILT); // HPF
#endif /* STM32SYNTH_FILT_CMSIS */
        }

#ifdef STM32SYNTH_DRUM_TESTMODE
        if((_configChord->noteNum[0] >> 8) == STM32SYNTH_MIDINN_LOWESTDRUM)
        {
            distortion16 = _config->distortion[_configChord->channel];
        }
        else
        {
#endif /* STM32SYNTH_DRUM_TESTMODE */
            distortion16 = drumConfigList[drumConfigIndex].distortion;
#ifdef STM32SYNTH_DRUM_TESTMODE
        }
#endif /* STM32SYNTH_DRUM_TESTMODE */
    }
    else
    {
        uint16_t nn[2];
        nn[1] = _configChord->noteNum[0] + _config->pitch[_configChord->channel] + _config->vib_master.val + _config->vib[_configChord->channel].val + envelope_freq_nn;
        nn[0] = nn[1] + _config->waveform[_configChord->channel][0].pitch;
        nn[1] = nn[1] + _config->waveform[_configChord->channel][1].pitch;

        for (uint8_t ww = 0; ww < STM32SYNTH_WAVEFORM_NUM_PERCHORD; ww++)
        {
            if ((_config->waveform[_configChord->channel][ww].sin_level == 0) && (_config->waveform[_configChord->channel][ww].squ_level == 0) && (_config->waveform[_configChord->channel][ww].tri_level == 0))
            {
                continue;
            }
            
            stm32synth_chord_makerad(_config, _configChord, nn[ww], radBuff, ww); // light weight frequency change

            stm32synth_chord_addsine(_config, _configChord, chordBuff + STM32SYNTH_PRE_SAMPLE, radBuff, ww);
            stm32synth_chord_addsque(_config, _configChord, chordBuff + STM32SYNTH_PRE_SAMPLE, radBuff, ww);
            stm32synth_chord_addtrgl(_config, _configChord, chordBuff + STM32SYNTH_PRE_SAMPLE, radBuff, ww);
        }

#ifdef STM32SYNTH_FILTER
#ifdef STM32SYNTH_CHORDFILTER
        // if (_config->filter[_configChord->channel].cutoff_freq_nn.relative < (127 - 0x40))
        if (_config->filter[_configChord->channel].cutoff_freq_nn.absolute < (127 << 8))
        {
            // update relative
            /*
            _configChord->lpf.para.q_factor = _config->filter[_configChord->channel].q_factor;
            _configChord->lpf.para.cutoff_freq_nn.absolute = nn[0] + _config->filter[_configChord->channel].cutoff_freq_nn.relative;
            stm32synth_component_updateLPF(&_configChord->lpf, (int32_t)_config->wow[_configChord->channel].val);
            // */

            // update absolute
            //*
            if(((_config->wow[_configChord->channel].amp != 0) && (_config->wow[_configChord->channel].freq != 0)) || ((_config->envelope[_configChord->channel].cutoff.finish_value != 0) && (_config->envelope[_configChord->channel].cutoff.time_ms != 0)))
            {
                if(_config->filter[_configChord->channel].type == STM32SYNTH_FILTERTYPE_HPF)
                {

                }
                else
                {
                    stm32synth_component_updateLPF(&_configChord->lpf, (int32_t)_config->wow[_configChord->channel].val + envelope_cutoff);
                }
            }
            // */

            // pre-sample for filter
            arm_copy_q15(_configChord->presample, chordBuff, STM32SYNTH_PRE_SAMPLE);

            // prepare pre-sample for filter (for next cycle)
            arm_copy_q15((chordBuff + STM32SYNTH_HALF_NUM_SAMPLING), _configChord->presample, STM32SYNTH_PRE_SAMPLE);

#ifdef STM32SYNTH_FILT_CMSIS
            if(_config->filter[_configChord->channel].type == STM32SYNTH_FILTERTYPE_HPF)
            {
                arm_biquad_cascade_df1_fast_q15(&_configChord->hpf.instance, chordBuff, chordBuff, STM32SYNTH_SAMPLE_FORFILT); // Filter
            }
            else
            {
                arm_biquad_cascade_df1_fast_q15(&_configChord->lpf.instance, chordBuff, chordBuff, STM32SYNTH_SAMPLE_FORFILT); // Filter
            }
#endif                                                                                                                     /* STM32SYNTH_FILT_CMSIS */

#endif /* STM32SYNTH_CHORDFILTER */
#endif /* STM32SYNTH_FILTER */
        }

        distortion16 = _config->distortion[_configChord->channel];
    }

    // Distortion (Clipping)
    if (distortion16 < 128)
    {
        q15_t chordMax;
        uint32_t chordMaxIndex;
        q15_t distortion = distortion16 << STM32SYNTH_CHORD_DISTORTION_SHIFT;
        arm_absmax_q15(chordBuff + STM32SYNTH_PRE_SAMPLE, STM32SYNTH_HALF_NUM_SAMPLING, &chordMax, &chordMaxIndex);
        arm_clip_q15(chordBuff + STM32SYNTH_PRE_SAMPLE, chordBuff + STM32SYNTH_PRE_SAMPLE, -distortion, distortion, STM32SYNTH_HALF_NUM_SAMPLING);
        if (chordMax > distortion)
        {
            amp = amp * (float32_t)chordMax / (float32_t)distortion;
        }
    }

    // Amp
    stm32synth_component_f32toq15fract(amp, &scaleFract, &shift);
    arm_scale_q15(chordBuff + STM32SYNTH_PRE_SAMPLE, scaleFract, shift, chordBuff + STM32SYNTH_PRE_SAMPLE, STM32SYNTH_HALF_NUM_SAMPLING);

    // Add to master array
    arm_add_q15(chordBuff + STM32SYNTH_PRE_SAMPLE, _pbuff, _pbuff, STM32SYNTH_HALF_NUM_SAMPLING);

    return res;
}

stm32synth_res_t stm32synth_chord_adsrCurve(stm32synth_config_t *_config, stm32synth_chord_t *_configChord, float32_t *_amp)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;
    float32_t adsr_time = 0;
    int32_t adsr_time_count = 0;

    switch (_configChord->adsr.state)
    {
    case STM32SYNTH_ADSRSTATE_ATTACK:
        adsr_time_count = _config->adsr[_configChord->channel].attack_ms + 1;
        adsr_time = adsr_time_count;
        break;
    case STM32SYNTH_ADSRSTATE_DECAY:
        adsr_time_count = _config->adsr[_configChord->channel].decay_ms;
        adsr_time = adsr_time_count;
        break;
    case STM32SYNTH_ADSRSTATE_SUSTAIN:
    {
        *_amp = 1.0f;
        if ((_config->phonic[_configChord->channel] == STM32SYNTH_PHONIC_POLY_SUSTON) || (_config->phonic[_configChord->channel] == STM32SYNTH_PHONIC_MONO_SUSTON))
        {
            return res;
        }
        adsr_time_count = -100;
        adsr_time = adsr_time_count;
        break;
    }
    case STM32SYNTH_ADSRSTATE_RELEASE:
        adsr_time_count = _config->adsr[_configChord->channel].release_ms;
        adsr_time = adsr_time_count;
        break;
    default:
        *_amp = 0;
        res = STM32SYNTH_RES_NG;
        break;
    }

    _configChord->adsr.count += (1000 * STM32SYNTH_HALF_NUM_SAMPLING) / STM32SYNTH_SAMPLE_FREQ;
    if (_configChord->adsr.count > adsr_time_count)
    {
        _configChord->adsr.count = 0;
        _configChord->adsr.state++;
    }
    *_amp = powf(10.0f, -2.0f * (float32_t)_configChord->adsr.count / adsr_time);

    switch (_configChord->adsr.state)
    {
    case STM32SYNTH_ADSRSTATE_ATTACK:
        *_amp = _config->adsr[_configChord->channel].attack_level * (1.0f - (*_amp));
        break;
    case STM32SYNTH_ADSRSTATE_DECAY:
        *_amp = 1.0f + ((_config->adsr[_configChord->channel].attack_level - 1.0f) * (*_amp));
        break;
    case STM32SYNTH_ADSRSTATE_SUSTAIN:
        *_amp = 1.0f;
        break;
    case STM32SYNTH_ADSRSTATE_RELEASE:
        //*_amp = *_amp;
        break;
    case STM32SYNTH_ADSRSTATE_DONE:
    default:
        *_amp = 0.0f;
        stm32synth_component_disableChord(_configChord);
        res = STM32SYNTH_RES_NG;
        break;
    }

    return res;
}

stm32synth_res_t stm32synth_chord_envelope(stm32synth_config_envelopec_t *_envelopec, uint32_t *_envelopeCount, int16_t *_outval)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    if (_envelopec->finish_value == 0)
    {
        (*_outval) = 0;
        return STM32SYNTH_RES_NG;
    }

    if ((*_envelopeCount) > _envelopec->time_ms)
    {
        (*_outval) = _envelopec->finish_value;
        return STM32SYNTH_RES_OK;
    }

    (*_envelopeCount) += (1000 * STM32SYNTH_HALF_NUM_SAMPLING) / STM32SYNTH_SAMPLE_FREQ;
    (*_outval) = _envelopec->finish_value + (int16_t)((float32_t)(-_envelopec->finish_value) * powf(10.0f, -2.0f * (float32_t)(*_envelopeCount) / (float32_t)_envelopec->time_ms));

    return res;
}

stm32synth_res_t stm32synth_chord_makerad(stm32synth_config_t *_config, stm32synth_chord_t *_configChord, uint16_t _nn, q15_t *_radBuff, stm32synth_waveformnum_t _wnum)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    float32_t freq = STM32SYNTH_TUNING * powf(2.0f, ((float32_t)_nn / 3072.0f) - 5.75f);

    // float32_t delta_omega = freq * STM32SYNTH_DOUBLE_PI / (float32_t)STM32SYNTH_SAMPLE_FREQ;

    q15_t delta_omega_q15 = (q15_t)(freq * (STM32SYNTH_Q15_MAX << 1) / STM32SYNTH_SAMPLE_FREQ);
    q15_t temp = _configChord->rad[_wnum]; // init;

    q15_t* radBuff_p = _radBuff;
    for (uint32_t t = 0; t < STM32SYNTH_HALF_NUM_SAMPLING_BY_4; t++)
    {
    	*radBuff_p = temp;
    	*(radBuff_p + 1) = *(radBuff_p) + delta_omega_q15;
    	*(radBuff_p + 2) = *(radBuff_p + 1) + delta_omega_q15;
    	*(radBuff_p + 3) = *(radBuff_p + 2) + delta_omega_q15;
        temp = *(radBuff_p + 3) + delta_omega_q15;
        radBuff_p += 4;
    }
    _configChord->rad[_wnum] = temp;

    return res;
}

stm32synth_res_t stm32synth_chord_addsine(stm32synth_config_t *_config, stm32synth_chord_t *_configChord, q15_t *_chordBuff, q15_t *_radBuff, stm32synth_waveformnum_t _wnum)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    if (_config->waveform[_configChord->channel][_wnum].sin_level < 0.05f)
    {
        res = STM32SYNTH_RES_NG;
        return res;
    }

    q15_t buff[STM32SYNTH_HALF_NUM_SAMPLING] = {0};
    int8_t shift;
    q15_t scaleFract;
    stm32synth_component_f32toq15fract(_config->waveform[_configChord->channel][_wnum].sin_level / (float32_t)(1 << (STM32SYNTH_CHORD_BASE_AMP_SHIFT - 1)), &scaleFract, &shift);

#ifndef STM32SYNTH_SIN_LUT
    // CORDIC

    // Baremetal Zero-Overhead mode
    q15_t *p_tmp_in_buff = (q15_t *)_radBuff;
    q15_t *p_tmp_out_buff = (q15_t *)buff;

    // Write of input data in Write Data register, and increment input buffer pointer
    cordicHW->Instance->WDATA = (int32_t)((*p_tmp_in_buff++) << 16);
    for (uint16_t index = (STM32SYNTH_HALF_NUM_SAMPLING - 1); index > 0; index--)
    {
        cordicHW->Instance->WDATA = (int32_t)((*p_tmp_in_buff++) << 16);
        (*p_tmp_out_buff++) = (q15_t)(cordicHW->Instance->RDATA & 0x0000FFFF);
    }
    (*p_tmp_out_buff) = (q15_t)(cordicHW->Instance->RDATA & 0x0000FFFF);

#else
    q15_t *buff_p = buff;
    q15_t *radBuff_p = _radBuff;

    for (uint16_t t = 0; t < STM32SYNTH_HALF_NUM_SAMPLING; t++)
    {
        uint16_t phase = (uint16_t)(*radBuff_p++);
        uint16_t quadrant = (uint16_t)(phase >> 14); // quadrant: top 2 bits of 16-bit phase
        uint16_t pos = (uint16_t)(phase & 0x3FFFu); // position within quadrant: lower 14 bits (0..16383)

        // if in mirrored quadrant (1 or 3), mirror the position for lookup
        if (quadrant & 1u)
        {
            pos = (uint16_t)(0x3FFFu - pos);
        }

        // index into 256-entry LUT and 6-bit fractional part
        uint16_t idx = (uint16_t)(pos >> 6);
        uint16_t frac = (uint16_t)(pos & 0x3Fu);

        int32_t v0 = (int32_t)sine_lut[idx];
        int32_t v1 = (int32_t)((idx < 255) ? sine_lut[idx + 1] : sine_lut[255]);
        int32_t diff = v1 - v0;

        // linear interpolation: v = v0 + diff * frac / 64
        int32_t interp = v0 + ((diff * (int32_t)frac) >> 6);

        // apply sign for lower half of circle (quadrant 2 and 3 are negative)
        if (quadrant >= 2u)
        {
            interp = -interp;
        }

        *buff_p++ = (q15_t)interp;
    }
#endif /* STM32SYNTH_SIN_LUT */

    arm_scale_q15(buff, scaleFract, shift, buff, STM32SYNTH_HALF_NUM_SAMPLING);
    arm_add_q15(_chordBuff, buff, _chordBuff, STM32SYNTH_HALF_NUM_SAMPLING);

    return res;
}

stm32synth_res_t stm32synth_chord_addsque(stm32synth_config_t *_config, stm32synth_chord_t *_configChord, q15_t *_chordBuff, q15_t *_radBuff, stm32synth_waveformnum_t _wnum)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    if (_config->waveform[_configChord->channel][_wnum].squ_level < 0.05f)
    {
        res = STM32SYNTH_RES_NG;
        return res;
    }

    if ((_config->waveform[_configChord->channel][_wnum].squ_duty <= 0.0f) || (_config->waveform[_configChord->channel][_wnum].squ_duty >= 1.0f))
    {
        res = STM32SYNTH_RES_NG;
        return res;
    }

    float32_t squ_duty, squ_level;
    squ_duty = _config->waveform[_configChord->channel][_wnum].squ_duty;
    squ_level = _config->waveform[_configChord->channel][_wnum].squ_level;

    q15_t buff[STM32SYNTH_HALF_NUM_SAMPLING] = {0};

    /*
    int8_t shift;
    q15_t scaleFract;
    stm32synth_component_f32toq15fract(squ_level, &scaleFract, &shift);
    //*/
    q15_t level = (int16_t)(STM32SYNTH_CHORD_BASE_AMP * squ_level);

    int32_t dutyHighRad_32 = (int32_t)(squ_duty * STM32SYNTH_Q15_MAX);
    q15_t dutyHighRad = (int16_t)(2 * dutyHighRad_32 - STM32SYNTH_Q15_MAX);

    for (uint16_t t = 0; t < STM32SYNTH_HALF_NUM_SAMPLING; t++)
    {
        if (_radBuff[t] < dutyHighRad)
        {
            buff[t] = level;
        }
        else
        {
            buff[t] = -level;
        }
    }
    //arm_scale_q15(buff, scaleFract, shift, buff, STM32SYNTH_HALF_NUM_SAMPLING);
    arm_add_q15(_chordBuff, buff, _chordBuff, STM32SYNTH_HALF_NUM_SAMPLING);

    return res;
}

stm32synth_res_t stm32synth_chord_addtrgl(stm32synth_config_t *_config, stm32synth_chord_t *_configChord, q15_t *_chordBuff, q15_t *_radBuff, stm32synth_waveformnum_t _wnum)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    if ((_config->waveform[_configChord->channel][_wnum].tri_level < 0.05f) || (_config->waveform[_configChord->channel][_wnum].tri_peak < 0.0f) || (_config->waveform[_configChord->channel][_wnum].tri_peak > 1.0f))
    {
        res = STM32SYNTH_RES_NG;
        return res;
    }

    q15_t buff[STM32SYNTH_HALF_NUM_SAMPLING] = {0};

    /*
    int8_t shift;
    q15_t scaleFract;
    stm32synth_component_f32toq15fract(_config->waveform[_configChord->channel][_wnum].tri_level, &scaleFract, &shift);
    //*/
    float32_t tri_level = _config->waveform[_configChord->channel][_wnum].tri_level;

    float32_t peakPoint_f32 = _config->waveform[_configChord->channel][_wnum].tri_peak * (float32_t)STM32SYNTH_Q15_MAX;
    q15_t peakPoint_q15 = (int16_t)(2.0f * peakPoint_f32 - (float32_t)STM32SYNTH_Q15_MAX);

    float32_t temp_denominator = (float32_t)STM32SYNTH_Q15_MAX - peakPoint_f32;

    float32_t doubel_amp_up = tri_level * (float32_t)(STM32SYNTH_CHORD_BASE_AMP) / peakPoint_f32;
    float32_t doubel_intercept_up = doubel_amp_up * temp_denominator;
    float32_t doubel_amp_down = -tri_level * (float32_t)(STM32SYNTH_CHORD_BASE_AMP) / temp_denominator;
    float32_t doubel_intercept_down = doubel_amp_down * peakPoint_f32;

    for (uint16_t t = 0; t < STM32SYNTH_HALF_NUM_SAMPLING; t++)
    {
        if (_radBuff[t] < peakPoint_q15)
        {
            buff[t] = (int16_t)(doubel_amp_up * (float32_t)_radBuff[t] + doubel_intercept_up);
        }
        else
        {
            buff[t] = (int16_t)(doubel_amp_down * (float32_t)_radBuff[t] - doubel_intercept_down);
        }
    }

    //arm_scale_q15(buff, scaleFract, shift, buff, STM32SYNTH_HALF_NUM_SAMPLING);
    arm_add_q15(_chordBuff, buff, _chordBuff, STM32SYNTH_HALF_NUM_SAMPLING);

    return res;
}

stm32synth_res_t stm32synth_chord_addnoise(stm32synth_config_t *_configg, float32_t _amp, q15_t *_chordBuff)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    q15_t buff[STM32SYNTH_SAMPLE_FORFILT] = {0};

    // scale
    float32_t scale = _amp * 0.007f;
    int8_t shift;
    q15_t scaleFract;
    stm32synth_component_f32toq15fract(scale, &scaleFract, &shift);

    // lightweight rand: xorshift32
    static uint32_t xs_state = 0;
    if (xs_state == 0)
    {
        xs_state = 0xA5A5A5A5u; //seed
    }

    for (uint16_t t = 0; t < STM32SYNTH_SAMPLE_FORFILT; t += 2)
    {
        // xorshift32 -- 3 xor/shift ops
        xs_state ^= xs_state << 13;
        xs_state ^= xs_state >> 17;
        xs_state ^= xs_state << 5;

        uint16_t lo = (uint16_t)(xs_state & 0xFFFFu);
        uint16_t hi = (uint16_t)(xs_state >> 16);

        buff[t]     = (q15_t)((int16_t)lo);
        buff[t + 1] = (q15_t)((int16_t)hi);
    }
    
    arm_scale_q15(buff, scaleFract, shift, buff, STM32SYNTH_SAMPLE_FORFILT);
    arm_add_q15(_chordBuff, buff, _chordBuff, STM32SYNTH_SAMPLE_FORFILT);

    return res;
}

#ifdef STM32SYNTH_DRUM_TESTMODE
stm32synth_res_t stm32synth_drum_change_to_test(uint8_t _midiCC, uint8_t _val)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    switch (_midiCC)
    {
    case STM32SYNTH_MIDICC_USER_DRUM_NN:
        drumConfigList[0].nn = _val;
        break;

    case STM32SYNTH_MIDICC_USER_DRUM_TYPE:
        drumConfigList[0].waveformType = _val;
        break;

    case STM32SYNTH_MIDICC_USER_DRUM_LPF_FCNN:
        drumConfigList[0].lpffc_nn = (_val == 127) ? (STM32SYNTH_MIDINN_FILTER_DISABLE) : (_val);
        break;

    case STM32SYNTH_MIDICC_USER_DRUM_HPF_FCNN:
        drumConfigList[0].hpffc_nn = (_val == 0) ? (STM32SYNTH_MIDINN_FILTER_DISABLE) : (_val);
        break;

    case STM32SYNTH_MIDICC_USER_DRUM_LPF_Q:
        drumConfigList[0].lpfq = ((float32_t)_val / 127.0f) * 2.0f;
        break;

    case STM32SYNTH_MIDICC_USER_DRUM_HPF_Q:
        drumConfigList[0].hpfq = ((float32_t)_val / 127.0f) * 2.0f;
        break;

    default:
        break;
    }

    return res;
}

stm32synth_res_t stm32synth_drum_init()
{
    drumConfigList[0].waveformType = STM32SYNTH_CHORD_DRUM_TYPE_SIN | STM32SYNTH_CHORD_DRUM_TYPE_SQU | STM32SYNTH_CHORD_DRUM_TYPE_TRI | STM32SYNTH_CHORD_DRUM_TYPE_NOISE;
    return STM32SYNTH_RES_OK;
}
#endif /* STM32SYNTH_DRUM_TESTMODE */

#ifdef STM32SYNTH_DRUM_TESTMODE
static stm32synth_config_drum_t drumConfigList[STM32SYNTH_DRUMCHORD_NUMBER] = {
    {STM32SYNTH_MIDINN_D3, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN | STM32SYNTH_CHORD_DRUM_TYPE_SQU | STM32SYNTH_CHORD_DRUM_TYPE_TRI | STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 0.717, 1, 0}, // NoteNum:34 Test
#else
static const stm32synth_config_drum_t drumConfigList[STM32SYNTH_DRUMCHORD_NUMBER] = {
#endif /* STM32SYNTH_DRUM_TESTMODE */
    // NN, PITCHforWave2, LPF_FCNN, HPF_FCNN, TYPE, DISTORTION, LPF_Q, HPF_Q, WN_Amp, freqenv_diff
    {STM32SYNTH_MIDINN_D3, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 110, 0.717, 0.717, 1, -11776},                 // NoteNum:35 Acoustic Bass Drum
    {STM32SYNTH_MIDINN_D3, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 110, 0.717, 0.717, 1, -11776},                 // NoteNum:36 Drum1
    {STM32SYNTH_MIDINN_G4, 0, STM32SYNTH_MIDINN_D7, STM32SYNTH_MIDINN_E7, STM32SYNTH_CHORD_DRUM_TYPE_TRI | STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 2, 2, 2, 0},                   // NoteNum:37 Side Stick
    {STM32SYNTH_MIDINN_F3, 1024, STM32SYNTH_MIDINN_C9, STM32SYNTH_MIDINN_A7, STM32SYNTH_CHORD_DRUM_TYPE_SQU | STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 64, 2, 2, 0.5, 0},               // NoteNum:38 Acoustic Snare
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_C6, STM32SYNTH_MIDINN_B5, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 2, 2, 2, 0},                                                  // NoteNum:39 Hand Clap
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_DS7, STM32SYNTH_MIDINN_G7, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 2, 2, 2, 0},                                                 // NoteNum:40 Electric Snare
    {STM32SYNTH_MIDINN_E2, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 90, 2, 2, 1, -4864},                           // NoteNum:41 Low Floor Tom
    {STM32SYNTH_MIDINN_GS5, 572, STM32SYNTH_MIDINN_FS9, STM32SYNTH_MIDINN_D9, STM32SYNTH_CHORD_DRUM_TYPE_SQU | STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 2, 2, 0.5, 0},             // NoteNum:42 Closed Hi-Hat
    {STM32SYNTH_MIDINN_GS2, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 90, 2, 2, 1, -4864},                          // NoteNum:43 High Floor Tom
    {STM32SYNTH_MIDINN_GS5, 572, STM32SYNTH_MIDINN_FS9, STM32SYNTH_MIDINN_D9, STM32SYNTH_CHORD_DRUM_TYPE_SQU | STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 2, 2, 1, 0},               // NoteNum:44 Pedal Hi-Hat
    {STM32SYNTH_MIDINN_FS3, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 90, 2, 2, 1, -4864},                          // NoteNum:45 Low Tom
    {STM32SYNTH_MIDINN_GS5, 572, STM32SYNTH_MIDINN_FS9, STM32SYNTH_MIDINN_D9, STM32SYNTH_CHORD_DRUM_TYPE_SQU | STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 2, 2, 1.5, 0},             // NoteNum:46 Open Hi-Hat
    {STM32SYNTH_MIDINN_A2, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_TRI, 90, 2, 2, 1, -4864},                           // NoteNum:47 Low Mid Tom
    {STM32SYNTH_MIDINN_D3, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_TRI, 90, 2, 2, 1, -4864},                           // NoteNum:48 Hi Mid Tom
    {STM32SYNTH_MIDINN_C3, 80, STM32SYNTH_MIDINN_E9, STM32SYNTH_MIDINN_AS6, STM32SYNTH_CHORD_DRUM_TYPE_SQU | STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 77, 2, 1.5, 2, 0},                // NoteNum:49 Crash Cymbal1
    {STM32SYNTH_MIDINN_AS3, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 90, 2, 2, 1, -4864},                          // NoteNum:50 High Tom
    {STM32SYNTH_MIDINN_GS5, 572, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_DS8, STM32SYNTH_CHORD_DRUM_TYPE_SQU | STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 2, 2, 0.5, 0}, // NoteNum:51 Ride Cymbal1
    {STM32SYNTH_MIDINN_AS4, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_E7, STM32SYNTH_CHORD_DRUM_TYPE_SQU | STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 2, 2, 1, 0},      // NoteNum:52 Chinese Cymbal
    {STM32SYNTH_MIDINN_AS5, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_G8, STM32SYNTH_CHORD_DRUM_TYPE_SQU | STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 2, 2, 1, 0},      // NoteNum:53 Ride Bell
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_E9, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 2, 2, 1, 0},                                      // NoteNum:54 Tambourine
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_G6, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 2, 2, 1, 0},                                      // NoteNum:55 Splash Cymbal
    {STM32SYNTH_MIDINN_GS3, 1024, STM32SYNTH_MIDINN_CS6, STM32SYNTH_MIDINN_CS6, STM32SYNTH_CHORD_DRUM_TYPE_SQU, 128, 2, 2, 1, 0},                                                // NoteNum:56 Cowbell
    {STM32SYNTH_MIDINN_D3, 80, STM32SYNTH_MIDINN_E9, STM32SYNTH_MIDINN_AS6, STM32SYNTH_CHORD_DRUM_TYPE_SQU | STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 77, 2, 1.5, 2, 0},                // NoteNum:57 Crash Cymbal2
    {STM32SYNTH_MIDINN_GS3, 1024, STM32SYNTH_MIDINN_CS6, STM32SYNTH_MIDINN_CS6, STM32SYNTH_CHORD_DRUM_TYPE_SQU, 128, 2, 2, 1, 0},                                                // NoteNum:58 Vibraslap
    {STM32SYNTH_MIDINN_AS5, 572, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_DS8, STM32SYNTH_CHORD_DRUM_TYPE_SQU | STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 2, 2, 0.5, 0}, // NoteNum:59 Ride Cymbal2
    {STM32SYNTH_MIDINN_E4, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 128, 0.717, 0.717, 1, 0},                      // NoteNum:60 Hi Bongo
    {STM32SYNTH_MIDINN_E3, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 128, 0.717, 0.717, 1, 0},                      // NoteNum:61 Low Bongo
    {STM32SYNTH_MIDINN_E4, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 128, 0.717, 0.717, 1, 0},                      // NoteNum:62 Mute Hi Conga
    {STM32SYNTH_MIDINN_E5, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 128, 0.717, 0.717, 1, 0},                      // NoteNum:63 Open Hi Conga
    {STM32SYNTH_MIDINN_D4, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 128, 0.717, 0.717, 1, 0},                      // NoteNum:64 Low Conga
    {STM32SYNTH_MIDINN_DS4, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 128, 0.717, 0.717, 1, 0},                     // NoteNum:65 High Timbale
    {STM32SYNTH_MIDINN_C3, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 128, 0.717, 0.717, 1, 0},                      // NoteNum:66 Low Timbale
    {STM32SYNTH_MIDINN_CS8, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 128, 0.717, 0.717, 1, 0},                     // NoteNum:67 High Agogo
    {STM32SYNTH_MIDINN_B6, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_SIN, 128, 0.717, 0.717, 1, 0},                      // NoteNum:68 Low Agogo
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_AS5, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 0.717, 1, 0},                             // NoteNum:69 Cabassa
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 2, 1, 0},                      // NoteNum:70 Maracas
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 0.717, 1, 0},                  // NoteNum:71 Short Whiatle
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 0.717, 1, 0},                  // NoteNum:72 Long Whistle
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 0.717, 1, 0},                  // NoteNum:73 Short Guiro
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 0.717, 1, 0},                  // NoteNum:74 Long Guiro
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 0.717, 1, 0},                  // NoteNum:75 Claves
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 0.717, 1, 0},                  // NoteNum:76 Hi Wood Block
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 0.717, 1, 0},                  // NoteNum:77 Low Wood Block
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 0.717, 1, 0},                  // NoteNum:78 Mute Cuica
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 0.717, 1, 0},                  // NoteNum:79 Open Cuica
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 0.717, 1, 0},                  // NoteNum:80 Mute Triangle
    {STM32SYNTH_MIDINN_NONE, 0, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_MIDINN_FILTER_DISABLE, STM32SYNTH_CHORD_DRUM_TYPE_NOISE, 128, 0.717, 0.717, 1, 0},                  // NoteNum:81 Open Triangle
};

#ifdef STM32SYNTH_SIN_LUT
// Define sine lookup table (quarter wave 0 to π/2)
static const q15_t sine_lut[256] = {
    0x0000, 0x00C9, 0x0192, 0x025B, 0x0324, 0x03ED, 0x04B6, 0x057F,
    0x0648, 0x0711, 0x07D9, 0x08A2, 0x096A, 0x0A33, 0x0AFB, 0x0BC4,
    0x0C8C, 0x0D54, 0x0E1C, 0x0EE3, 0x0FAB, 0x1072, 0x113A, 0x1201,
    0x12C8, 0x138F, 0x1456, 0x151C, 0x15E2, 0x16A8, 0x176E, 0x1833,
    0x18F9, 0x19BE, 0x1A82, 0x1B47, 0x1C0B, 0x1CCF, 0x1D93, 0x1E57,
    0x1F1A, 0x1FDD, 0x209F, 0x2161, 0x2223, 0x22E5, 0x23A6, 0x2467,
    0x2528, 0x25E8, 0x26A8, 0x2767, 0x2826, 0x28E5, 0x29A3, 0x2A61,
    0x2B1F, 0x2BDC, 0x2C99, 0x2D55, 0x2E11, 0x2ECC, 0x2F87, 0x3041,
    0x30FB, 0x31B5, 0x326E, 0x3326, 0x33DE, 0x3496, 0x354D, 0x3604,
    0x36BA, 0x376F, 0x3824, 0x38D9, 0x398C, 0x3A40, 0x3AF2, 0x3BA5,
    0x3C56, 0x3D07, 0x3DB8, 0x3E68, 0x3F17, 0x3FC5, 0x4073, 0x4121,
    0x41CE, 0x427A, 0x4325, 0x43D0, 0x447A, 0x4524, 0x45CD, 0x4675,
    0x471C, 0x47C3, 0x4869, 0x490F, 0x49B4, 0x4A58, 0x4AFB, 0x4B9E,
    0x4C40, 0x4CE1, 0x4D81, 0x4E21, 0x4EC0, 0x4F5E, 0x4FFB, 0x5098,
    0x5134, 0x51CF, 0x5269, 0x5302, 0x539B, 0x5433, 0x54CA, 0x5560,
    0x55F5, 0x568A, 0x571D, 0x57B0, 0x5842, 0x58D3, 0x5964, 0x59F3,
    0x5A82, 0x5B10, 0x5B9D, 0x5C29, 0x5CB4, 0x5D3E, 0x5DC7, 0x5E50,
    0x5ED7, 0x5F5E, 0x5FE3, 0x6068, 0x60EC, 0x616F, 0x61F1, 0x6272,
    0x62F2, 0x6371, 0x63EF, 0x646C, 0x64E8, 0x6563, 0x65DD, 0x6656,
    0x66CF, 0x6746, 0x67BC, 0x6832, 0x68A6, 0x6919, 0x698B, 0x69FD,
    0x6A6D, 0x6ADC, 0x6B4A, 0x6BB8, 0x6C24, 0x6C8F, 0x6CF9, 0x6D62,
    0x6DCA, 0x6E31, 0x6E97, 0x6EFC, 0x6F5F, 0x6FC2, 0x7023, 0x7083,
    0x70E2, 0x7140, 0x719D, 0x71F9, 0x7254, 0x72AE, 0x7307, 0x735E,
    0x73B5, 0x740A, 0x745E, 0x74B1, 0x7504, 0x7555, 0x75A5, 0x75F3,
    0x7641, 0x768D, 0x76D8, 0x7722, 0x776B, 0x77B3, 0x77FA, 0x783F,
    0x7884, 0x78C7, 0x7909, 0x794A, 0x798A, 0x79C9, 0x7A06, 0x7A42,
    0x7A7D, 0x7AB6, 0x7AEF, 0x7B26, 0x7B5C, 0x7B91, 0x7BC5, 0x7BF8,
    0x7C29, 0x7C59, 0x7C88, 0x7CB6, 0x7CE3, 0x7D0E, 0x7D39, 0x7D62,
    0x7D89, 0x7DB0, 0x7DD5, 0x7DFA, 0x7E1D, 0x7E3E, 0x7E5F, 0x7E7E,
    0x7E9C, 0x7EB9, 0x7ED5, 0x7EEF, 0x7F09, 0x7F21, 0x7F37, 0x7F4D,
    0x7F61, 0x7F74, 0x7F86, 0x7F97, 0x7FA6, 0x7FB4, 0x7FC1, 0x7FCD,
    0x7FD8, 0x7FE1, 0x7FE9, 0x7FF0, 0x7FF6, 0x7FFA, 0x7FFD, 0x7FFF
};
#endif /* STM32SYNTH_SIN_LUT */
