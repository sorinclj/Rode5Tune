# RODE NT1 5th Generation - analysis and design decisions

## Published specification (RODE datasheet)
* 1" gold-sputtered HF6 capsule, cardioid, JFET impedance converter
* Frequency range 20 Hz - 20 kHz
* Self-noise about 4 dBA, max SPL about 142 dB, sensitivity about -32 dBV/Pa
* XLR (+48 V) and USB-C (32-bit float) outputs, on-board DSP (Aphex) available via RODE Central

## What this means for the plugin
1. **Neutral, low-noise base** -> ideal source for voicing-based emulation: every target is "NT1 + a few dB of shaping".
2. **Slight presence/air lift** -> the NT1 Neutralizer (-1.5 dB @ 7.5 kHz, -0.5 dB shelf @ 14 kHz) levels the starting point. These values are a conservative approximation (the datasheet plot has no numeric table).
3. **4 dBA self-noise** -> room to add drive/saturation and air without exposing hiss.
4. **Cardioid + close-talking** -> the NT1 already has proximity effect. PROXIMITY therefore ranges from -100 % (tighten) to +100 % (more bass build-up).
5. **142 dB SPL** -> the hardware will not clip before the plugin; headroom is controlled with TRIM/OUTPUT.

## What the best emulators contain, and what Rode5tune implements
| Feature (Slate VMS / Antelope / UAD class) | Rode5tune |
|---|---|
| Multiple mic models | 10 flagship models |
| Selectable preamp / electronics colour | Native / Clean / Transformer / Tube / FET + Drive |
| Proximity effect modelling | Yes, per-mic strength |
| Presence / air shaping | Yes, per-mic presence frequency |
| Low cut | Yes |
| Source-mic correction | NT1 Neutralizer |
| Sibilance control | Built-in split-band de-esser |
| Metering | VU meters in/out |
| Dry/wet | Yes, latency compensated |
| Oversampling on non-linear stage | 2x |
| Polar pattern / axis angle | Not possible with a single-pattern mic (needs dual-capsule hardware) |

## Roadmap to the "next level"
* **Measured profiles:** record the same voice at the same distance with the NT1 and a target mic, compute the long-term average spectrum difference (or deconvolve an impulse response) and replace the 5-band curve in `MicProfiles.h`, or load it as a convolution IR.
* Add a gentle optional compressor/limiter stage (Antelope-style "mic + comp").
* Add Off-axis/room-reject simulation and a de-plosive.
* Preset manager and A/B comparison slots.
