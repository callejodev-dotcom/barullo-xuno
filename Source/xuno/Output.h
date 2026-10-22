// Output.h — Paso dos 190 476 Hz internos á frecuencia do DAW, en estéreo e sen aliasing.
//
// Filtro de Kaiser (90 dB) en táboa polifásica, levado a fase mínima polo cepstro real: só usa
// mostras pasadas, así que non hai latencia que declarar (o atraso de grupo é de poucas mostras
// internas, coma estar uns centímetros máis lonxe do altofalante). É o mesmo deseño do Barullo 7FM.
#pragma once
#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>
#include <mutex>

namespace xuno {

class Resampler
{
public:
    static constexpr int L = 64;   // fases por mostra interna (interpólase linealmente entre elas)

    void design (double inRate, double outRate)
    {
        static std::mutex m;
        static double cIn = -1, cOut = -1;
        static std::vector<float> cTable;
        static int cTaps = 0;
        {
            std::lock_guard<std::mutex> lk (m);
            if (std::abs (cIn - inRate) < 1e-9 && std::abs (cOut - outRate) < 1e-9) { table = cTable; taps = cTaps; alloc(); return; }
        }
        designNow (inRate, outRate);
        std::lock_guard<std::mutex> lk (m);
        cIn = inRate; cOut = outRate; cTable = table; cTaps = taps;
    }

    void reset() { std::fill (hl.begin(), hl.end(), 0.0f); std::fill (hr.begin(), hr.end(), 0.0f); head = 0; }
    inline void push (float l, float r) { head = (head + 1) & mask; hl[head] = l; hr[head] = r; }

    // saída no instante (última mostra interna + frac), frac en [0, 1)
    inline void out (double frac, float& l, float& r) const
    {
        const double p = frac * L;
        const int ip = std::min ((int) p, L - 1);
        const float a = (float) (p - ip);
        float sl = 0, sr = 0;
        size_t idx = head;
        const float* t = table.data() + ip;
        for (int k = 0; k < taps; ++k)
        {
            const float c = t[0] + (t[1] - t[0]) * a;
            sl += hl[idx] * c; sr += hr[idx] * c;
            idx = (idx - 1) & mask; t += L;
        }
        l = sl; r = sr;
    }

private:
    std::vector<float> table, hl, hr;
    size_t mask = 0, head = 0;
    int taps = 0;

    void alloc()
    {
        size_t H = 1; while ((int) H < taps + 1) H <<= 1;
        hl.assign (H, 0.0f); hr.assign (H, 0.0f); mask = H - 1; head = 0;
    }

    void designNow (double inRate, double outRate)
    {
        const double pi = 3.14159265358979323846;
        const double fpass = std::min (20000.0, 0.45 * std::min (inRate, outRate));
        const double fstop = std::min (inRate, outRate) - fpass;
        const double A = 90.0, beta = 0.1102 * (A - 8.7);
        const double dw = 2 * pi * (fstop - fpass) / inRate;
        const int N = (int) std::ceil ((A - 8) / (2.285 * dw)) + 1;
        const double cut = (fpass + fstop) / 2 / inRate;
        const int P = N * L;
        std::vector<double> g ((size_t) P);
        const double i0b = besselI0 (beta);
        for (int i = 0; i < P; ++i)
        {
            const double t = (i - (P - 1) / 2.0) / L;
            const double x = 2 * cut * t;
            const double sinc = std::abs (x) < 1e-12 ? 1.0 : std::sin (pi * x) / (pi * x);
            const double r = 2.0 * i / (P - 1) - 1.0;
            g[(size_t) i] = 2 * cut * sinc * besselI0 (beta * std::sqrt (std::max (0.0, 1 - r * r))) / i0b;
        }

        // fase mínima polo cepstro real
        size_t M = 1; while (M < g.size() * 8) M <<= 1;
        std::vector<std::complex<double>> X (M);
        for (size_t i = 0; i < g.size(); ++i) X[i] = g[i];
        fft (X, false);
        for (auto& v : X) v = std::log (std::max (std::abs (v), 1e-9));
        fft (X, true);
        for (size_t i = 1; i < M / 2; ++i) X[i] *= 2.0;
        for (size_t i = M / 2 + 1; i < M; ++i) X[i] = 0.0;
        for (auto& v : X) v = std::complex<double> (v.real(), 0.0);
        fft (X, false);
        for (auto& v : X) v = std::exp (v);
        fft (X, true);

        taps = (int) (g.size() + L - 1) / L + 1;
        table.assign ((size_t) (taps * L + 2), 0.0f);
        for (size_t i = 0; i < g.size() && i < table.size(); ++i) table[i] = (float) X[i].real();
        for (int ph = 0; ph < L; ++ph)
        {
            double s = 0; for (int k = 0; k < taps; ++k) s += table[(size_t) (ph + k * L)];
            if (std::abs (s) > 1e-12) for (int k = 0; k < taps; ++k) table[(size_t) (ph + k * L)] = (float) (table[(size_t) (ph + k * L)] / s);
        }
        alloc();
    }

    static double besselI0 (double x)
    {
        double s = 1, t = 1;
        for (int k = 1; k < 60; ++k) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; if (t < 1e-14 * s) break; }
        return s;
    }

    static void fft (std::vector<std::complex<double>>& a, bool inverse)
    {
        const size_t n = a.size();
        for (size_t i = 1, j = 0; i < n; ++i)
        {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap (a[i], a[j]);
        }
        const double pi = 3.14159265358979323846;
        for (size_t len = 2; len <= n; len <<= 1)
        {
            const double ang = 2 * pi / (double) len * (inverse ? 1 : -1);
            const std::complex<double> wl (std::cos (ang), std::sin (ang));
            for (size_t i = 0; i < n; i += len)
            {
                std::complex<double> w (1);
                for (size_t j = 0; j < len / 2; ++j)
                {
                    const auto u = a[i + j], v = a[i + j + len / 2] * w;
                    a[i + j] = u + v; a[i + j + len / 2] = u - v;
                    w *= wl;
                }
            }
        }
        if (inverse) for (auto& v : a) v /= (double) n;
    }
};

} // namespace xuno
