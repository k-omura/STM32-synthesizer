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

# local import
from midi_para import MidiMsg, MidiCC, NoteNum, Parameter, GApara

FRAME_SIZE    = 2048         # フレームサイズ
SAMPLE_RATE   = 44100        # サンプリングレート
NGEN          = 50           # GA世代数

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