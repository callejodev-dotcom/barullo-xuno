// Cpu.h — O programa da CPU de voces do Juno-106 (IC29, µPD7811G a 12 MHz), reescrito rutina a rutina
// desde o desensamblado: as mesmas variables de 8 e 16 bits, as mesmas multiplicacións de 8×8 (coa
// parte baixa que non calcula), os mesmos redondeos, desbordamentos e marcas.
//
// A CPU non fai son: calcula as tensións de control (CV) cun DAC de 12 bits e reparte por multiplexores
// con mostraxe e retención, e programa os dous 8253 que marcan o ton de cada DCO. Todo iso sae daquí:
// VoiceCv (por voz) e GlobalCv. A parte analóxica (Voice.h, Board.h) le estes valores.
//
// O bucle principal dura uns 4,23 ms. Cada orde de nota (voz acesa ou apagada, pedal) chega por
// interrupción e fai que o bucle volva empezar (a ROM borra a pila e salta a $02EB). Por iso o bucle
// vai en seccións, cada unha no seu momento dentro da volta (Synth.h decide cando toca cada unha).
#pragma once
#include "Rom.h"
#include <array>
#include <cstdint>
#include <algorithm>

namespace xuno {

constexpr int MAXV = 12;   // 6 no orixinal; 12 coma dous Juno-106 xuntos (Función)

struct VoiceCv
{
    uint16_t divider = 0x1dfb;  // conta do 8253
    bool dividerNew = false;    // escrita nesta volta: o 8253 cárgaa no seguinte medio ciclo
    bool reset = false;         // palabra de modo escrita: o contador recomeza e OUT sobe
    uint16_t ramp = 0x100;      // CV da rampa (12 bits)
    uint16_t vcf = 0;           // CV do VCF (14 bits; o DAC colle os 12 de arriba)
    uint16_t vca = 0;           // CV do VCA (14 bits)
};

struct GlobalCv
{
    uint16_t res = 0, vcaMaster = 0x2000, sub = 0, pwm = 0x3fff, noise = 0;
    uint8_t latch = 0x15;   // IC40: bit 0 chorus aceso, 1 chorus I, 2–3 posición do HPF, 4 serra
    uint8_t pf = 0x40;      // bits 6–7: preescalado do reloxo dos DCO (0x00 16', 0x40 8', 0xC0 4')
};

class Cpu
{
public:
    Cpu() { coldStart(); }

    int nv = 6;
    std::array<VoiceCv, MAXV> vcv {};
    GlobalCv gcv;

    // ---- o que le a propia CPU no seu conversor A/D (a placa do bender e o panel esquerdo), xa en 0–255
    uint8_t adcTune = 0x80;          // afinación: 0x80 = centro, ±½ semitón
    uint8_t adcPorta = 0;            // portamento (0 = apagado)
    bool leverPushed = false;        // a panca empurrada cara adiante (LFO TRIG)
    uint8_t adcLfoSens = 0;          // canto LFO dá a panca (e a roda MIDI)
    uint8_t adcVcfBendSens = 0, adcDcoBendSens = 0;
    bool leverDown = false;          // panca cara á esquerda
    uint8_t adcBendCv = 0;           // canto se move a panca (0–255)

    void coldStart()
    {
        // $0280
        resetVoiceBits = noteGate = voiceRun = atkPhase = dcyPhase = rlsPhase = 0;
        voiceNote.fill (0x3c);                 // as voces arrancan en Do central
        notePitch.fill (0x3c00);
        env.fill (0);
        gcv = GlobalCv {};
        flags1 = 0x40;
        switch1 = 0x2a; switch2 = 0x06;
        envSustain = 0x3f80; vcfCutoff = 0x3f80; vcaMaster = 0x2000; lfoRateCoeff = 0x0200;
        envDcyCoeff = envRlsCoeff = rom::ENV_DECAY[0]; envAtk = 0;
        subLev = noise = vcfReso = 0;
        vcfEnvMod = vcfKeyTrack = pwmDepth = lfoToVcf = lfoToPitchScaler = 0;
        lfoFlag = 0; lfoVal = 0; pwmSignal = 0x3fff; lfoToPitch = vcfLfoSignal = 0;
        lfoHoldOff = lfoDelayEnv = 0; lfoHoldoffRate = 0x4000; lfoAttackRate = 0xffff;
        tuningAmt = 0; midiBendAmt = 0; midiModDepth = 0; lfoModToPitch = 0;
        vcfBendAmt = 0; dcoBend = 0; tuneLfoBend = 0; portaCoeff = 0; modSwDeclick = 0;
        bendPol2 = 0; bendVal1 = bendVal2 = 0; midiBendPending = false; adcSecondSet = false;
        for (auto& c : vcv) c = VoiceCv {};
    }

    // ============================================================ órdenes do asignador (porto serie)
    // $0080–$0085 voz apagada
    void voiceOff (int v)
    {
        const uint16_t bit = mask (v);
        noteGate &= (uint16_t) ~bit;
        if (! (flags1 & 1)) rlsPhase &= (uint16_t) ~bit;   // co pedal non se marca
    }

    // $0088–$008D voz acesa
    void voiceOn (int v, int note)
    {
        const uint16_t bit = mask (v);
        noteGate |= bit;
        atkPhase |= bit;
        if (voiceNote[(size_t) v] != (uint8_t) note)
        {
            voiceNote[(size_t) v] = (uint8_t) note;
            if (voiceRun & bit) { if (rlsPhase & bit) rlsPhase &= (uint16_t) ~bit; }
            else resetVoiceBits |= bit;                     // a voz estaba parada: recomeza o 8253
        }
        else if (voiceRun & bit)
        {
            if (rlsPhase & bit) rlsPhase &= (uint16_t) ~bit;
        }
    }

    // $0086 / $0087
    void sustain (bool on) { if (on) flags1 |= 1; else flags1 &= (uint8_t) ~1; }

    // $008E en diante: un valor de cada vez, na orde do SysEx (os interruptores xa na forma da CPU)
    void setPatchByte (int index, uint8_t d)
    {
        d &= 0x7f;
        switch (index)
        {
            case SW1_:       setSwitch1 (d); break;
            case SW2_:       setSwitch2 (d); break;
            case 0:  lfoRateCoeff = rom::LFO_SPEED[d]; break;                       // $0198
            case 1:  lfoAttackRate = rom::LFO_DELAY_RAMP[(d >> 4) & 7];             // $0200
                     lfoHoldoffRate = rom::ENV_ATTACK[d]; break;
            case 2:  lfoToPitchScaler = rom::LFO_DEPTH[d]; break;                   // $023A
            case 3:  pwmDepth = (uint8_t) (d << 1); break;                         // $01AB
            case 4:  noise = store16 (d); break;                                    // $01C8
            case 5:  vcfCutoff = store16 (d); break;                                // $01CC
            case 6:  vcfReso = store16 (d); break;                                  // $01D0
            case 7:  vcfEnvMod = (uint8_t) (d << 1); break;                        // $01AF
            case 8:  lfoToVcf = (uint8_t) (d << 1); break;                         // $01B3
            case 9:  vcfKeyTrack = (uint8_t) (d << 1); break;                      // $01B7
            case 10: vcaMaster = store16 (d); break;                                // $01D4
            case 11: envAtk = d; break;                                             // $01E2
            case 12: envDcyCoeff = rom::ENV_DECAY[d]; break;                        // $01ED
            case 13: envSustain = store16 (d); break;                               // $01C4
            case 14: envRlsCoeff = rom::ENV_DECAY[d]; break;                        // $01E9
            case 15: subLev = store16 (d); break;                                   // $01C0
            default: break;
        }
    }
    static constexpr int SW1_ = 16, SW2_ = 17;

    // $0223 / $0227: bend polo MIDI (7 bits de magnitude); $01A7: roda de modulación
    void midiBend (bool down, uint8_t mag)
    {
        if (down) flags1 |= 0x10; else flags1 &= (uint8_t) ~0x10;
        mag &= 0x7f;
        midiBendAmt = mag == 0 ? 0 : (uint8_t) ((mag << 1) | 1);
        midiBendPending = true;
    }
    void midiMod (uint8_t v) { midiModDepth = (uint8_t) ((v & 0x7f) << 1); }

    // ============================================================ o bucle principal, por seccións
    // S0: $02EC–$0410 (comezo da volta ata o nivel do sub)
    void loopStart()
    {
        gcv.latch = ic40Latch;
        const uint16_t all = allMask();
        voiceRun = (flags1 & 1) ? (uint16_t) ((voiceRun | noteGate) & all) : (uint16_t) (noteGate & all);
        gcv.res = vcfReso;
        lfoEnvelopeAndAmounts();
        gcv.vcaMaster = vcaMaster;
        basePitch();
        for (int v = 0; v < nv; ++v) portamento (v);
        gcv.sub = subLev;
    }

    // S1: $041C–$04A1 o ton dunha voz (cada volta, as seis seguidas)
    void dcoPitch (int v)
    {
        uint16_t ea = (uint16_t) (notePitch[(size_t) v] + tuneLfoBend);
        uint8_t frac = (uint8_t) (ea & 0xff);
        int n = ea >> 8;
        int idx;
        if (n <= 0x2f) { idx = 0; frac = 0; }
        else if (n >= 0x97) { idx = 0x66; frac = 0; }
        else idx = n - 0x30;
        // divisor: táboa da nota menos a interpolación cara á seguinte
        const uint16_t d0 = rom::CLOCK_DIV[idx], d1 = rom::CLOCK_DIV[idx + 1];
        const uint16_t div = (uint16_t) (d0 - mul8x16 (frac, (uint16_t) (d0 - d1)));
        auto& c = vcv[(size_t) v];
        const uint16_t bit = mask (v);
        if (resetVoiceBits & bit) { resetVoiceBits &= (uint16_t) ~bit; c.reset = true; }
        c.divider = div; c.dividerNew = true;
        // CV da rampa: táboa da nota máis a interpolación (ao DAC aliñada á esquerda: <<2 e despois >>2)
        const uint16_t r0 = rom::RAMP_CV[idx], r1 = rom::RAMP_CV[idx + 1];
        c.ramp = (uint16_t) ((r0 + mul8x16 (frac, (uint16_t) (r1 - r0))) & 0x0fff);
    }

    // S2: $04D5–$04F6 a base do VCF (corte ± LFO ± bend), común ás voces
    void vcfBase()
    {
        bool under = false;
        uint16_t ea = vcfCutoff;
        if (lfoFlag & 2) { if (vcfLfoSignal > ea) under = true; ea = (uint16_t) (ea - vcfLfoSignal); }
        else ea = (uint16_t) (ea + vcfLfoSignal);
        if (flags1 & 0x20) { if (vcfBendAmt > ea) under = true; ea = (uint16_t) (ea - vcfBendAmt); }
        else { if ((uint32_t) ea + vcfBendAmt > 0xffff) under = false; ea = (uint16_t) (ea + vcfBendAmt); }
        vcfBaseVal = ea; vcfOverflow1 = under;
    }

    // S3: $0503–$064E unha voz: envolvente, CV do VCF e CV do VCA
    void voiceTick (int v)
    {
        const uint16_t bit = mask (v);
        uint16_t e = env[(size_t) v];
        if (voiceRun & bit)
        {
            if (! (rlsPhase & bit)) e = attack (bit, e);                    // $0563
            else
            {
                if (atkPhase & bit) atkPhase &= (uint16_t) ~bit;           // $0517
                e = decay (e);
            }
        }
        else
        {
            // $0538: a voz non está a soar
            if ((atkPhase & bit) && (dcyPhase & bit)) { atkPhase &= (uint16_t) ~bit; e = decay (e); }
            else
            {
                atkPhase &= (uint16_t) ~bit;
                rlsPhase &= (uint16_t) ~bit;
                dcyPhase &= (uint16_t) ~bit;
                e = calcDecay (e, envRlsCoeff);                              // $0558
            }
        }
        env[(size_t) v] = e;

        // CV do VCF ($05C0)
        bool under = vcfOverflow1;
        uint16_t ea = vcfBaseVal;
        const uint16_t envAmt = mul8x16 (vcfEnvMod, e);
        if (switch2 & 2) { if ((uint32_t) ea + envAmt > 0xffff) under = false; ea = (uint16_t) (ea + envAmt); }
        else { if (envAmt > ea) under = true; ea = (uint16_t) (ea - envAmt); }
        const uint16_t p = notePitch[(size_t) v];
        const uint16_t ps = (uint16_t) ((p >> 2) + (p >> 3));                // ×0,375
        if (ps > 0x1680)
        {
            const uint16_t k = mul8x16 (vcfKeyTrack, (uint16_t) (ps - 0x1680));
            if ((uint32_t) ea + k > 0xffff) under = false;
            ea = (uint16_t) (ea + k);
        }
        else
        {
            const uint16_t k = mul8x16 (vcfKeyTrack, (uint16_t) (0x1680 - ps));
            if (k > ea) under = true;
            ea = (uint16_t) (ea - k);
        }
        if (ea & 0xc000) ea = under ? 0x0000 : 0x3fff;
        vcv[(size_t) v].vcf = ea;

        // CV do VCA: envolvente ou porta ($05A6, $073A)
        vcv[(size_t) v].vca = (switch2 & 4) ? e : ((voiceRun & bit) ? 0x3fff : 0x0000);
    }

    // S4: $0653–$0735 a panca e os mandos (unha volta un grupo do conversor, a seguinte o outro)
    void adcAndBend()
    {
        adcSecondSet = ! adcSecondSet;
        if (adcSecondSet)
        {
            const uint8_t pol = leverDown ? 0x80 : 0x00;
            uint8_t b;
            if (((pol ^ bendPol2) & 0x80) || adcBendCv != bendVal1)
            {
                // $06BD: moveuse a panca
                bendPol2 = pol;
                flags1 &= (uint8_t) ~0x20;
                if (pol & 0x80) flags1 |= 0x20;
                bendVal1 = bendVal2 = adcBendCv;
                midiBendPending = false;
            }
            else if (midiBendPending)
            {
                // $06D0: chegou bend polo MIDI
                flags1 &= (uint8_t) ~0x20;
                if (flags1 & 0x10) flags1 |= 0x20;
                bendVal2 = midiBendAmt;
                midiBendPending = false;
            }
            b = bendVal2;
            vcfBendAmt = (uint16_t) ((b * adcVcfBendSens) >> 4);
            uint16_t ea = (uint16_t) (b * adcDcoBendSens);
            const uint8_t d = (uint8_t) (ea >> 8);
            const uint16_t half = (uint16_t) (ea >> 1);
            ea = (uint16_t) ((ea >> 2) + half + d);           // ×1,75 «arredondado» como na ROM
            const uint8_t semis = (uint8_t) (ea >> 12);
            const uint16_t fracw = (uint16_t) (ea << 4);
            dcoBend = (uint16_t) ((semis << 8) | (fracw >> 8));
        }
        else
        {
            // $06E2: portamento, panca empurrada / roda MIDI e afinación
            portaCoeff = adcPorta == 0 ? 0 : rom::PORTA[adcPorta >> 1];
            uint8_t c;
            if (leverPushed)
            {
                // $0726: a panca sobe de 10 en 10 por volta ata a sensibilidade (sen clic)
                if (adcLfoSens > modSwDeclick) { int x = modSwDeclick + 10; modSwDeclick = (uint8_t) std::min (x, 255); c = modSwDeclick; }
                else c = adcLfoSens;
            }
            else { modSwDeclick = 0; c = 0; }
            const uint8_t wheel = (uint8_t) ((midiModDepth * adcLfoSens) >> 8);
            lfoModToPitch = std::max (wheel, c);
            if (adcTune & 0x80) { tuningAmt = adcTune & 0x7f; flags1 &= 0x7f; }
            else { tuningAmt = (uint8_t) (0x80 - adcTune); flags1 |= 0x80; }
        }
    }

    // S5: $074E–$07B5 o LFO, o PWM e o nivel do ruído
    void lfoAndPwm()
    {
        uint16_t ea = lfoVal;
        if (lfoFlag & 1)
        {
            if (lfoRateCoeff > ea) { ea = 0; ++lfoFlag; }
            else ea = (uint16_t) (ea - lfoRateCoeff);
        }
        else
        {
            ea = (uint16_t) (ea + lfoRateCoeff);
            if (ea & 0xe000) { ea = 0x1fff; ++lfoFlag; }
        }
        lfoVal = ea;
        uint16_t bc = (lfoFlag & 2) ? (uint16_t) (0x2000 - ea) : (uint16_t) (ea + 0x2000);
        if (! (switch2 & 1)) bc = 0x3fff;                          // PWM manual
        uint16_t pw = (uint16_t) (0x3fff - mul8x16 (pwmDepth, bc));
        if (! (flags1 & 0x40)) pw = 0;                             // sen pulso: anchura 0
        pwmSignal = pw;
        gcv.pwm = pwmSignal;
        gcv.noise = noise;
    }

    // o LFO como se ve fóra (para a pantalla): −1..+1
    float lfoNow() const { return (float) lfoVal / 8191.0f * ((lfoFlag & 2) ? -1.0f : 1.0f); }
    uint16_t envOf (int v) const { return env[(size_t) v]; }
    bool gateOf (int v) const { return (voiceRun >> v) & 1; }
    uint16_t pitchOf (int v) const { return notePitch[(size_t) v]; }
    int noteOf (int v) const { return voiceNote[(size_t) v]; }
    uint8_t lfoOnset() const { return onset; }

private:
    // RAM ($FF00–$FF7F); as marcas por voz en 16 bits para ata 12 voces
    uint16_t resetVoiceBits = 0, noteGate = 0, voiceRun = 0, atkPhase = 0, dcyPhase = 0, rlsPhase = 0;
    std::array<uint8_t, MAXV> voiceNote {};
    std::array<uint16_t, MAXV> notePitch {};
    std::array<uint16_t, MAXV> env {};
    uint8_t flags1 = 0;          // $FF1E: 0 pedal, 1–3 atraso do LFO, 4 bend MIDI cara abaixo, 5 bend restando, 6 pulso, 7 afinación negativa
    uint8_t switch1 = 0, switch2 = 0, ic40Latch = 0x15;
    uint16_t envDcyCoeff = 0, envRlsCoeff = 0, envSustain = 0;
    uint8_t envAtk = 0;
    uint16_t subLev = 0, noise = 0, vcfCutoff = 0, vcfReso = 0, vcaMaster = 0;
    uint8_t vcfEnvMod = 0, vcfKeyTrack = 0, pwmDepth = 0, lfoToVcf = 0, lfoToPitchScaler = 0;
    uint8_t lfoFlag = 0;
    uint16_t lfoRateCoeff = 0, lfoVal = 0, pwmSignal = 0, lfoToPitch = 0, vcfLfoSignal = 0;
    uint16_t lfoHoldOff = 0, lfoHoldoffRate = 0, lfoDelayEnv = 0, lfoAttackRate = 0;
    uint8_t tuningAmt = 0, midiBendAmt = 0, midiModDepth = 0, lfoModToPitch = 0;
    uint16_t vcfBendAmt = 0, dcoBend = 0, tuneLfoBend = 0;
    uint8_t portaCoeff = 0, modSwDeclick = 0;
    uint8_t bendPol2 = 0, bendVal1 = 0, bendVal2 = 0;
    bool midiBendPending = false, adcSecondSet = false;
    uint16_t vcfBaseVal = 0; bool vcfOverflow1 = false;
    uint8_t onset = 0;

    uint16_t mask (int v) const { return (uint16_t) (1u << v); }
    uint16_t allMask() const { return (uint16_t) ((1u << nv) - 1); }

    // $01D7: 0–127 → 0–$3F80
    static uint16_t store16 (uint8_t d) { return (uint16_t) (d << 7); }

    // multiplicación de 8 por 16 bits en dous MUL de 8×8: (a·BC) >> 8 sen a parte baixa do produto baixo
    static uint16_t mul8x16 (uint8_t a, uint16_t bc)
    {
        return (uint16_t) (a * (bc >> 8) + ((a * (bc & 0xff)) >> 8));
    }

    // $083D: valor × coeficiente (0.16) con tres MUL de 8×8; a cuarta parte (baixa × baixa) non se calcula
    static uint16_t calcDecay (uint16_t value, uint16_t coeff)
    {
        const uint8_t vh = (uint8_t) (value >> 8), vl = (uint8_t) (value & 0xff);
        const uint8_t ch = (uint8_t) (coeff >> 8), cl = (uint8_t) (coeff & 0xff);
        return (uint16_t) (vh * ch + ((vh * cl) >> 8) + ((vl * ch) >> 8));
    }

    uint16_t attack (uint16_t bit, uint16_t e)
    {
        dcyPhase &= (uint16_t) ~bit;
        uint32_t s = (uint32_t) e + rom::ENV_ATTACK[envAtk];
        if ((s >> 8) & 0xc0 || s > 0xffff)
        {
            rlsPhase |= bit; dcyPhase |= bit;
            return 0x3fff;
        }
        return (uint16_t) s;
    }

    uint16_t decay (uint16_t e)
    {
        // $051E: cara ao nivel de sustain; se xa está debaixo, queda nel
        if (e <= envSustain) return envSustain;
        return (uint16_t) (calcDecay ((uint16_t) (e - envSustain), envDcyCoeff) + envSustain);
    }

    // Os interruptores chegan á CPU de voces cos bits 0–3 de SW1 e 0–2 de SW2 ao revés que no SysEx
    // (a ROM le o rexistro e o pulso en lóxica negativa); Synth fai a volta antes de mandalos.
    void setSwitch1 (uint8_t d)
    {
        // $014F
        switch1 = d;
        uint8_t a = (uint8_t) ((ic40Latch & 0x0c) | (d & 0x10));
        if (! (d & 0x20)) a |= 1;
        if (d & 0x40) a |= 2;
        ic40Latch = a;
        flags1 = (d & 0x08) ? (uint8_t) (flags1 & ~0x40) : (uint8_t) (flags1 | 0x40);
        gcv.pf = ! (d & 4) ? 0xc0 : ! (d & 2) ? 0x40 : 0x00;
    }

    void setSwitch2 (uint8_t d)
    {
        // $0184
        switch2 = d;
        const uint8_t hpf = (uint8_t) (((d ^ 0x18) & 0x18) >> 1);
        ic40Latch = (uint8_t) ((ic40Latch & 0xf3) | hpf);
    }

    // $030D–$03A0: atraso do LFO (espera e subida) e canto LFO vai ao ton e ao VCF
    void lfoEnvelopeAndAmounts()
    {
        if ((voiceRun & allMask()) == 0) flags1 |= 0x08;
        else if (flags1 & 0x08)
        {
            lfoHoldOff = 0; lfoDelayEnv = 0;
            flags1 &= 0xf1;
        }
        uint8_t b;
        if (! (flags1 & 0x02))
        {
            lfoHoldOff = (uint16_t) (lfoHoldOff + lfoHoldoffRate);
            if ((lfoHoldOff >> 8) & 0xc0) { flags1 |= 0x02; b = ramp(); }
            else b = 0;
        }
        else if (! (flags1 & 0x04)) b = ramp();
        else b = 0xff;
        onset = b;

        uint8_t a = (uint8_t) ((lfoToPitchScaler * b) >> 8);
        const int s = a + lfoModToPitch;
        a = s > 0xff ? 0xff : (uint8_t) s;
        lfoToPitch = (uint16_t) (mul8x16 (a, lfoVal) >> 3);
        const uint8_t av = (uint8_t) ((lfoToVcf * b) >> 8);
        vcfLfoSignal = (uint16_t) (mul8x16 (av, lfoVal) >> 1);
    }

    uint8_t ramp()
    {
        // $0388
        const uint32_t s = (uint32_t) lfoDelayEnv + lfoAttackRate;
        if (s > 0xffff) { flags1 |= 0x04; return 0xff; }
        lfoDelayEnv = (uint16_t) s;
        return (uint8_t) (s >> 8);
    }

    // $03AD: ton base (8.8: semitón e 1/256) = $1818 ± afinación ± LFO ± bend
    void basePitch()
    {
        uint16_t ea = 0x1818;
        ea = (flags1 & 0x80) ? (uint16_t) (ea - tuningAmt) : (uint16_t) (ea + tuningAmt);
        ea = (lfoFlag & 2) ? (uint16_t) (ea - lfoToPitch) : (uint16_t) (ea + lfoToPitch);
        ea = (flags1 & 0x20) ? (uint16_t) (ea - dcoBend) : (uint16_t) (ea + dcoBend);
        tuneLfoBend = ea;
    }

    // $03D7: o ton de cada voz achégase á nota en pasos de portaCoeff/256 de semitón
    void portamento (int v)
    {
        const uint16_t target = (uint16_t) (voiceNote[(size_t) v] << 8);
        uint16_t ea = notePitch[(size_t) v];
        if (portaCoeff == 0 || ea == target) ea = target;
        else if (ea > target) { ea = (uint16_t) (ea - portaCoeff); if (! (ea > target)) ea = target; }
        else { ea = (uint16_t) (ea + portaCoeff); if (! (ea < target)) ea = target; }
        notePitch[(size_t) v] = ea;
    }
};

} // namespace xuno
