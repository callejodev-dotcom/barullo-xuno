// Patch.h — Os sons no formato do Juno-106: 16 deslizadores (0–127) e dous bytes de interruptores,
// os mesmos 18 bytes que manda polo MIDI (SysEx de Roland, mensaxes $30, $31 e $32).
#pragma once
#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>

namespace xuno {

// orde dos 18 bytes (a do SysEx)
enum PatchByte
{
    LFO_RATE = 0, LFO_DELAY, DCO_LFO, DCO_PWM, DCO_NOISE, VCF_FREQ, VCF_RES, VCF_ENV, VCF_LFO, VCF_KYBD,
    VCA_LEVEL, ENV_A, ENV_D, ENV_S, ENV_R, DCO_SUB, SW1, SW2, PATCH_SIZE
};

// SW1: bit 0 16', 1 8', 2 4', 3 pulso, 4 serra, 5 chorus apagado (0 = aceso), 6 chorus I (0 = II)
// SW2: bit 0 PWM manual (0 = LFO), 1 envolvente do VCF negativa, 2 VCA en GATE, 3–4 HPF (3 − posición)
using Patch = std::array<uint8_t, PATCH_SIZE>;

inline Patch initPatch()
{
    Patch p {};
    p[VCF_FREQ] = 127; p[VCA_LEVEL] = 64; p[ENV_S] = 127; p[ENV_R] = 10; p[LFO_RATE] = 64;
    p[SW1] = 0x02 | 0x10 | 0x20;   // 8', serra, chorus apagado
    p[SW2] = 0x01 | (2 << 3);      // PWM manual, HPF na posición 1 (plano)
    return p;
}

inline void sanitize (Patch& p)
{
    for (int i = 0; i < 16; ++i) p[(size_t) i] &= 0x7f;
    p[SW1] &= 0x7f; p[SW2] &= 0x7f;
}

// rexistro como o le a CPU: primeiro o 4', despois o 8'; se non, 16'
inline int rangeOf (const Patch& p) { return (p[SW1] & 4) ? 2 : (p[SW1] & 2) ? 1 : 0; }
inline void setRange (Patch& p, int r) { p[SW1] = (uint8_t) ((p[SW1] & ~7) | (1 << std::clamp (r, 0, 2))); }

// forma única: o rexistro cun só bit (como o le a CPU) e sen os bits que o Juno non usa
inline void normalize (Patch& p)
{
    sanitize (p);
    setRange (p, rangeOf (p));
    p[SW2] &= 0x1f;
}
inline int hpfOf (const Patch& p) { return 3 - ((p[SW2] >> 3) & 3); }
inline void setHpf (Patch& p, int pos) { p[SW2] = (uint8_t) ((p[SW2] & ~0x18) | ((3 - std::clamp (pos, 0, 3)) << 3)); }
inline bool chorusOn (const Patch& p) { return ! (p[SW1] & 0x20); }
inline bool chorusII (const Patch& p) { return ! (p[SW1] & 0x40); }

// pegada dun son (FNV-1a de 64 bits), para recoñecer os sons e bancos coñecidos
inline uint64_t patchHash (const Patch& p)
{
    uint64_t h = 1469598103934665603ULL;
    for (auto b : p) { h ^= b; h *= 1099511628211ULL; }
    return h;
}

inline uint64_t bankHash (const Patch* v, int n)
{
    uint64_t h = 1469598103934665603ULL;
    for (int k = 0; k < n; ++k) for (auto b : v[k]) { h ^= b; h *= 1099511628211ULL; }
    return h;
}

// ---------------------------------------------------------------- SysEx
// $30: o son dunha memoria (o teclado mándao ao cambiar de son), $31: o son do panel (MANUAL)
inline std::vector<uint8_t> patchToSysex (const Patch& p, int number, bool manual = false, int channel = 0)
{
    std::vector<uint8_t> d { 0xF0, 0x41, (uint8_t) (manual ? 0x31 : 0x30), (uint8_t) (channel & 15), (uint8_t) (number & 0x7f) };
    for (auto b : p) d.push_back (b & 0x7f);
    d.push_back (0xF7);
    return d;
}

inline std::vector<uint8_t> bankToSysex (const std::vector<Patch>& bank)
{
    std::vector<uint8_t> d;
    for (size_t k = 0; k < bank.size(); ++k)
    {
        const auto m = patchToSysex (bank[k], (int) k);
        d.insert (d.end(), m.begin(), m.end());
    }
    return d;
}

struct SysexPatch { Patch patch {}; int number = 0; bool manual = false; };

// todas as mensaxes $30/$31 dun ficheiro ou dun fluxo (as outras ignóranse)
inline std::vector<SysexPatch> parseSysex (const std::vector<uint8_t>& d)
{
    std::vector<SysexPatch> out;
    for (size_t i = 0; i < d.size(); ++i)
    {
        if (d[i] != 0xF0) continue;
        size_t e = i + 1;
        while (e < d.size() && d[e] != 0xF7 && ! (d[e] & 0x80)) ++e;
        if (e < d.size() && d[e] == 0xF7 && e - i == 23 && d[i + 1] == 0x41 && (d[i + 2] == 0x30 || d[i + 2] == 0x31))
        {
            SysexPatch sp;
            sp.manual = d[i + 2] == 0x31;
            sp.number = d[i + 4] & 0x7f;
            for (int k = 0; k < PATCH_SIZE; ++k) sp.patch[(size_t) k] = d[i + 5 + (size_t) k];
            normalize (sp.patch);
            out.push_back (sp);
        }
        i = e;
    }
    return out;
}

// un ficheiro como banco: se os números de son son distintos, cada un vai ao seu sitio (A11–B88);
// se non (p. ex. todos $31), en orde
inline std::vector<Patch> sysexToBank (const std::vector<SysexPatch>& sp)
{
    std::vector<Patch> bank;
    if (sp.empty()) return bank;
    std::array<bool, 128> seen {};
    bool distinct = true;
    for (auto& s : sp) { if (seen[(size_t) s.number]) { distinct = false; break; } seen[(size_t) s.number] = true; }
    if (distinct && sp.size() > 1)
    {
        int top = 0;
        for (auto& s : sp) top = std::max (top, s.number);
        bank.assign ((size_t) top + 1, initPatch());
        for (auto& s : sp) bank[(size_t) s.number] = s.patch;
    }
    else
        for (auto& s : sp) if (bank.size() < 128) bank.push_back (s.patch);
    return bank;
}

// nomes curtos dos parámetros (para o visor, máx. 16 caracteres)
inline const char* paramName (int i)
{
    static const char* n[PATCH_SIZE] = { "LFO VELOCIDADE", "LFO ATRASO", "DCO LFO", "DCO PWM", "DCO RUIDO", "VCF FREC.",
        "VCF RESONANCIA", "VCF ENV", "VCF LFO", "VCF TECLADO", "VCA NIVEL", "ENV A", "ENV D", "ENV S", "ENV R",
        "DCO SUB", "INTERRUPTORES 1", "INTERRUPTORES 2" };
    return (i >= 0 && i < PATCH_SIZE) ? n[i] : "?";
}

} // namespace xuno
