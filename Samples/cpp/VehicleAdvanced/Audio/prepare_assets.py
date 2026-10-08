"""Recreate the bundled PCM loops with Python's standard library.

Run from this directory. Network is needed only to recreate source recordings,
never to build or run the sample. See CREDITS.md for licences and source URLs.
"""
import array
import math
from pathlib import Path
import random
import struct
import urllib.request
import wave

ROOT = Path(__file__).resolve().parent
BASE = "https://opengameart.org/sites/default/files/"


def save(name, samples, rate):
    rms = math.sqrt(sum(x * x for x in samples) / len(samples))
    peak = max(abs(x) for x in samples)
    gain = min(0.18 / max(rms, 1e-9), 0.9 / max(peak, 1e-9))
    with wave.open(str(ROOT / name), "wb") as wav:
        wav.setparams((1, 2, rate, 0, "NONE", "not compressed"))
        wav.writeframes(b"".join(struct.pack("<h", round(x * gain * 32767)) for x in samples))


for index in range(7):
    source = ("loop_0.wav" if index == 0 else f"loop_{index}_0.wav") if index < 6 else "tires_squal_loop.wav"
    target = f"engine_{index}.wav" if index < 6 else "tyre_squeal.wav"
    # Decode without keeping a second copy of the downloaded recordings.
    import io
    with wave.open(io.BytesIO(urllib.request.urlopen(BASE + source).read()), "rb") as wav:
        assert wav.getnchannels() == 1
        width, rate = wav.getsampwidth(), wav.getframerate()
        data = wav.readframes(wav.getnframes())
        samples = [int.from_bytes(data[i:i + width], "little", signed=True) / float(1 << (width * 8 - 1))
                   for i in range(0, len(data), width)]
    save(target, samples, rate)

# Original, seeded asphalt rolling-noise loop. Circular filtering avoids a seam.
rate = 44100
rng = random.Random(7)
noise = [rng.uniform(-1, 1) for _ in range(rate * 4)]


def lowpass(values, cutoff):
    blend = 1 - math.exp(-2 * math.pi * cutoff / rate)
    result = [0.] * len(values)
    state = 0.
    for _ in range(3):
        for i, value in enumerate(values):
            state += blend * (value - state)
            result[i] = state
    return result


low, high = lowpass(noise, 220), lowpass(noise, 2800)
save("tyre_roll.wav", [b - a for a, b in zip(low, high)], rate)
