import numpy as np
from scipy import signal

SAMPLE_RATE   = 44100            # サンプリングレート

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


def create(parameters, sample_count, sample_rate, note_number):
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
