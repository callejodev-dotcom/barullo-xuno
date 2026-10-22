// Synth.h — O Juno-106 enteiro visto desde o plugin: o asignador, a CPU de voces co seu bucle,
// as seis voces analóxicas, a placa (HPF, VCA xeral, chorus) e o paso á frecuencia do DAW.
//
// Tempos do orixinal que se reproducen:
//  · o bucle da CPU de voces dura 4,23 ms e cada sección vai no seu momento dentro da volta;
//  · cada orde de nota (voz acesa ou apagada, pedal) fai volver empezar o bucle;
//  · co tempo «coma o orixinal», as ordes do asignador chegan polo porto serie a 31 250 baudios
//    (3 bytes unha nota, 20 un son enteiro: un cambio de son tarda uns 6 ms en asentar).
// Co tempo «ao instante» (por defecto) as ordes chegan na mostra exacta e a voz que se acende
// fai a súa parte do bucle nese momento, sen esperar á súa quenda.
#pragma once
#include "Cpu.h"
#include "Assigner.h"
#include "Voice.h"
#include "Board.h"
#include "Output.h"
#include "Patch.h"
#include <deque>

namespace xuno {

class Synth
{
public:
    struct Options
    {
        bool instant = true;        // as notas no seu instante (se non, co tempo do porto serie)
        bool variance = true;       // cada voz coas súas tolerancias, coma nun aparello
        bool analogNoise = true;    // o ruído das etapas do BBD
        int voices = 6;             // 6 (orixinal) ou 12
    };

    static constexpr double LOOP_SECONDS = 4.2335e-3;

    Synth() { asg.sendOn = [this] (int v, int n) { queueVoiceOn (v, n); }; asg.sendOff = [this] (int v) { queueVoiceOff (v); }; reset(); }

    void prepare (double hostRate)
    {
        step = INTERNAL_RATE / hostRate;
        rs.design (INTERNAL_RATE, hostRate);
        hpf.prepare (INTERNAL_RATE);
        chorus.prepare (INTERNAL_RATE);
        noise.prepare (INTERNAL_RATE);
        dcCoef = 1.0 - std::exp (-2 * PI * 6.0 / INTERNAL_RATE);
        cvCoef = 1.0 - std::exp (-1.0 / (0.3e-3 * INTERNAL_RATE));
        levelCoef = 1.0 - std::exp (-1.0 / (1.0e-3 * INTERNAL_RATE));
        reset();
    }

    void reset()
    {
        cpu.coldStart();
        cpu.nv = opt.voices;
        asg.nv = opt.voices;
        asg.reset();
        for (auto& v : voices) v.reset();
        applyVariance();
        hpf.reset(); chorus.reset(); noise.reset(); rs.reset();
        serial.clear();
        tick = 0; chipPos = 0.0; loopPos = 0.0; section = 0;
        level = 1.0;
        sendPatchNow (patch);
        runSection (0);   // a primeira volta empeza xa
        section = 1;
        applyCv (0, true);
    }

    // ---------------------------------------------------------------- opcións
    const Options& options() const { return opt; }
    void setOptions (const Options& o)
    {
        const bool voicesChanged = o.voices != opt.voices;
        opt = o;
        opt.voices = o.voices == 12 ? 12 : 6;
        applyVariance();
        chorus.a.noise = chorus.b.noise = opt.analogNoise ? 2.0e-4 : 0.0;
        if (voicesChanged)
        {
            for (int v = 0; v < MAXV; ++v) queueVoiceOff (v);
            cpu.nv = asg.nv = opt.voices;
            asg.reset();
        }
    }

    // ---------------------------------------------------------------- son
    const Patch& getPatch() const { return patch; }
    void setPatch (const Patch& p)
    {
        patch = p; sanitize (patch);
        if (opt.instant) sendPatchNow (patch);
        else
        {
            // FD, $8E e os 18 valores co «running status» da CPU de voces
            serialBytes (2);
            for (int i = 0; i < PATCH_SIZE; ++i) { serialBytes (1); queue (Cmd::Param, order (i), patch[(size_t) order (i)]); }
        }
    }
    void setPatchByte (int index, uint8_t value)
    {
        if (index < 0 || index >= PATCH_SIZE) return;
        patch[(size_t) index] = (uint8_t) (value & 0x7f);
        if (opt.instant) sendByte (index, patch[(size_t) index]);
        else { serialBytes (3); queue (Cmd::Param, index, patch[(size_t) index]); }
    }

    // ---------------------------------------------------------------- tocar
    void noteOn (int note) { asg.keyDown (note); runAssigner(); }
    void noteOff (int note) { asg.keyUp (note); runAssigner(); }
    void allNotesOff() { asg.allKeysUp(); runAssigner(); }
    void allSoundOff()
    {
        allNotesOff();
        serial.clear();
        for (int v = 0; v < MAXV; ++v) cpu.voiceOff (v);
        cpu.sustain (false);
        for (auto& v : voices) { v.vcf.reset(); v.vcaCv = v.vcaTarget = 0; }
    }
    void sustain (bool on)
    {
        if (on == sustainOn) return;
        sustainOn = on;
        if (opt.instant) { cpu.sustain (on); restartLoop(); }
        else { serialBytes (2); queue (Cmd::Sustain, on ? 1 : 0, 0); }
    }
    void setMode (int m) { if (m != asg.mode) { asg.allKeysUp(); runAssigner(); asg.mode = std::clamp (m, 0, 2); asg.reset(); } }
    int mode() const { return asg.mode; }
    void setTranspose (int t) { asg.transpose = std::clamp (t, -12, 12); }

    // roda de afinación do MIDI: o asignador quédase cos 8 bits de arriba e manda signo e 7 bits
    void pitchBend14 (int v)
    {
        const int b8 = std::clamp (v, 0, 16383) >> 6;
        const bool down = b8 < 128;
        const uint8_t mag = (uint8_t) (down ? 127 - b8 : b8 - 128);
        if (opt.instant) cpu.midiBend (down, mag);
        else { serialBytes (3); queue (Cmd::Bend, down ? 1 : 0, mag); }
    }
    void modWheel (int v7)
    {
        if (opt.instant) cpu.midiMod ((uint8_t) v7);
        else { serialBytes (3); queue (Cmd::Mod, 0, (uint8_t) v7); }
    }

    // mandos que le a propia CPU de voces (0–255, xa como os ve despois do seu conversor)
    void setTune (int adc) { cpu.adcTune = (uint8_t) std::clamp (adc, 0, 255); }
    void setPortamento (int adc, bool on) { cpu.adcPorta = on ? (uint8_t) std::clamp (adc, 0, 255) : 0; }
    void setBendSens (int dco, int vcf) { cpu.adcDcoBendSens = (uint8_t) std::clamp (dco, 0, 255); cpu.adcVcfBendSens = (uint8_t) std::clamp (vcf, 0, 255); }
    void setLfoSens (int s) { cpu.adcLfoSens = (uint8_t) std::clamp (s, 0, 255); }
    // a panca: −1..+1 cara aos lados, e empurrada cara adiante
    void setLever (double x, bool pushed)
    {
        cpu.leverDown = x < 0;
        cpu.adcBendCv = (uint8_t) std::clamp ((int) std::lround (std::abs (x) * 255.0), 0, 255);
        cpu.leverPushed = pushed;
    }

    // ---------------------------------------------------------------- audio
    // n mostras á frecuencia do DAW, en estéreo
    void render (float* L, float* R, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            chipPos += step;
            while (chipPos >= 1.0) { chipPos -= 1.0; internalSample(); }
            rs.out (chipPos, L[i], R[i]);
        }
    }

    // ---------------------------------------------------------------- para a pantalla
    int activeVoices() const
    {
        int n = 0;
        for (int v = 0; v < opt.voices; ++v) n += (cpu.gateOf (v) || cpu.envOf (v) > 0x10) ? 1 : 0;
        return n;
    }
    float voiceLevel (int v) const { return (float) cpu.envOf (v) / 16383.0f; }
    bool voiceGate (int v) const { return cpu.gateOf (v); }
    int voiceNote (int v) const { return cpu.noteOf (v); }
    float lfo() const { return cpu.lfoNow(); }
    float lfoOnset() const { return (float) cpu.lfoOnset() / 255.0f; }

    // ganancia de saída: unha serra soa, filtro aberto e VCA LEVEL no medio → ±0,15
    double outGain = 0.3;

private:
    Options opt;
    Cpu cpu;
    Assigner asg;
    std::array<Voice, MAXV> voices {};
    Hpf hpf;
    Chorus chorus;
    NoiseSource noise;
    Resampler rs;
    Patch patch = initPatch();
    bool sustainOn = false;

    int64_t tick = 0;          // tic de 8 MHz ao comezo da mostra interna actual
    double chipPos = 0.0, step = 1.0;
    double loopPos = 0.0;      // mostras internas desde que empezou a volta do bucle
    int section = 0;
    std::array<bool, MAXV> tickedThisLoop {};
    double dcCoef = 0.0, cvCoef = 0.0, levelCoef = 0.0, level = 1.0, levelTarget = 1.0;

    // ---------------------------------------------------------------- porto serie (tempo do orixinal)
    enum class Cmd { VoiceOn, VoiceOff, Sustain, Param, Bend, Mod };
    struct Pending { int64_t at; Cmd cmd; int a; int b; };
    std::deque<Pending> serial;
    int64_t serialFree = 0;    // tic en que queda libre a liña
    void serialBytes (int n)
    {
        // 320 µs por byte; a liña vai en cola
        serialFree = std::max (serialFree, tick) + (int64_t) n * 2560;
    }
    void queue (Cmd c, int a, int b) { serial.push_back ({ serialFree, c, a, b }); }

    void queueVoiceOn (int v, int note)
    {
        if (opt.instant) { cpu.voiceOn (v, note); restartLoop(); voiceNow (v); return; }
        serialBytes (3); queue (Cmd::VoiceOn, v, note);
    }
    void queueVoiceOff (int v)
    {
        if (opt.instant) { cpu.voiceOff (v); restartLoop(); return; }
        serialBytes (2); queue (Cmd::VoiceOff, v, 0);
    }

    void serviceSerial()
    {
        while (! serial.empty() && serial.front().at <= tick)
        {
            const auto p = serial.front();
            serial.pop_front();
            switch (p.cmd)
            {
                case Cmd::VoiceOn:  cpu.voiceOn (p.a, p.b); restartLoop(); break;
                case Cmd::VoiceOff: cpu.voiceOff (p.a); restartLoop(); break;
                case Cmd::Sustain:  cpu.sustain (p.a != 0); restartLoop(); break;
                case Cmd::Param:    sendByte (p.a, (uint8_t) p.b); break;
                case Cmd::Bend:     cpu.midiBend (p.a != 0, (uint8_t) p.b); break;
                case Cmd::Mod:      cpu.midiMod ((uint8_t) p.b); break;
            }
        }
    }

    void runAssigner()
    {
        if (asg.scan()) rescanAt = tick + 2 * 8000;   // o unísono volve mirar as teclas na seguinte volta (~2 ms)
    }
    int64_t rescanAt = -1;

    // ---------------------------------------------------------------- envío do son á CPU
    // orde na que o asignador manda un son enteiro: SW1, SW2 e os 16 deslizadores
    static int order (int i) { return i < 2 ? SW1 + i : i - 2; }
    void sendByte (int index, uint8_t v)
    {
        if (index == SW1) cpu.setPatchByte (Cpu::SW1_, (uint8_t) (v ^ 0x0f));
        else if (index == SW2) cpu.setPatchByte (Cpu::SW2_, (uint8_t) (v ^ 0x07));
        else cpu.setPatchByte (index, v);
    }
    void sendPatchNow (const Patch& p) { for (int i = 0; i < PATCH_SIZE; ++i) sendByte (order (i), p[(size_t) order (i)]); }

    // ---------------------------------------------------------------- o bucle da CPU
    // seccións: 0 comezo · 1..nv DCO · nv+1 base do VCF · nv+2..2nv+1 voces · 2nv+2 conversor · 2nv+3 LFO
    double sectionAt (int s) const
    {
        const int nv = opt.voices;
        if (s == 0) return 0.0;
        if (s <= nv) return 0.14 + 0.33 * (s - 1) / nv;
        if (s == nv + 1) return 0.47;
        if (s <= 2 * nv + 1) return 0.49 + 0.36 * (s - nv - 2) / nv;
        if (s == 2 * nv + 2) return 0.86;
        if (s == 2 * nv + 3) return 0.95;
        return 1.0;
    }
    int numSections() const { return 2 * opt.voices + 4; }

    void runSection (int s)
    {
        const int nv = opt.voices;
        if (s == 0) { cpu.loopStart(); tickedThisLoop.fill (false); }
        else if (s <= nv) cpu.dcoPitch (s - 1);
        else if (s == nv + 1) cpu.vcfBase();
        else if (s <= 2 * nv + 1) { const int v = s - nv - 2; if (! tickedThisLoop[(size_t) v]) cpu.voiceTick (v); tickedThisLoop[(size_t) v] = true; }
        else if (s == 2 * nv + 2) cpu.adcAndBend();
        else if (s == 2 * nv + 3) cpu.lfoAndPwm();
    }

    // unha orde de nota: a ROM borra a pila e volve ao comezo do bucle
    void restartLoop()
    {
        loopPos = 0.0;
        runSection (0);
        section = 1;
    }

    // co tempo «ao instante»: a voz fai xa a súa parte (ton, envolvente, CV)
    void voiceNow (int v)
    {
        cpu.dcoPitch (v);
        cpu.vcfBase();
        cpu.voiceTick (v);
        tickedThisLoop[(size_t) v] = true;
        applyCv (tick, false);
    }

    // ---------------------------------------------------------------- unha mostra interna
    void internalSample()
    {
        if (rescanAt >= 0 && tick >= rescanAt) { rescanAt = -1; runAssigner(); }
        if (! serial.empty()) serviceSerial();

        // o bucle da CPU
        const double loopLen = LOOP_SECONDS * INTERNAL_RATE;
        bool ran = false;
        while (section < numSections() && loopPos >= sectionAt (section) * loopLen) { runSection (section++); ran = true; }
        if (loopPos >= loopLen) { loopPos -= loopLen; runSection (0); section = 1; ran = true; }
        if (ran) applyCv (tick, false);
        loopPos += 1.0;

        const int64_t t0 = tick, t1 = tick + TICKS_PER_SAMPLE;
        const auto& g = cpu.gcv;
        const double theta = pulseThreshold (g.pwm);
        const bool sawOn = (g.latch & 0x10) != 0;
        const double subLevel = subGain ((double) g.sub / 16383.0) * 1.19;   // +1,5 dB sobre a serra
        const double noiseLevel = noiseGain ((double) g.noise / 16383.0);
        const double nz = noiseLevel > 0.0 ? noise.run() * noiseLevel : 0.0;
        const double r = (double) g.res / 16256.0;
        const double comp = 1.0 + 0.5 * r * r;

        double sum = 0.0;
        for (int vi = 0; vi < opt.voices; ++vi)
        {
            auto& v = voices[(size_t) vi];
            auto& d = v.dco;
            v.blep = Blep {};
            const double sawAmp = sawOn ? 1.0 : 0.0;

            // palabra de modo do 8253 (nota nova nunha voz parada): OUT sobe xa
            if (d.restartPending)
            {
                d.restartPending = false;
                d.restart (t0, [&] (int64_t)
                {
                    v.blep.add (-sawAmp * d.ramp, 1.0);
                    d.ramp = 0.0;
                    if (d.pulse) { d.pulse = false; v.blep.add (-1.0, 1.0); }
                    d.sub = ! d.sub;
                    v.blep.add ((d.sub ? 1.0 : -1.0) * subLevel, 1.0);
                });
            }

            // o comparador mira o nivel novo ao comezo da mostra
            d.theta = theta;
            const bool want = d.ramp > theta;
            if (want != d.pulse) { d.pulse = want; v.blep.add (want ? 1.0 : -1.0, 1.0); }

            double tc = (double) t0;
            for (;;)
            {
                const double te = (double) d.nextEdge;
                const double tx = (! d.pulse && d.slope > 0.0 && theta > d.ramp) ? tc + (theta - d.ramp) / d.slope : 1e300;
                const double tn = std::min (te, tx);
                if (tn >= (double) t1) break;
                d.ramp += d.slope * (tn - tc);
                tc = tn;
                const double dd = ((double) t1 - tn) / TICKS_PER_SAMPLE;
                if (tx < te) { d.pulse = true; v.blep.add (1.0, dd); continue; }
                // transición do 8253
                d.advanceTo (d.nextEdge + 1, [&] (int64_t)
                {
                    v.blep.add (-sawAmp * d.ramp, dd);
                    d.ramp = 0.0;
                    if (d.pulse) { d.pulse = false; v.blep.add (-1.0, dd); }
                    d.sub = ! d.sub;
                    v.blep.add ((d.sub ? 1.0 : -1.0) * subLevel, dd);
                });
            }
            d.ramp += d.slope * ((double) t1 - tc);

            const double naive = sawAmp * (d.ramp - 0.5) + (d.pulse ? 0.5 : -0.5) + (d.sub ? 0.5 : -0.5) * subLevel;
            const double mixed = v.held + v.blep.prev;          // a mostra anterior, xa enteira
            v.held = naive + v.blep.now;

            // acoplamento en alterna, ruído, VCF e VCA
            v.dcState += dcCoef * (mixed - v.dcState);
            const double x = (mixed - v.dcState) + nz;

            v.vcfCode += cvCoef * (v.vcfCodeTarget - v.vcfCode);
            v.vcaCv += cvCoef * (v.vcaTarget - v.vcaCv);
            if (--v.coefCounter <= 0)
            {
                v.coefCounter = 4;
                const double fc = std::min (vcfHz (v.vcfCode + v.vcfOffset), 0.45 * INTERNAL_RATE);
                v.vcf.g = std::tan (PI * fc / INTERNAL_RATE);
                v.vcf.k = 4.6 * r * v.resScale;
            }
            const double y = v.vcf.process (x * DRIVE * comp) / DRIVE;
            sum += y * vcaGain (v.vcaCv) * v.vcaScale;
        }

        // a placa: HPF, VCA xeral (±10 dB), chorus
        hpf.pos = (g.latch >> 2) & 3;
        double h = hpf.run (sum);
        level += levelCoef * (levelTarget - level);
        h *= level;
        chorus.on = (g.latch & 1) != 0;
        chorus.modeI = (g.latch & 2) != 0;
        double l, rr;
        chorus.run (h, l, rr);
        rs.push ((float) (l * outGain), (float) (rr * outGain));
        tick = t1;
    }

    static constexpr double DRIVE = 1.4;

    // o que escribiu a CPU pasa ás voces (DAC, multiplexores, 8253)
    void applyCv (int64_t t, bool force)
    {
        const auto& g = cpu.gcv;
        // VCA xeral (uPC1252H2): lineal en dB, −10 dB abaixo, 0 no medio, +10 arriba
        levelTarget = std::pow (10.0, (-10.0 + 20.0 * (double) g.vcaMaster / 16256.0) / 20.0);
        if (force) level = levelTarget;
        for (int vi = 0; vi < MAXV; ++vi)
        {
            auto& v = voices[(size_t) vi];
            auto& c = cpu.vcv[(size_t) vi];
            auto& d = v.dco;
            const int tpc = g.pf == 0xc0 ? 2 : g.pf == 0x40 ? 4 : 8;
            if (tpc != d.ticksPerClock)
            {
                // cambio de rexistro: o que falta para a seguinte transición, co reloxo novo
                const int64_t left = std::max<int64_t> (0, d.nextEdge - t);
                d.nextEdge = t + left * tpc / d.ticksPerClock;
                d.ticksPerClock = tpc;
            }
            if (c.dividerNew || force) { d.write (c.divider); c.dividerNew = false; }
            if (c.reset) { c.reset = false; d.restartPending = true; }
            if (force) d.restart (t, [&] (int64_t) { d.ramp = 0.0; d.pulse = false; });
            d.setRampCv (c.ramp);
            v.vcfCodeTarget = (double) (c.vcf >> 2);
            v.vcaTarget = (double) c.vca / 16383.0;
            if (force) { v.vcfCode = v.vcfCodeTarget; v.vcaCv = v.vcaTarget; }
        }
    }

    // tolerancias de cada voz: sempre as mesmas (un aparello concreto), ou ningunha
    void applyVariance()
    {
        static const double vcf[MAXV] = { 9, -14, 4, -6, 13, -3, -9, 6, 11, -12, 2, -5 };     // códigos (±0,05 oitavas)
        static const double res[MAXV] = { 1.02, 0.97, 1.0, 1.03, 0.98, 1.01, 0.99, 1.02, 0.97, 1.0, 1.03, 0.98 };
        static const double vca[MAXV] = { 0.25, -0.3, 0.1, -0.15, 0.3, -0.05, 0.2, -0.25, 0.05, 0.15, -0.2, 0.0 };   // dB
        static const double amp[MAXV] = { 1.006, 0.992, 1.0, 1.009, 0.995, 1.003, 0.994, 1.007, 0.998, 1.004, 0.991, 1.002 };
        for (int v = 0; v < MAXV; ++v)
        {
            auto& x = voices[(size_t) v];
            x.vcfOffset = opt.variance ? vcf[v] : 0.0;
            x.resScale = opt.variance ? res[v] : 1.0;
            x.vcaScale = opt.variance ? std::pow (10.0, vca[v] / 20.0) : 1.0;
            x.dco.amp = opt.variance ? amp[v] : 1.0;
        }
    }
};

} // namespace xuno
