import json
import os
import numpy as np
from scipy.io import wavfile

JSON_FILE = "./instSample/nsynth-test/examples.json"
AUDIO_FOLDER = "./instSample/nsynth-test/audio"

def get_waveform_by_midi(gm_id, note_number, velocity=127):
    """
    MIDIパラメータからNSynth(test)のWAVを検索し、wavfile.readの解析結果を返す関数
    
    Parameters:
        gm_id (int): General MIDIの楽器プログラム番号 (1 ~ 128)
        note_number (int): MIDIノート番号 (0 ~ 127)
        velocity (int): MIDIベロシティ (0 ~ 127)
        
    Returns:
        tuple: (sampling_rate, data_array, file_name) 該当なしの場合は (None, None, None)
    """
    # 1. General MIDI (1-128) から NSynthの instrument_family_str への簡易マッピング
    # ※NSynthに存在しないカテゴリ（SFXやパーカッション等）は、便宜上近いものに割り当てています。
    def gm_to_nsynth_family(gm_num):
        if 1 <= gm_num <= 8:    return "keyboard"   # Acoustic Piano系
        if 9 <= gm_num <= 16:   return "keyboard"   # Chromatic Percussion系 (オルガン/チェレスタ等)
        if 17 <= gm_num <= 24:  return "keyboard"   # Organ系
        if 25 <= gm_num <= 32:  return "guitar"     # Guitar系
        if 33 <= gm_num <= 40:  return "bass"       # Bass系
        if 41 <= gm_num <= 48:  return "string"     # Strings系
        if 49 <= gm_num <= 56:  return "string"     # Ensemble系
        if 57 <= gm_num <= 64:  return "brass"      # Brass系
        if 65 <= gm_num <= 72:  return "reed"       # Reed系 (サックス/オーボエ等)
        if 73 <= gm_num <= 80:  return "flute"      # Pipe系 (フルート/オカリナ等)
        if 81 <= gm_num <= 88:  return "synth_lead" # Synth Lead系
        if 89 <= gm_num <= 96:  return "vocal"      # Synth Pad系 (声に近い質感として割り当て)
        if 97 <= gm_num <= 104: return "synth_lead" # Synth SFX系
        if 105 <= gm_num <= 112: return "string"    # Ethnic系 (シタール/バンジョー等)
        if 113 <= gm_num <= 120: return "mallet"    # Percussive系 (ティンパニ/スチールドラム等)
        if 121 <= gm_num <= 128: return "synth_lead" # Sound Effects系
        return "keyboard"

    target_family = gm_to_nsynth_family(gm_id)

    # 2. NSynthの固定ベロシティ値 (25, 50, 75, 100, 127) に近似
    nsynth_velocities = [25, 50, 75, 100, 127]
    target_velocity = min(nsynth_velocities, key=lambda x: abs(x - velocity))

    # 3. JSONメタデータの読み込み
    if not os.path.exists(JSON_FILE):
        print(f"Error: JSONファイルが見つかりません。パスを確認してください: {JSON_FILE}")
        return None, None, None

    with open(JSON_FILE, "r") as f:
        metadata = json.load(f)

    # 4. 条件（ファミリー、ピッチ、ベロシティ）に合致するサンプルを検索
    matched_sample_id = None
    for sample_id, info in metadata.items():
        if (info["instrument_family_str"] == target_family and 
            info["pitch"] == note_number and 
            info["velocity"] == target_velocity):
            matched_sample_id = sample_id
            break  # 最初に見つかった楽器のサンプルを採用

    # 5. WAVファイルの解析
    if matched_sample_id:
        wav_filename = f"{matched_sample_id}.wav"
        wav_path = os.path.join(AUDIO_FOLDER, wav_filename)
        
        if os.path.exists(wav_path):
            # scipy.io.wavfile.read を実行
            sample_rate, data = wavfile.read(wav_path)
            return sample_rate, data, wav_filename
        else:
            print(f"Warning: メタデータにはありますが、WAVファイルが存在しません: {wav_path}")
            return None, None, None
    else:
        print(f"該当するサンプルが nsynth-test 内に見つかりませんでした。 (Family: {target_family}, Pitch: {note_number}, Vel: {target_velocity})")
        return None, None, None

# --- 実行例 ---
# if __name__ == "__main__":
#     # 例：General MIDI=1 (Acoustic Grand Piano), ノート番号=60 (真ん中のド), ベロシティ=90
#     test_gm = 1
#     test_note = 60
#     test_vel = 90
#     sr, waveform_data, file_name = get_waveform_by_midi(
#         gm_id=test_gm, 
#         note_number=test_note, 
#         velocity=test_vel, 
#     )
#     if waveform_data is not None:
#         print("【解析成功】")
#         print(f"採用ファイル名: {file_name}")
#         print(f"サンプリングレート: {sr} Hz")
#         print(f"データ型: {waveform_data.dtype}")
#         print(f"配列の形状 (データポイント数): {waveform_data.shape}")
#         print(f"振幅の最大値: {np.max(waveform_data)}")
#         print(f"振幅の最小値: {np.min(waveform_data)}")
#         print(f"最初の5つの振幅データ: {waveform_data[:5]}")
