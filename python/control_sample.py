#py -m pip install python-rtmidi

from enum import Enum, IntEnum

import random
import time
import rtmidi
import json

class MidiMsg(IntEnum):
    NOTEOFF =  0x80
    NOTEON =  0x90
    AFTERTOUCH =  0xA0
    CONTROLCHANGE =  0xB0
    PROGRAMCHANGE =  0xC0
    CHANNELAFTERTOUCH =  0xD0
    PITCHBEND =  0xE0

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

    ## パラメータ初期化
    synthPara = [
        #Basis
        Parameter("synth-pitch", 0,  MidiMsg.PITCHBEND, 0, 0, 127, 64),

        #Waveform
        Parameter("synth-sin1-volume", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1SINEAMP,  0, 127, 100),
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
        Parameter("synth-octave-shift2", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2OCTAVE,  0, 127, 64),
        Parameter("synth-distortion", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_DISTORTION_LEVEL, 0, 127, 127),
        
        #ADSR
        Parameter("synth-attack-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ATTACKTIME,  0, 127, 0),
        Parameter("synth-attack-level", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ATTACKLEVEL, 0, 127, 32),
        Parameter("synth-decay-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_DECAYTIME, 0, 127, 10),
        Parameter("synth-release-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_RELEASETIME,  0, 127, 50),
        
        #Filter
        Parameter("synth-filter-type", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_FILTERTYPE, 0,1, 0),
        Parameter("synth-filter-cutoff", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_FILTERCUTOFF, 0, 127, 127),
        Parameter("synth-filter-q", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_FILTERQFACTOR, 0, 127, 58),
        
        #LFO
        Parameter("synth-vib-type", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_VIB_TYPE, 0, 2, 0),
        Parameter("synth-vib-peak", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_VIB_PEAK, 0, 127 ,0),
        Parameter("synth-vib-amp", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_VIB_AMP, 0, 127 ,0),
        Parameter("synth-vib-freq", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_VIB_FREQ, 0, 127, 0 ),
        Parameter("synth-tre-type", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_TRE_TYPE, 0, 2, 0),
        Parameter("synth-tre-peak", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_TRE_PEAK, 0, 127 ,0),
        Parameter("synth-tre-amp", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_TRE_AMP , 0, 127, 0),
        Parameter("synth-tre-freq", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_TRE_FREQ , 0, 127, 0),
        Parameter("synth-wow-type", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WOW_TYPE , 0, 2, 0),
        Parameter("synth-wow-peak", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WOW_PEAK , 0, 127, 0),
        Parameter("synth-wow-amp", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WOW_AMP, 0, 127 ,0),
        Parameter("synth-wow-freq", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WOW_FREQ, 0, 127 ,0),
        
        #Envelope
        Parameter("synth-env-vol-diff", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_VOL_DIFF, 0, 127 ,64),
        Parameter("synth-env-vol-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_VOL_TIME , 0, 127, 10),
        Parameter("synth-env-freq-diff", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_FREQ_DIFF , 0, 127, 64),
        Parameter("synth-env-freq-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_FREQ_TIME , 0, 127, 10),
        Parameter("synth-env-fc-diff", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_WOW_DIFF, 0, 127 ,64),
        Parameter("synth-env-fc-time", 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_WOW_TIME , 0, 127, 0)
    ]
    
    ## 繰り返し
    for i in range(1):

        ## パラメータ調整
        for para in synthPara:
            #パラメータの値を演算
            #para.setVal(random.randint(para.valueMin, para.valueMax)) #各パラメータをランダムに設定
            
            #パラメータの値をシンセにセット
            midiout.send_message(para.getMidiMsg())
            time.sleep(0.01) #シンセのデータ受信が詰まらないように念の為ディレイ

        ## ノートナンバー50-90まで鳴らす
        for nn in range(50, 90, 1):
            #鳴らす
            note_on = [MidiMsg.NOTEON, nn, 100]
            midiout.send_message(note_on)
            time.sleep(0.1)

            #止める
            note_off = [MidiMsg.NOTEOFF, nn, 0]
            midiout.send_message(note_off)
            time.sleep(0.05)
    
    ## 決定したパラメータの値をjson書き出し
    data = dict()
    for para in synthPara:
        data.setdefault(para.jsonHeader, para.value)

    #print(json.dumps(data))
    
    # jsonファイルに書き出し
    with open('data.json', 'w') as f:
        json.dump(data, f)

if __name__ == "__main__":
    main()