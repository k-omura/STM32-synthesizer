/*
 * stm32_synth_midi.h
 *
 *  Created on: Apr 7, 2024
 *      Author: k-omura
 */

#include "stm32_synth.h"

#ifndef STM32_SYNTH_COMPONENTS_STM32_SYNTH_MIDI_H_
#define STM32_SYNTH_COMPONENTS_STM32_SYNTH_MIDI_H_

#define STM32SYNTH_MIDIBUFF_SIZE (256)
#define STM32SYNTH_MIDINN_DRUMCH (9)

#ifndef STM32SYNTH_DRUM_TESTMODE
#define STM32SYNTH_MIDINN_LOWESTDRUM (35)
#else
#define STM32SYNTH_MIDINN_LOWESTDRUM (34)
#endif /* STM32SYNTH_DRUM_TESTMODE */

#define STM32SYNTH_MIDINN_HIGHESTDRUM (STM32SYNTH_MIDINN_LOWESTDRUM + STM32SYNTH_DRUMCHORD_NUMBER)

// enum
typedef enum
{
	STM32SYNTH_MIDINN_NONE = 0,
	STM32SYNTH_MIDINN_CM1 = 0,
	STM32SYNTH_MIDINN_CSM1,
	STM32SYNTH_MIDINN_DM1,
	STM32SYNTH_MIDINN_DSM1,
	STM32SYNTH_MIDINN_EM1,
	STM32SYNTH_MIDINN_FM1,
	STM32SYNTH_MIDINN_FSM1,
	STM32SYNTH_MIDINN_GM1,
	STM32SYNTH_MIDINN_GSM1,
	STM32SYNTH_MIDINN_AM1,
	STM32SYNTH_MIDINN_ASM1,
	STM32SYNTH_MIDINN_BM1,
	STM32SYNTH_MIDINN_C0,
	STM32SYNTH_MIDINN_CS0,
	STM32SYNTH_MIDINN_D0,
	STM32SYNTH_MIDINN_DS0,
	STM32SYNTH_MIDINN_E0,
	STM32SYNTH_MIDINN_F0,
	STM32SYNTH_MIDINN_FS0,
	STM32SYNTH_MIDINN_G0,
	STM32SYNTH_MIDINN_GS0,
	STM32SYNTH_MIDINN_A0,
	STM32SYNTH_MIDINN_AS0,
	STM32SYNTH_MIDINN_B0,
	STM32SYNTH_MIDINN_C1,
	STM32SYNTH_MIDINN_CS1,
	STM32SYNTH_MIDINN_D1,
	STM32SYNTH_MIDINN_DS1,
	STM32SYNTH_MIDINN_E1,
	STM32SYNTH_MIDINN_F1,
	STM32SYNTH_MIDINN_FS1,
	STM32SYNTH_MIDINN_G1,
	STM32SYNTH_MIDINN_GS1,
	STM32SYNTH_MIDINN_A1,
	STM32SYNTH_MIDINN_AS1,
	STM32SYNTH_MIDINN_B1,
	STM32SYNTH_MIDINN_C2,
	STM32SYNTH_MIDINN_CS2,
	STM32SYNTH_MIDINN_D2,
	STM32SYNTH_MIDINN_DS2,
	STM32SYNTH_MIDINN_E2,
	STM32SYNTH_MIDINN_F2,
	STM32SYNTH_MIDINN_FS2,
	STM32SYNTH_MIDINN_G2,
	STM32SYNTH_MIDINN_GS2,
	STM32SYNTH_MIDINN_A2,
	STM32SYNTH_MIDINN_AS2,
	STM32SYNTH_MIDINN_B2,
	STM32SYNTH_MIDINN_C3,
	STM32SYNTH_MIDINN_CS3,
	STM32SYNTH_MIDINN_D3,
	STM32SYNTH_MIDINN_DS3,
	STM32SYNTH_MIDINN_E3,
	STM32SYNTH_MIDINN_F3,
	STM32SYNTH_MIDINN_FS3,
	STM32SYNTH_MIDINN_G3,
	STM32SYNTH_MIDINN_GS3,
	STM32SYNTH_MIDINN_A3,
	STM32SYNTH_MIDINN_AS3,
	STM32SYNTH_MIDINN_B3,
	STM32SYNTH_MIDINN_C4,
	STM32SYNTH_MIDINN_CS4,
	STM32SYNTH_MIDINN_D4,
	STM32SYNTH_MIDINN_DS4,
	STM32SYNTH_MIDINN_E4,
	STM32SYNTH_MIDINN_F4,
	STM32SYNTH_MIDINN_FS4,
	STM32SYNTH_MIDINN_G4,
	STM32SYNTH_MIDINN_GS4,
	STM32SYNTH_MIDINN_A4,
	STM32SYNTH_MIDINN_AS4,
	STM32SYNTH_MIDINN_B4,
	STM32SYNTH_MIDINN_C5,
	STM32SYNTH_MIDINN_CS5,
	STM32SYNTH_MIDINN_D5,
	STM32SYNTH_MIDINN_DS5,
	STM32SYNTH_MIDINN_E5,
	STM32SYNTH_MIDINN_F5,
	STM32SYNTH_MIDINN_FS5,
	STM32SYNTH_MIDINN_G5,
	STM32SYNTH_MIDINN_GS5,
	STM32SYNTH_MIDINN_A5,
	STM32SYNTH_MIDINN_AS5,
	STM32SYNTH_MIDINN_B5,
	STM32SYNTH_MIDINN_C6,
	STM32SYNTH_MIDINN_CS6,
	STM32SYNTH_MIDINN_D6,
	STM32SYNTH_MIDINN_DS6,
	STM32SYNTH_MIDINN_E6,
	STM32SYNTH_MIDINN_F6,
	STM32SYNTH_MIDINN_FS6,
	STM32SYNTH_MIDINN_G6,
	STM32SYNTH_MIDINN_GS6,
	STM32SYNTH_MIDINN_A6,
	STM32SYNTH_MIDINN_AS6,
	STM32SYNTH_MIDINN_B6,
	STM32SYNTH_MIDINN_C7,
	STM32SYNTH_MIDINN_CS7,
	STM32SYNTH_MIDINN_D7,
	STM32SYNTH_MIDINN_DS7,
	STM32SYNTH_MIDINN_E7,
	STM32SYNTH_MIDINN_F7,
	STM32SYNTH_MIDINN_FS7,
	STM32SYNTH_MIDINN_G7,
	STM32SYNTH_MIDINN_GS7,
	STM32SYNTH_MIDINN_A7,
	STM32SYNTH_MIDINN_AS7,
	STM32SYNTH_MIDINN_B7,
	STM32SYNTH_MIDINN_C8,
	STM32SYNTH_MIDINN_CS8,
	STM32SYNTH_MIDINN_D8,
	STM32SYNTH_MIDINN_DS8,
	STM32SYNTH_MIDINN_E8,
	STM32SYNTH_MIDINN_F8,
	STM32SYNTH_MIDINN_FS8,
	STM32SYNTH_MIDINN_G8,
	STM32SYNTH_MIDINN_GS8,
	STM32SYNTH_MIDINN_A8,
	STM32SYNTH_MIDINN_AS8,
	STM32SYNTH_MIDINN_B8,
	STM32SYNTH_MIDINN_C9,
	STM32SYNTH_MIDINN_CS9,
	STM32SYNTH_MIDINN_D9,
	STM32SYNTH_MIDINN_DS9,
	STM32SYNTH_MIDINN_E9,
	STM32SYNTH_MIDINN_F9,
	STM32SYNTH_MIDINN_FS9,
	STM32SYNTH_MIDINN_FILTER_DISABLE = 0xFF
} stm32synth_midi_notenum_t;

typedef enum
{
	STM32SYNTH_MIDIEVT0_NOTEOFF = 0x8,
	STM32SYNTH_MIDIEVT0_NOTEON = 0x9,
	STM32SYNTH_MIDIEVT0_AFTERTOUCH = 0xA,
	STM32SYNTH_MIDIEVT0_CONTROLCHANGE = 0xB,
	STM32SYNTH_MIDIEVT0_PROGRAMCHANGE = 0xC,
	STM32SYNTH_MIDIEVT0_CHANNELAFTERTOUCH = 0xD,
	STM32SYNTH_MIDIEVT0_PITCHBEND = 0xE,
} stm32synth_midi_evnt0_t;

typedef enum
{
	STM32SYNTH_FILTERTYPE_LPF = 0,
	STM32SYNTH_FILTERTYPE_HPF,
	STM32SYNTH_FILTERTYPE_LSF
} stm32synth_midi_filtertype_t;

typedef enum
{
	STM32SYNTH_CABLENUMFILT_OFF = 0,
	STM32SYNTH_CABLENUMFILT_ON,
	STM32SYNTH_CABLENUMFILT_ON_DEEP
} stm32synth_midi_cablenumfilt_t;

typedef enum
{
	STM32SYNTH_MIDICC_BANKSLECT = 0,			  // Allows user to switch bank for patch selection. Program change used with Bank Select. MIDI can access 16,384 patches per MIDI channel.
	STM32SYNTH_MIDICC_MODWHELL,					  // Generally this CC controls a vibrato effect (pitch, loudness, brighness). What is modulated is based on the patch.
	STM32SYNTH_MIDICC_BREATHCONT,				  // Oftentimes associated with aftertouch messages. It was originally intended for use with a breath MIDI controller in which blowing harder produced higher MIDI control values. It can be used for modulation as well.
	STM32SYNTH_MIDICC_FOOTCONT = 4,				  // Often used with aftertouch messages. It can send a continuous stream of values based on how the pedal is used.
	STM32SYNTH_MIDICC_PORTAMENTOTIME,			  // Controls portamento rate to slide between 2 notes played subsequently.
	STM32SYNTH_MIDICC_DATAENTRY,				  // Controls Value for NRPN or RPN parameters.
	STM32SYNTH_MIDICC_VOLUME,					  // Controls the volume of the channel.
	STM32SYNTH_MIDICC_BALANCE,					  // Controls the left and right balance, generally for stereo patches. A value of 64 equals the center.
	STM32SYNTH_MIDICC_PAN = 10,					  // Controls the left and right balance, generally for mono patches. A value of 64 equals the center.
	STM32SYNTH_MIDICC_EXPRESSION,				  // Expression is a percentage of volume (CC7).
	STM32SYNTH_MIDICC_EFFECTCONT1,				  // Usually used to control a parameter of an effect within the synth or workstation.
	STM32SYNTH_MIDICC_EFFECTCONT2,				  // Usually used to control a parameter of an effect within the synth or workstation.
	STM32SYNTH_MIDICC_GENERAL16 = 16,			  //
	STM32SYNTH_MIDICC_GENERAL17,				  //
	STM32SYNTH_MIDICC_GENERAL18,				  //
	STM32SYNTH_MIDICC_GENERAL19,				  //
	STM32SYNTH_MIDICC_USER_ATTACKLEVEL = 20,	  //
	STM32SYNTH_MIDICC_USER_WAVE1SINEAMP,		  //
	STM32SYNTH_MIDICC_USER_WAVE1SQUAMP,			  //
	STM32SYNTH_MIDICC_USER_WAVE1SQUDUTY,		  //
	STM32SYNTH_MIDICC_USER_WAVE1TRIAMP,			  //
	STM32SYNTH_MIDICC_USER_WAVE1TRIPEAK,		  //
	STM32SYNTH_MIDICC_USER_WAVE1PITCH,			  //
	STM32SYNTH_MIDICC_USER_WAVE1OCTAVE,			  //
	STM32SYNTH_MIDICC_USER_WAVE2SINEAMP,		  //
	STM32SYNTH_MIDICC_USER_WAVE2SQUAMP,			  //
	STM32SYNTH_MIDICC_USER_WAVE2SQUDUTY,		  //
	STM32SYNTH_MIDICC_USER_WAVE2TRIAMP,			  //
	STM32SYNTH_MIDICC_USER_WAVE2TRIPEAK,		  //
	STM32SYNTH_MIDICC_USER_WAVE2PITCH,			  //
	STM32SYNTH_MIDICC_USER_WAVE2OCTAVE,			  //
	STM32SYNTH_MIDICC_USER_DISTORTION_LEVEL,	  //
	STM32SYNTH_MIDICC_USER_PITCHBYNOTE,			  //
	STM32SYNTH_MIDICC_USER_FILTERTYPE,			  //
	STM32SYNTH_MIDICC_USER_CH0MIRRORCH,			  //
	STM32SYNTH_MIDICC_USER_TRE_TYPE,			  //
	STM32SYNTH_MIDICC_USER_TRE_PEAK,			  //
	STM32SYNTH_MIDICC_USER_TRE_FREQ,			  //
	STM32SYNTH_MIDICC_USER_VIB_TYPE,			  //
	STM32SYNTH_MIDICC_USER_VIB_PEAK,			  //
	STM32SYNTH_MIDICC_USER_VIB_AMP,				  //
	STM32SYNTH_MIDICC_USER_VIB_FREQ,			  //
	STM32SYNTH_MIDICC_USER_WOW_TYPE,			  //
	STM32SYNTH_MIDICC_USER_WOW_PEAK,			  //
	STM32SYNTH_MIDICC_USER_WOW_AMP,				  //
	STM32SYNTH_MIDICC_USER_WOW_FREQ,			  //
	STM32SYNTH_MIDICC_USER_ENV_VOL_DIFF,		  //
	STM32SYNTH_MIDICC_USER_ENV_VOL_TIME,		  //
	STM32SYNTH_MIDICC_USER_ENV_FREQ_DIFF,		  //
	STM32SYNTH_MIDICC_USER_ENV_FREQ_TIME,		  //
	STM32SYNTH_MIDICC_USER_ENV_WOW_DIFF,		  //
	STM32SYNTH_MIDICC_USER_ENV_WOW_TIME,		  //
	STM32SYNTH_MIDICC_USER_DRUM_NN,				  //
	STM32SYNTH_MIDICC_USER_DRUM_TYPE,			  //
	STM32SYNTH_MIDICC_USER_DRUM_LPF_FCNN,		  //
	STM32SYNTH_MIDICC_USER_DRUM_HPF_FCNN,		  //
	STM32SYNTH_MIDICC_USER_DRUM_LPF_Q,			  //
	STM32SYNTH_MIDICC_USER_DRUM_HPF_Q,			  //
	STM32SYNTH_MIDICC_DAMPERPEDAL = 64,			  // ≤63 off, ≥64 on. On/off switch that controls sustain pedal. Nearly every synth will react to CC 64. (See also Sostenuto CC 66)
	STM32SYNTH_MIDICC_PORTAMENTO,				  // ≤63 off, ≥64 on. On/off switch
	STM32SYNTH_MIDICC_SOSTENTOPEDAL,			  // ≤63 off, ≥64 on. On/off switch – Like the Sustain controller (CC 64), However, it only holds notes that were “On” when the pedal was pressed. People use it to “hold” chords” and play melodies over the held chord.
	STM32SYNTH_MIDICC_SOFTPEDAL,				  // ≤63 off, ≥64 on.	On/off switch – Lowers the volume of notes played.
	STM32SYNTH_MIDICC_LEGATOFOOTSWITCH,			  // ≤63 off, ≥64 on. On/off switch – Turns Legato effect between 2 subsequent notes on or off.
	STM32SYNTH_MIDICC_HOLD2,					  // ≤63 off, ≥64 on	Another way to “hold notes” (see MIDI CC 64 and MIDI CC 66). However notes fade out according to their release parameter rather than when the pedal is released.
	STM32SYNTH_MIDICC_SOUNDCONT1,				  // Usually controls the way a sound is produced. Default = Sound Variation.
	STM32SYNTH_MIDICC_SOUNDCONT2,				  // Allows shaping the Voltage Controlled Filter (VCF). Default = Resonance also (Timbre or Harmonics)
	STM32SYNTH_MIDICC_USER_FILTERQFACTOR = 71,	  // Resonance of the filter(q factor)
	STM32SYNTH_MIDICC_SOUNDCONT3,				  // Controls release time of the Voltage controlled Amplifier (VCA). Default = Release Time.
	STM32SYNTH_MIDICC_USER_RELEASETIME = 72,	  // Release Time
	STM32SYNTH_MIDICC_SOUNDCONT4,				  // Controls the “Attack’ of a sound. The attack is the amount of time it takes for the sound to reach maximum amplitude.
	STM32SYNTH_MIDICC_USER_ATTACKTIME = 73,		  // Controls the “Attack’ of a sound. The attack is the amount of time it takes for the sound to reach maximum amplitude.
	STM32SYNTH_MIDICC_SOUNDCONT5,				  // Controls VCFs cutoff frequency of the filter.
	STM32SYNTH_MIDICC_USER_FILTERCUTOFF = 74,	  // cutoff frequency of the filter
	STM32SYNTH_MIDICC_GENERAL75,				  // Generic – Some manufacturers may use to further shave their sounds.
	STM32SYNTH_MIDICC_GENERAL76,				  // Generic – Some manufacturers may use to further shave their sounds.
	STM32SYNTH_MIDICC_GENERAL77,				  // Generic – Some manufacturers may use to further shave their sounds.
	STM32SYNTH_MIDICC_GENERAL78,				  // Generic – Some manufacturers may use to further shave their sounds.
	STM32SYNTH_MIDICC_GENERAL79,				  // Generic – Some manufacturers may use to further shave their sounds.
	STM32SYNTH_MIDICC_GENERAL80,				  // Decay Generic or on/off switch ≤63 off, ≥64 on
	STM32SYNTH_MIDICC_USER_DECAYTIME = 80,		  // Decay time
	STM32SYNTH_MIDICC_GENERAL81,				  // Hi-Pass Filter Frequency or Generic on/off switch ≤63 off, ≥64 on
	STM32SYNTH_MIDICC_GENERAL82,				  // Generic on/off switch ≤63 off, ≥64 on
	STM32SYNTH_MIDICC_GENERAL83,				  // Generic on/off switch ≤63 off, ≥64 on
	STM32SYNTH_MIDICC_PORTAMENTOCC,				  // Controls the amount of Portamento.
	STM32SYNTH_MIDICC_HIGHRESVELOCITYPREFIX = 88, // Extends the range of possible velocity values
	STM32SYNTH_MIDICC_EFFECTDEPTH1 = 91,		  // Usually controls reverb send amount
	STM32SYNTH_MIDICC_USER_REVERB_LEVEL = 91,	  // Reverb amount
	STM32SYNTH_MIDICC_EFFECTDEPTH2,				  // Usually controls tremolo amount
	STM32SYNTH_MIDICC_USER_TRE_AMP = 92,		  //
	STM32SYNTH_MIDICC_EFFECTDEPTH3,				  // Usually controls chorus amount
	STM32SYNTH_MIDICC_EFFECTDEPTH4,				  // Usually controls detune amount
	STM32SYNTH_MIDICC_EFFECTDEPTH5,				  // Usually controls phaser amount
	STM32SYNTH_MIDICC_DATAINCREMENT,			  // Usually used to increment data for RPN and NRPN messages.
	STM32SYNTH_MIDICC_DATADECREMENT,			  // Usually used to decrement data for RPN and NRPN messages.
	STM32SYNTH_MIDICC_NRPNLSB,					  // For controllers 6, 38, 96, and 97, it selects the NRPN parameter.
	STM32SYNTH_MIDICC_NRPNMSB,					  // For controllers 6, 38, 96, and 97, it selects the NRPN parameter.
	STM32SYNTH_MIDICC_RPNLSB,					  // For controllers 6, 38, 96, and 97, it selects the RPN parameter.
	STM32SYNTH_MIDICC_RPNMSB,					  // For controllers 6, 38, 96, and 97, it selects the RPN parameter.
	STM32SYNTH_MIDICC_USER_MTRE_TYPE,			  //
	STM32SYNTH_MIDICC_USER_MTRE_PEAK,			  //
	STM32SYNTH_MIDICC_USER_MTRE_AMP,			  //
	STM32SYNTH_MIDICC_USER_MTRE_FREQ,			  //
	STM32SYNTH_MIDICC_USER_MVIB_TYPE,			  //
	STM32SYNTH_MIDICC_USER_MVIB_PEAK,			  //
	STM32SYNTH_MIDICC_USER_MVIB_AMP,			  //
	STM32SYNTH_MIDICC_USER_MVIB_FREQ,			  //
	STM32SYNTH_MIDICC_USER_MWOW_TYPE,			  //
	STM32SYNTH_MIDICC_USER_MWOW_PEAK,			  //
	STM32SYNTH_MIDICC_USER_MWOW_AMP,			  //
	STM32SYNTH_MIDICC_USER_MWOW_FREQ,			  //
	STM32SYNTH_MIDICC_USER_MFILTER_TYPE,		  //
	STM32SYNTH_MIDICC_USER_MFILTER_QFACTOR,		  //
	STM32SYNTH_MIDICC_USER_MFILTER_CUTOFF,		  //
	STM32SYNTH_MIDICC_USER_PRESET = 118,		  // Preset
	STM32SYNTH_MIDICC_USER_MODECONTROL = 119,	  //
	STM32SYNTH_MIDICC_ALLSOUNDOFF = 120,		  // Mutes all sound. It does so regardless of release time or sustain. (See MIDI CC 123)
	STM32SYNTH_MIDICC_RESETALL,					  // It will reset all controllers to their default.
	STM32SYNTH_MIDICC_LOCALSWITCH,				  // Turns internal connection of a MIDI keyboard or workstation, etc. on or off. If you use a computer, you will most likely want local control off to avoid notes being played twice. Once locally and twice when the note is sent back from the computer to your keyboard.
	STM32SYNTH_MIDICC_ALLNOTESOFF,				  // Mutes all sounding notes. Release time will still be maintained, and notes held by sustain will not turn off until sustain pedal is depressed.
	STM32SYNTH_MIDICC_OMNIMODEOFF,				  // Sets to “Omni Off” mode.
	STM32SYNTH_MIDICC_OMNIMODEON,				  // Sets to “Omni On” mode.
	STM32SYNTH_MIDICC_MONOMODE,					  // Sets device mode to Monophonic. The value equals the number of channels, or 0 if the number of channels equals the number of voices in the receiver.
	STM32SYNTH_MIDICC_POLYMODE					  // Sets device mode to Polyphonic.
} stm32synth_midi_cc_t;

typedef enum
{
	STM32SYNTH_MODECONTROL_SAVECONFIG = 10, // Save config to Flash
	STM32SYNTH_MODECONTROL_ERASECONFIG,		// Erase config on Flash

	STM32SYNTH_MODECONTROL_CABLENUMFILTOFF = 20, // CableNum filter off
	STM32SYNTH_MODECONTROL_CABLENUMFILTON,		 // CableNum filter on
	STM32SYNTH_MODECONTROL_CABLENUMFILTONDEEP,	 // Deep CableNum filter on

	STM32SYNTH_MODECONTROL_GOTODFU = 100, // Goto DFU mode
} stm32synth_midi_modecontrol_t;

// struct
typedef struct
{
	stm32synth_midi_notenum_t nn;
	int16_t pitch2;
	stm32synth_midi_notenum_t lpffc_nn;
	stm32synth_midi_notenum_t hpffc_nn;
	uint8_t waveformType;
	int16_t distortion;
	float32_t lpfq;
	float32_t hpfq;
	float32_t wn_amp;
	int16_t freqenv_diff;
} stm32synth_config_drum_t;

typedef struct
{
	uint8_t buff[STM32SYNTH_MIDIBUFF_SIZE][4];
	uint16_t wpos;
	uint16_t rpos;
} stm32synth_midiinput_ringbuff_t;

stm32synth_res_t stm32synth_component_inputmidi(stm32synth_config_t *_config, uint8_t *_midi);
stm32synth_res_t stm32synth_component_midi_writebuff(uint8_t *_midi);
stm32synth_res_t stm32synth_component_midi_readbuff(uint8_t *_midi);
stm32synth_res_t stm32synth_component_midi_buff2input(stm32synth_config_t *_config);

#endif /* STM32_SYNTH_COMPONENTS_STM32_SYNTH_MIDI_H_ */
