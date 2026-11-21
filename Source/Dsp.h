#pragma once
// Rode5tune - core DSP building blocks (pure C++17, no JUCE dependency)
#include "MicProfiles.h"
#include <algorithm>
#include <cmath>
#include <complex>

namespace r5t
{
constexpr int    kNumSlots = 11;   // 0 lowcut, 1-2 neutralizer, 3-7 mic, 8 proximity, 9 presence, 10 air
constexpr double kPi       = 3.14159265358979323846;

struct Settings
{
    int   mic = 0;
    int   preamp = 0;          // PreampType
    float trimDb = 0.f;
    float lowCutHz = 20.f;
    float proximity = 0.f;     // -1..+1
    float presenceDb = 0.f;
    float airDb = 0.f;
    float drive = 0.5f;        // 0..1
    float deess = 0.f;         // 0..1
    float outDb = 0.f;
    float mix = 1.f;           // 0..1
    bool  neutralize = true;
};

struct Biquad { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; bool active = false; };
struct BiquadState { double z1 = 0, z2 = 0; };

inline Biquad makeBiquad (const BandSpec& s, double fs)
{
    Biquad c;
    if (s.type == BandType::None) return c;
    if (s.type != BandType::HighPass && std::fabs (s.gainDb) < 0.01f) return c;

    const double f     = std::clamp ((double) s.freq, 10.0, fs * 0.45);
    const double w0    = 2.0 * kPi * f / fs;
    const double cw    = std::cos (w0), sw = std::sin (w0);
    const double q     = std::max (0.1, (double) s.q);
    const double alpha = sw / (2.0 * q);
    const double A     = std::pow (10.0, (double) s.gainDb / 40.0);
    const double sA2a  = 2.0 * std::sqrt (A) * alpha;
    double b0, b1, b2, a0, a1, a2;

    switch (s.type)
    {
        case BandType::Peak:
            b0 = 1 + alpha * A; b1 = -2 * cw; b2 = 1 - alpha * A;
            a0 = 1 + alpha / A; a1 = -2 * cw; a2 = 1 - alpha / A; break;
        case BandType::LowShelf:
            b0 = A * ((A + 1) - (A - 1) * cw + sA2a);
            b1 = 2 * A * ((A - 1) - (A + 1) * cw);
            b2 = A * ((A + 1) - (A - 1) * cw - sA2a);
            a0 = (A + 1) + (A - 1) * cw + sA2a;
            a1 = -2 * ((A - 1) + (A + 1) * cw);
            a2 = (A + 1) + (A - 1) * cw - sA2a; break;
        case BandType::HighShelf:
            b0 = A * ((A + 1) + (A - 1) * cw + sA2a);
            b1 = -2 * A * ((A - 1) + (A + 1) * cw);
            b2 = A * ((A + 1) + (A - 1) * cw - sA2a);
            a0 = (A + 1) - (A - 1) * cw + sA2a;
            a1 = 2 * ((A - 1) - (A + 1) * cw);
            a2 = (A + 1) - (A - 1) * cw - sA2a; break;
        case BandType::HighPass:
        default:
            b0 = (1 + cw) / 2; b1 = -(1 + cw); b2 = (1 + cw) / 2;
            a0 = 1 + alpha; a1 = -2 * cw; a2 = 1 - alpha; break;
    }
    c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0; c.a1 = a1 / a0; c.a2 = a2 / a0;
    c.active = true;
    return c;
}

inline double run (const Biquad& c, BiquadState& s, double x)
{
    const double y = c.b0 * x + s.z1;
    s.z1 = c.b1 * x - c.a1 * y + s.z2;
    s.z2 = c.b2 * x - c.a2 * y;
    return y;
}

inline double magnitudeDb (const Biquad& c, double f, double fs)
{
    if (! c.active) return 0.0;
    const double w = 2.0 * kPi * f / fs;
    const std::complex<double> z1 = std::polar (1.0, -w), z2 = std::polar (1.0, -2.0 * w);
    const auto num = c.b0 + c.b1 * z1 + c.b2 * z2;
    const auto den = 1.0 + c.a1 * z1 + c.a2 * z2;
    return 20.0 * std::log10 (std::abs (num / den) + 1e-12);
}

// Reference neutralizer: gently flattens the NT1 5th Gen's slight presence/air lift (approximation).
inline void buildBands (const Settings& s, BandSpec (&o)[kNumSlots])
{
    const auto& m = kMics[std::clamp (s.mic, 0, kNumMics - 1)];
    o[0]  = { s.lowCutHz > 21.f ? BandType::HighPass : BandType::None, s.lowCutHz, 0.f, 0.7071f };
    o[1]  = { s.neutralize ? BandType::Peak : BandType::None, 7500.f, -1.5f, 0.9f };
    o[2]  = { s.neutralize ? BandType::HighShelf : BandType::None, 14000.f, -0.5f, 0.7f };
    for (int i = 0; i < 5; ++i) o[3 + i] = m.bands[i];
    o[8]  = { BandType::LowShelf, 160.f, s.proximity * 6.0f * m.proxFactor, 0.6f };
    o[9]  = { BandType::Peak, m.presenceHz, s.presenceDb, 0.9f };
    o[10] = { BandType::HighShelf, 11000.f, s.airDb, 0.6f };
}

inline void buildBiquads (const Settings& s, double fs, Biquad (&out)[kNumSlots])
{
    BandSpec b[kNumSlots];
    buildBands (s, b);
    for (int i = 0; i < kNumSlots; ++i) out[i] = makeBiquad (b[i], fs);
}

inline double totalResponseDb (const Biquad (&bq)[kNumSlots], double f, double fs)
{
    double db = 0.0;
    for (int i = 0; i < kNumSlots; ++i) db += magnitudeDb (bq[i], f, fs);
    return db;
}

// Resolve effective preamp type + drive amount for the current settings.
inline void resolvePreamp (const Settings& s, int& type, float& d)
{
    const auto& m = kMics[std::clamp (s.mic, 0, kNumMics - 1)];
    if (s.preamp == PreampNative) { type = m.nativePreamp; d = m.nativeDrive * s.drive * 2.0f; }
    else                          { type = s.preamp;       d = s.drive; }
    d = std::min (d, 1.5f);
}

struct Saturator
{
    int   type = PreampClean;
    float k = 1.f, bias = 0.f, biasOut = 0.f, lpCoef = 0.f;
    float lp[2] = { 0.f, 0.f };

    bool active() const { return type != PreampClean; }

    void configure (int t, float d, double fs)
    {
        type = (d > 0.005f) ? t : (int) PreampClean;
        k = 1.f + 5.f * d;
        const float dd = std::min (d, 1.f);
        if (type == PreampTube)        { bias = 0.25f * dd; biasOut = std::tanh (bias); }
        else if (type == PreampFet)    { bias = 0.20f * dd; biasOut = bias / (1.f + bias); }
        else if (type == PreampTransformer) lpCoef = 1.f - (float) std::exp (-2.0 * kPi * 150.0 / fs);
    }

    inline float process (float x, int ch)
    {
        switch (type)
        {
            case PreampTube:  return (std::tanh (k * x + bias) - biasOut) / k;
            case PreampFet:   { const float xb = k * x + bias; return (xb / (1.f + std::fabs (xb)) - biasOut) * (1.f + bias) * (1.f + bias) / k; }
            case PreampTransformer:
            {
                lp[ch] += lpCoef * (x - lp[ch]);
                const float a = 0.8f;
                return std::tanh (k * (x + a * lp[ch])) / k - a * lp[ch];
            }
            default: return x;
        }
    }
};

struct DeEsser
{
    Biquad hp; BiquadState st[2]; double env[2] = { 0, 0 };
    double atk = 0, rel = 0, thrDb = -10; bool on = false;

    void prepare (double fs)
    {
        hp  = makeBiquad ({ BandType::HighPass, 5500.f, 0.f, 0.7071f }, fs);
        atk = std::exp (-1.0 / (0.0005 * fs));
        rel = std::exp (-1.0 / (0.05 * fs));
        reset();
    }
    void reset() { st[0] = st[1] = {}; env[0] = env[1] = 0; }
    void setAmount (float a) { on = a > 0.01f; thrDb = -10.0 - 35.0 * a; }

    inline float process (float x, int ch)
    {
        const double band = run (hp, st[ch], x);
        const double r = std::fabs (band);
        env[ch] = r + (r > env[ch] ? atk : rel) * (env[ch] - r);
        const double over = 20.0 * std::log10 (env[ch] + 1e-9) - thrDb;
        double g = 1.0;
        if (over > 0.0) g = std::pow (10.0, -std::min (12.0, over * 0.75) / 20.0);
        return (float) (x - band + band * g);
    }
};
} // namespace r5t
