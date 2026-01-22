/*
 * stm32_synth_midi.c
 *
 *  Created on: Feb 11, 2024
 *      Author: k-omura
 */

#include "stm32_synth.h"
#include "components/stm32_synth_midi.h"
#include "components/stm32_synth_component.h"
#include "components/stm32_synth_program.h"
#include "components/stm32_synth_flashconfig.h"
#include "components/stm32_synth_dfu.h"

static stm32synth_midiinput_ringbuff_t midibuff;
static stm32synth_midi_cablenumfilt_t cablenumfilt = STM32SYNTH_CABLENUMFILT_OFF;

stm32synth_res_t stm32synth_midi_inputmidiCC(stm32synth_config_t *_config, uint8_t _cablenum, uint8_t _ch, uint8_t _cc, uint8_t _val);

stm32synth_res_t stm32synth_component_inputmidi(stm32synth_config_t *_config, uint8_t *_midi)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    uint8_t cableNumber = _midi[0] >> 4;
    uint8_t cIndex = _midi[0] & 0x0f;
    uint8_t evnt0 = _midi[1];
    uint8_t evnt1 = _midi[2];
    uint8_t evnt2 = _midi[3];

    stm32synth_midi_evnt0_t message = (stm32synth_midi_evnt0_t)((evnt0 & 0xF0) >> 4);
    uint8_t channel = evnt0 & 0x0F;

    if ((message == STM32SYNTH_MIDIEVT0_NOTEON) && (evnt2 == 0))
    {
        message = STM32SYNTH_MIDIEVT0_NOTEOFF;
    }

    switch (message)
    {
    case STM32SYNTH_MIDIEVT0_NOTEOFF: // Note off
    {
        stm32synth_chord_t inputChord;
        inputChord.channel = channel;
        inputChord.noteNum[0] = evnt1 << 8;

        uint16_t ch0NoteMirror = _config->ch0NoteMirror;
        if ((channel == 0) && (ch0NoteMirror > 1))
        {
            for (uint8_t i = 0; i < 16; i++)
            {
                if (ch0NoteMirror & 0b0000000000000001)
                {
                    inputChord.channel = i;
                    stm32synth_component_noteoffChord(_config, &inputChord);
                }
                ch0NoteMirror >>= 1;
            }
        }
        else
        {
            stm32synth_component_noteoffChord(_config, &inputChord);
        }

        break;
    }
    case STM32SYNTH_MIDIEVT0_NOTEON: // Note on
    {
        stm32synth_chord_t inputChord;
        inputChord.channel = channel;
        inputChord.noteNum[0] = evnt1 << 8;
        inputChord.velocity = (float32_t)evnt2 / 127.0f;

        uint16_t ch0NoteMirror = _config->ch0NoteMirror;
        if ((channel == 0) && (ch0NoteMirror > 1))
        {
            for (uint8_t i = 0; i < 16; i++)
            {
                if (ch0NoteMirror & 0b0000000000000001)
                {
                    inputChord.channel = i;
                    stm32synth_component_noteonChord(_config, &inputChord);
                }
                ch0NoteMirror >>= 1;
            }
        }
        else
        {
            stm32synth_component_noteonChord(_config, &inputChord);
        }

        break;
    }
    case STM32SYNTH_MIDIEVT0_AFTERTOUCH: // Aftertouch
    {
        break;
    }
    case STM32SYNTH_MIDIEVT0_PITCHBEND: // Pitch Bend
    {
        if (evnt2 == 0)
        {
            evnt2 = 1;
        }

        int16_t tmp = _config->pitch[channel] % 0x0100;
        tmp = _config->pitch[channel] - tmp;
        _config->pitch[channel] = tmp + (((int16_t)evnt2 - 0x40) * 4);
        break;
    }
    case STM32SYNTH_MIDIEVT0_PROGRAMCHANGE: // Program Change
    {
        stm32synth_program_set(_config, channel, evnt1);
        break;
    }
    case STM32SYNTH_MIDIEVT0_CONTROLCHANGE: // Control Change
    {
        if ((cablenumfilt == STM32SYNTH_CABLENUMFILT_ON_DEEP) && (cableNumber == 0) && (evnt1 != STM32SYNTH_MIDICC_USER_MODECONTROL))
        {
            break; // Ignore volume control when CableNum filter is on
        }

        stm32synth_midi_inputmidiCC(_config, cableNumber, channel, evnt1, evnt2);
        break;
    }
    default:
        break;
    }

    return res;
}

stm32synth_res_t stm32synth_component_midi_writebuff(uint8_t *_midi)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    memcpy(midibuff.buff[midibuff.wpos], _midi, 4);
    midibuff.wpos++;
    if (midibuff.wpos >= STM32SYNTH_MIDIBUFF_SIZE)
    {
        midibuff.wpos = 0;
    }

    return res;
}

stm32synth_res_t stm32synth_component_midi_readbuff(uint8_t *_midi)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    if (midibuff.rpos == midibuff.wpos)
    {
        return STM32SYNTH_RES_NG;
    }

    memcpy(_midi, midibuff.buff[midibuff.rpos], 4);
    midibuff.rpos++;
    if (midibuff.rpos >= STM32SYNTH_MIDIBUFF_SIZE)
    {
        midibuff.rpos = 0;
    }

    return res;
}

stm32synth_res_t stm32synth_component_midi_buff2input(stm32synth_config_t *_config)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

    while (midibuff.rpos != midibuff.wpos)
    {
        stm32synth_component_inputmidi(_config, midibuff.buff[midibuff.rpos]);
        midibuff.rpos++;
        if (midibuff.rpos >= STM32SYNTH_MIDIBUFF_SIZE)
        {
            midibuff.rpos = 0;
        }
    }

    return res;
}

// Private
stm32synth_res_t stm32synth_midi_inputmidiCC(stm32synth_config_t *_config, uint8_t _cablenum, uint8_t _ch, uint8_t _cc, uint8_t _val)
{
    stm32synth_res_t res = STM32SYNTH_RES_OK;

#ifndef STM32SYNTH_DRUM_TESTMODE
	if(_ch == STM32SYNTH_MIDINN_DRUMCH)
	{
		return res;
	}
#endif /* STM32SYNTH_DRUM_TESTMODE */

    switch (_cc)
    {
    case STM32SYNTH_MIDICC_MODWHELL: // modulation wheel
        float32_t freq = (float32_t)_val / 8.0f;

        uint16_t ch0NoteMirror = _config->ch0NoteMirror;
        if ((_ch == 0) && (ch0NoteMirror > 1))
        {
            for (uint8_t i = 0; i < 16; i++)
            {
                if (ch0NoteMirror & 0b0000000000000001)
                {
                    _config->vib[i].freq = freq;
                }
                ch0NoteMirror >>= 1;
            }
        }
        else
        {
            _config->vib[_ch].freq = freq;
        }

        break;

    case STM32SYNTH_MIDICC_VOLUME: // Volume for channel
    // case STM32SYNTH_MIDICC_EXPRESSION:
    {
        if ((cablenumfilt == STM32SYNTH_CABLENUMFILT_ON) && (_cablenum == 0))
        {
            break; // Ignore volume control when CableNum filter is on
        }

        float32_t vol = (float32_t)_val / 128.0f;

        uint16_t ch0NoteMirror = _config->ch0NoteMirror;
        if ((_ch == 0) && (ch0NoteMirror > 1))
        {
            for (uint8_t i = 0; i < 16; i++)
            {
                if (ch0NoteMirror & 0b0000000000000001)
                {
                    _config->volume[i] = vol;
                }
                ch0NoteMirror >>= 1;
            }
        }
        else
        {
            _config->volume[_ch] = vol;
        }
        break;
    }

    case STM32SYNTH_MIDICC_PAN:
        _config->pan.r_level = (float32_t)_val / 64.0f;
        _config->pan.l_level = (float32_t)(128 - _val) / 64.0f;

        stm32synth_component_f32toq15fract(_config->pan.l_level, &_config->pan.l_scaleFract, &_config->pan.l_shift);
        stm32synth_component_f32toq15fract(_config->pan.r_level, &_config->pan.r_scaleFract, &_config->pan.r_shift);

        break;

    case STM32SYNTH_MIDICC_USER_ATTACKTIME: // Attack time
        _config->adsr[_ch].attack_ms = (uint16_t)(_val * 1000.0f / 127.0f);
        break;

    case STM32SYNTH_MIDICC_USER_DECAYTIME: // Decay time
        _config->adsr[_ch].decay_ms = (uint16_t)(_val * 1000.0f / 127.0f);
        break;

    case STM32SYNTH_MIDICC_USER_ATTACKLEVEL: // Attack Level
        _config->adsr[_ch].attack_level = (float32_t)_val * 4.0f / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_RELEASETIME: // Release time
        _config->adsr[_ch].release_ms = (uint16_t)(_val * 500.0f / 127.0f);
        break;

    case STM32SYNTH_MIDICC_USER_WAVE1SINEAMP: // waveform sine amp
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_0].sin_level = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_WAVE1SQUAMP: // waveform squ amp
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_0].squ_level = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_WAVE1SQUDUTY: // waveform squ duty
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_0].squ_duty = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_WAVE1TRIAMP: // waveform tri amp
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_0].tri_level = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_WAVE1TRIPEAK: // waveform tri peak position
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_0].tri_peak = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_WAVE1PITCH: // waveform pitch
    {
        if (_val == 0)
        {
            _val = 1;
        }

        int16_t tmp = _config->waveform[_ch][STM32SYNTH_WAVEFORM_0].pitch % 0x0100;
        tmp = _config->waveform[_ch][STM32SYNTH_WAVEFORM_0].pitch - tmp;
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_0].pitch = tmp + (((int16_t)_val - 0x40) * 4);
        break;
    }

    case STM32SYNTH_MIDICC_USER_WAVE1OCTAVE: // waveform octave
        int16_t tmp = _config->waveform[_ch][STM32SYNTH_WAVEFORM_0].pitch % 0x0100;
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_0].pitch = tmp + (((int16_t)_val - 0x40) << 8);
        break;

    case STM32SYNTH_MIDICC_USER_WAVE2SINEAMP: // waveform sine amp
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_1].sin_level = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_WAVE2SQUAMP: // waveform squ amp
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_1].squ_level = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_WAVE2SQUDUTY: // waveform squ duty
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_1].squ_duty = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_WAVE2TRIAMP: // waveform tri amp
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_1].tri_level = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_WAVE2TRIPEAK: // waveform tri peak position
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_1].tri_peak = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_DISTORTION_LEVEL: // waveform distortion level
        _config->distortion[_ch] = _val + 1;
        break;

    case STM32SYNTH_MIDICC_USER_WAVE2PITCH: // waveform pitch
    {
        if (_val == 0)
        {
            _val = 1;
        }

        int16_t tmp = _config->waveform[_ch][STM32SYNTH_WAVEFORM_1].pitch % 0x0100;
        tmp = _config->waveform[_ch][STM32SYNTH_WAVEFORM_1].pitch - tmp;
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_1].pitch = tmp + (((int16_t)_val - 0x40) * 4);
        break;
    }

    case STM32SYNTH_MIDICC_USER_WAVE2OCTAVE: // waveform octave
    {
        int16_t tmp = _config->waveform[_ch][STM32SYNTH_WAVEFORM_1].pitch % 0x0100;
        _config->waveform[_ch][STM32SYNTH_WAVEFORM_1].pitch = tmp + (((int16_t)_val - 0x40) << 8);
        break;
    }

    case STM32SYNTH_MIDICC_USER_TRE_TYPE: // LFO TRE form type
        if (_val > STM32SYNTH_LFO_SIN)
        {
            _val = STM32SYNTH_LFO_SIN;
        }
        _config->tre[_ch].type = _val;
        break;

    case STM32SYNTH_MIDICC_USER_TRE_PEAK: // LFO TRE Peak/Duty
        _config->tre[_ch].para.peak = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_TRE_AMP: // LFO TRE amp
        _config->tre[_ch].amp = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_TRE_FREQ: // LFO TRE freq
        _config->tre[_ch].freq = (float32_t)_val / 8.0f;
        break;

    case STM32SYNTH_MIDICC_USER_VIB_TYPE: // LFO VIB form type
        if (_val > STM32SYNTH_LFO_SIN)
        {
            _val = STM32SYNTH_LFO_SIN;
        }
        _config->vib[_ch].type = _val;
        break;

    case STM32SYNTH_MIDICC_USER_VIB_PEAK: // LFO VIB Peak/Duty
        _config->vib[_ch].para.peak = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_VIB_AMP: // LFO VIB amp
        _config->vib[_ch].amp = _val;
        break;

    case STM32SYNTH_MIDICC_USER_VIB_FREQ: // LFO VIB freq
        _config->vib[_ch].freq = (float32_t)_val / 8.0f;
        break;

#ifdef STM32SYNTH_FILTER
#ifdef STM32SYNTH_CHORDFILTER
    case STM32SYNTH_MIDICC_USER_WOW_TYPE: // LFO WOW form type
        if (_val > STM32SYNTH_LFO_SIN)
        {
            _val = STM32SYNTH_LFO_SIN;
        }
        _config->wow[_ch].type = _val;
        break;

    case STM32SYNTH_MIDICC_USER_WOW_PEAK: // LFO WOW Peak/Duty
        _config->wow[_ch].para.peak = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_WOW_AMP: // LFO WOW amp
        _config->wow[_ch].amp = (uint16_t)_val << 6;
        break;

    case STM32SYNTH_MIDICC_USER_WOW_FREQ: // LFO WOW freq
        _config->wow[_ch].freq = (float32_t)_val / 8.0f;
        break;
#endif
#endif

    case STM32SYNTH_MIDICC_USER_MTRE_TYPE: // Master LFO TRE form type
        if (_val > STM32SYNTH_LFO_SIN)
        {
            _val = STM32SYNTH_LFO_SIN;
        }
        _config->tre_master.type = _val;
        break;

    case STM32SYNTH_MIDICC_USER_MTRE_PEAK: // Master LFO TRE Peak/Duty
        _config->tre_master.para.peak = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_MTRE_AMP: // Master LFO TRE amp
        _config->tre_master.amp = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_MTRE_FREQ: // Master LFO TRE freq
        _config->tre_master.freq = (float32_t)_val / 8.0f;
        break;

    case STM32SYNTH_MIDICC_USER_MVIB_TYPE: // Master LFO VIB form type
        if (_val > STM32SYNTH_LFO_SIN)
        {
            _val = STM32SYNTH_LFO_SIN;
        }
        _config->vib_master.type = _val;
        break;

    case STM32SYNTH_MIDICC_USER_MVIB_PEAK: // Master LFO VIB Peak/Duty
        _config->vib_master.para.peak = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_MVIB_AMP: // Master LFO VIB amp
        _config->vib_master.amp = _val;
        break;

    case STM32SYNTH_MIDICC_USER_MVIB_FREQ: // Master LFO VIB freq
        _config->vib_master.freq = (float32_t)_val / 8.0f;
        break;

#ifdef STM32SYNTH_FILTER
    case STM32SYNTH_MIDICC_USER_MWOW_TYPE: // Master LFO WOW form type
        if (_val > STM32SYNTH_LFO_SIN)
        {
            _val = STM32SYNTH_LFO_SIN;
        }
        _config->wow_master.type = _val;
        break;

    case STM32SYNTH_MIDICC_USER_MWOW_PEAK: // Master LFO WOW Peak/Duty
        _config->wow_master.para.peak = (float32_t)_val / 127.0f;
        break;

    case STM32SYNTH_MIDICC_USER_MWOW_AMP: // Master LFO WOW amp
        _config->wow_master.amp = (uint16_t)_val << 4;
        break;

    case STM32SYNTH_MIDICC_USER_MWOW_FREQ: // Master LFO WOW freq
        _config->wow_master.freq = (float32_t)_val / 8.0f;
        break;
#endif

    case STM32SYNTH_MIDICC_USER_ENV_VOL_DIFF: //
        _config->envelope[_ch].volume.finish_value = (int16_t)_val - 0x40;
        break;

    case STM32SYNTH_MIDICC_USER_ENV_VOL_TIME: //
        if ((cablenumfilt == STM32SYNTH_CABLENUMFILT_ON) && (_cablenum == 0))
        {
            break; // Ignore volume control when CableNum filter is on
        }
        _config->envelope[_ch].volume.time_ms = (uint16_t)(_val / 127.0f * 10000.0f);
        break;

    case STM32SYNTH_MIDICC_USER_ENV_FREQ_DIFF: //
        _config->envelope[_ch].freq.finish_value = (int16_t)(_val - 0x40) << ((_ch == STM32SYNTH_MIDINN_DRUMCH) ? (8) : (4));
        break;

    case STM32SYNTH_MIDICC_USER_ENV_FREQ_TIME: //
        _config->envelope[_ch].freq.time_ms = (uint16_t)(_val / 127.0f * 10000.0f);
        break;

    case STM32SYNTH_MIDICC_USER_ENV_WOW_DIFF: //
        _config->envelope[_ch].cutoff.finish_value = (int16_t)(_val - 0x40) << ((_ch == STM32SYNTH_MIDINN_DRUMCH) ? (8) : (4));
        break;

    case STM32SYNTH_MIDICC_USER_ENV_WOW_TIME: //
        _config->envelope[_ch].cutoff.time_ms = (uint16_t)(_val / 127.0f * 10000.0f);
        break;

#ifdef STM32SYNTH_FILTER
#ifdef STM32SYNTH_CHORDFILTER
    case STM32SYNTH_MIDICC_USER_FILTERTYPE: // Filter type
        if (_val > STM32SYNTH_FILTERTYPE_HPF)
        {
            _val = STM32SYNTH_FILTERTYPE_LPF;
        }
        _config->filter[_ch].type = _val;
        break;

    case STM32SYNTH_MIDICC_USER_FILTERQFACTOR: // LPF Q Factor (Resonance)
        _config->filter[_ch].q_factor = ((float32_t)_val / 127.0f) * 2.0f;
        break;

    case STM32SYNTH_MIDICC_USER_FILTERCUTOFF: // LPF cutoff freq
        //_config->lpf[_ch].cutoff_freq_nn.relative = (int16_t)(_val - 0x40);
        _config->filter[_ch].cutoff_freq_nn.absolute = (uint16_t)_val << 8;
        break;
#endif
    case STM32SYNTH_MIDICC_USER_MFILTER_TYPE: // Master Filter type
        if (_val != STM32SYNTH_FILTERTYPE_LSF)
        {
            _val = STM32SYNTH_FILTERTYPE_LPF;
        }
        _config->filter_master.para.type = _val;
        break;

    case STM32SYNTH_MIDICC_USER_MFILTER_QFACTOR: // Master LPF Q Factor
        _config->filter_master.para.q_factor = ((float32_t)_val / 127.0f) * 2.0f;
        break;

    case STM32SYNTH_MIDICC_USER_MFILTER_CUTOFF: // Master LPF cutoff freq
        _config->filter_master.para.cutoff_freq_nn.absolute = (uint16_t)_val << 8;
        break;
#endif /* STM32SYNTH_FILTER */

#ifdef STM32SYNTH_REVERB
    case STM32SYNTH_MIDICC_USER_REVERB_LEVEL: // Reverb Level
        if ((cablenumfilt == STM32SYNTH_CABLENUMFILT_ON) && (_cablenum == 0))
        {
            break; // Ignore reverb control when CableNum filter is on
        }
        _config->reverb.level = (float32_t)_val * 0.8f / 127.0f; // Make sure it is below 0.8
        stm32synth_component_f32toq15fract(_config->reverb.level, &_config->reverb.scaleFract, &_config->reverb.shift);
        break;
#endif /* STM32SYNTH_REVERB */

    case STM32SYNTH_MIDICC_USER_PRESET: // Set Multi Channel
        stm32synth_multich(_val);
        break;

    case STM32SYNTH_MIDICC_USER_PITCHBYNOTE: // Adjust pitch by note
    {
        int16_t tmp = _config->pitch[_ch] % 0x0100;
        _config->pitch[_ch] = tmp + (((int16_t)_val - 0x40) << 8);
        break;
    }

    case STM32SYNTH_MIDICC_USER_CH0MIRRORCH: // CH0 Mirror mode channel list bitmap
        _config->ch0NoteMirror = _val;
        break;

#ifdef STM32SYNTH_DRUM_TESTMODE
    case STM32SYNTH_MIDICC_USER_DRUM_NN:
    case STM32SYNTH_MIDICC_USER_DRUM_TYPE:
    case STM32SYNTH_MIDICC_USER_DRUM_LPF_Q:
    case STM32SYNTH_MIDICC_USER_DRUM_LPF_FCNN:
    case STM32SYNTH_MIDICC_USER_DRUM_HPF_Q:
    case STM32SYNTH_MIDICC_USER_DRUM_HPF_FCNN:
        stm32synth_drum_change_to_test(_cc, _val);
        break;
#endif /* STM32SYNTH_DRUM_TESTMODE */

    case STM32SYNTH_MIDICC_RESETALL: // default setting
        stm32synth_component_initChord();
        break;

    case STM32SYNTH_MIDICC_ALLSOUNDOFF: // all notes sound (immediately)
        stm32synth_component_disableChordAll();
        stm32synth_component_initChord();
        break;

    case STM32SYNTH_MIDICC_ALLNOTESOFF: // all notes off (with release)
        stm32synth_component_noteoffAll();
        stm32synth_component_initChord();
        break;

    case STM32SYNTH_MIDICC_MONOMODE: // Sets device mode to Monophonic
        _config->phonic[_ch] = (_val == 0) ? (STM32SYNTH_PHONIC_MONO_SUSTON) : (STM32SYNTH_PHONIC_MONO);
        break;

    case STM32SYNTH_MIDICC_POLYMODE: // Sets device mode to Polyphonic
        _config->phonic[_ch] = (_val == 0) ? (STM32SYNTH_PHONIC_POLY_SUSTON) : (STM32SYNTH_PHONIC_POLY);
        break;

    case STM32SYNTH_MIDICC_USER_MODECONTROL: // STM32 Mode Control
    {
        switch (_val)
        {
#ifdef SYNTH_USE_FLASH_CONFIG
        case STM32SYNTH_MODECONTROL_SAVECONFIG:
            stm32_synth_write_config_sector(_config);
            break;

        case STM32SYNTH_MODECONTROL_ERASECONFIG:
            stm32_synth_elase_config_sector();
            break;
#endif
        case STM32SYNTH_MODECONTROL_GOTODFU:
            stm32_synth_goto_dfu();
            break;

        case STM32SYNTH_MODECONTROL_CABLENUMFILTOFF:
            cablenumfilt = STM32SYNTH_CABLENUMFILT_OFF;
            break;

        case STM32SYNTH_MODECONTROL_CABLENUMFILTON:
            cablenumfilt = STM32SYNTH_CABLENUMFILT_ON;
            break;

        case STM32SYNTH_MODECONTROL_CABLENUMFILTONDEEP:
            cablenumfilt = STM32SYNTH_CABLENUMFILT_ON_DEEP;
            break;
        }

        break;
    }
    default:
        break;
    }

    return res;
}
