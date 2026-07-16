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

# local import
from midi_para import MidiMsg, MidiCC, NoteNum, Parameter, GApara
import synth_waveform
#import nsynth

FRAME_SIZE_TARGET    = 4096      # フレームサイズ(ターゲット音声のスペクトラム計算に使用)
FRAME_SIZE_SAMPLE    = 4096      # フレームサイズ(テスト用音声のスペクトラム計算に使用)
SAMPLE_RATE          = 44100     # サンプリングレート
MAX_EVAL_FREQ        = 10000     # 10 kHz 以上を評価に含めない
TEST_PLAY_DURATION   = 0.05      # パラメータ変更時に再生する音声長
BEST_PLAY_DURATION   = 1.0       # パラメータ変更時に再生する音声長
NGEN                 = 3000      # GA世代数
MELSPECTRUM_MODE     = False     # メルスペクトラムを使用するかどうか
MEL_BANDS            = 64        # メルバンド数

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


def _prepare_average_spectrum(audio, frame_size, max_freq):
    if audio.ndim > 1:
        audio = audio[:, 0]

    audio = np.asarray(audio, dtype=np.float32)
    if np.max(np.abs(audio)) > 0:
        audio = audio / np.max(np.abs(audio))

    total_samples = audio.shape[0]
    if total_samples == 0:
        audio = np.zeros(frame_size, dtype=np.float32)
        total_samples = frame_size

    spectra = []
    for start in range(0, total_samples, frame_size):
        frame = audio[start:start + frame_size]
        if frame.shape[0] < frame_size:
            padded = np.zeros(frame_size, dtype=np.float32)
            padded[:frame.shape[0]] = frame
            frame = padded

        if MELSPECTRUM_MODE:
            spectra.append(_prepare_mel_spectrum(frame, frame_size, max_freq))
        else:
            spectra.append(_prepare_fft_spectrum(frame, frame_size, max_freq))

    return np.mean(np.asarray(spectra, dtype=np.float32), axis=0)


def prepare_target_spectrum(wave_data, sample_rate, frame_size, max_freq=MAX_EVAL_FREQ):
    if wave_data.ndim > 1:
        wave_data = wave_data[:, 0]

    audio = np.asarray(wave_data, dtype=np.float32)
    if sample_rate != SAMPLE_RATE:
        num_samples = int(len(audio) * SAMPLE_RATE / sample_rate)
        audio = signal.resample(audio, num_samples)

    return _prepare_average_spectrum(audio, frame_size, max_freq)


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


def main():
    rate, base_audio_data = wavfile.read("instSample/000_ACOUSTIC_GRAND_PIANO.wav")
    instInput = prepare_target_spectrum(base_audio_data, rate, FRAME_SIZE_TARGET)

    plot.ion()
    if MELSPECTRUM_MODE:
        frequencies = librosa.mel_frequencies(n_mels=MEL_BANDS, fmax=MAX_EVAL_FREQ)
        xlabel = 'Mel center frequency (Hz)'
    else:
        max_bin = _spectrum_range(FRAME_SIZE_TARGET, SAMPLE_RATE, MAX_EVAL_FREQ)
        frequencies = np.arange(1, max_bin + 1) * SAMPLE_RATE / FRAME_SIZE_TARGET
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
        Parameter("synth-pitch1", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1PITCH, 0, 127, 64),
        Parameter("synth-octave-shift1", GApara.NO, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE1OCTAVE, 52, 76, 64),

        # Waveform 2
        Parameter("synth-sin2-volume", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2SINEAMP, 0, 127, 0),
        Parameter("synth-tri2-volume", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2TRIAMP, 0, 127, 0),
        Parameter("synth-tri2-peak", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2TRIPEAK, 0, 127, 64),
        Parameter("synth-squ2-volume", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2SQUAMP, 0, 127, 0),
        Parameter("synth-squ2-duty", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2SQUDUTY, 0, 127, 64),
        Parameter("synth-pitch2", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2PITCH, 0, 127, 64),
        Parameter("synth-octave-shift2", GApara.YES, 0, MidiMsg.CONTROLCHANGE, MidiCC.USER_WAVE2OCTAVE, 52, 76, 64),
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

        playback_samples = int(SAMPLE_RATE * TEST_PLAY_DURATION)
        generated = synth_waveform.create(synthPara, playback_samples, SAMPLE_RATE, NoteNum.NN_C5)

        spectrum_source = generated[:FRAME_SIZE_SAMPLE]
        if spectrum_source.shape[0] < FRAME_SIZE_SAMPLE:
            padded = np.zeros(FRAME_SIZE_SAMPLE, dtype=np.float32)
            padded[:spectrum_source.shape[0]] = spectrum_source
            spectrum_source = padded

        generated_spectrum = compute_spectrum(spectrum_source, FRAME_SIZE_SAMPLE, MAX_EVAL_FREQ)
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
    toolbox.register("mutate", mutate_bounded, mu=0, sigma=40, indpb=0.6)
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
        best_wave = synth_waveform.create(synthPara, int(SAMPLE_RATE * BEST_PLAY_DURATION), SAMPLE_RATE, NoteNum.NN_C5)
        play_audio(best_wave, SAMPLE_RATE)

    best_ind = tools.selBest(population, k=1)[0]
    print(f"Best diffInputSum: {best_ind.fitness.values[0]:.2f}")

    for i, para in enumerate(synthPara):
        para.setVal(int(best_ind[i]))

    best_generated = synth_waveform.create(synthPara, FRAME_SIZE_SAMPLE, SAMPLE_RATE, NoteNum.NN_C5)
    best_spectrum = compute_spectrum(best_generated, FRAME_SIZE_SAMPLE, MAX_EVAL_FREQ)
    if MELSPECTRUM_MODE:
        xf = librosa.mel_frequencies(n_mels=MEL_BANDS, fmax=MAX_EVAL_FREQ)
    else:
        max_bin = _spectrum_range(FRAME_SIZE_SAMPLE, SAMPLE_RATE, MAX_EVAL_FREQ)
        xf = np.arange(1, max_bin + 1) * SAMPLE_RATE / FRAME_SIZE_SAMPLE

    plot.plot(xf, instInput, color='orange', label='target')
    generated_line, = plot.plot(xf, best_spectrum, color='red', label='generated')
    diff_line, = plot.plot(xf, instInput - best_spectrum, color='grey', label='diff')
    plot.ylim(-150, 0)
    plot.legend()
    plot.draw()

    for i in range(3):
        print(f"Validation iteration {i+1}/3")
        validation = synth_waveform.create(synthPara, FRAME_SIZE_SAMPLE, SAMPLE_RATE, NoteNum.NN_C5)
        validation_spectrum = compute_spectrum(validation, FRAME_SIZE_SAMPLE, MAX_EVAL_FREQ)
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
