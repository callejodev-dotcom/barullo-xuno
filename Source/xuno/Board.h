// Board.h — O que hai despois das seis voces, na placa: o filtro paso alto de catro posicións, o
// VCA xeral (uPC1252H2, o deslizador VCA LEVEL, ±10 dB), o chorus de dous BBD MN3009 e o xerador de ruído.
// Todo a 190 476 Hz, coma as voces.
#pragma once
#include "Voice.h"
#include <array>
#include <cmath>

namespace xuno {

// ================================================================ filtros simples (transformada bilineal)
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline double run (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }

    // H(s) = (n0 + n1 s + n2 s²) / (d0 + d1 s + d2 s²), con prewarp en fw (Hz)
    void analog (double n0, double n1, double n2, double d0, double d1, double d2, double fs, double fw = 0)
    {
        const double K = fw > 0 ? 2 * PI * fw / std::tan (PI * fw / fs) : 2 * fs, K2 = K * K;
        const double a0 = d0 + d1 * K + d2 * K2;
        b0 = (n0 + n1 * K + n2 * K2) / a0; b1 = 2 * (n0 - n2 * K2) / a0; b2 = (n0 - n1 * K + n2 * K2) / a0;
        a1 = 2 * (d0 - d2 * K2) / a0; a2 = (d0 - d1 * K + d2 * K2) / a0;
    }
    void lowpass (double fc, double q, double fs) { const double w = 2 * PI * fc; analog (1, 0, 0, 1, 1 / (q * w), 1 / (w * w), fs, fc); }
    void highpass1 (double fc, double fs) { const double w = 2 * PI * fc; analog (0, 1 / w, 0, 1, 1 / w, 0, fs, fc); }
    void lowpass1 (double fc, double fs) { const double w = 2 * PI * fc; analog (1, 0, 0, 1, 1 / w, 0, fs, fc); }
};

// ================================================================ HPF (IC40 elixe o condensador cun CD4051)
// 0: realce de graves (dous pasos con M5218 e un sumador inversor, valores do esquema)
// 1: plano · 2: paso alto de 236 Hz (0,015 µF, 44,9 kΩ) · 3: paso alto de 754 Hz (0,0047 µF)
struct Hpf
{
    int pos = 1;
    Biquad boost, hp236, hp754;

    void prepare (double fs)
    {
        // H(s) = 1 + (R45/R44)·G2(s)·H1(s)
        const double R1 = 47e3, C1 = 0.047e-6, CA = 0.01e-6, Rg = 10e3, Rf = 100e3, Cf = 0.022e-6, R43 = 47e3, R44 = 220e3, R45 = 47e3;
        const double t1z = R1 * C1, t1p = R1 * (C1 + CA), t2z = (Rg * Rf / (Rg + Rf)) * Cf, t2p = Rf * Cf;
        const double ag = (R45 / R44) * (1 + Rf / Rg), direct = R45 / R43;
        const double D0 = 1, D1 = t1p + t2p, D2 = t1p * t2p;
        boost.analog (direct * D0 + ag, direct * D1 + ag * (t1z + t2z), direct * D2 + ag * t1z * t2z, D0, D1, D2, fs);
        hp236.highpass1 (1.0 / (2 * PI * 44.9e3 * 0.015e-6), fs);
        hp754.highpass1 (1.0 / (2 * PI * 44.9e3 * 0.0047e-6), fs);
    }
    void reset() { boost.reset(); hp236.reset(); hp754.reset(); }

    inline double run (double x)
    {
        // os catro van sempre, así o cambio de posición non arranca un filtro baleiro
        const double b = boost.run (x), h1 = hp236.run (x), h2 = hp754.run (x);
        switch (pos) { case 0: return b; case 2: return h1; case 3: return h2; default: return x; }
    }
};

// ================================================================ BBD MN3009 (256 etapas = 128 mostras)
// O sinal pásase ao reloxo do BBD de verdade: en cada pulso de reloxo collese unha mostra (interpolada
// entre dúas internas) e sae a de hai 128 pulsos, retida ata o seguinte. O reloxo vén dun VCO que move
// o LFO do chorus, así que o atraso é 128 / f_reloxo. Filtros de entrada e saída: Butterworth de
// 2.º orde a 9,66 kHz e un polo a 20 kHz (os dous filtros de transistores do esquema).
struct Bbd
{
    std::array<double, 256> line {};
    int w = 0;
    double phase = 0.0, held = 0.0, prevIn = 0.0;
    Biquad pre1, pre2, post1, post2;
    uint32_t seed = 0x9e3779b9u;
    double noise = 0.0;     // ruído das etapas (0 = apagado)

    void prepare (double fs)
    {
        pre1.lowpass (9661, 0.70710678, fs); pre2.lowpass1 (20000, fs);
        post1.lowpass (9661, 0.70710678, fs); post2.lowpass1 (20000, fs);
    }
    void reset() { line.fill (0); w = 0; phase = 0; held = 0; prevIn = 0; pre1.reset(); pre2.reset(); post1.reset(); post2.reset(); }

    // unha mostra interna; fclk/fs pulsos de reloxo por mostra
    inline double run (double x, double clocksPerSample)
    {
        const double in = pre2.run (pre1.run (x));
        double out = 0.0, t = 0.0, prevHeld = held;
        phase += clocksPerSample;
        while (phase >= 1.0)
        {
            phase -= 1.0;
            // instante do pulso dentro da mostra (0 = comezo, 1 = final)
            const double f = 1.0 - phase / clocksPerSample;
            double v = prevIn + (in - prevIn) * f;
            // as cargas saturan suavemente por riba de ±0,7 (o MN3009 vai preto do límite)
            const double a = std::abs (v);
            if (a > 0.7) v = (v > 0 ? 1.0 : -1.0) * (0.7 + (a - 0.7) / (1.0 + 2.0 * (a - 0.7)));
            line[(size_t) w] = v;
            w = (w + 1) & 255;
            // retención de orde cero: a media dentro da mostra (o salto cae no seu sitio)
            out += prevHeld * (f - t);
            t = f;
            held = line[(size_t) ((w - 129) & 255)];   // o de hai 128 pulsos
            if (noise > 0.0) { seed = seed * 1664525u + 1013904223u; held += noise * ((double) (seed >> 8) / 8388608.0 - 1.0); }
            prevHeld = held;
        }
        out += prevHeld * (1.0 - t);
        prevIn = in;
        return post2.run (post1.run (out));
    }
};

// ================================================================ chorus
// Un LFO triangular move os dous BBD en contrafase. Medidas dun Juno-106 (KR-106): chorus I a
// 0,514 Hz con ±2,13 ms arredor de 3,3 ms; chorus II a 0,863 Hz con ±1,71 ms. Cada saída suma o
// sinal directo e un BBD (0,863 e 1,257, medido); co chorus apagado o BBD non se oe.
struct Chorus
{
    Bbd a, b;
    double lfoPhase = 0.0;
    double fs = INTERNAL_RATE;
    double mix = 0.0;          // 0 apagado → 1 aceso, sen clic
    bool on = false, modeI = true;
    static constexpr double CENTER = 3.30e-3;

    void prepare (double rate) { fs = rate; a.prepare (fs); b.prepare (fs); }
    void reset() { a.reset(); b.reset(); lfoPhase = 0.0; mix = on ? 1.0 : 0.0; }

    inline void run (double x, double& L, double& R)
    {
        const double rate = modeI ? 0.514 : 0.863, depth = modeI ? 2.13e-3 : 1.71e-3;
        lfoPhase += rate / fs;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;
        const double tri = 1.0 - 4.0 * std::abs (lfoPhase - 0.5);
        const double d0 = CENTER + depth * tri, d1 = CENTER - depth * tri;
        const double w0 = a.run (x, 128.0 / d0 / fs), w1 = b.run (x, 128.0 / d1 / fs);
        mix += ((on ? 1.0 : 0.0) - mix) * 0.0005;
        const double dry = 1.0 - mix * (1.0 - 0.863 / 1.5);
        L = dry * x + mix * (1.257 / 1.5) * w0;
        R = dry * x + mix * (1.257 / 1.5) * w1;
    }
};

// ================================================================ ruído (un só xerador para as seis voces)
struct NoiseSource
{
    uint32_t seed = 22222u;
    Biquad tilt;
    void prepare (double fs) { tilt.lowpass1 (12000, fs); }
    void reset() { tilt.reset(); seed = 22222u; }
    inline double run()
    {
        seed = seed * 1664525u + 1013904223u;
        const double w = (double) (int32_t) seed / 2147483648.0;
        return tilt.run (w) * 1.13;   // ≈ o mesmo valor eficaz ca unha serra
    }
};

} // namespace xuno
