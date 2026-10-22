// Assigner.h — O asignador de voces do Juno-106 (IC1, a outra µPD7811G): de que teclas hai premidas
// a que voz toca cada nota. Reescrito desde o desensamblado ($0A18–$0B80).
//
// O asignador non garda notas senón un mapa de bits (as teclas e o MIDI xuntos) e compárao co da
// volta anterior; os cambios trátaos de arriba abaixo, así que dun acorde que chega de vez a nota
// máis aguda colle voz primeiro. Unha tecla xa premida non volve disparar.
//
//  POLY 1: a voz libre que leva máis tempo calada, salvo que algunha das libres tocase xa esta
//          nota (entón volve a esa); as colas de relaxación seguen soando. Sen voz libre, a nota perdese.
//  POLY 2: sempre a primeira voz libre (a 1, a 2…): corta as colas, bo para o portamento.
//  POLY 1 + POLY 2 xuntos: unísono, as seis voces nunha nota; manda a última premida e, ao soltar,
//          volve á máis aguda das que quedan.
#pragma once
#include "Cpu.h"
#include <array>
#include <bitset>
#include <functional>

namespace xuno {

class Assigner
{
public:
    enum Mode { POLY1 = 0, POLY2 = 1, UNISON = 2 };

    int nv = 6;
    int mode = POLY1;
    int transpose = 0;   // −12..+12, sumado á nota que vai á CPU de voces

    // a quen se lle mandan as ordes (Synth: á CPU de voces, no seu momento)
    std::function<void (int voice, int note)> sendOn;
    std::function<void (int voice)> sendOff;

    void reset()
    {
        // $0B81
        for (int v = 0; v < MAXV; ++v) { noteTbl[(size_t) v] = 0x80; voiceTbl[(size_t) v] = (uint8_t) v; }
        held.reset(); cur.reset();
        unisonHeld = false;
    }

    Assigner() { reset(); }

    // teclas (MIDI ou pantalla): o mapa de bits cambia e o asignador mira os cambios
    void keyDown (int note) { if (note >= 0 && note < 128) held.set ((size_t) note); }
    void keyUp (int note) { if (note >= 0 && note < 128) held.reset ((size_t) note); }
    void allKeysUp() { held.reset(); }

    // unha volta do asignador: os cambios entre o mapa actual e o anterior, de arriba abaixo
    // devolve true se hai que volver mirar axiña (o unísono soltou e debe redisparar)
    bool scan()
    {
        bool rescan = false;
        scanFlag = 0;   // $01E8 ($FFD0)
        for (int n = 127; n >= 0; --n)
        {
            const bool h = held[(size_t) n], c = cur[(size_t) n];
            if (h == c) continue;
            cur[(size_t) n] = h;
            if (h) noteOn (n); else noteOff (n);
        }
        if (scanFlag == 0xff) { cur.reset(); rescan = true; }   // $0203: o unísono esquece o mapa
        return rescan;
    }

    bool anyHeld() const { return held.any(); }
    // para a pantalla: que nota ten cada voz (−1 libre)
    int voiceNote (int v) const { return (noteTbl[(size_t) v] & 0x80) ? -1 : (noteTbl[(size_t) v] & 0x7f); }

private:
    std::array<uint8_t, MAXV> noteTbl {};   // $FF88: nota de cada voz; bit 7 = libre
    std::array<uint8_t, MAXV> voiceTbl {};  // $FF80: orde das voces (as ocupadas diante)
    std::bitset<128> held, cur;
    bool unisonHeld = false;
    int scanFlag = 0;

    int out (int note) const { return std::clamp (note + transpose, 0, 127); }

    void noteOn (int n)
    {
        if (mode == POLY2) poly2On (n);
        else if (mode == POLY1) poly1On (n);
        else
        {
            // $0A42: na volta só entra a primeira nota acesa
            if (scanFlag != 0) return;
            for (int v = 0; v < nv; ++v) if (sendOn) sendOn (v, out (n));   // $0B55: non toca as táboas
            scanFlag = 0x80;
        }
    }

    void noteOff (int n)
    {
        if (mode == POLY2) poly2Off (n);
        else if (mode == POLY1) poly1Off (n);
        else
        {
            // $0A69
            if (scanFlag != 0) return;
            for (int v = 0; v < nv; ++v) if (sendOff) sendOff (v);
            scanFlag = 0xff;
        }
    }

    // $0A76: a primeira voz libre por número
    void poly2On (int n)
    {
        for (int v = 0; v < nv; ++v)
            if (noteTbl[(size_t) v] & 0x80)
            {
                noteTbl[(size_t) v] = (uint8_t) n;
                if (sendOn) sendOn (v, out (n));
                return;
            }
    }

    // $0B30: a primeira voz ocupada con esta nota
    void poly2Off (int n)
    {
        for (int v = 0; v < nv; ++v)
            if (! (noteTbl[(size_t) v] & 0x80) && noteTbl[(size_t) v] == n)
            {
                noteTbl[(size_t) v] |= 0x80;
                if (sendOff) sendOff (v);
                return;
            }
    }

    // $0AAF
    void poly1On (int n)
    {
        const int last = nv - 1;
        if (! (noteTbl[(size_t) voiceTbl[(size_t) last]] & 0x80)) return;   // ningunha libre
        int pos = last;
        int voice = -1;
        // desde atrás, polas libres: se unha tocou esta nota, vólvese a ela
        for (; pos >= 0; --pos)
        {
            const int v = voiceTbl[(size_t) pos];
            if (! (noteTbl[(size_t) v] & 0x80)) break;                       // primeira ocupada
            if ((noteTbl[(size_t) v] & 0x7f) == n)
            {
                // $0AD7: leva esa voz ao principio da táboa
                for (int p = pos; p > 0; --p) std::swap (voiceTbl[(size_t) p], voiceTbl[(size_t) p - 1]);
                voice = v;
                break;
            }
        }
        if (voice < 0) voice = voiceTbl[(size_t) (pos + 1)];                  // $0AD2: a libre máis antiga
        noteTbl[(size_t) voice] = (uint8_t) n;
        if (sendOn) sendOn (voice, out (n));
    }

    // $0AF8: a voz que solta vai ao final da táboa
    void poly1Off (int n)
    {
        for (int pos = 0; pos < nv; ++pos)
        {
            const int v = voiceTbl[(size_t) pos];
            if ((noteTbl[(size_t) v] & 0x7f) != n) continue;
            for (int p = pos; p < nv - 1; ++p) std::swap (voiceTbl[(size_t) p], voiceTbl[(size_t) p + 1]);
            noteTbl[(size_t) v] |= 0x80;
            if (sendOff) sendOff (v);
            return;
        }
    }
};

} // namespace xuno
