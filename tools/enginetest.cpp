// Proba do motor sen JUCE: afinación, tempos das envolventes e do LFO, corte do VCF e velocidade.
// c++ -std=c++17 -O2 -I Source tools/enginetest.cpp -o enginetest && ./enginetest [saida.wav]
#include "xuno/Synth.h"
#include <cstdio>
#include <vector>
#include <chrono>
#include <complex>

using namespace xuno;

static void writeWav (const char* path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path, "wb");
    if (! f) return;
    const uint32_t n = (uint32_t) L.size(), bytes = n * 4;
    auto u32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + bytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    u32 (16); u16 (1); u16 (2); u32 ((uint32_t) sr); u32 ((uint32_t) sr * 4); u16 (4); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (bytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t a = (int16_t) std::lround (std::clamp (L[i], -1.0f, 1.0f) * 32767), b = (int16_t) std::lround (std::clamp (R[i], -1.0f, 1.0f) * 32767);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
    std::printf ("WAV: %s\n", path);
}

// frecuencia por cruces por cero con interpolación (sinal periódico limpo)
static double freqOf (const std::vector<float>& x, int sr, int a, int b)
{
    double first = -1, last = -1; int cnt = 0;
    for (int i = a + 1; i < b; ++i)
        if (x[(size_t) i - 1] < 0 && x[(size_t) i] >= 0)
        {
            const double t = i - 1 + x[(size_t) i - 1] / (x[(size_t) i - 1] - x[(size_t) i]);
            if (first < 0) first = t; last = t; ++cnt;
        }
    return cnt > 1 ? (cnt - 1) * sr / (last - first) : 0.0;
}

int main (int argc, char** argv)
{
    const int sr = 48000;
    int fails = 0;

    // ---- afinación: serra soa, filtro aberto, sen chorus, 8'
    {
        Synth s; s.prepare (sr); s.setTune (0x78);
        Patch p = initPatch(); p[SW1] = 0x02 | 0x10 | 0x20; p[VCF_FREQ] = 127; p[ENV_S] = 127; p[VCA_LEVEL] = 64;
        s.setPatch (p);
        std::printf ("afinación (afinación −8/256, 8'):\n");
        double worst = 0;
        for (int note : { 36, 48, 57, 60, 69, 72, 84, 96 })
        {
            s.noteOn (note);
            std::vector<float> L ((size_t) sr / 2), R ((size_t) sr / 2);
            s.render (L.data(), R.data(), sr / 2);
            s.noteOff (note);
            std::vector<float> t ((size_t) sr / 4), u ((size_t) sr / 4);
            s.render (t.data(), u.data(), sr / 4);
            const double f = freqOf (L, sr, sr / 10, sr / 2);
            const double ideal = 440.0 * std::pow (2.0, (note - 69) / 12.0);
            const double c = 1200 * std::log2 (f / ideal);
            worst = std::max (worst, std::abs (c));
            std::printf ("  nota %3d: %9.3f Hz (%+.2f cents)\n", note, f, c);
        }
        if (worst > 4.0) { ++fails; std::printf ("  FALLO: máis de 4 cents\n"); }
    }

    // ---- LFO: velocidade ao máximo e ao medio (vibrato, medido no ton)
    {
        for (int rate : { 127, 64, 20 })
        {
            Synth s; s.prepare (sr); s.setTune (0x78);
            Patch p = initPatch(); p[LFO_RATE] = (uint8_t) rate; p[DCO_LFO] = 127; p[ENV_S] = 127;
            s.setPatch (p);
            s.noteOn (69);
            const int n = sr * 4;
            std::vector<float> L ((size_t) n), R ((size_t) n);
            s.render (L.data(), R.data(), n);
            // ton instantáneo por cruces e conta de máximos
            std::vector<double> fz;
            double prev = -1;
            for (int i = 1; i < n; ++i)
                if (L[(size_t) i - 1] < 0 && L[(size_t) i] >= 0)
                {
                    const double t = i - 1 + L[(size_t) i - 1] / (L[(size_t) i - 1] - L[(size_t) i]);
                    if (prev > 0) fz.push_back (sr / (t - prev));
                    prev = t;
                }
            // cruces da frecuencia pola media
            double mean = 0; for (double v : fz) mean += v; mean /= (double) fz.size();
            int cross = 0; for (size_t i = 1; i < fz.size(); ++i) if (fz[i - 1] < mean && fz[i] >= mean) ++cross;
            double lo = 1e9, hi = 0; for (size_t i = fz.size() / 4; i < fz.size(); ++i) { lo = std::min (lo, fz[i]); hi = std::max (hi, fz[i]); }
            std::printf ("LFO %3d: ~%.2f Hz, vibrato %+.0f/%+.0f cents\n", rate, cross / 4.0, 1200 * std::log2 (lo / mean), 1200 * std::log2 (hi / mean));
        }
    }

    // ---- envolvente: tempos de ataque e relaxación (no VCA, en GATE non)
    {
        for (int a : { 0, 64, 100, 127 })
        {
            Synth s; s.prepare (sr);
            Patch p = initPatch(); p[ENV_A] = (uint8_t) a; p[ENV_D] = 0; p[ENV_S] = 127; p[ENV_R] = 60; p[SW1] = 0x02 | 0x08 | 0x20; p[DCO_PWM] = 0;
            s.setPatch (p);
            s.noteOn (60);
            const int n = sr * 4;
            std::vector<float> L ((size_t) n), R ((size_t) n);
            s.render (L.data(), R.data(), n);
            // envolvente por picos en ventás de 2 ms
            const int w = sr / 500; double peak = 0; std::vector<double> e;
            for (int i = 0; i + w < n; i += w) { double m = 0; for (int k = 0; k < w; ++k) m = std::max (m, (double) std::abs (L[(size_t) (i + k)])); e.push_back (m); peak = std::max (peak, m); }
            int t90 = -1; for (size_t i = 0; i < e.size(); ++i) if (e[i] >= 0.9 * peak) { t90 = (int) i; break; }
            std::printf ("ataque %3d: 90 %% aos %.1f ms\n", a, t90 * 2.0);
        }
    }

    // ---- corte do VCF: resonancia ao máximo, sen osciladores: a auto-oscilación dá o corte
    {
        std::printf ("VCF auto-oscilando (resonancia 127, sen DCO), por deslizador:\n");
        for (int fr : { 20, 40, 49, 60, 80, 100 })
        {
            Synth s; s.prepare (sr);
            Patch p = initPatch(); p[SW1] = 0x02 | 0x20; p[DCO_PWM] = 0; p[VCF_FREQ] = (uint8_t) fr; p[VCF_RES] = 127; p[VCF_KYBD] = 0; p[ENV_S] = 127;
            s.setPatch (p);
            s.noteOn (60);
            const int n = sr;
            std::vector<float> L ((size_t) n), R ((size_t) n);
            s.render (L.data(), R.data(), n);
            double pk = 0; for (int i = n / 2; i < n; ++i) pk = std::max (pk, (double) std::abs (L[(size_t) i]));
            std::printf ("  VCF %3d: %8.1f Hz (código %d → modelo %.1f Hz), pico %.3f\n", fr, freqOf (L, sr, n / 2, n),
                         (fr << 7) >> 2, vcfHz ((fr << 7) >> 2), pk);
        }
    }

    // ---- velocidade: acorde de seis notas con chorus
    {
        Synth s; s.prepare (sr);
        Patch p = initPatch(); p[SW1] = 0x02 | 0x08 | 0x10 | 0x00 | 0x40; p[DCO_SUB] = 80; p[VCF_FREQ] = 70; p[VCF_RES] = 40; p[VCF_ENV] = 60; p[ENV_A] = 20; p[ENV_D] = 70; p[ENV_S] = 60; p[ENV_R] = 60; p[LFO_RATE] = 50; p[DCO_LFO] = 10;
        s.setPatch (p);
        const int n = sr * 8;
        std::vector<float> L ((size_t) n), R ((size_t) n);
        const auto c0 = std::chrono::steady_clock::now();
        int pos = 0;
        for (int k = 0; k < 4; ++k)
        {
            for (int note : { 48, 55, 60, 64, 67, 72 }) s.noteOn (note + k * 2);
            s.render (L.data() + pos, R.data() + pos, sr); pos += sr;
            for (int note : { 48, 55, 60, 64, 67, 72 }) s.noteOff (note + k * 2);
            s.render (L.data() + pos, R.data() + pos, sr); pos += sr;
        }
        const double cpu = std::chrono::duration<double> (std::chrono::steady_clock::now() - c0).count();
        float pk = 0; for (int i = 0; i < n; ++i) pk = std::max (pk, std::abs (L[(size_t) i]));
        std::printf ("acorde: %.2f s de CPU para 8 s (%.0fx tempo real), pico %.3f\n", cpu, 8.0 / cpu, pk);
        if (argc > 1) writeWav (argv[1], L, R, sr);
    }
    return fails;
}
