#py -m pip install -U python-rtmidi pyaudio matplotlib numpy scipy

from enum import Enum, IntEnum

import random
import time
import rtmidi
import json
import pyaudio
import matplotlib.pyplot as plot
import numpy as np
import scipy.io.wavfile as wavfile

from scipy import signal
from deap import base, creator, tools

FRAME_SIZE    = 2048         # フレームサイズ
SAMPLE_RATE   = 44100        # サンプリングレート
NGEN          = 50           # GA世代数

class MidiMsg(IntEnum):
    NOTEOFF = 0x80,
    NOTEON = 0x90,
    AFTERTOUCH = 0xA0,
    CONTROLCHANGE = 0xB0,
    PROGRAMCHANGE = 0xC0,
    CHANNELAFTERTOUCH = 0xD0,
    PITCHBEND = 0xE0

class MidiCC(IntEnum):
    BANKSLECT = 0,               # Allows user to switch bank for patch selection. Program change used with Bank Select. MIDI can access 16,384 patches per MIDI channel.
    MODWHELL = 1,			    # Generally this CC controls a vibrato effect (pitch, loudness, brighness). What is modulated is based on the patch.
    BREATHCONT = 2,              # Oftentimes associated with aftertouch messages. It was originally intended for use with a breath MIDI controller in which blowing harder produced higher MIDI control values. It can be used for modulation as well.
    FOOTCONT = 4,                # Often used with aftertouch messages. It can send a continuous stream of values based on how the pedal is used.
    PORTAMENTOTIME = 5,          # Controls portamento rate to slide between 2 notes played subsequently.
    DATAENTRY = 6,               # Controls Value for NRPN or RPN parameters.
    VOLUME = 7,                  # Controls the volume of the channel.
    BALANCE = 8,                 # Controls the left and right balance, generally for stereo patches. A value of 64 equals the center.
    PAN = 10,                    # Controls the left and right balance, generally for mono patches. A value of 64 equals the center.
    EXPRESSION = 1,              # Expression is a percentage of volume (CC7).
    EFFECTCONT1 = 2,             # Usually used to control a parameter of an effect within the synth or workstation.
    EFFECTCONT2 = 3,             # Usually used to control a parameter of an effect within the synth or workstation.
    GENERAL16 = 16,              #
    GENERAL17 = 17,              #
    GENERAL18 = 18,              #
    GENERAL19 = 19,              #
    USER_ATTACKLEVEL = 20,       #
    USER_WAVE1SINEAMP = 21,		#
    USER_WAVE1SQUAMP = 22,		#
    USER_WAVE1SQUDUTY = 23,		#
    USER_WAVE1TRIAMP = 24,		#
    USER_WAVE1TRIPEAK = 25,		#
    USER_WAVE1PITCH = 26,		#
    USER_WAVE1OCTAVE = 27,		#
    USER_WAVE2SINEAMP = 28,		#
    USER_WAVE2SQUAMP = 29,		#
    USER_WAVE2SQUDUTY = 30,		#
    USER_WAVE2TRIAMP = 31,		#
    USER_WAVE2TRIPEAK = 32,	    #
    USER_WAVE2PITCH = 33,   	    #
    USER_WAVE2OCTAVE = 34,	    #
    USER_DISTORTION_LEVEL = 35,  #
    USER_PITCHBYNOTE = 36,       #
    USER_FILTERTYPE = 37,#
    USER_CH0MIRRORCH = 38,       #
    USER_TRE_TYPE = 39,          #
    USER_TRE_PEAK = 40,          #
    USER_TRE_FREQ = 41,          #
    USER_VIB_TYPE = 42,          #
    USER_VIB_PEAK = 43,          #
    USER_VIB_AMP = 44,           #
    USER_VIB_FREQ = 45,          #
    USER_WOW_TYPE = 46,          #
    USER_WOW_PEAK = 47,          #
    USER_WOW_AMP = 48,           #
    USER_WOW_FREQ = 49,          #
    USER_ENV_VOL_DIFF = 50,		#
    USER_ENV_VOL_TIME = 51,		#
    USER_ENV_FREQ_DIFF = 52,		#
    USER_ENV_FREQ_TIME = 53,		#
    USER_ENV_WOW_DIFF = 54,		#
    USER_ENV_WOW_TIME = 55,		#
    USER_DRUM_NN = 56,			#
    USER_DRUM_TYPE = 57,		    #
    USER_DRUM_LPF_FCNN = 58,		#
    USER_DRUM_HPF_FCNN = 59,		#
    USER_DRUM_LPF_Q = 60,		#
    USER_DRUM_HPF_Q = 61,		#
    DAMPERPEDAL = 64,            # ≤63 off, ≥64 on. On/off switch that controls sustain pedal. Nearly every synth will react to CC 64. (See also Sostenuto CC 66)
    PORTAMENTO = 65,             # ≤63 off, ≥64 on. On/off switch
    SOSTENTOPEDAL = 66,          # ≤63 off, ≥64 on. On/off switch – Like the Sustain controller (CC 64), However, it only holds notes that were “On” when the pedal was pressed. People use it to “hold” chords” and play melodies over the held chord.
    SOFTPEDAL = 67,              # ≤63 off, ≥64 on.	On/off switch – Lowers the volume of notes played.
    LEGATOFOOTSWITCH = 68,       # ≤63 off, ≥64 on. On/off switch – Turns Legato effect between 2 subsequent notes on or off.
    HOLD2 = 69,                  # ≤63 off, ≥64 on	Another way to “hold notes” (see MIDI CC 64 and MIDI CC 66). However notes fade out according to their release parameter rather than when the pedal is released.
    SOUNDCONT1 = 70,             # Usually controls the way a sound is produced. Default =  Sound Variation.
    SOUNDCONT2 = 71,             # Allows shaping the Voltage Controlled Filter (VCF). Default =  Resonance also (Timbre or Harmonics)
    USER_FILTERQFACTOR = 71,        # Resonance of the filter(q factor)
    SOUNDCONT3 = 72,             # Controls release time of the Voltage controlled Amplifier (VCA). Default =  Release Time.
    USER_RELEASETIME = 72,       # Release Time
    SOUNDCONT4 = 73,             # Controls the “Attack’ of a sound. The attack is the amount of time it takes for the sound to reach maximum amplitude.
    USER_ATTACKTIME = 73,        # Controls the “Attack’ of a sound. The attack is the amount of time it takes for the sound to reach maximum amplitude.
    SOUNDCONT5 = 74,             # Controls VCFs cutoff frequency of the filter.
    USER_FILTERCUTOFF = 74,         # cutoff frequency of the filter
    GENERAL75 = 75,              # Generic – Some manufacturers may use to further shave their sounds.
    GENERAL76 = 76,              # Generic – Some manufacturers may use to further shave their sounds.
    GENERAL77 = 77,              # Generic – Some manufacturers may use to further shave their sounds.
    GENERAL78 = 78,              # Generic – Some manufacturers may use to further shave their sounds.
    GENERAL79 = 79,              # Generic – Some manufacturers may use to further shave their sounds.
    GENERAL80 = 80,              # Decay Generic or on/off switch ≤63 off, ≥64 on
    USER_DECAYTIME = 80,         # Decay time
    GENERAL81 = 81,              # Hi-Pass Filter Frequency or Generic on/off switch ≤63 off, ≥64 on
    GENERAL82 = 82,              # Generic on/off switch ≤63 off, ≥64 on
    GENERAL83 = 83,              # Generic on/off switch ≤63 off, ≥64 on
    PORTAMENTOCC = 84,           # Controls the amount of Portamento.
    HIGHRESVELOCITYPREFIX = 88,  # Extends the range of possible velocity values
    EFFECTDEPTH1 = 91,           # Usually controls reverb send amount
    USER_REVERB_LEVEL = 91,      # Reverb level
    EFFECTDEPTH2 = 92,           # Usually controls tremolo amount
    USER_TRE_AMP = 92,           #
    EFFECTDEPTH3 = 93,           # Usually controls chorus amount
    EFFECTDEPTH4 = 94,           # Usually controls detune amount
    EFFECTDEPTH5 = 95,           # Usually controls phaser amount
    DATAINCREMENT = 96,          # Usually used to increment data for RPN and NRPN messages.
    DATADECREMENT = 97,          # Usually used to decrement data for RPN and NRPN messages.
    NRPNLSB = 98,                # For controllers 6, 38, 96, and 97, it selects the NRPN parameter.
    NRPNMSB = 99,                # For controllers 6, 38, 96, and 97, it selects the NRPN parameter.
    RPNLSB = 100,                # For controllers 6, 38, 96, and 97, it selects the RPN parameter.
    RPNMSB = 101,                # For controllers 6, 38, 96, and 97, it selects the RPN parameter.
    USER_MTRE_TYPE = 102,		#
    USER_MTRE_PEAK = 103,		#
    USER_MTRE_AMP = 104,	        #
    USER_MTRE_FREQ = 105,		#
    USER_MVIB_TYPE = 106,		#
    USER_MVIB_PEAK = 107,		#
    USER_MVIB_AMP = 108,			#
    USER_MVIB_FREQ = 109,		#
    USER_MWOW_TYPE = 110,		#
    USER_MWOW_PEAK = 111,		#
    USER_MWOW_AMP = 112,			#
    USER_MWOW_FREQ = 113,		#
    USER_MFILTER_TYPE = 114,		#
    USER_MFILTER_QFACTOR = 115,	#
    USER_MFILTER_CUTOFF = 116,	#
    USER_PRESET = 118,           # Preset
    USER_MODECONTROL = 119,       # 
    ALLSOUNDOFF = 120,           # Mutes all sound. It does so regardless of release time or sustain. (See MIDI CC 123)
    RESETALL = 121,              # It will reset all controllers to their default.
    LOCALSWITCH = 122,           # Turns internal connection of a MIDI keyboard or workstation, etc. on or off. If you use a computer, you will most likely want local control off to avoid notes being played twice. Once locally and twice when the note is sent back from the computer to your keyboard.
    ALLNOTESOFF = 123,           # Mutes all sounding notes. Release time will still be maintained, and notes held by sustain will not turn off until sustain pedal is depressed.
    OMNIMODEOFF = 124,           # Sets to “Omni Off” mode.
    OMNIMODEON = 125,            # Sets to “Omni On” mode.
    MONOMODE = 126,              # Sets device mode to Monophonic. The value equals the number of channels, or 0 if the number of channels equals the number of voices in the receiver.
    POLYMODE = 127               # Sets device mode to Polyphonic.

class NoteNum(IntEnum):
	NN_CM1 = 0,
	NN_CSM1 = 1,
	NN_DM1 = 2,
	NN_DSM1 = 3,
	NN_EM1 = 4,
	NN_FM1 = 5,
	NN_FSM1 = 6,
	NN_GM1 = 7,
	NN_GSM1 = 8,
	NN_AM1 = 9,
	NN_ASM1 = 10,
	NN_BM1 = 11,
	NN_C0 = 12,
	NN_CS0 = 13,
	NN_D0 = 14,
	NN_DS0 = 15,
	NN_E0 = 16,
	NN_F0 = 17,
	NN_FS0 = 18,
	NN_G0 = 19,
	NN_GS0 = 20,
	NN_A0 = 21,
	NN_AS0 = 22,
	NN_B0 = 23,
	NN_C1 = 24,
	NN_CS1 = 25,
	NN_D1 = 26,
	NN_DS1 = 27,
	NN_E1 = 28,
	NN_F1 = 29,
	NN_FS1 = 30,
	NN_G1 = 31,
	NN_GS1 = 32,
	NN_A1 = 33,
	NN_AS1 = 34,
	NN_B1 = 35,
	NN_C2 = 36,
	NN_CS2 = 37,
	NN_D2 = 38,
	NN_DS2 = 39,
	NN_E2 = 40,
	NN_F2 = 41,
	NN_FS2 = 42,
	NN_G2 = 43,
	NN_GS2 = 44,
	NN_A2 = 45,
	NN_AS2 = 46,
	NN_B2 = 47,
	NN_C3 = 48,
	NN_CS3 = 49,
	NN_D3 = 50,
	NN_DS3 = 51,
	NN_E3 = 52,
	NN_F3 = 53,
	NN_FS3 = 54,
	NN_G3 = 55,
	NN_GS3 = 56,
	NN_A3 = 57,
	NN_AS3 = 58,
	NN_B3 = 59,
	NN_C4 = 60,
	NN_CS4 = 61,
	NN_D4 = 62,
	NN_DS4 = 63,
	NN_E4 = 64,
	NN_F4 = 65,
	NN_FS4 = 66,
	NN_G4 = 67,
	NN_GS4 = 68,
	NN_A4 = 69,
	NN_AS4 = 70,
	NN_B4 = 71,
	NN_C5 = 72,
	NN_CS5 = 73,
	NN_D5 = 74,
	NN_DS5 = 75,
	NN_E5 = 76,
	NN_F5 = 77,
	NN_FS5 = 78,
	NN_G5 = 79,
	NN_GS5 = 80,
	NN_A5 = 81,
	NN_AS5 = 82,
	NN_B5 = 83,
	NN_C6 = 84,
	NN_CS6 = 85,
	NN_D6 = 86,
	NN_DS6 = 87,
	NN_E6 = 88,
	NN_F6 = 89,
	NN_FS6 = 90,
	NN_G6 = 91,
	NN_GS6 = 92,
	NN_A6 = 93,
	NN_AS6 = 94,
	NN_B6 = 95,
	NN_C7 = 96,
	NN_CS7 = 97,
	NN_D7 = 98,
	NN_DS7 = 99,
	NN_E7 = 100,
	NN_F7 = 101,
	NN_FS7 = 102,
	NN_G7 = 103,
	NN_GS7 = 104,
	NN_A7 = 105,
	NN_AS7 = 106,
	NN_B7 = 107,
	NN_C8 = 108,
	NN_CS8 = 109,
	NN_D8 = 110,
	NN_DS8 = 111,
	NN_E8 = 112,
	NN_F8 = 113,
	NN_FS8 = 114,
	NN_G8 = 115,
	NN_GS8 = 116,
	NN_A8 = 117,
	NN_AS8 = 118,
	NN_B8 = 119,
	NN_C9 = 120,
	NN_CS9 = 121,
	NN_D9 = 122,
	NN_DS9 = 123,
	NN_E9 = 124,
	NN_F9 = 125,
	NN_FS9 = 126,

class Parameter:
    def __init__(self, _jsonHeader, _channel, _midiMsg, _midiCC, _valueMin, _valueMax, _resetValue):
        self.jsonHeader = _jsonHeader
        self.channel = _channel
        self.midiMsg = _midiMsg
        self.midiCC = _midiCC
        self.valueMin= _valueMin
        self.valueMax = _valueMax
        self.value = _resetValue

    def setVal(self, _value):
        self.value = _value

    def getMidiMsg(self):
        if self.midiMsg == MidiMsg.CONTROLCHANGE:
            return [self.midiMsg + self.channel, self.midiCC, self.value]
        elif self.midiMsg == MidiMsg.PITCHBEND:
            return [self.midiMsg + self.channel, self.value]
        else:
            raise ValueError("Unsupported MIDI message type")
    
def main():
    
    ## USB-MIDIポートをオープン
    midiout = rtmidi.MidiOut()
    available_ports = midiout.get_ports()

    if available_ports:
        midiout.open_port(0)
    else:
        return
    
    ## ボリューム設定
    volume = [MidiMsg.CONTROLCHANGE, MidiCC.VOLUME, 100]
    midiout.send_message(volume)

    # Base audio analysis
    # FFT用窓関数の準備
    hammingWindow = np.hamming(FRAME_SIZE)

    # read WAV
    rate, baseAudioData = wavfile.read("instSample/piano.wav")
    
    # ステレオ音声の場合は、片方のチャネルを選択
    if baseAudioData.ndim > 1:
        baseAudioData = baseAudioData[:, 0]
    
    # numpy配列に変換
    baseAudioData = np.asarray(baseAudioData)

    # ダウンサンプリング/アップサンプリング（SAMPLE_RATEに統一）
    if rate != SAMPLE_RATE:
        num_samples = int(len(baseAudioData) * SAMPLE_RATE / rate)
        baseAudioData = signal.resample(baseAudioData, num_samples)

    # 16ビット音声を-1から1の範囲に正規化し、先頭のFRAME_SIZEサンプルを切り出す
    baseAudioData = baseAudioData[:FRAME_SIZE]
    baseAudioData = baseAudioData / 32768.0
    baseAudioData = hammingWindow * baseAudioData
    instInput = np.fft.fft(baseAudioData)
    instInput = np.abs(instInput[1:int(FRAME_SIZE/2)])
    instInput = 20 * np.log10(instInput)
    instInput = instInput - np.max(instInput)

    ## パラメータ初期化
    synthParaInitForGA = [
        #Basis
        Parameter("synth-pitch", 0,  MidiMsg.PITCHBEND, 0, 0, 127, 64),

        #Waveform
        Parameter("synth-sin1-volume", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1SINEAMP,  0, 127, 80),
        Parameter("synth-tri1-volume", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1TRIAMP,  0, 127, 0),
        Parameter("synth-tri1-peak", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1TRIPEAK, 0, 127, 0),
        Parameter("synth-squ1-volume", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1SQUAMP, 0, 127, 0),
        Parameter("synth-squ1-duty", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1SQUDUTY,  0, 127, 64),
        Parameter("synth-pitch1", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1PITCH,  0, 127, 64),
        Parameter("synth-octave-shift1", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1OCTAVE, 0, 127, 64),
        Parameter("synth-sin2-volume", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2SINEAMP, 0, 127, 0),
        Parameter("synth-tri2-volume", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2TRIAMP, 0, 127, 0),
        Parameter("synth-tri2-peak", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2TRIPEAK,  0, 127, 64),
        Parameter("synth-squ2-volume", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2SQUAMP, 0, 127, 0),
        Parameter("synth-squ2-duty", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2SQUDUTY, 0, 127, 64),
        Parameter("synth-pitch2", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2PITCH, 0, 127, 64),
        Parameter("synth-octave-shift2", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2OCTAVE,  0, 127, 76),
        Parameter("synth-distortion", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_DISTORTION_LEVEL, 0, 127, 127),

        #Envelope
        Parameter("synth-env-vol-diff", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_VOL_DIFF, 0, 127 ,64),
        Parameter("synth-env-vol-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_VOL_TIME , 0, 127, 0),
        Parameter("synth-env-freq-diff", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_FREQ_DIFF , 0, 127, 64),
        Parameter("synth-env-freq-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_FREQ_TIME , 0, 127, 0),
        Parameter("synth-env-fc-diff", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_WOW_DIFF, 0, 127 ,64),
        Parameter("synth-env-fc-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_WOW_TIME , 0, 127, 0),

        #Filter
        Parameter("synth-filter-type", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_FILTERTYPE, 0, 1, 0),
        #Parameter("synth-filter-cutoff", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_FILTERCUTOFF, 0, 127, 127),
        #Parameter("synth-filter-q", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_FILTERQFACTOR, 0, 127, 58),

        #Effects
        Parameter("synth-reverb-level", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_REVERB_LEVEL, 0, 127, 64)
    ]

    synthPara = [
        #Basis
        #Parameter("synth-pitch", 0,  MidiMsg.PITCHBEND, 0, 0, 127, 64),

        #Waveform
        Parameter("synth-sin1-volume", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1SINEAMP,  0, 127, 80),
        Parameter("synth-tri1-volume", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1TRIAMP,  0, 127, 0),
        Parameter("synth-tri1-peak", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1TRIPEAK, 0, 127, 0),
        Parameter("synth-squ1-volume", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1SQUAMP, 0, 127, 0),
        Parameter("synth-squ1-duty", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1SQUDUTY,  0, 127, 64),
        #Parameter("synth-pitch1", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1PITCH,  0, 127, 64),
        #Parameter("synth-octave-shift1", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1OCTAVE, 0, 127, 64),
        Parameter("synth-sin2-volume", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2SINEAMP, 0, 127, 0),
        Parameter("synth-tri2-volume", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2TRIAMP, 0, 127, 0),
        Parameter("synth-tri2-peak", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2TRIPEAK,  0, 127, 64),
        Parameter("synth-squ2-volume", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2SQUAMP, 0, 127, 0),
        Parameter("synth-squ2-duty", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2SQUDUTY, 0, 127, 64),
        #Parameter("synth-pitch2", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2PITCH, 0, 127, 64),
        #Parameter("synth-octave-shift2", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2OCTAVE,  0, 127, 64),
        #Parameter("synth-distortion", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_DISTORTION_LEVEL, 0, 127, 127),
        
        #ADSR
        #Parameter("synth-attack-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ATTACKTIME,  0, 127, 0),
        #Parameter("synth-attack-level", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ATTACKLEVEL, 0, 127, 32),
        #Parameter("synth-decay-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_DECAYTIME, 0, 127, 10),
        #Parameter("synth-release-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_RELEASETIME,  0, 127, 50),
        
        #Filter
        #Parameter("synth-filter-type", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_FILTERTYPE, 0, 1, 0),
        Parameter("synth-filter-cutoff", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_FILTERCUTOFF, 0, 127, 127),
        Parameter("synth-filter-q", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_FILTERQFACTOR, 0, 127, 58),
        
        #LFO
        #Parameter("synth-vib-type", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_VIB_TYPE, 0, 2, 0),
        #Parameter("synth-vib-peak", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_VIB_PEAK, 0, 127 ,0),
        #Parameter("synth-vib-amp", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_VIB_AMP, 0, 127 ,0),
        #Parameter("synth-vib-freq", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_VIB_FREQ, 0, 127, 0 ),
        #Parameter("synth-tre-type", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_TRE_TYPE, 0, 2, 0),
        #Parameter("synth-tre-peak", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_TRE_PEAK, 0, 127 ,0),
        #Parameter("synth-tre-amp", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_TRE_AMP , 0, 127, 0),
        #Parameter("synth-tre-freq", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_TRE_FREQ , 0, 127, 0),
        #Parameter("synth-wow-type", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WOW_TYPE , 0, 2, 0),
        #Parameter("synth-wow-peak", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WOW_PEAK , 0, 127, 0),
        #Parameter("synth-wow-amp", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WOW_AMP, 0, 127 ,0),
        #Parameter("synth-wow-freq", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WOW_FREQ, 0, 127 ,0),
        
        #Envelope
        #Parameter("synth-env-vol-diff", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_VOL_DIFF, 0, 127 ,64),
        #Parameter("synth-env-vol-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_VOL_TIME , 0, 127, 0),
        #Parameter("synth-env-freq-diff", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_FREQ_DIFF , 0, 127, 64),
        #Parameter("synth-env-freq-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_FREQ_TIME , 0, 127, 0),
        #Parameter("synth-env-fc-diff", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_WOW_DIFF, 0, 127 ,64),
        #Parameter("synth-env-fc-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_WOW_TIME , 0, 127, 0),

        #Effects
        #Parameter("synth-reverb-level", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_REVERB_LEVEL, 0, 127, 0)
    ]

    #initialize midi parameters
    for para in synthPara:
        para.setVal(para.value) #reset value
        midiout.send_message(para.getMidiMsg())
        time.sleep(0.01) #シンセのデータ受信が詰まらないように念の為ディレイ

    #initialize midi parameters
    for para in synthParaInitForGA:
        para.setVal(para.value) #reset value
        midiout.send_message(para.getMidiMsg())
        time.sleep(0.01) #シンセのデータ受信が詰まらないように念の為ディレイ

    #鳴らす
    nn = NoteNum.NN_C5
    note_on = [MidiMsg.NOTEON, nn, 100]
    midiout.send_message(note_on)
    time.sleep(0.1)

    #audio stream start
    pa = pyaudio.PyAudio()
    streamDeviceIndex = None

    for i in range(pa.get_device_count()):
        if pa.get_device_info_by_index(i).get('name') == 'USB PnP Sound Device':
            streamDeviceIndex = i
            print(pa.get_device_info_by_index(i).get('index'), pa.get_device_info_by_index(i).get('name'), "<-selected")
        else:
            print(pa.get_device_info_by_index(i).get('index'), pa.get_device_info_by_index(i).get('name'))

    stream = pa.open(format = pyaudio.paInt16,
        rate = SAMPLE_RATE,
        channels = 1,
        input_device_index = streamDeviceIndex,
        input = True,
        frames_per_buffer = FRAME_SIZE)

    # 遺伝的アルゴリズムの設定
    creator.create("FitnessSingle", base.Fitness, weights=(-1.0,))  # 最小化
    creator.create("Individual", list, fitness=creator.FitnessSingle)

    toolbox = base.Toolbox()
    
    # 属性：各パラメータの値を0-127のランダム値で初期化
    def create_individual():
        ind = creator.Individual([random.randint(p.valueMin, p.valueMax) for p in synthPara])
        return ind
    
    # 突然変異関数：0-127の範囲内で値を変更
    def mutate_bounded(individual, mu, sigma, indpb):
        for i in range(len(individual)):
            if random.random() < indpb:
                #"""
                # ガウス分布に従う変更を加える
                individual[i] += random.gauss(mu, sigma)
                # 0-127の範囲内にクリップして整数に変換
                individual[i] = max(synthPara[i].valueMin, min(synthPara[i].valueMax, int(individual[i])))
                #"""
                #individual[i] = random.randint(synthPara[i].valueMin, synthPara[i].valueMax)
        return (individual,)
    
    # 交叉関数：0-127の範囲内で一点交叉
    def crossover_bounded(ind1, ind2):
        cxpoint = random.randint(1, len(ind1) - 1)
        ind1[cxpoint:], ind2[cxpoint:] = ind2[cxpoint:], ind1[cxpoint:]
        """
        # 交叉後に範囲内にクリップ
        for i in range(len(ind1)):
            ind1[i] = max(synthPara[i].valueMin, min(synthPara[i].valueMax, int(ind1[i])))
            ind2[i] = max(synthPara[i].valueMin, min(synthPara[i].valueMax, int(ind2[i])))
        """
        return ind1, ind2
    
    toolbox.register("individual", create_individual)
    toolbox.register("population", tools.initRepeat, list, toolbox.individual)
    
    # 評価関数：パラメータセットでシンセを駆動し、diffInputSumを計算
    def evaluate_parameters(individual):
        # パラメータをシンセに送信
        for i, para in enumerate(synthPara):
            para.setVal(int(individual[i]))
            midiout.send_message(para.getMidiMsg())
            time.sleep(0.005)
        
        time.sleep(1.0)  # シンセの応答待機
        
        # マイク入力を取得
        data = stream.read(FRAME_SIZE, exception_on_overflow=False)
        ndarray = np.frombuffer(data, dtype='int16')
        ndarray = hammingWindow * ndarray
        micInput = np.fft.fft(ndarray)
        micInput = np.abs(micInput[1:int(FRAME_SIZE/2)])
        micInput = 20 * np.log10(micInput)
        micInput = micInput - np.max(micInput)
        
        # diffInputSum を計算
        diffInput = np.sqrt(np.square(instInput - micInput))
        diffInputSum = np.sum(diffInput)
        
        return (diffInputSum,)
    
    toolbox.register("evaluate", evaluate_parameters)
    toolbox.register("mate", crossover_bounded)
    toolbox.register("mutate", mutate_bounded, mu=0, sigma=10, indpb=0.3)
    toolbox.register("select", tools.selTournament, tournsize=3)
    
    # 遺伝的アルゴリズムの実行
    population = toolbox.population(n=10)
    
    print("Genetic Algorithm Optimization Started...")
    for gen in range(NGEN):
        print(f"Generation {gen+1}/{NGEN}")
        offspring = toolbox.select(population, len(population))
        offspring = [toolbox.clone(ind) for ind in offspring]
        
        for i in range(1, len(offspring), 2):
            if random.random() < 0.7:
                toolbox.mate(offspring[i-1], offspring[i])
                del offspring[i-1].fitness.values
                del offspring[i].fitness.values
        
        for i in range(len(offspring)):
            if random.random() < 0.3:
                toolbox.mutate(offspring[i])
                del offspring[i].fitness.values
        
        invalid_ind = [ind for ind in offspring if not ind.fitness.valid]
        fitnesses = toolbox.map(toolbox.evaluate, invalid_ind)
        for ind, fit in zip(invalid_ind, fitnesses):
            ind.fitness.values = fit
        
        population[:] = offspring

    # 最適なパラメータを取得
    best_ind = tools.selBest(population, k=1)[0]
    print(f"Best diffInputSum: {best_ind.fitness.values[0]:.2f}")
    
    # 最適パラメータをシンセに適用
    for i, para in enumerate(synthPara):
        para.setVal(int(best_ind[i]))
        midiout.send_message(para.getMidiMsg())
        time.sleep(0.01)
    
    # シンセの反応時間
    time.sleep(0.5)

    # Plot base
    micInput = np.arange(1, int(FRAME_SIZE/2))
    xf = np.arange(1, int(FRAME_SIZE/2)) * SAMPLE_RATE / FRAME_SIZE
    instInputLine, = plot.plot(xf, instInput, color='orange')
    diffLine, = plot.plot(xf, instInput - micInput, color='grey')
    micInputLine, = plot.plot(xf, micInput, color='red')
    plot.ylim(-150, 0)
    plot.draw()

    ## 繰り返し（最適パラメータでの検証）
    for i in range(3):
        print(f"Validation iteration {i+1}/3")

        # Get audio data
        data = stream.read(FRAME_SIZE, exception_on_overflow = False)

        # FFT
        ndarray = np.frombuffer(data, dtype='int16')
        ndarray = hammingWindow * ndarray
        micInput = np.fft.fft(ndarray)
        micInput = np.abs(micInput[1:int(FRAME_SIZE/2)])
        micInput = 20 * np.log10(micInput)
        micInput = micInput - np.max(micInput)

        #calc difference
        diffInput = np.sqrt(np.square(instInput - micInput))
        diffInputSum = np.sum(diffInput)
        diffInput = -np.abs(diffInput)
        
        print(f"  diffInputSum: {diffInputSum:.2f}")

        # update plot data
        micInputLine.set_ydata(micInput)
        diffLine.set_ydata(diffInput)
        plot.pause(0.1)

        time.sleep(3)
    
    #止める
    note_off = [MidiMsg.NOTEOFF, nn, 0]
    midiout.send_message(note_off)
    time.sleep(0.05)

    ## ノートナンバー50-90まで鳴らす
    for nn in range(50, 90, 1):
        #鳴らす
        note_on = [MidiMsg.NOTEON, nn, 100]
        midiout.send_message(note_on)
        time.sleep(0.1)

        #止める
        note_off = [MidiMsg.NOTEOFF, nn, 0]
        midiout.send_message(note_off)
        time.sleep(0.02)

    #audio stream stop
    stream.stop_stream()
    stream.close()
    pa.terminate()

    ## 決定したパラメータの値をjson書き出し
    data = dict()
    for para in synthPara:
        data.setdefault(para.jsonHeader, para.value)

    # jsonファイルに書き出し
    #print(json.dumps(data))
    #with open('data.json', 'w') as f:
    #    json.dump(data, f)

if __name__ == "__main__":
    main()