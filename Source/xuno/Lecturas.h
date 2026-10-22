// Lecturas.h — O que significa cada deslizador e cada mando, calculado coas táboas da ROM e co
// modelo do motor (Hz, ms, cents, oitavas, dB). Só números: o texto pono cada interface (Barullo
// Xuno e Barullo Xuntos usan estas mesmas contas).
#pragma once
#include "Rom.h"
#include "Voice.h"
#include <algorithm>
#include <cmath>

namespace xuno::lectura {

constexpr double LOOP_MS = 4.2335;   // o bucle da CPU de voces

inline double lfoHz (int v)
{
    const int half = (8192 + rom::LFO_SPEED[v] - 1) / rom::LFO_SPEED[v];
    return 1000.0 / (4.0 * half * LOOP_MS);
}
inline double lfoAtrasoMs (int v)
{
    const int inc = rom::ENV_ATTACK[v], ramp = rom::LFO_DELAY_RAMP[(v >> 4) & 7];
    const int ticks = (0x4000 + inc - 1) / inc + (ramp >= 0xffff ? 0 : (65536 + ramp - 1) / ramp);
    return v == 0 ? 0.0 : ticks * LOOP_MS;
}
inline double dcoLfoCents (int v) { return rom::LFO_DEPTH[v] / 255.0 * 398.0; }
inline double pwmPorcento (int v)
{
    const double pw = 16383.0 - (2.0 * v) * 16383.0 / 256.0;
    return (1.0 - pulseThreshold ((uint16_t) pw)) * 100.0;
}
inline double vcfFrecuencia (int v) { return vcfHz ((double) ((v << 7) >> 2)); }
inline double vcfEnvOitavas (int v) { return v * 254.0 / 256.0 * 32.0 / 285.5; }
inline double vcfLfoOitavas (int v) { return v * 254.0 / 256.0 * 8.0 / 285.5; }
inline double vcaDb (int v) { return -10.0 + 20.0 * (double) (v << 7) / 16256.0; }
inline double ataqueMs (int v) { const int inc = rom::ENV_ATTACK[v]; return (double) ((0x3fff + inc - 1) / inc) * LOOP_MS; }
inline double caidaMs (int v)   // ata −20 dB
{
    const double c = rom::ENV_DECAY[v] / 65536.0;
    return std::ceil (std::log (0.1) / std::log (c)) * LOOP_MS;
}
// os mandos que le a propia CPU (0–255)
inline double portamentoMs (int a) { if (a == 0) return 0.0; const int c = rom::PORTA[a >> 1]; return 12.0 * 256.0 / std::max (1, c) * LOOP_MS; }   // unha oitava
inline double bendDcoSemitonos (double a) { return 0.754 * 255.0 * a / 4096.0; }
inline double bendVcfOitavas (double a) { return 255.0 * a / 16.0 / 4.0 / 285.5; }
inline double lfoModCents (double a) { return a / 255.0 * 398.0; }

} // namespace xuno::lectura
