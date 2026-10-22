// Voice.h — A parte analóxica dunha voz: o DCO (un contador do 8253 que descarga un integrador),
// a mestura das formas de onda, o VCF (IR3109 dentro do 80017A) e o VCA (BA662).
//
// O tempo do DCO lévase en tics do reloxo mestre de 8 MHz, en enteiros: unha mostra interna son
// 42 tics (8 MHz / 42 = 190 476 Hz). O 8253 conta a 4, 2 ou 1 MHz (4', 8', 16'); en modo 3 a súa
// saída sobe e baixa cada medio ciclo, e cada subida descarga o condensador da rampa. Os saltos
// (descarga, comparador do pulso, biestable do sub) van no seu instante exacto con polyBLEP, así que
// o DCO non ten máis aliasing que o dun sinal analóxico mostrado a 190 kHz.
#pragma once
#include "Cpu.h"
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace xuno {

constexpr int TICKS_PER_SAMPLE = 42;
constexpr double MASTER_CLOCK = 8.0e6;
constexpr double INTERNAL_RATE = MASTER_CLOCK / TICKS_PER_SAMPLE;   // 190 476,19 Hz
constexpr double PI = 3.14159265358979323846;

// CV da rampa × conta do 8253 que dá a rampa enteira (mediana da táboa da ROM ata onde o DAC satura)
constexpr double RAMP_NORM = 1963500.0;

// anchura do pulso: o comparador dispara cando a rampa pasa este nivel. CV $3FFF → metade (cadrada);
// CV 0 (pulso apagado) → por riba do pico, nunca dispara. Co deslizador do panel ao máximo (105,
// limitado por R5/R6) queda nun 4 %, coma o orixinal.
inline double pulseThreshold (uint16_t pwCv) { return 1.06 - 0.56 * (double) pwCv / 16383.0; }

// tanh(x)/x racional (para a linearización por tramos do filtro)
inline double tanhXdX (double x)
{
    const double a = x * x;
    if (a > 9.0) return 1.0 / std::sqrt (a);
    return (27.0 + a) / (27.0 + 9.0 * a);
}

// polyBLEP: un salto h no instante d (fracción de mostra xa pasada desde o salto) corrixe
// esta mostra en h·(−(1−d)²/2) e a anterior en h·(d²/2); a saída vai unha mostra atrasada
struct Blep
{
    double now = 0, prev = 0;
    inline void add (double h, double d) { now += h * (-(1.0 - d) * (1.0 - d) * 0.5); prev += h * (d * d * 0.5); }
};

// ================================================================ DCO
struct Dco
{
    // estado do 8253 (canle en modo 3)
    int64_t nextEdge = 0;     // tic da seguinte transición da saída
    bool outHigh = true;
    uint16_t count = 0x1dfb, pending = 0x1dfb;
    int ticksPerClock = 4;    // 2 (4'), 4 (8'), 8 (16')
    // integrador e formas de onda
    double ramp = 0.0;        // tensión da rampa (1 = pico nominal)
    double slope = 0.0;       // por tic
    double amp = 1.0;         // tolerancia do condensador (variación entre voces)
    double theta = 0.5;       // nivel do comparador do pulso
    bool pulse = false, sub = false;
    bool restartPending = false;

    void setRange (uint8_t pf) { ticksPerClock = pf == 0xc0 ? 2 : pf == 0x40 ? 4 : 8; }
    void setRampCv (uint16_t cv) { slope = amp * (double) cv / (RAMP_NORM * (double) ticksPerClock); }

    // a CPU escribe a conta: en modo 3 cárgase no seguinte medio ciclo
    void write (uint16_t n) { pending = n == 0 ? 0xffff : n; }

    // palabra de modo: OUT sobe xa e a conta recomeza (se estaba baixa, iso descarga a rampa)
    template <typename Edge>
    void restart (int64_t t, Edge&& edge)
    {
        if (! outHigh) { outHigh = true; edge (t); }
        count = pending;
        nextEdge = t + (int64_t) ticksPerClock * ((count + 1) / 2);
    }

    // avanza unha mostra interna [t0, t0+42): chama a onEdge(t) en cada descarga
    template <typename Edge>
    void advanceTo (int64_t t1, Edge&& edge)
    {
        while (nextEdge < t1)
        {
            const int64_t t = nextEdge;
            outHigh = ! outHigh;
            count = pending;   // a conta nova entra en cada medio ciclo
            nextEdge = t + (int64_t) ticksPerClock * (outHigh ? (count + 1) / 2 : count / 2);
            if (outHigh) edge (t);
        }
    }
};

// ================================================================ VCF (IR3109: catro OTA en serie, realimentación global)
// Cada etapa é un integrador OTA: dy/dt = ωc·tanh(x − y). Resólvese sen atraso (TPT) cos tanh
// linearizados no punto da mostra anterior (o método «barato» de T. Voipio): estable a calquera
// corte e resonancia, e coa saturación suave dos OTA.
struct Vcf
{
    double s[4] {}, y[4] {}, u = 0.0;
    double g = 0.1, k = 0.0;
    static constexpr double FB_SAT = 0.45;   // o VCA da realimentación (BA662) satura antes ca os OTA
    static constexpr double IN_SCALE = 0.6;  // a entrada e a realimentación chegan ao primeiro OTA por un divisor

    void reset() { for (int i = 0; i < 4; ++i) s[i] = y[i] = 0.0; u = 0.0; }

    inline double process (double x)
    {
        const double t0 = tanhXdX (IN_SCALE * (u - y[0])), t1 = tanhXdX (y[0] - y[1]), t2 = tanhXdX (y[1] - y[2]), t3 = tanhXdX (y[2] - y[3]);
        const double g0 = g * t0, g1 = g * t1, g2 = g * t2, g3 = g * t3;
        const double a0 = g0 / (1.0 + g0), a1 = g1 / (1.0 + g1), a2 = g2 / (1.0 + g2), a3 = g3 / (1.0 + g3);
        const double b0 = s[0] / (1.0 + g0), b1 = s[1] / (1.0 + g1), b2 = s[2] / (1.0 + g2), b3 = s[3] / (1.0 + g3);
        // y3 = A·u + B; a realimentación, k·FB_SAT·tanh(y3/FB_SAT), linearizada igual
        const double A = a3 * a2 * a1 * a0;
        const double B = b3 + a3 * (b2 + a2 * (b1 + a1 * b0));
        const double kf = k * tanhXdX (y[3] / FB_SAT);
        u = (x - kf * B) / (1.0 + kf * A);
        y[0] = a0 * u + b0;
        y[1] = a1 * y[0] + b1;
        y[2] = a2 * y[1] + b2;
        y[3] = a3 * y[2] + b3;
        for (int i = 0; i < 4; ++i) s[i] = 2.0 * y[i] - s[i];
        return y[3];
    }

    double energy() const { return std::abs (s[0]) + std::abs (s[1]) + std::abs (s[2]) + std::abs (s[3]); }
};

// corte do VCF a partir do código do DAC (12 bits). As dúas referencias do axuste do manual de servizo:
// código 1568 → 248 Hz e 2139 → 992 Hz (dúas oitavas en 571 códigos), medidas coa resonancia ao
// máximo, é dicir, auto-oscilando; o modelo oscila un 9 % por debaixo do seu corte, así que o corte
// vai un 9 % por riba. Por enriba dos 50 kHz, un xeonllo suave (medidas dunha voz real, KR-106).
inline double vcfHz (double code12)
{
    const double lin = 248.0 * 1.093 * std::exp2 ((code12 - 1568.0) / 285.5);
    return lin / std::sqrt (1.0 + (lin / 60000.0) * (lin / 60000.0));
}

// VCA (BA662 controlado por un transistor): case lineal, co arranque suave medido nunha voz real
inline double vcaGain (double e)
{
    const double d = e - 0.015;
    return (std::sqrt (d * d + 0.006 * 0.006) + d) * 0.5 / 0.985;
}

// nivel do sub e do ruído segundo o seu CV (o DAC move un transistor en paralelo): case lineal,
// con xeonllo preto do cero (medidas de KR-106 nunha placa real)
inline double subGain (double t)
{
    const double d = t - 0.1025;
    return 1.115 * (std::sqrt (d * d + 0.025 * 0.025) + d) * 0.5;
}
inline double noiseGain (double t)
{
    if (t <= 0.0) return 0.0;
    const double d = t - 0.0594;
    return 1.0632 * (std::sqrt (d * d + 0.0146 * 0.0146) + d) * 0.5;
}

// ================================================================ unha voz enteira
struct Voice
{
    Dco dco;
    Vcf vcf;
    Blep blep;
    double held = 0.0;       // a mostra anterior da mestura (sae unha atrasada polo polyBLEP)
    double dcState = 0.0;    // acoplamento en alterna antes do VCF
    double vcfCode = 0.0, vcfCodeTarget = 0.0;
    double vcaCv = 0.0, vcaTarget = 0.0;
    double res = 0.0;
    // tolerancias desta voz (0 se a variación está apagada)
    double vcfOffset = 0.0, resScale = 1.0, vcaScale = 1.0;
    int coefCounter = 0;
    bool silent = true;

    void reset()
    {
        dco = Dco{}; vcf.reset(); blep = Blep{};
        held = dcState = 0.0;
        vcfCode = vcfCodeTarget = 0.0; vcaCv = vcaTarget = 0.0;
        coefCounter = 0; silent = true;
    }
};

} // namespace xuno
