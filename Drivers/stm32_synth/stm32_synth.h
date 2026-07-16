/*
 * stm32_synth.h
 *
 *  Created on: Feb 9, 2024
 *      Author: k-omura
 */

#ifndef STM32_SYNTH_H_
#define STM32_SYNTH_H_

// include
#include "main.h"
#include "stm32_synth_userparam.h"

#ifndef ARM_MATH_CM4
#define ARM_MATH_CM4
#include "arm_math.h"
#endif

//- user start
//- user end
// include end

// define
#define STM32SYNTH_HALF_NUM_SAMPLING (512)                                                    //!< Must be power of 4.
#define STM32SYNTH_GND_LEVEL (STM32SYNTH_AMP_MAX >> 1)                                        //!< audio GND level(half to max)
#define STM32SYNTH_HALF_NUM_SAMPLING_BY_4 (STM32SYNTH_HALF_NUM_SAMPLING >> 2)                 //!< STM32SYNTH_HALF_NUM_SAMPLING / 4
#define STM32SYNTH_NUM_SAMPLING (STM32SYNTH_HALF_NUM_SAMPLING + STM32SYNTH_HALF_NUM_SAMPLING) //!< buff size for mono
#define STM32SYNTH_NUM_I2SBUFF (STM32SYNTH_NUM_SAMPLING + STM32SYNTH_NUM_SAMPLING)            //!< buff size for i2s (L+R)
#define STM32SYNTH_MAX_FREQ (STM32SYNTH_SAMPLE_FREQ >> 1)                                     //!< maximum frequency (Hz)
#define STM32SYNTH_MAX_FREQ_NOTE (34579)                                                      //!< maximum frequency note
#define STM32SYNTH_MIN_FREQ (20)                                                              //!< minimum frequency (Hz)
#define STM32SYNTH_MAX_CHORD (32)                                                             //!< maximum number of chords
#define STM32SYNTH_CHANNEL_NUMBER (16)                                                        //!< Number of MIDI channels
#define STM32SYNTH_WAVEFORM_NUM_PERCHORD (2)                                                  //!< Number of waveforms per chord
#define STM32SYNTH_REVERB_NUM (4)                                                             //!< Number of reverb buffers

#ifndef STM32SYNTH_DRUM_TESTMODE
#define STM32SYNTH_DRUMCHORD_NUMBER (47) //!< Number of drum chords
#else
#define STM32SYNTH_DRUMCHORD_NUMBER (48) //!< Number of drum chords. One extra for test mode.
#endif                                   /* STM32SYNTH_DRUM_TESTMODE */

#define STM32SYNTH_M_PIF32 (3.141592653589793f)   //!< Pi for float32
#define STM32SYNTH_DOUBLE_PI (6.283185307179586f) //!< Double Pi for float32
#define STM32SYNTH_Q15_RANGE (0xFFFF)             //!< Range of q15_t (16-bit signed fixed point)
#define STM32SYNTH_Q15_MAX (0x7FFF)               //!< Maximum positive value of q15_t (32767)

//- user start
//- user end
// define end

// enum
/**
 * @brief Result type for STM32 Synthesizer API functions
 */
typedef enum
{
    STM32SYNTH_RES_OK = 0,
    STM32SYNTH_RES_NG,
} stm32synth_res_t;

/**
 * @brief MIDI note numbers for special channels
 */
typedef enum
{
    STM32SYNTH_PARA_NOCHANGE = 0,
    STM32SYNTH_PARA_CHANGE,
} stm32synth_parastate_t;

/**
 * @brief MIDI note numbers for special channels
 */
typedef enum
{
    STM32SYNTH_LFO_SQU = 0,
    STM32SYNTH_LFO_TRI,
    STM32SYNTH_LFO_SIN,
} stm32synth_lfowaveform_t;

/**
 * @brief Phonic modes
 */
typedef enum
{
    STM32SYNTH_PHONIC_POLY_SUSTON = 0,
    STM32SYNTH_PHONIC_POLY,
    STM32SYNTH_PHONIC_MONO_SUSTON,
    STM32SYNTH_PHONIC_MONO,
} stm32synth_phonic_t;

/**
 * @brief Multi-channel modes
 */
typedef enum
{
    STM32SYNTH_MULTICH_NONE = 0,
    STM32SYNTH_MULTICH_ALLCHON,
} stm32synth_multich_t;

/**
 * @brief Waveform numbers
 */
typedef enum
{
    STM32SYNTH_WAVEFORM_0 = 0,
    STM32SYNTH_WAVEFORM_1,
} stm32synth_waveformnum_t;

//- user start
//- user end
// enum end

// struct
/**
 * @brief Configuration structure for filter settings per chord
 */
typedef struct
{
    uint8_t type; //!< Filter type (e.g., low-pass, band-pass, etc.)
    union Cutoff
    {
        int16_t relative;  //!< Relative cutoff frequency (in semitones, where 0 is the original frequency, positive values increase the cutoff frequency, and negative values decrease it)
        uint16_t absolute; //!< Absolute cutoff frequency (in MIDI note number format, where the value is the MIDI note number multiplied by 256, e.g., 60 << 8 for middle C)
    } cutoff_freq_nn;

    float32_t q_factor; //!< Q factor for the filter (higher values result in a narrower bandwidth around the cutoff frequency)

    // user add here start
    // user add here end
} stm32synth_config_filter_chord_t;

/**
 * @brief Configuration structure for filter settings (can be used for master filter or chord filter)
 */
typedef struct
{
#ifdef STM32SYNTH_FILT_CMSIS
    arm_biquad_casd_df1_inst_q15 instance;
    q15_t pCoeffs[6];
    q15_t pState[4];
#endif /* STM32SYNTH_FILT_CMSIS */

    stm32synth_config_filter_chord_t para;
    stm32synth_parastate_t state;

    // user add here start
    // user add here end
} stm32synth_config_filter_t;

/**
 * @brief Configuration structure for the entire synthesizer, including all channels, waveforms, envelopes, LFOs, and optional reverb and filter settings
 */
typedef struct
{
    float32_t val;

    stm32synth_lfowaveform_t type;
    float32_t rad;
    float32_t amp;
    float32_t freq;

    union Paramater
    {
        float32_t duty; // 0.0 - 1.0
        float32_t peak; // 0.0 - 1.0
    } para;

    // user add here start
    // user add here end
} stm32synth_config_lfo_t;

/**
 * @brief Configuration structure for the entire synthesizer, including all channels, waveforms, envelopes, LFOs, and optional reverb and filter settings
 */
typedef struct
{
    uint16_t attack_ms;
    uint16_t decay_ms;
    float32_t attack_level;
    uint16_t release_ms;

    // user add here start
    // user add here end
} stm32synth_config_adsr_t;

/**
 * @brief Configuration structure for the entire synthesizer, including all channels, waveforms, envelopes, LFOs, and optional reverb and filter settings
 */
typedef struct
{
    int16_t finish_value;
    uint16_t time_ms;

    // user add here start
    // user add here end
} stm32synth_config_envelopec_t;

/**
 * @brief Configuration structure for the entire synthesizer, including all channels, waveforms, envelopes, LFOs, and optional reverb and filter settings
 */
typedef struct
{
    stm32synth_config_envelopec_t volume;
    stm32synth_config_envelopec_t freq;
    stm32synth_config_envelopec_t cutoff;

    // user add here start
    // user add here end
} stm32synth_config_envelope_t;

/**
 * @brief Configuration structure for a single waveform within a chord, including levels for sine, square, and triangle components, as well as pitch offset
 */
typedef struct
{
    float32_t sin_level;
    float32_t squ_level;
    float32_t squ_duty;
    float32_t tri_level;
    float32_t tri_peak;
    int16_t pitch;

    // user add here start
    // user add here end
} stm32synth_config_waveform_t;

/**
 * @brief Configuration structure for the entire synthesizer, including all channels, waveforms, envelopes, LFOs, and optional reverb and filter settings
 */
#ifdef STM32SYNTH_REVERB
typedef struct
{
    float32_t level;

    q15_t scaleFract;
    int8_t shift;

    // user add here start
    // user add here end
} stm32synth_config_reverb_t;

#endif /* STM32SYNTH_REVERB */

/**
 * @brief Configuration structure for the entire synthesizer, including all channels, waveforms, envelopes, LFOs, and optional reverb and filter settings
 */
typedef struct
{
    float32_t r_level;
    float32_t l_level;

    q15_t r_scaleFract;
    int8_t r_shift;

    q15_t l_scaleFract;
    int8_t l_shift;

    // user add here start
    // user add here end
} stm32synth_config_pan_t;

/**
 * @brief Configuration structure for the entire synthesizer, including all channels, waveforms, envelopes, LFOs, and optional reverb and filter settings
 */
typedef struct
{
    struct Buff
    {
        q15_t *back;
#ifdef STM32SYNTH_REVERB
        q15_t (*reverb)[STM32SYNTH_HALF_NUM_SAMPLING];
#endif /* STM32SYNTH_REVERB */
        uint16_t *dac;
    } buff;

    float32_t volume[STM32SYNTH_CHANNEL_NUMBER];
    int16_t pitch[STM32SYNTH_CHANNEL_NUMBER];
    int16_t distortion[STM32SYNTH_CHANNEL_NUMBER];

    stm32synth_config_waveform_t waveform[STM32SYNTH_CHANNEL_NUMBER][STM32SYNTH_WAVEFORM_NUM_PERCHORD];

    stm32synth_config_adsr_t adsr[STM32SYNTH_CHANNEL_NUMBER];
    stm32synth_config_envelope_t envelope[STM32SYNTH_CHANNEL_NUMBER];
    stm32synth_phonic_t phonic[STM32SYNTH_CHANNEL_NUMBER];
    stm32synth_config_pan_t pan;

    stm32synth_config_lfo_t vib[STM32SYNTH_CHANNEL_NUMBER];
    stm32synth_config_lfo_t tre[STM32SYNTH_CHANNEL_NUMBER];

    stm32synth_config_lfo_t vib_master, tre_master;

#ifdef STM32SYNTH_REVERB
    stm32synth_config_reverb_t reverb;
#endif /* STM32SYNTH_REVERB */

    uint16_t ch0NoteMirror;

#ifdef STM32SYNTH_FILTER
#ifdef STM32SYNTH_CHORDFILTER
    stm32synth_config_filter_chord_t filter[STM32SYNTH_CHANNEL_NUMBER];
    stm32synth_config_lfo_t wow[STM32SYNTH_CHANNEL_NUMBER];
#endif
    stm32synth_config_filter_t filter_master;
    stm32synth_config_lfo_t wow_master;
#endif /* STM32SYNTH_FILTER */

    // user add here start
    // user add here end
} stm32synth_config_t;

//- user start
//- user end
// struct end

// function pointer

// api-----------------------
//- default start
stm32synth_res_t stm32synth_init(
    uint16_t *_dacBuff
#ifdef STM32SYNTH_SIN_CORDIC
    ,
    CORDIC_HandleTypeDef *_cordicHW
#endif /* STM32SYNTH_SIN_CORDIC*/
);

stm32synth_res_t stm32synth_hundleloop();
stm32synth_res_t stm32synth_dacDmaCmplt_hundle();
stm32synth_res_t stm32synth_dacDmaHalfCmplt_hundle();

stm32synth_res_t stm32synth_getConfig(stm32synth_config_t *_configBufftoCopy);
stm32synth_res_t stm32synth_setConfig(stm32synth_config_t *_configBuff);

stm32synth_res_t stm32synth_program(uint8_t _ch, uint8_t _program);
stm32synth_res_t stm32synth_multich(stm32synth_multich_t _preset);

stm32synth_res_t stm32synth_inputMIDI(uint8_t *_rcvdMidi);
stm32synth_res_t stm32synth_midi_writebuff(uint8_t *_midi);
stm32synth_res_t stm32synth_midi_readbuff(uint8_t *_midi);
stm32synth_res_t stm32synth_midi_buff2input();
//- default end

//- user start
//- user end
//--------------------------

#endif /* STM32F4_SYNTH_STM32F4_SYNTH_H_ */
