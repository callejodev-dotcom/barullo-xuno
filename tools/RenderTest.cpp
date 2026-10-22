// Proba sen interface: os 128 sons propios, independencia do tamaño de bloque, SysEx recibido polo
// MIDI, gardar/recuperar o estado, os modos de asignación, A/B, presets e desfacer (do núcleo), o
// limitador e capturas da interface.
//   BXRenderTest [saida.wav]                proba enteira (devolve o número de fallos)
//   BXRenderTest --snapshot saida.png [táboa]
//   BXRenderTest --niveis saida.txt         sonoridade de cada son (para tools/banco.py)
//   BXRenderTest --render saida.wav [son]   un son: notas e acordes
//   BXRenderTest --midi banco.syx ficheiro.mid saida.wav
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Banco.h"
#include <cstdio>
#include <cstring>

static void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, (unsigned) b.getNumChannels(), 24, {}, 0)))
        {
            os.release();
            w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
            std::printf ("WAV: %s\n", f.getFullPathName().toRawUTF8());
        }
}

struct Ev { double t; int type; int a; int b; };   // 0 on, 1 off, 2 programa, 3 cc, 5 bend

static juce::AudioBuffer<float> play (BXProcessor& p, const std::vector<Ev>& seq, double len, int blockSize, double sr, double* cpu = nullptr)
{
    p.setPlayConfigDetails (0, 2, sr, blockSize);
    p.prepareToPlay (sr, blockSize);
    const int total = (int) (len * sr);
    juce::AudioBuffer<float> out (2, total), buf (2, blockSize);
    size_t k = 0;
    const auto c0 = juce::Time::getMillisecondCounterHiRes();
    for (int pos = 0; pos < total; pos += blockSize)
    {
        const int n = juce::jmin (blockSize, total - pos);
        juce::MidiBuffer midi;
        while (k < seq.size() && (int) (seq[k].t * sr) < pos + n)
        {
            const int at = juce::jmax (0, (int) (seq[k].t * sr) - pos);
            const auto& e = seq[k++];
            juce::MidiMessage m = e.type == 0 ? juce::MidiMessage::noteOn (1, e.a, (juce::uint8) e.b)
                               : e.type == 1 ? juce::MidiMessage::noteOff (1, e.a)
                               : e.type == 2 ? juce::MidiMessage::programChange (1, e.a)
                               : e.type == 3 ? juce::MidiMessage::controllerEvent (1, e.a, e.b)
                               : juce::MidiMessage::pitchWheel (1, e.a);
            midi.addEvent (m, at);
        }
        buf.setSize (2, n, false, false, true);
        buf.clear();
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, n);
    }
    if (cpu) *cpu = (juce::Time::getMillisecondCounterHiRes() - c0) / 1000.0;
    return out;
}

// cada son: unha nota, un acorde, o bend e a roda
static std::vector<Ev> tour (int first, int count, double per, bool modulate = true)
{
    std::vector<Ev> seq;
    for (int v = 0; v < count; ++v)
    {
        const double t0 = v * per + 0.02;
        seq.push_back ({ t0, 2, first + v, 0 });
        seq.push_back ({ t0 + 0.05, 0, 60, 100 });
        seq.push_back ({ t0 + 0.55, 1, 60, 0 });
        for (int n : { 48, 55, 64, 67 }) seq.push_back ({ t0 + 0.65, 0, n, 90 });
        if (modulate)
        {
            seq.push_back ({ t0 + 0.75, 3, 1, 90 });
            seq.push_back ({ t0 + 0.85, 5, 12000, 0 });
            seq.push_back ({ t0 + 1.0, 5, 8192, 0 });
            seq.push_back ({ t0 + 1.05, 3, 1, 0 });
        }
        for (int n : { 48, 55, 64, 67 }) seq.push_back ({ t0 + 1.15, 1, n, 0 });
        seq.push_back ({ t0 + per - 0.06, 3, 120, 0 });   // todo calado antes do seguinte
    }
    return seq;
}

// a pegada dun son: FNV-1a de 64 bits das mostras (un cambio de organización non pode movela nin un bit)
static uint64_t pegada (const juce::AudioBuffer<float>& b)
{
    uint64_t h = 1469598103934665603ull;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float x = b.getSample (c, i);
            uint32_t u; std::memcpy (&u, &x, 4);
            for (int k = 0; k < 4; ++k) { h ^= (u >> (8 * k)) & 0xFF; h *= 1099511628211ull; }
        }
    return h;
}

static void setParam (BXProcessor& p, const char* id, float v)
{
    if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

static float getParam (BXProcessor& p, const char* id) { return p.apvts.getRawParameterValue (id)->load(); }

static void snapshot (const juce::File& png, bool chart)
{
    BXProcessor p;
    p.prepareToPlay (48000, 512);
    p.selectSource (0);
    p.selectPatch (8);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    auto* ce = dynamic_cast<BXEditor*> (ed.get());
    ed->setSize (ui::MainView::W, ui::MainView::H);
    for (int n : { 60, 64, 67 }) p.keyboardState.noteOn (1, n, 1.0f);
    ce->view().showChart (chart);
    juce::AudioBuffer<float> buf (2, 512);
    for (int blk = 0; blk < 40; ++blk) { juce::MidiBuffer m; buf.clear(); p.processBlock (buf, m); p.serviceMessages(); ce->view().timerCallback(); }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    png.deleteFile();
    juce::FileOutputStream os (png);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("PNG: %s\n", png.getFullPathName().toRawUTF8());
}

int main (int argc, char** argv)
{
    std::setvbuf (stdout, nullptr, _IONBF, 0);
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000;
    const juce::String mode = argc > 1 ? juce::String (argv[1]) : juce::String();

    if (mode == "--snapshot" && argc > 2) { snapshot (juce::File (juce::String (argv[2])), argc > 3); return 0; }

    if (mode == "--niveis" && argc > 2)
    {
        // sonoridade dun acorde de catro notas (ou dunha nota nos baixos) en cada son
        juce::String out;
        for (int i = 0; i < 128; ++i)
        {
            BXProcessor p;
            std::vector<Ev> seq { { 0.0, 2, i, 0 } };
            const bool bass = juce::String (xuno::bank::PATCHES[i].name).startsWith ("Baixo");
            if (bass) seq.push_back ({ 0.05, 0, 36, 100 });
            else for (int n : { 48, 55, 60, 64 }) seq.push_back ({ 0.05, 0, n, 100 });
            auto o = play (p, seq, 1.5, 512, sr);
            // o máximo do valor eficaz en ventás de 100 ms (sonoridade a curto prazo)
            const int w = (int) (0.1 * sr);
            double best = 0;
            for (int a = (int) (0.05 * sr); a + w <= o.getNumSamples(); a += w / 4)
            {
                double e = 0;
                for (int s = a; s < a + w; ++s) e += 0.5 * (o.getSample (0, s) * o.getSample (0, s) + o.getSample (1, s) * o.getSample (1, s));
                best = std::max (best, e / w);
            }
            const double db = 10.0 * std::log10 (best + 1e-12);
            out << i << " " << juce::String (db, 2) << "\n";
            std::printf ("%3d %-24s %7.2f dB\n", i, xuno::bank::PATCHES[i].name, db);
        }
        juce::File (juce::String (argv[2])).replaceWithText (out);
        return 0;
    }

    if (mode == "--render" && argc > 2)
    {
        BXProcessor p;
        const int first = argc > 3 ? atoi (argv[3]) : 0;
        const int count = argc > 3 ? 1 : 128;
        auto o = play (p, tour (first, count, 2.0, false), count * 2.0 + 0.5, 512, sr);
        writeWav (juce::File (juce::String (argv[2])), o, sr);
        return 0;
    }

    if (mode == "--midi" && argc > 4)
    {
        BXProcessor p;
        juce::String msg;
        if (juce::String (argv[2]) != "-" && ! p.loadSyxFile (juce::File (juce::String (argv[2])), msg)) { std::printf ("%s\n", msg.toRawUTF8()); return 1; }
        juce::MidiFile mf;
        juce::FileInputStream in { juce::File (juce::String (argv[3])) };
        if (! mf.readFrom (in)) { std::printf ("MIDI?\n"); return 1; }
        mf.convertTimestampTicksToSeconds();
        juce::MidiMessageSequence seq;
        for (int t = 0; t < mf.getNumTracks(); ++t) seq.addSequence (*mf.getTrack (t), 0);
        const int bs = 256;
        p.setPlayConfigDetails (0, 2, sr, bs);
        p.prepareToPlay (sr, bs);
        const int total = (int) ((seq.getEndTime() + 2.0) * sr);
        juce::AudioBuffer<float> out (2, total), buf (2, bs);
        int k = 0;
        for (int pos = 0; pos < total; pos += bs)
        {
            const int n = juce::jmin (bs, total - pos);
            juce::MidiBuffer midi;
            while (k < seq.getNumEvents() && (int) (seq.getEventPointer (k)->message.getTimeStamp() * sr) < pos + n)
            {
                const auto& m = seq.getEventPointer (k)->message;
                if (! m.isMetaEvent()) midi.addEvent (m, juce::jmax (0, (int) (m.getTimeStamp() * sr) - pos));
                ++k;
            }
            buf.setSize (2, n, false, false, true); buf.clear();
            p.processBlock (buf, midi);
            p.serviceMessages();
            for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, n);
        }
        writeWav (juce::File (juce::String (argv[4])), out, sr);
        return 0;
    }

    int fails = 0;
    auto check = [&fails] (bool ok, const char* what, const juce::String& extra = {})
    {
        std::printf ("%-46s %s %s\n", what, ok ? "OK" : "FALLO", extra.toRawUTF8());
        fails += ok ? 0 : 1;
    };

    {   // ao abrir, cada parámetro está no seu valor de partida (o panel é A11, que é de onde saen)
        BXProcessor p;
        juce::String bad;
        for (auto* prm : p.getParameters())
            if (std::abs (prm->getValue() - prm->getDefaultValue()) > 1.0e-6f) bad << prm->getName (40) << " ";
        check (bad.isEmpty(), "parámetros nos seus valores de partida", bad);
        // e o texto das eleccións volve ao mesmo valor, tamén se chega byte a byte (coma desde CLAP)
        juce::String badText;
        for (auto* prm : p.getParameters())
        {
            const auto t = prm->getText (1.0f, 64);
            juce::String asBytes;
            for (const char* c = t.toRawUTF8(); *c != 0; ++c) asBytes += juce::String::charToString ((juce::juce_wchar) (uint8_t) *c);
            if (std::abs (prm->getValueForText (asBytes) - 1.0f) > 1.0e-3f && prm->getNumSteps() <= 4) badText << prm->getName (40) << " ";
        }
        check (badText.isEmpty(), "texto das eleccións ida e volta", badText);
    }
    {   // SysEx: un son ($30) e un parámetro ($32), coma os manda un Juno-106
        BXProcessor p;
        p.prepareToPlay (sr, 512);
        xuno::Patch s {};
        for (int b = 0; b < 18; ++b) s[(size_t) b] = xuno::bank::PATCHES[40].p[b];
        xuno::normalize (s);
        auto sx = xuno::patchToSysex (s, 5);
        juce::MidiBuffer m;
        m.addEvent (juce::MidiMessage (sx.data(), (int) sx.size()), 0);
        const uint8_t ipr[] = { 0xF0, 0x41, 0x32, 0x00, 0x05, 0x21, 0xF7 };   // VCF FREQ = 33
        m.addEvent (juce::MidiMessage (ipr, (int) sizeof (ipr)), 10);
        juce::AudioBuffer<float> buf (2, 512);
        p.processBlock (buf, m);
        p.serviceMessages();
        auto expect = s; expect[xuno::VCF_FREQ] = 33;
        check (p.panelPatch() == expect && p.isManual(), "SysEx recibido (son + parámetro)");
    }
    {   // cambio de programa polo MIDI: o panel vai ao son
        BXProcessor p;
        p.selectSource (0);
        auto o = play (p, { { 0.01, 2, 70, 0 } }, 0.1, 512, sr);
        p.serviceMessages();
        check (p.getPatchNum() == 70 && p.panelPatch() == p.patchAt (0, 70) && ! p.isEdited(), "cambio de programa = son B17");
    }
    {   // estado
        BXProcessor a, b;
        a.selectSource (0); a.selectPatch (12);
        if (auto* prm = a.apvts.getParameter ("vcfRes")) prm->setValueNotifyingHost (prm->convertTo0to1 (99));
        a.writeTo (77);
        a.selectPatch (3);
        if (auto* prm = a.apvts.getParameter ("envA")) prm->setValueNotifyingHost (prm->convertTo0to1 (55));
        a.uiScale = 0.8f;
        juce::MemoryBlock mb; a.getStateInformation (mb);
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        const bool ok = b.getSource() == BXProcessor::INTERNAL && b.getPatchNum() == 3 && b.panelPatch() == a.panelPatch()
                        && b.patchAt (BXProcessor::INTERNAL, 77) == a.patchAt (BXProcessor::INTERNAL, 77) && b.isEdited()
                        && std::abs (b.uiScale - 0.8f) < 1e-6f;
        check (ok, "estado gardado/recuperado", "(" + juce::String ((int) mb.getSize()) + " bytes)");
    }
    {   // ficheiro .syx: gardar o banco e volvelo abrir dá os mesmos bytes
        BXProcessor p;
        const auto f = juce::File::createTempFile (".syx");
        p.selectSource (0);
        const bool saved = p.saveBankFile (f, 0);
        juce::String msg;
        const bool loaded = p.loadSyxFile (f, msg);
        bool same = saved && loaded && p.getSource() == BXProcessor::FILE_BANK;
        for (int i = 0; i < 128 && same; ++i) same = p.patchAt (BXProcessor::FILE_BANK, i) == p.patchAt (0, i);
        juce::MemoryBlock a, b2;
        f.loadFileAsData (a);
        const auto again = juce::File::createTempFile (".syx");
        p.saveBankFile (again, BXProcessor::FILE_BANK);
        again.loadFileAsData (b2);
        check (same && a == b2 && a.getSize() == 128 * 24, "banco .syx: gardar e abrir", "(" + juce::String ((int) a.getSize()) + " bytes)");
        f.deleteFile(); again.deleteFile();
    }
    {   // asignación: POLY 1 deixa soar as colas, POLY 2 reutiliza a voz 1, o unísono acende as seis
        auto voicesFor = [&] (int assign)
        {
            BXProcessor p;
            p.selectSource (0); p.selectPatch (8);   // cordas, relaxación longa (e o son leva POLY 1: a asignación, despois)
            if (auto* prm = p.apvts.getParameter ("poly")) prm->setValueNotifyingHost (prm->convertTo0to1 ((float) assign));
            p.setPlayConfigDetails (0, 2, sr, 512); p.prepareToPlay (sr, 512);
            juce::AudioBuffer<float> buf (2, 512);
            int maxActive = 0;
            for (int k = 0; k < 120; ++k)
            {
                juce::MidiBuffer m;
                if (k % 20 == 0) m.addEvent (juce::MidiMessage::noteOn (1, 60 + k / 20, (juce::uint8) 100), 0);
                if (k % 20 == 10) m.addEvent (juce::MidiMessage::noteOff (1, 60 + k / 20), 0);
                buf.clear(); p.processBlock (buf, m);
                maxActive = juce::jmax (maxActive, p.activeVoices.load());
            }
            return maxActive;
        };
        const int a = voicesFor (0), b = voicesFor (1), c = voicesFor (2);
        check (a >= 3 && b <= 2 && c == 6, "POLY 1 / POLY 2 / unísono", juce::String (a) + " / " + juce::String (b) + " / " + juce::String (c) + " voces");
    }
    {   // tamaños de bloque
        BXProcessor a, b;
        double c1 = 0, c2 = 0;
        auto x = play (a, tour (0, 16, 1.6), 16 * 1.6 + 0.2, 512, sr, &c1);
        auto y = play (b, tour (0, 16, 1.6), 16 * 1.6 + 0.2, 37, sr, &c2);
        float d = 0;
        for (int c = 0; c < 2; ++c) for (int i = 0; i < x.getNumSamples(); ++i) d = juce::jmax (d, std::abs (x.getSample (c, i) - y.getSample (c, i)));
        const double secs = x.getNumSamples() / sr;
        check (d < 1.0e-6f, "bloques de 512 e de 37 mostras: iguais", juce::String::formatted ("(diferenza %g; %.2f s de CPU para %.1f s, %.0fx)", d, c1, secs, secs / c1));
        std::printf ("pegada da saída: %016llx\n", (unsigned long long) pegada (x));
        if (argc > 1 && ! mode.startsWith ("--")) writeWav (juce::File (mode), x, sr);
    }
    {   // o portamento e a asignación van co son: os propios, WRITE, o estado, o MIDI; os do .syx non os tocan
        BXProcessor p;
        auto play = [&] { return juce::String ((int) getParam (p, "portaOn")) + "/" + juce::String ((int) getParam (p, "porta")) + "/" + juce::String ((int) getParam (p, "poly")); };
        p.selectSource (0);
        p.selectPatch (64 + 5 * 8 + 2);                           // B63 Deslizante: porta aceso e POLY 2
        const bool propio = play() == "1/90/1" && ! p.isEdited();
        p.selectPatch (8);                                        // A21 Cordas: sen porta, POLY 1
        const bool volve = play() == "0/90/0";
        setParam (p, "portaOn", 1.0f); setParam (p, "porta", 150.0f); setParam (p, "poly", 2.0f);
        const bool editado = p.isEdited();
        p.writeTo (77);
        p.selectPatch (0); p.selectPatch (77);
        const bool gardado = play() == "1/150/2" && ! p.isEdited();
        juce::MemoryBlock mb; p.getStateInformation (mb);
        BXProcessor b; b.setStateInformation (mb.getData(), (int) mb.getSize());
        b.selectPatch (0);
        b.selectPatch (77);
        const bool estado = juce::String ((int) getParam (b, "portaOn")) + "/" + juce::String ((int) getParam (b, "porta")) + "/" + juce::String ((int) getParam (b, "poly")) == "1/150/2";
        // os dun .syx non o levan: queda coma estaba
        const auto f = juce::File::createTempFile (".syx");
        p.saveBankFile (f, 0);
        juce::String msg; p.loadSyxFile (f, msg); f.deleteFile();
        setParam (p, "poly", 1.0f);
        p.selectPatch (3);
        const bool syx = play() == "1/150/1";
        // cambio de programa polo MIDI: o motor xa vai en POLY 2 antes de que a interface mova os parámetros
        BXProcessor m;
        m.selectSource (0);
        m.setPlayConfigDetails (0, 2, sr, 512); m.prepareToPlay (sr, 512);
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer mm; mm.addEvent (juce::MidiMessage::programChange (1, 64 + 5 * 8 + 2), 0);
        m.processBlock (buf, mm);
        const bool xa = m.synthMode() == 1;
        juce::MidiBuffer baleiro; m.processBlock (buf, baleiro);
        const bool sigue = m.synthMode() == 1;
        m.serviceMessages();
        m.processBlock (buf, baleiro);
        const bool despois = m.synthMode() == 1 && getParam (m, "poly") > 0.5f && getParam (m, "portaOn") > 0.5f;
        check (propio && volve && editado && gardado && estado && syx && xa && sigue && despois, "portamento e asignación co son",
               juce::String::formatted ("(%d%d%d%d%d%d%d%d%d)", propio, volve, editado, gardado, estado, syx, xa, sigue, despois));
    }
    {   // A/B (do núcleo): cada lado co seu panel, a súa memoria e a súa función
        BXProcessor p;
        p.selectSource (0); p.selectPatch (5); setParam (p, "envA", 55);
        const auto panelA = p.panelPatch();
        const auto mem100 = p.patchAt (BXProcessor::INTERNAL, 100);
        p.estado.cambiarLado();                                   // B: copia de A
        bool ok = p.estado.lado() == 1 && p.getPatchNum() == 5 && p.panelPatch() == panelA;
        p.selectPatch (20); p.writeTo (100); setParam (p, "volume", -6.0f); setParam (p, "voices12", 1.0f);
        const auto panelB = p.panelPatch();
        p.estado.cambiarLado();                                   // volve a A
        ok = ok && p.estado.lado() == 0 && p.getSource() == 0 && p.getPatchNum() == 5 && p.panelPatch() == panelA
                && p.patchAt (BXProcessor::INTERNAL, 100) == mem100 && std::abs (getParam (p, "volume")) < 0.01f && getParam (p, "voices12") < 0.5f;
        p.estado.cambiarLado();                                   // B outra vez
        ok = ok && p.estado.lado() == 1 && p.getSource() == BXProcessor::INTERNAL && p.getPatchNum() == 100 && p.panelPatch() == panelB
                && p.patchAt (BXProcessor::INTERNAL, 100) == panelB && std::abs (getParam (p, "volume") + 6.0f) < 0.01f && getParam (p, "voices12") > 0.5f;
        check (ok, "A/B (panel, memoria e función)");
    }
    {   // presets propios (do núcleo): gárdano todo e cárgano igual
        BXProcessor p;
        const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("barulloxuno_presets_proba");
        dir.deleteRecursively();
        p.estado.setCarpeta (dir);
        p.selectSource (0); p.selectPatch (33); setParam (p, "vcfFreq", 20); p.writeTo (2); setParam (p, "envR", 99); setParam (p, "volume", -3.0f);
        const auto panel = p.panelPatch(); const auto mem2 = p.patchAt (BXProcessor::INTERNAL, 2);
        const bool saved = p.estado.gardar ("Proba: Xuno/1");
        p.selectSource (0); p.selectPatch (0); p.initPanel(); setParam (p, "volume", 0.0f);
        const auto list = p.estado.presets();
        const bool loaded = list.size() == 1 && list[0].hasFileExtension ("bxpreset") && p.estado.cargar (list[0]);
        const bool same = p.getSource() == BXProcessor::INTERNAL && p.getPatchNum() == 2 && p.panelPatch() == panel && p.isEdited()
                          && p.patchAt (BXProcessor::INTERNAL, 2) == mem2 && std::abs (getParam (p, "volume") + 3.0f) < 0.01f;
        check (saved && loaded && same, "presets propios: gardar e cargar", list.isEmpty() ? juce::String() : "(" + list[0].getFileName() + ")");
        dir.deleteRecursively();
    }
    {   // desfacer e refacer: un deslizador, escoller un son e WRITE, cada un no seu paso
        BXProcessor p;
        p.selectSource (0); p.selectPatch (0);
        p.apvts.copyState(); p.undo.clearUndoHistory();
        const auto start = p.panelPatch(); const auto mem77 = p.patchAt (BXProcessor::INTERNAL, 77);
        p.estado.novoPaso(); setParam (p, "vcfRes", 99); p.apvts.copyState();
        const auto edited = p.panelPatch();
        p.estado.novoPaso(); p.cambiarMemoria ("son", [&] { p.selectPatch (9); }); p.apvts.copyState();
        const auto nine = p.panelPatch();
        p.estado.novoPaso(); p.cambiarMemoria ("write", [&] { p.writeTo (77); }); p.apvts.copyState();
        p.estado.novoPaso();
        const bool after = p.getSource() == BXProcessor::INTERNAL && p.getPatchNum() == 77 && p.patchAt (BXProcessor::INTERNAL, 77) == nine;
        p.undo.undo();
        const bool u1 = p.getSource() == 0 && p.getPatchNum() == 9 && p.patchAt (BXProcessor::INTERNAL, 77) == mem77 && p.panelPatch() == nine;
        p.undo.undo();
        const bool u2 = p.getPatchNum() == 0 && p.panelPatch() == edited;
        p.undo.undo();
        const bool u3 = p.panelPatch() == start && ! p.isEdited();
        p.undo.redo(); p.undo.redo(); p.undo.redo();
        const bool r = p.getSource() == BXProcessor::INTERNAL && p.getPatchNum() == 77 && p.patchAt (BXProcessor::INTERNAL, 77) == nine && p.panelPatch() == nine;
        check (after && u1 && u2 && u3 && r, "desfacer e refacer (deslizador, son e WRITE)");
    }
    {   // o limitador do núcleo: o son máis forte, un acorde e o volume a +6 dB non pasan de −1 dBFS
        BXProcessor p;
        p.selectSource (0);
        setParam (p, "volume", 6.0f);
        std::vector<Ev> seq { { 0.0, 2, 35, 0 } };
        for (int n : { 36, 43, 48, 52, 55, 60 }) seq.push_back ({ 0.05, 0, n, 127 });
        auto o = play (p, seq, 1.5, 512, sr);
        const float pk = juce::jmax (o.getMagnitude (0, 0, o.getNumSamples()), o.getMagnitude (1, 0, o.getNumSamples()));
        check (pk <= 0.8913f, "teito a -1 dBFS", juce::String::formatted ("(pico %.4f)", pk));
    }
    {   // os 128 sons: nada de NaN e os niveis
        BXProcessor p;
        p.selectSource (0);
        const double per = 1.6;
        auto o = play (p, tour (0, 128, per), 128 * per + 0.2, 512, sr);
        int nan = 0;
        for (int c = 0; c < 2; ++c) for (int i = 0; i < o.getNumSamples(); ++i) if (! std::isfinite (o.getSample (c, i))) { ++nan; break; }
        std::vector<float> peaks;
        float worst = 0, quiet = 1; int wi = 0, qi = 0;
        for (int v = 0; v < 128; ++v)
        {
            const int a = (int) (v * per * sr), b = juce::jmin (o.getNumSamples(), (int) ((v + 1) * per * sr));
            const float pk = juce::jmax (o.getMagnitude (0, a, b - a), o.getMagnitude (1, a, b - a));
            peaks.push_back (pk);
            if (pk > worst) { worst = pk; wi = v; }
            if (pk < quiet) { quiet = pk; qi = v; }
        }
        std::sort (peaks.begin(), peaks.end());
        std::printf ("128 sons: pico máis alto %.3f (%s %s), máis baixo %.4f (%s %s); mediana %.3f\n", worst,
                     BXProcessor::patchLabel (wi).toRawUTF8(), xuno::bank::PATCHES[wi].name, quiet, BXProcessor::patchLabel (qi).toRawUTF8(),
                     xuno::bank::PATCHES[qi].name, peaks[64]);
        check (nan == 0 && quiet > 0.01f, "os 128 sons soan e sen NaN");
    }
    return fails;
}
