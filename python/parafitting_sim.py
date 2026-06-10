#py -m pip install -U matplotlib numpy scipy librosa

from enum import Enum, IntEnum

import random
import time
import json
import librosa
import matplotlib.pyplot as plot
import numpy as np
import scipy.io.wavfile as wavfile

try:
    import sounddevice as sd
except ImportError:
    sd = None

from scipy import signal
from deap import base, creator, tools

FRAME_SIZE    = 2048         # フレームサイズ
SAMPLE_RATE   = 44100        # サンプリングレート
MAX_EVAL_FREQ = 10000        # 10 kHz 以上を評価に含めない
MEL_BANDS     = 64           # メルバンド数
PLAYBACK_DURATION_SECONDS = 0.5  # パラメータ変更時に再生する音声長
NGEN          = 1000           # GA世代数
MELSPECTRUM_MODE = True           # メルスペクトラムを使用するかどうか

class GApara(IntEnum):
    NO = 0,
    YES = 1
    
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
    def __init__(self, _jsonHeader, _gapara, _channel, _midiMsg, _midiCC, _valueMin, _valueMax, _resetValue):
        self.gapara = _gapara
        self.jsonHeader = _jsonHeader
        self.channel = _channel
        self.midiMsg = _midiMsg
        self.midiCC = _midiCC
        self.valueMin= _valueMin
        self.valueMax = _valueMax
        self.value = _resetValue
        self.resetValue = _resetValue

    def setResetVal(self):
        self.value = self.resetValue

    def setVal(self, _value):
        if self.gapara == GApara.NO:
            self.value = self.resetValue
            return
        self.value = _value

    def get_normalized(self):
        if self.valueMax == self.valueMin:
            return 0.0
        return (self.value - self.valueMin) / (self.valueMax - self.valueMin)
    
def note_number_to_frequency(note_number):
    return 440.0 * 2 ** ((note_number - 69) / 12.0)


def note_number_q8_to_frequency(note_number_q8):
    return 440.0 * 2 ** (((note_number_q8 / 256.0) - 69.0) / 12.0)


def lfo_shape(rad, lfo_type, peak=0.5, duty=0.5):
    rad = np.mod(rad, 2.0 * np.pi)
    if lfo_type == 0:
        return np.sin(rad)
    if lfo_type == 1:
        peak = 0.5 if peak <= 0.0 else min(0.99, max(0.01, peak))
        output = np.empty_like(rad)
        rising = rad < (2.0 * np.pi * peak)
        output[rising] = 2.0 * rad[rising] / (2.0 * np.pi * peak) - 1.0
        output[~rising] = 1.0 - 2.0 * (rad[~rising] - 2.0 * np.pi * peak) / (2.0 * np.pi * (1.0 - peak))
        return output
    duty = min(0.99, max(0.01, duty))
    return np.where(rad < 2.0 * np.pi * duty, 1.0, -1.0)


def lfo_value(lfo_type, amp, freq, peak, duty, t):
    if amp == 0 or freq == 0:
        return np.zeros_like(t, dtype=np.float32)
    rad = 2.0 * np.pi * freq * t
    return amp * lfo_shape(rad, lfo_type, peak, duty)


def envelope_value(finish_value, time_ms, elapsed_ms):
    if finish_value == 0:
        return 0.0
    if time_ms <= 0.0:
        return float(finish_value)
    ratio = np.minimum(elapsed_ms / time_ms, 1.0)
    return finish_value * (1.0 - np.exp(-2.0 * ratio))


def design_biquad_coefficients(filter_type, cutoff_freq, q):
    omega = 2.0 * np.pi * cutoff_freq / SAMPLE_RATE
    sin_omega = np.sin(omega)
    cos_omega = np.cos(omega)
    alpha = sin_omega / (2.0 * q) if q > 0 else 0.001
    if filter_type == 1:
        b0 = (1.0 + cos_omega) / 2.0
        b1 = -(1.0 + cos_omega)
        b2 = b0
    else:
        b0 = (1.0 - cos_omega) / 2.0
        b1 = 1.0 - cos_omega
        b2 = b0
    a0 = 1.0 + alpha
    a1 = -2.0 * cos_omega
    a2 = 1.0 - alpha
    return b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0


def apply_biquad(b0, b1, b2, a1, a2, signal_data):
    output = np.zeros_like(signal_data)
    x1 = x2 = 0.0
    y1 = y2 = 0.0
    for i, x0 in enumerate(signal_data):
        y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        output[i] = y0
        x2 = x1
        x1 = x0
        y2 = y1
        y1 = y0
    return output


def _spectrum_range(frame_size, sample_rate, max_freq):
    max_bin = int(max_freq * frame_size / sample_rate)
    max_bin = max(1, min(max_bin, frame_size // 2 - 1))
    return max_bin


def _prepare_fft_spectrum(audio, frame_size, max_freq):
    if audio.ndim > 1:
        audio = audio[:, 0]

    audio = np.asarray(audio, dtype=np.float32)
    if np.max(np.abs(audio)) > 0:
        audio = audio / np.max(np.abs(audio))

    if audio.shape[0] < frame_size:
        padded = np.zeros(frame_size, dtype=np.float32)
        padded[:audio.shape[0]] = audio
        audio = padded

    audio = audio[:frame_size]
    windowed = np.hamming(frame_size) * audio
    spectrum = np.fft.fft(windowed)
    max_bin = _spectrum_range(frame_size, SAMPLE_RATE, max_freq)
    magnitude = np.abs(spectrum[1:max_bin + 1])
    magnitude_db = 20 * np.log10(magnitude + 1e-9)
    return magnitude_db - np.max(magnitude_db)


def _prepare_mel_spectrum(audio, frame_size, max_freq):
    if audio.ndim > 1:
        audio = audio[:, 0]

    audio = np.asarray(audio, dtype=np.float32)
    if np.max(np.abs(audio)) > 0:
        audio = audio / np.max(np.abs(audio))

    if audio.shape[0] < frame_size:
        padded = np.zeros(frame_size, dtype=np.float32)
        padded[:audio.shape[0]] = audio
        audio = padded

    audio = audio[:frame_size]
    S = librosa.feature.melspectrogram(
        y=audio,
        sr=SAMPLE_RATE,
        n_fft=frame_size,
        hop_length=frame_size,
        window='hamming',
        n_mels=MEL_BANDS,
        fmax=max_freq,
        power=2.0,
    )
    mel_db = librosa.power_to_db(S[:, 0], ref=np.max)
    return mel_db


def prepare_target_spectrum(wave_data, sample_rate, frame_size, max_freq=MAX_EVAL_FREQ):
    if wave_data.ndim > 1:
        wave_data = wave_data[:, 0]

    audio = np.asarray(wave_data, dtype=np.float32)
    if sample_rate != SAMPLE_RATE:
        num_samples = int(len(audio) * SAMPLE_RATE / sample_rate)
        audio = signal.resample(audio, num_samples)

    if MELSPECTRUM_MODE:
        return _prepare_mel_spectrum(audio, frame_size, max_freq)
    return _prepare_fft_spectrum(audio, frame_size, max_freq)


def compute_spectrum(signal_data, frame_size, max_freq=MAX_EVAL_FREQ):
    if MELSPECTRUM_MODE:
        return _prepare_mel_spectrum(signal_data, frame_size, max_freq)
    return _prepare_fft_spectrum(signal_data, frame_size, max_freq)


def play_audio(waveform, sample_rate):
    normalized = waveform.astype(np.float32)
    max_abs = np.max(np.abs(normalized))
    if max_abs > 0:
        normalized = normalized / max_abs

    if sd is not None:
        sd.stop()
        sd.play(normalized, sample_rate, blocking=False)
        return

    int16_wave = np.int16(normalized * 32767)
    import tempfile
    import subprocess
    import os

    with tempfile.NamedTemporaryFile(delete=False, suffix='.wav') as temp_wav:
        wavfile.write(temp_wav.name, sample_rate, int16_wave)
        temp_path = temp_wav.name

    try:
        subprocess.run(['afplay', temp_path], check=False)
    finally:
        try:
            os.remove(temp_path)
        except OSError:
            pass


def synthesize_waveform(parameters, sample_count, sample_rate, note_number):
    param_map = {para.jsonHeader: para for para in parameters}

    def raw_value(name, default=0):
        return param_map[name].value if name in param_map else default

    def normalized(name, default=0.0):
        return param_map[name].get_normalized() if name in param_map else default

    base_nn_q8 = note_number * 256
    pitch_bend_semitones = (raw_value("synth-pitch", 64) - 64) * 2.0 / 64.0
    pitch_bend_q8 = int(pitch_bend_semitones * 256.0)

    def voice_offset_q8(voice_index):
        pitch_key = "synth-pitch1" if voice_index == 1 else "synth-pitch2"
        octave_key = "synth-octave-shift1" if voice_index == 1 else "synth-octave-shift2"
        pitch_q8 = (raw_value(pitch_key, 64) - 64) * 4
        octave_q8 = (raw_value(octave_key, 64) - 64) * 256
        return pitch_q8 + octave_q8

    t = np.arange(sample_count, dtype=np.float32) / sample_rate

    attack_level = raw_value("synth-attack-level", 32) * 4.0 / 127.0
    attack_ms = raw_value("synth-attack-time", 0) * 1000.0 / 127.0
    decay_ms = raw_value("synth-decay-time", 10) * 1000.0 / 127.0
    release_ms = raw_value("synth-release-time", 50) * 500.0 / 127.0

    env_vol_finish = raw_value("synth-env-vol-diff", 64) - 64
    env_vol_time = raw_value("synth-env-vol-time", 10) * 10000.0 / 127.0
    env_freq_finish_q8 = (raw_value("synth-env-freq-diff", 64) - 64) * 16
    env_freq_time = raw_value("synth-env-freq-time", 10) * 10000.0 / 127.0
    env_fc_finish_q8 = (raw_value("synth-env-fc-diff", 64) - 64) * 16
    env_fc_time = raw_value("synth-env-fc-time", 0) * 10000.0 / 127.0

    tre_amp = raw_value("synth-tre-amp", 0) / 127.0
    tre_freq = raw_value("synth-tre-freq", 0) / 8.0
    tre_type = raw_value("synth-tre-type", 0)
    tre_peak = normalized("synth-tre-peak", 0.5)
    tre_duty = normalized("synth-tre-peak", 0.5)

    vib_amp = raw_value("synth-vib-amp", 0)
    vib_freq = raw_value("synth-vib-freq", 0) / 8.0
    vib_type = raw_value("synth-vib-type", 0)
    vib_peak = normalized("synth-vib-peak", 0.5)
    vib_duty = normalized("synth-vib-peak", 0.5)

    wow_amp = raw_value("synth-wow-amp", 0) << 6
    wow_freq = raw_value("synth-wow-freq", 0) / 8.0
    wow_type = raw_value("synth-wow-type", 0)
    wow_peak = normalized("synth-wow-peak", 0.5)
    wow_duty = normalized("synth-wow-peak", 0.5)

    tre_value = lfo_value(tre_type, tre_amp, tre_freq, tre_peak, tre_duty, t)
    vib_value_q8 = lfo_value(vib_type, vib_amp, vib_freq, vib_peak, vib_duty, t)
    wow_value_q8 = lfo_value(wow_type, wow_amp, wow_freq, wow_peak, wow_duty, t)

    env_vol = envelope_value(env_vol_finish, env_vol_time, t * 1000.0)
    env_freq_q8 = envelope_value(env_freq_finish_q8, env_freq_time, t * 1000.0)
    env_fc_q8 = envelope_value(env_fc_finish_q8, env_fc_time, t * 1000.0)

    voice1_q8 = base_nn_q8 + pitch_bend_q8 + voice_offset_q8(1) + np.round(vib_value_q8).astype(np.int32) + np.round(env_freq_q8).astype(np.int32)
    voice2_q8 = base_nn_q8 + pitch_bend_q8 + voice_offset_q8(2) + np.round(vib_value_q8).astype(np.int32) + np.round(env_freq_q8).astype(np.int32)
    voice1_freq = note_number_q8_to_frequency(voice1_q8)
    voice2_freq = note_number_q8_to_frequency(voice2_q8)
    phase1 = 2.0 * np.pi * np.cumsum(voice1_freq) / sample_rate
    phase2 = 2.0 * np.pi * np.cumsum(voice2_freq) / sample_rate

    sin1 = normalized("synth-sin1-volume") * np.sin(phase1)
    tri1_width = 0.5 + 0.5 * normalized("synth-tri1-peak")
    tri1 = normalized("synth-tri1-volume") * signal.sawtooth(phase1, width=max(0.01, min(0.99, tri1_width)))
    sq1_duty = (raw_value("synth-squ1-duty", 64) / 127.0)
    sq1 = normalized("synth-squ1-volume") * np.where(np.mod(phase1, 2.0 * np.pi) < 2.0 * np.pi * sq1_duty, 1.0, -1.0)

    sin2 = normalized("synth-sin2-volume") * np.sin(phase2)
    tri2_width = 0.5 + 0.5 * normalized("synth-tri2-peak")
    tri2 = normalized("synth-tri2-volume") * signal.sawtooth(phase2, width=max(0.01, min(0.99, tri2_width)))
    sq2_duty = (raw_value("synth-squ2-duty", 64) / 127.0)
    sq2 = normalized("synth-squ2-volume") * np.where(np.mod(phase2, 2.0 * np.pi) < 2.0 * np.pi * sq2_duty, 1.0, -1.0)

    mixed = sin1 + tri1 + sq1 + sin2 + tri2 + sq2
    if np.max(np.abs(mixed)) > 0:
        mixed = mixed / np.max(np.abs(mixed))

    filter_type = 1 if raw_value("synth-filter-type", 0) >= 1 else 0
    filter_q = max(0.1, raw_value("synth-filter-q", 58) / 127.0 * 2.0)
    cutoff_q8 = (raw_value("synth-filter-cutoff", 127) << 8) + np.round(wow_value_q8).astype(np.int32) + np.round(env_fc_q8).astype(np.int32)
    cutoff_q8 = np.clip(cutoff_q8, 0, 127 << 8)
    cutoff_freq = note_number_q8_to_frequency(cutoff_q8)
    cutoff_freq = np.minimum(cutoff_freq, sample_rate * 0.49)

    b0, b1, b2, a1, a2 = design_biquad_coefficients(filter_type, float(np.median(cutoff_freq)), filter_q)
    filtered = apply_biquad(b0, b1, b2, a1, a2, mixed)

    distortion_value = raw_value("synth-distortion", 127) + 1
    if distortion_value < 128:
        threshold = max(0.01, distortion_value / 128.0)
        pre_max = np.max(np.abs(filtered))
        filtered = np.clip(filtered, -threshold, threshold)
        if pre_max > threshold:
            filtered = filtered * (pre_max / threshold)

    adsr = np.ones_like(t)
    if attack_ms > 0:
        mask = t * 1000.0 < attack_ms
        adsr[mask] = attack_level * (1.0 - np.exp(-2.0 * (t[mask] * 1000.0) / attack_ms))
        if decay_ms > 0:
            decay_mask = (t * 1000.0 >= attack_ms) & (t * 1000.0 < attack_ms + decay_ms)
            dt = t[decay_mask] * 1000.0 - attack_ms
            adsr[decay_mask] = 1.0 + (attack_level - 1.0) * np.exp(-2.0 * dt / decay_ms)
            adsr[t * 1000.0 >= attack_ms + decay_ms] = 1.0
    else:
        if decay_ms > 0:
            decay_mask = t * 1000.0 < decay_ms
            adsr[decay_mask] = 1.0 + (attack_level - 1.0) * np.exp(-2.0 * (t[decay_mask] * 1000.0) / decay_ms)
            adsr[t * 1000.0 >= decay_ms] = 1.0

    mixed = filtered * adsr * (1.0 + tre_value) * (1.0 + env_vol / 64.0)
    if np.max(np.abs(mixed)) > 0:
        mixed = mixed / np.max(np.abs(mixed))

    return mixed


def main():
    rate, base_audio_data = wavfile.read("instSample/000_ACOUSTIC_GRAND_PIANO.wav")
    instInput = prepare_target_spectrum(base_audio_data, rate, FRAME_SIZE)

    plot.ion()
    if MELSPECTRUM_MODE:
        frequencies = librosa.mel_frequencies(n_mels=MEL_BANDS, fmax=MAX_EVAL_FREQ)
        xlabel = 'Mel center frequency (Hz)'
    else:
        max_bin = _spectrum_range(FRAME_SIZE, SAMPLE_RATE, MAX_EVAL_FREQ)
        frequencies = np.arange(1, max_bin + 1) * SAMPLE_RATE / FRAME_SIZE
        xlabel = 'Frequency (Hz)'

    fig, ax = plot.subplots()
    ax.plot(frequencies, instInput, color='orange', label='target')
    generated_line, = ax.plot(frequencies, np.full_like(frequencies, -150.0), color='red', label='generated')
    diff_line, = ax.plot(frequencies, np.full_like(frequencies, -150.0), color='grey', label='diff')
    ax.set_ylim(-150, 0)
    ax.set_xlabel(xlabel)
    ax.set_ylabel('Magnitude (dB)')
    ax.legend()
    plot.show()

    synthPara = [
        # Basis
        Parameter("synth-pitch", GApara.NO, 0, MidiMsg.PITCHBEND, 0, 0, 127, 64),

        # Waveform 1
        Parameter("synth-sin1-volume", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1SINEAMP, 0, 127, 100),
        Parameter("synth-tri1-volume", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1TRIAMP, 0, 127, 0),
        Parameter("synth-tri1-peak", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1TRIPEAK, 0, 127, 0),
        Parameter("synth-squ1-volume", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1SQUAMP, 0, 127, 0),
        Parameter("synth-squ1-duty", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1SQUDUTY, 0, 127, 64),
        Parameter("synth-pitch1", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1PITCH, 0, 127, 64),
        Parameter("synth-octave-shift1", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1OCTAVE, 40, 88, 64),

        # Waveform 2
        Parameter("synth-sin2-volume", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2SINEAMP, 0, 127, 0),
        Parameter("synth-tri2-volume", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2TRIAMP, 0, 127, 0),
        Parameter("synth-tri2-peak", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2TRIPEAK, 0, 127, 64),
        Parameter("synth-squ2-volume", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2SQUAMP, 0, 127, 0),
        Parameter("synth-squ2-duty", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2SQUDUTY, 0, 127, 64),
        Parameter("synth-pitch2", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2PITCH, 0, 127, 64),
        Parameter("synth-octave-shift2", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2OCTAVE, 40, 88, 64),
        Parameter("synth-distortion", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_DISTORTION_LEVEL, 0, 127, 127),

        # ADSR
        Parameter("synth-attack-time", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ATTACKTIME, 0, 127, 0),
        Parameter("synth-attack-level", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ATTACKLEVEL, 0, 127, 32),
        Parameter("synth-decay-time", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_DECAYTIME, 0, 127, 10),
        Parameter("synth-release-time", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_RELEASETIME, 0, 127, 127),

        # Filter
        Parameter("synth-filter-type", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_FILTERTYPE, 0, 1, 0),
        Parameter("synth-filter-cutoff", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_FILTERCUTOFF, 50, 127, 127),
        Parameter("synth-filter-q", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_FILTERQFACTOR, 0, 127, 58),

        # LFO
        Parameter("synth-vib-type", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_VIB_TYPE, 0, 2, 0),
        Parameter("synth-vib-peak", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_VIB_PEAK, 0, 127, 0),
        Parameter("synth-vib-amp", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_VIB_AMP, 0, 127, 0),
        Parameter("synth-vib-freq", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_VIB_FREQ, 0, 127, 0),
        Parameter("synth-tre-type", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_TRE_TYPE, 0, 2, 0),
        Parameter("synth-tre-peak", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_TRE_PEAK, 0, 127, 0),
        Parameter("synth-tre-amp", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_TRE_AMP, 0, 127, 0),
        Parameter("synth-tre-freq", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_TRE_FREQ, 0, 127, 0),
        Parameter("synth-wow-type", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WOW_TYPE, 0, 2, 0),
        Parameter("synth-wow-peak", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WOW_PEAK, 0, 127, 0),
        Parameter("synth-wow-amp", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WOW_AMP, 0, 127, 0),
        Parameter("synth-wow-freq", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WOW_FREQ, 0, 127, 0),

        # Envelope
        Parameter("synth-env-vol-diff", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_VOL_DIFF, 0, 127, 64),
        Parameter("synth-env-vol-time", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_VOL_TIME, 0, 127, 10),
        Parameter("synth-env-freq-diff", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_FREQ_DIFF, 0, 127, 64),
        Parameter("synth-env-freq-time", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_FREQ_TIME, 0, 127, 10),
        Parameter("synth-env-fc-diff", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_WOW_DIFF, 0, 127, 64),
        Parameter("synth-env-fc-time", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_ENV_WOW_TIME, 0, 127, 0),
    ]

    creator.create("FitnessSingle", base.Fitness, weights=(-1.0,))
    creator.create("Individual", list, fitness=creator.FitnessSingle)

    toolbox = base.Toolbox()

    def create_individual():
        individual = []
        for p in synthPara:
            if p.gapara == GApara.NO:
                individual.append(p.resetValue)
            else:
                individual.append(random.randint(p.valueMin, p.valueMax))
        return creator.Individual(individual)

    def mutate_bounded(individual, mu, sigma, indpb):
        for i in range(len(individual)):
            if synthPara[i].gapara == GApara.NO:
                individual[i] = synthPara[i].resetValue
                continue
            if random.random() < indpb:
                if random.random() < 0.15:
                    individual[i] = random.randint(synthPara[i].valueMin, synthPara[i].valueMax)
                else:
                    individual[i] += random.gauss(mu, sigma)
                    individual[i] = int(round(individual[i]))
                    individual[i] = max(synthPara[i].valueMin, min(synthPara[i].valueMax, individual[i]))
        return (individual,)

    def crossover_bounded(ind1, ind2):
        if len(ind1) < 2:
            return ind1, ind2
        cxpoint1 = random.randint(1, len(ind1) - 2)
        cxpoint2 = random.randint(cxpoint1 + 1, len(ind1) - 1)
        ind1[cxpoint1:cxpoint2], ind2[cxpoint1:cxpoint2] = ind2[cxpoint1:cxpoint2], ind1[cxpoint1:cxpoint2]
        # Ensure GA.NO parameters are always reset values
        for i in range(len(ind1)):
            if synthPara[i].gapara == GApara.NO:
                ind1[i] = synthPara[i].resetValue
                ind2[i] = synthPara[i].resetValue
        return ind1, ind2

    def evaluate_parameters(individual):
        for i, para in enumerate(synthPara):
            if para.gapara == GApara.NO:
                para.setResetVal()
            else:
                para.setVal(int(individual[i]))

        playback_samples = int(SAMPLE_RATE * PLAYBACK_DURATION_SECONDS)
        generated = synthesize_waveform(synthPara, playback_samples, SAMPLE_RATE, NoteNum.NN_C5)

        spectrum_source = generated[:FRAME_SIZE]
        if spectrum_source.shape[0] < FRAME_SIZE:
            padded = np.zeros(FRAME_SIZE, dtype=np.float32)
            padded[:spectrum_source.shape[0]] = spectrum_source
            spectrum_source = padded

        generated_spectrum = compute_spectrum(spectrum_source, FRAME_SIZE, MAX_EVAL_FREQ)
        diff_input = np.sqrt(np.square(instInput - generated_spectrum))
        diff_input_sum = np.sum(diff_input)

        generated_line.set_ydata(generated_spectrum)
        diff_line.set_ydata(instInput - generated_spectrum)
        plot.draw()
        plot.pause(0.001)

        return (diff_input_sum,)

    toolbox.register("individual", create_individual)
    toolbox.register("population", tools.initRepeat, list, toolbox.individual)
    toolbox.register("evaluate", evaluate_parameters)
    toolbox.register("mate", crossover_bounded)
    toolbox.register("mutate", mutate_bounded, mu=0, sigma=30, indpb=0.4)
    toolbox.register("select", tools.selTournament, tournsize=3)

    population = toolbox.population(n=10)
    print("Genetic Algorithm Optimization Started...")
    for gen in range(NGEN):
        print(f"Generation {gen+1}/{NGEN}")
        offspring = toolbox.select(population, len(population))
        offspring = [toolbox.clone(ind) for ind in offspring]

        for i in range(1, len(offspring), 2):
            if random.random() < 0.7:
                toolbox.mate(offspring[i - 1], offspring[i])
                del offspring[i - 1].fitness.values
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

        # Play the current generation's best candidate once per generation,
        # without blocking the GA evaluation itself.
        generation_best = tools.selBest(population, k=1)[0]
        for i, para in enumerate(synthPara):
            para.setVal(int(generation_best[i]))
        best_wave = synthesize_waveform(synthPara, int(SAMPLE_RATE * PLAYBACK_DURATION_SECONDS), SAMPLE_RATE, NoteNum.NN_C5)
        play_audio(best_wave, SAMPLE_RATE)

    best_ind = tools.selBest(population, k=1)[0]
    print(f"Best diffInputSum: {best_ind.fitness.values[0]:.2f}")

    for i, para in enumerate(synthPara):
        para.setVal(int(best_ind[i]))

    best_generated = synthesize_waveform(synthPara, FRAME_SIZE, SAMPLE_RATE, NoteNum.NN_C5)
    best_spectrum = compute_spectrum(best_generated, FRAME_SIZE, MAX_EVAL_FREQ)
    if MELSPECTRUM_MODE:
        xf = librosa.mel_frequencies(n_mels=MEL_BANDS, fmax=MAX_EVAL_FREQ)
    else:
        max_bin = _spectrum_range(FRAME_SIZE, SAMPLE_RATE, MAX_EVAL_FREQ)
        xf = np.arange(1, max_bin + 1) * SAMPLE_RATE / FRAME_SIZE

    plot.plot(xf, instInput, color='orange', label='target')
    generated_line, = plot.plot(xf, best_spectrum, color='red', label='generated')
    diff_line, = plot.plot(xf, instInput - best_spectrum, color='grey', label='diff')
    plot.ylim(-150, 0)
    plot.legend()
    plot.draw()

    for i in range(3):
        print(f"Validation iteration {i+1}/3")
        validation = synthesize_waveform(synthPara, FRAME_SIZE, SAMPLE_RATE, NoteNum.NN_C5)
        validation_spectrum = compute_spectrum(validation, FRAME_SIZE)
        diff_input = np.sqrt(np.square(instInput - validation_spectrum))
        diff_input_sum = np.sum(diff_input)
        print(f"  diffInputSum: {diff_input_sum:.2f}")

        generated_line.set_ydata(validation_spectrum)
        diff_line.set_ydata(instInput - validation_spectrum)
        plot.pause(0.1)
        time.sleep(0.5)

    data = {para.jsonHeader: para.value for para in synthPara}
    # print(json.dumps(data))
    with open('data.json', 'w') as f:
       json.dump(data, f)


if __name__ == "__main__":
    main()
