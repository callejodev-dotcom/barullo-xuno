#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Banco.h"
#include "Conhecidos.h"
#include <set>

using escoita::U;

const char* const BXProcessor::sliderIds[16] = { "lfoRate", "lfoDelay", "dcoLfo", "dcoPwm", "dcoNoise", "vcfFreq", "vcfRes",
    "vcfEnv", "vcfLfo", "vcfKybd", "vcaLevel", "envA", "envD", "envS", "envR", "dcoSub" };

BXProcessor::BXProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "BarulloXuno", createLayout())
{
    for (int i = 0; i < 16; ++i) sliderParams[(size_t) i] = apvts.getRawParameterValue (sliderIds[i]);
    rangeParam = apvts.getRawParameterValue ("range");
    pulseParam = apvts.getRawParameterValue ("pulse");
    sawParam = apvts.getRawParameterValue ("saw");
    chorusParam = apvts.getRawParameterValue ("chorus");
    pwmParam = apvts.getRawParameterValue ("pwmMode");
    envPolParam = apvts.getRawParameterValue ("envPol");
    vcaParam = apvts.getRawParameterValue ("vcaMode");
    hpfParam = apvts.getRawParameterValue ("hpf");
    volumeParam = apvts.getRawParameterValue ("volume");
    tuneParam = apvts.getRawParameterValue ("tune");
    portaParam = apvts.getRawParameterValue ("porta");
    portaOnParam = apvts.getRawParameterValue ("portaOn");
    bendDcoParam = apvts.getRawParameterValue ("bendDco");
    bendVcfParam = apvts.getRawParameterValue ("bendVcf");
    lfoModParam = apvts.getRawParameterValue ("lfoMod");
    polyParam = apvts.getRawParameterValue ("poly");
    transposeParam = apvts.getRawParameterValue ("transpose");
    instantParam = apvts.getRawParameterValue ("instant");
    varianceParam = apvts.getRawParameterValue ("variance");
    noiseParam = apvts.getRawParameterValue ("analogNoise");
    voices12Param = apvts.getRawParameterValue ("voices12");

    // o banco propio
    Bank own;
    own.name = "XUNO";
    own.title = U ("Os sons de Barullo Xuno: A, clásicos; B, Escoita");
    own.own = true;
    for (int k = 0; k < 128; ++k)
    {
        const auto& e = xuno::bank::PATCHES[k];
        auto& p = own.patches[(size_t) k];
        for (int b = 0; b < xuno::PATCH_SIZE; ++b) p[(size_t) b] = e.p[b];
        xuno::normalize (p);
        own.play[(size_t) k] = { true, e.portaOn, e.porta, e.mode };
    }
    own.hash = xuno::bankHash (own.patches.data(), 128);
    banks.push_back (own);
    internal = banks[0].patches;   // a memoria empeza cos sons propios
    internalPlay = banks[0].play;
    for (auto& p : fileBank) p = xuno::initPatch();

    // a carpeta de bancos lémbrase nun ficheiro de axustes do usuario
    juce::PropertiesFile::Options o;
    o.applicationName = "Barullo Xuno";
    o.folderName = "Escoita";
    o.filenameSuffix = ".settings";
    o.osxLibrarySubFolder = "Application Support";
    settings = std::make_unique<juce::PropertiesFile> (o);
   #ifndef BARULLO_NO_PLUGIN_ENTRY
    const auto saved = settings->getValue ("bankFolder");
    if (saved.isNotEmpty()) { bankFolder = juce::File (saved); rescanBankFolder(); }
   #endif

    source = INTERNAL; patchNum = 0;
    goTo (INTERNAL, 0);
    lastParamPatch = patchFromParams();
    synth.setPatch (lastParamPatch);
    publishBank();
    startTimerHz (30);
}

BXProcessor::~BXProcessor() { stopTimer(); }

// unha elección co seu texto lido con coidado: o envoltorio de CLAP pasa o texto do DAW como char*
// e JUCE leo coma se fose ASCII, así que «Unísono» ou «−» chegan byte a byte; recompóñense en UTF-8
static std::unique_ptr<juce::AudioParameterChoice> choice (const char* id, const juce::String& name, const juce::StringArray& options, int def)
{
    auto fromText = [options] (const juce::String& t)
    {
        std::string bytes;
        bool narrow = true;
        for (auto c : t) { if (c > 0xff) { narrow = false; break; } bytes.push_back ((char) c); }
        const auto fixed = (narrow ? escoita::textoDeBytes (bytes.data(), bytes.size()) : t).trim();
        for (int i = 0; i < options.size(); ++i)
            if (options[i].equalsIgnoreCase (fixed) || options[i].equalsIgnoreCase (t.trim())) return i;
        return juce::jlimit (0, options.size() - 1, fixed.getIntValue());
    };
    return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, options, def,
                                                         juce::AudioParameterChoiceAttributes().withValueFromStringFunction (fromText));
}

juce::AudioProcessorValueTreeState::ParameterLayout BXProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    // os interruptores van como eleccións de dúas: gardan o valor xa redondeado (un AudioParameterBool
    // garda o que lle escriban, 0,49 por exemplo, e ao recuperar o estado non volve ao 0)
    const StringArray onOff { "Apagado", "Aceso" };
    static const char* names[16] = { "LFO: velocidade", "LFO: atraso", "DCO: LFO", "DCO: PWM", "DCO: ruído", "VCF: frecuencia",
        "VCF: resonancia", "VCF: envolvente", "VCF: LFO", "VCF: teclado", "VCA: nivel", "ENV: ataque", "ENV: caída",
        "ENV: sustain", "ENV: relaxación", "DCO: sub" };
    // os valores de partida son os do primeiro son (A11), o que ten o panel ao abrir
    xuno::Patch init {};
    for (int b = 0; b < xuno::PATCH_SIZE; ++b) init[(size_t) b] = xuno::bank::PATCHES[0].p[b];
    xuno::normalize (init);
    for (int i = 0; i < 16; ++i)
        l.add (std::make_unique<AudioParameterInt> (ParameterID { sliderIds[i], 1 }, U (names[i]), 0, 127, init[(size_t) i]));
    l.add (choice ("range", U ("DCO: rexistro"), StringArray { "16'", "8'", "4'" }, xuno::rangeOf (init)));
    l.add (choice ("pulse", U ("DCO: pulso"), onOff, (init[xuno::SW1] & 0x08) ? 1 : 0));
    l.add (choice ("saw", U ("DCO: serra"), onOff, (init[xuno::SW1] & 0x10) ? 1 : 0));
    l.add (choice ("chorus", U ("Chorus"), StringArray { "Apagado", "I", "II" }, xuno::chorusOn (init) ? (xuno::chorusII (init) ? 2 : 1) : 0));
    l.add (choice ("pwmMode", U ("DCO: PWM con"), StringArray { "LFO", "Manual" }, init[xuno::SW2] & 1));
    l.add (choice ("envPol", U ("VCF: polaridade da envolvente"), StringArray { "+", U ("−") }, (init[xuno::SW2] >> 1) & 1));
    l.add (choice ("vcaMode", U ("VCA: modo"), StringArray { "Envolvente", "Porta" }, (init[xuno::SW2] >> 2) & 1));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "hpf", 1 }, U ("HPF"), 0, 3, xuno::hpfOf (init)));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Volume",
        NormalisableRange<float> (-30.0f, 6.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "tune", 1 }, U ("Afinación"), -128, 127, -8));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "porta", 1 }, U ("Portamento"), 0, 255, 90));
    l.add (choice ("portaOn", U ("Portamento aceso"), onOff, 0));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "bendDco", 1 }, U ("Bender: DCO"), 0, 255, 43));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "bendVcf", 1 }, U ("Bender: VCF"), 0, 255, 0));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "lfoMod", 1 }, U ("Bender: LFO"), 0, 255, 110));
    l.add (choice ("poly", U ("Asignación"), StringArray { "Poly 1", "Poly 2", U ("Unísono") }, 0));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "transpose", 1 }, U ("Transposición"), -12, 12, 0));
    l.add (choice ("instant", U ("Notas ao instante"), onOff, 1));
    l.add (choice ("variance", U ("Voces con tolerancias"), onOff, 1));
    l.add (choice ("analogNoise", U ("Ruído do chorus"), onOff, 1));
    l.add (choice ("voices12", U ("Doce voces"), onOff, 0));
    return l;
}

// ================================================================ panel ↔ parámetros
xuno::Patch BXProcessor::patchFromParams() const
{
    xuno::Patch p {};
    for (int i = 0; i < 16; ++i) p[(size_t) i] = (uint8_t) juce::jlimit (0, 127, (int) std::lround (sliderParams[(size_t) i]->load()));
    const int range = juce::jlimit (0, 2, (int) std::lround (rangeParam->load()));
    const int chorus = juce::jlimit (0, 2, (int) std::lround (chorusParam->load()));
    uint8_t sw1 = (uint8_t) (1 << range);
    if (pulseParam->load() > 0.5f) sw1 |= 0x08;
    if (sawParam->load() > 0.5f) sw1 |= 0x10;
    if (chorus == 0) sw1 |= (uint8_t) (0x20 | chorusLevelBit);
    else if (chorus == 1) sw1 |= 0x40;
    uint8_t sw2 = 0;
    if (pwmParam->load() > 0.5f) sw2 |= 0x01;
    if (envPolParam->load() > 0.5f) sw2 |= 0x02;
    if (vcaParam->load() > 0.5f) sw2 |= 0x04;
    sw2 |= (uint8_t) ((3 - juce::jlimit (0, 3, (int) std::lround (hpfParam->load()))) << 3);
    p[xuno::SW1] = sw1; p[xuno::SW2] = sw2;
    return p;
}

xuno::Patch BXProcessor::panelPatch() const { return patchFromParams(); }

void BXProcessor::setParamValue (const char* id, float value)
{
    if (auto* p = apvts.getParameter (id))
    {
        const float n = p->convertTo0to1 (value);
        if (std::abs (p->getValue() - n) < 1.0e-6f) return;
        p->beginChangeGesture();
        p->setValueNotifyingHost (n);
        p->endChangeGesture();
    }
}

void BXProcessor::setPanel (const xuno::Patch& in)
{
    auto p = in; xuno::normalize (p);
    for (int i = 0; i < 16; ++i) setParamValue (sliderIds[i], (float) p[(size_t) i]);
    setParamValue ("range", (float) xuno::rangeOf (p));
    setParamValue ("pulse", (p[xuno::SW1] & 0x08) ? 1.0f : 0.0f);
    setParamValue ("saw", (p[xuno::SW1] & 0x10) ? 1.0f : 0.0f);
    chorusLevelBit = (uint8_t) (p[xuno::SW1] & 0x40);
    setParamValue ("chorus", xuno::chorusOn (p) ? (xuno::chorusII (p) ? 2.0f : 1.0f) : 0.0f);
    setParamValue ("pwmMode", (p[xuno::SW2] & 1) ? 1.0f : 0.0f);
    setParamValue ("envPol", (p[xuno::SW2] & 2) ? 1.0f : 0.0f);
    setParamValue ("vcaMode", (p[xuno::SW2] & 4) ? 1.0f : 0.0f);
    setParamValue ("hpf", (float) xuno::hpfOf (p));
    changes.fetch_add (1);
}

BXProcessor::Play BXProcessor::panelPlay() const
{
    return { true, portaOnParam->load() > 0.5f, (int) std::lround (portaParam->load()), (int) std::lround (polyParam->load()) };
}

void BXProcessor::setPlay (const Play& p)
{
    if (! p.set) return;
    setParamValue ("portaOn", p.portaOn ? 1.0f : 0.0f);
    setParamValue ("porta", (float) p.porta);
    setParamValue ("poly", (float) p.mode);
}

// ================================================================ carpeta de bancos
void BXProcessor::setBankFolder (const juce::File& f)
{
    bankFolder = f;
    if (settings) { settings->setValue ("bankFolder", f.getFullPathName()); settings->saveIfNeeded(); }
    rescanBankFolder();
}

void BXProcessor::rescanBankFolder()
{
    const uint64_t current = (source >= 0 && source < numBanks()) ? banks[(size_t) source].hash : 0;
    banks.resize (1);
    if (! bankFolder.isDirectory()) { changes.fetch_add (1); return; }

    std::vector<Bank> known, others;
    std::set<uint64_t> seen { banks[0].hash };
    int files = 0;
    for (const auto& entry : juce::RangedDirectoryIterator (bankFolder, true, "*.syx;*.SYX;*.Syx", juce::File::findFiles))
    {
        if (++files > 512) break;
        const auto f = entry.getFile();
        if (f.getSize() <= 0 || f.getSize() > 1024 * 1024) continue;
        juce::MemoryBlock mb;
        if (! f.loadFileAsData (mb)) continue;
        const auto* bytes = static_cast<const uint8_t*> (mb.getData());
        const auto bank = xuno::sysexToBank (xuno::parseSysex (std::vector<uint8_t> (bytes, bytes + mb.getSize())));
        if (bank.size() < 2) continue;   // un son só non é un banco
        Bank b;
        b.size = (int) std::min<size_t> (bank.size(), 128);
        for (int k = 0; k < 128; ++k) b.patches[(size_t) k] = k < b.size ? bank[(size_t) k] : xuno::initPatch();
        b.hash = xuno::bankHash (b.patches.data(), b.size);
        if (! seen.insert (b.hash).second) continue;
        const int kb = xuno::known::bankIndex (b.hash);
        if (kb >= 0)
        {
            b.name = U (xuno::known::BANKS[kb].name);
            b.title = U (xuno::known::BANKS[kb].title);
            known.push_back (b);
        }
        else
        {
            b.name = f.getFileNameWithoutExtension().toUpperCase().substring (0, 10);
            b.title = U ("Ficheiro: ") + f.getFileName();
            others.push_back (b);
        }
    }
    std::sort (others.begin(), others.end(), [] (const Bank& a, const Bank& b) { return a.name.compareNatural (b.name) < 0; });
    for (auto& b : known) banks.push_back (b);
    for (auto& b : others) banks.push_back (b);
    if (source >= 0) source = current ? juce::jmax (0, sourceForHash (current)) : 0;
    changes.fetch_add (1);
}

int BXProcessor::sourceForHash (uint64_t h) const
{
    for (int i = 0; i < numBanks(); ++i) if (banks[(size_t) i].hash == h) return i;
    return -1;
}

// ================================================================ memoria
std::array<xuno::Patch, 128>& BXProcessor::bankOf (int src)
{
    if (src == INTERNAL) return internal;
    if (src == FILE_BANK) return fileBank;
    return banks[(size_t) juce::jlimit (0, numBanks() - 1, src)].patches;
}
const std::array<xuno::Patch, 128>& BXProcessor::bankOf (int src) const { return const_cast<BXProcessor*> (this)->bankOf (src); }

std::array<BXProcessor::Play, 128>& BXProcessor::playOf (int src)
{
    if (src == INTERNAL) return internalPlay;
    if (src == FILE_BANK) return fileBankPlay;
    return banks[(size_t) juce::jlimit (0, numBanks() - 1, src)].play;
}

const xuno::Patch& BXProcessor::patchAt (int src, int i) const { return bankOf (src)[(size_t) juce::jlimit (0, 127, i)]; }
const BXProcessor::Play& BXProcessor::playAt (int src, int i) const { return const_cast<BXProcessor*> (this)->playOf (src)[(size_t) juce::jlimit (0, 127, i)]; }

void BXProcessor::goTo (int src, int i)
{
    setPanel (bankOf (src)[(size_t) i]);
    setPlay (playOf (src)[(size_t) i]);
}

juce::String BXProcessor::patchLabel (int i)
{
    i = juce::jlimit (0, 127, i);
    return juce::String (i < 64 ? "A" : "B") + juce::String ((i % 64) / 8 + 1) + juce::String (i % 8 + 1);
}

juce::String BXProcessor::sourceName (int src) const
{
    if (src == INTERNAL) return "MEMORIA";
    if (src == FILE_BANK) return fileBankLoaded ? fileBankName : juce::String ("FICHEIRO");
    return banks[(size_t) juce::jlimit (0, numBanks() - 1, src)].name;
}

juce::String BXProcessor::sourceTitle (int src) const
{
    if (src == INTERNAL) return U ("Memoria: os teus sons gardados");
    if (src == FILE_BANK) return fileBankLoaded ? U ("Banco aberto dun ficheiro") : U ("Abre un .syx para velo aquí");
    return banks[(size_t) juce::jlimit (0, numBanks() - 1, src)].title;
}

juce::String BXProcessor::patchName (int src, int i) const
{
    i = juce::jlimit (0, 127, i);
    const auto& p = patchAt (src, i);
    // os propios polo seu sitio e polo son; os de fábrica do Juno-106, pola pegada
    for (int k = 0; k < 128; ++k)
        if (banks[0].patches[(size_t) k] == p) return U (xuno::bank::PATCHES[k].name);
    const auto h = xuno::patchHash (p);
    for (const auto& f : xuno::known::PATCHES) if (f.hash == h) return U (f.name);
    return {};
}

juce::String BXProcessor::patchDesc (int src, int i) const
{
    const auto& p = patchAt (src, i);
    for (int k = 0; k < 128; ++k)
        if (banks[0].patches[(size_t) k] == p) return U (xuno::bank::PATCHES[k].desc);
    return {};
}

const juce::String BXProcessor::getProgramName (int index)
{
    const auto n = patchName (source, index);
    return patchLabel (index) + (n.isNotEmpty() ? " " + n : juce::String());
}

void BXProcessor::publishBank()
{
    {
        const juce::SpinLock::ScopedLockType lk (bankLock);
        sharedBank = bankOf (source);
        sharedPlay = playOf (source);
        bankDirty = true;
    }
    changes.fetch_add (1);
}

void BXProcessor::selectSource (int src)
{
    if (src == FILE_BANK && ! fileBankLoaded) return;
    source = (src == FILE_BANK || src == INTERNAL) ? src : juce::jlimit (0, numBanks() - 1, src);
    manual = false;
    goTo (source, patchNum);
    publishBank();
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

void BXProcessor::selectPatch (int i)
{
    patchNum = juce::jlimit (0, 127, i);
    manual = false;
    goTo (source, patchNum);
    changes.fetch_add (1);
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

void BXProcessor::setManual() { manual = true; changes.fetch_add (1); }

bool BXProcessor::isEdited() const
{
    if (manual) return false;
    const auto& play = playAt (source, patchNum);
    return patchFromParams() != bankOf (source)[(size_t) patchNum] || (play.set && panelPlay() != play);
}

void BXProcessor::writeTo (int slot)
{
    slot = juce::jlimit (0, 127, slot);
    internal[(size_t) slot] = patchFromParams();
    internalPlay[(size_t) slot] = panelPlay();   // e tamén o portamento e a asignación
    source = INTERNAL; patchNum = slot; manual = false;
    publishBank();
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

void BXProcessor::copySourceToInternal()
{
    if (source == INTERNAL) return;
    internal = bankOf (source);
    internalPlay = playOf (source);
    source = INTERNAL;
    publishBank();
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

void BXProcessor::initPanel() { setPanel (xuno::initPatch()); manual = true; changes.fetch_add (1); }

// ================================================================ ficheiros .syx
bool BXProcessor::loadSyxFile (const juce::File& f, juce::String& message)
{
    juce::MemoryBlock mb;
    if (! f.loadFileAsData (mb) || mb.getSize() == 0 || mb.getSize() > 1024 * 1024)
    {
        message = U ("Non se puido ler o ficheiro.");
        return false;
    }
    const auto* bytes = static_cast<const uint8_t*> (mb.getData());
    const auto sp = xuno::parseSysex (std::vector<uint8_t> (bytes, bytes + mb.getSize()));
    if (sp.empty())
    {
        message = U ("Non parece un ficheiro de sons de Juno-106 (.syx con mensaxes de son de Roland).");
        return false;
    }
    if (sp.size() == 1)
    {
        setPanel (sp[0].patch);
        manual = true;
        message = U ("Son cargado no panel. Para gardalo, usa WRITE.");
        changes.fetch_add (1);
        return true;
    }
    const auto bank = xuno::sysexToBank (sp);
    for (size_t k = 0; k < 128; ++k) fileBank[k] = k < bank.size() ? bank[k] : xuno::initPatch();
    fileBankPlay.fill ({});
    fileBankName = f.getFileNameWithoutExtension().toUpperCase().substring (0, 10);
    fileBankLoaded = true;
    selectSource (FILE_BANK);
    message = U ("Banco aberto: ") + f.getFileName() + " (" + juce::String ((int) juce::jmin<size_t> (bank.size(), 128)) + " sons)";
    return true;
}

bool BXProcessor::saveBankFile (const juce::File& f, int src) const
{
    const auto& b = bankOf (src);
    const auto d = xuno::bankToSysex (std::vector<xuno::Patch> (b.begin(), b.end()));
    return f.replaceWithData (d.data(), d.size());
}

bool BXProcessor::savePatchFile (const juce::File& f) const
{
    const auto d = xuno::patchToSysex (patchFromParams(), patchNum, true);
    return f.replaceWithData (d.data(), d.size());
}

// ================================================================ SysEx que chega polo MIDI
void BXProcessor::pushSysex (const uint8_t* data, int size)
{
    if (size <= 0 || size > 1024) return;
    if (sysexFifo.getFreeSpace() < size + 2) return;
    const uint8_t hdr[2] = { (uint8_t) (size & 0xff), (uint8_t) ((size >> 8) & 0xff) };
    auto put = [this] (const uint8_t* src, int n)
    {
        const auto w = sysexFifo.write (n);
        if (w.blockSize1 > 0) std::memcpy (&sysexBuf[(size_t) w.startIndex1], src, (size_t) w.blockSize1);
        if (w.blockSize2 > 0) std::memcpy (&sysexBuf[(size_t) w.startIndex2], src + w.blockSize1, (size_t) w.blockSize2);
    };
    put (hdr, 2);
    put (data, size);
}

// no fío da interface: o panel segue o que xa soa
void BXProcessor::handleSysex (const std::vector<uint8_t>& d)
{
    if (d.size() < 5 || d[0] != 0x41) return;
    if ((d[1] == 0x30 || d[1] == 0x31) && d.size() >= 22)
    {
        xuno::Patch p {};
        for (int k = 0; k < xuno::PATCH_SIZE; ++k) p[(size_t) k] = d[4 + (size_t) k];
        xuno::normalize (p);
        setPanel (p);
        manual = true;
        notice = U ("RECIBIDO SON");
        noticeTime = juce::Time::getMillisecondCounter();
    }
    else if (d[1] == 0x32)
    {
        auto p = patchFromParams();
        const int c = d[3], v = d[4] & 0x7f;
        if (c < 16) p[(size_t) c] = (uint8_t) v;
        else if (c == 16) p[xuno::SW1] = (uint8_t) v;
        else if (c == 17) p[xuno::SW2] = (uint8_t) v;
        else return;
        setPanel (p);
    }
}

void BXProcessor::timerCallback()
{
    const int prog = pendingProgram.exchange (-1);
    if (prog >= 0)
    {
        // o fío de audio xa cambiou o son; aquí só se move o panel (e o portamento e a asignación,
        // que o motor xa usa: despois diso volve mandar o que digan os parámetros)
        auto forced = playForce.load();
        patchNum = prog; manual = false;
        goTo (source, patchNum);
        playForce.compare_exchange_strong (forced, 0u);
        updateHostDisplay (ChangeDetails().withProgramChanged (true));
    }
    while (sysexFifo.getNumReady() >= 2)
    {
        uint8_t hdr[2];
        auto get = [this] (uint8_t* dst, int n)
        {
            const auto r = sysexFifo.read (n);
            if (r.blockSize1 > 0) std::memcpy (dst, &sysexBuf[(size_t) r.startIndex1], (size_t) r.blockSize1);
            if (r.blockSize2 > 0) std::memcpy (dst + r.blockSize1, &sysexBuf[(size_t) r.startIndex2], (size_t) r.blockSize2);
        };
        get (hdr, 2);
        const int size = hdr[0] | (hdr[1] << 8);
        std::vector<uint8_t> msg ((size_t) size);
        get (msg.data(), size);
        handleSysex (msg);
    }
}

// ================================================================ audio
bool BXProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void BXProcessor::applyPerformance()
{
    xuno::Synth::Options o;
    o.instant = instantParam->load() > 0.5f;
    o.variance = varianceParam->load() > 0.5f;
    o.analogNoise = noiseParam->load() > 0.5f;
    o.voices = voices12Param->load() > 0.5f ? 12 : 6;
    const auto& cur = synth.options();
    if (o.instant != cur.instant || o.variance != cur.variance || o.analogNoise != cur.analogNoise || o.voices != cur.voices)
        synth.setOptions (o);
    synth.setTune (0x80 + (int) std::lround (tuneParam->load()));
    bool portaOn = portaOnParam->load() > 0.5f;
    int porta = (int) std::lround (portaParam->load()), mode = (int) std::lround (polyParam->load());
    if (const auto f = playForce.load(); (f & 1u) != 0)   // un cambio de programa que a interface aínda non levou aos parámetros
    {
        portaOn = (f & 2u) != 0; mode = (int) ((f >> 2) & 3u); porta = (int) ((f >> 8) & 0xffu);
    }
    synth.setPortamento (porta, portaOn);
    synth.setBendSens ((int) std::lround (bendDcoParam->load()), (int) std::lround (bendVcfParam->load()));
    synth.setLfoSens ((int) std::lround (lfoModParam->load()));
    synth.setMode (mode);
    synth.setTranspose ((int) std::lround (transposeParam->load()));
    synth.setLever ((double) lever.load(), leverPushed.load());
}

void BXProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    setLatencySamples (0);
    applyPerformance();
    synth.prepare (sampleRate);
    lastParamPatch = patchFromParams();
    synth.setPatch (lastParamPatch);
    synth.reset();
    {
        const juce::SpinLock::ScopedLockType lk (bankLock);
        audioBank = sharedBank; audioPlay = sharedPlay; bankDirty = false;
    }
    volS = juce::Decibels::decibelsToGain ((double) volumeParam->load());
    teito.preparar (sampleRate, 0.080, 0.891);
    right.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    keyboardState.reset();
}

// O MIDI vai ao Juno emulado: o pedal é o HOLD da CPU de voces (a voz solta queda no sustain, pero o
// asignador xa a dá por libre e pode collela outra nota) e o reparto é o do asignador. Por iso aquí
// non se usan Sostido e Voces do núcleo: retendo as notas, cambiarían o que soa.
void BXProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn()) synth.noteOn (m.getNoteNumber());
    else if (m.isNoteOff()) synth.noteOff (m.getNoteNumber());
    else if (m.isSustainPedalOn()) synth.sustain (true);
    else if (m.isSustainPedalOff()) synth.sustain (false);
    else if (m.isAllSoundOff()) synth.allSoundOff();
    else if (m.isAllNotesOff()) synth.allNotesOff();
    else if (m.isPitchWheel()) synth.pitchBend14 (m.getPitchWheelValue());
    else if (m.isResetAllControllers()) { synth.modWheel (0); synth.pitchBend14 (8192); synth.sustain (false); }
    else if (m.isControllerOfType (1)) synth.modWheel (m.getControllerValue());
    else if (m.isProgramChange())
    {
        const int p = m.getProgramChangeNumber() & 127;   // como o Juno-106: 128 memorias
        synth.setPatch (audioBank[(size_t) p]);
        if (const auto& play = audioPlay[(size_t) p]; play.set)
        {
            playForce.store (encodePlay (play));
            synth.setPortamento (play.porta, play.portaOn);
            synth.setMode (play.mode);
        }
        pendingProgram.store (p);
    }
    else if (m.isSysEx())
    {
        const auto* d = m.getSysExData();
        const int n = m.getSysExDataSize();
        if (n >= 5 && d[0] == 0x41)
        {
            if ((d[1] == 0x30 || d[1] == 0x31) && n >= 22)
            {
                xuno::Patch p {};
                for (int k = 0; k < xuno::PATCH_SIZE; ++k) p[(size_t) k] = d[4 + k];
                xuno::normalize (p);
                synth.setPatch (p);
            }
            else if (d[1] == 0x32)
            {
                const int c = d[3];
                if (c < 16) synth.setPatchByte (c, d[4]);
                else if (c == 16) synth.setPatchByte (xuno::SW1, d[4]);
                else if (c == 17) synth.setPatchByte (xuno::SW2, d[4]);
            }
            pushSysex (d, n);
        }
    }
}

void BXProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    {
        const juce::SpinLock::ScopedTryLockType lk (bankLock);
        if (lk.isLocked() && bankDirty) { audioBank = sharedBank; audioPlay = sharedPlay; bankDirty = false; }
    }
    applyPerformance();

    // o panel: cada deslizador que se moveu vai á CPU de voces, coma no orixinal
    const auto now = patchFromParams();
    if (now != lastParamPatch)
    {
        for (int i = 0; i < xuno::PATCH_SIZE; ++i)
            if (now[(size_t) i] != lastParamPatch[(size_t) i]) synth.setPatchByte (i, now[(size_t) i]);
        lastParamPatch = now;
    }

    if ((int) right.size() < n) right.resize ((size_t) n);   // por se o DAW manda máis do anunciado
    float* L = buffer.getWritePointer (0);
    float* R = right.data();
    int pos = 0;
    bool anyMidi = false;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, n, meta.samplePosition);
        if (t > pos) { synth.render (L + pos, R + pos, t - pos); pos = t; }
        handleMidi (meta.getMessage());
        anyMidi = true;
    }
    if (pos < n) synth.render (L + pos, R + pos, n - pos);
    if (anyMidi) midiActivity.fetch_add (1);

    // volume (suavizado) e o limitador de seguridade do núcleo: teito a −1 dBFS, ataque
    // instantáneo, solta en 80 ms e sen anticipación (non engade latencia)
    const double volT = juce::Decibels::decibelsToGain ((double) volumeParam->load());
    const double sm = 1.0 - std::exp (-1.0 / (0.02 * sr));
    const bool stereo = buffer.getNumChannels() > 1;
    float* outR = stereo ? buffer.getWritePointer (1) : nullptr;
    float peak = 0;
    for (int i = 0; i < n; ++i)
    {
        volS += (volT - volS) * sm;
        double l = (double) L[i] * volS, r = (double) R[i] * volS;
        if (stereo) teito.procesar (l, r);
        else l = teito.procesar (0.5 * (l + r));
        L[i] = (float) l;
        if (outR != nullptr) outR[i] = (float) r;
        peak = std::max ({ peak, std::abs ((float) l), std::abs ((float) r) });
    }
    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);

    activeVoices.store (synth.activeVoices());
    for (int v = 0; v < xuno::MAXV; ++v)
    {
        voiceLevel[(size_t) v].store (synth.voiceLevel (v));
        voiceGate[(size_t) v].store (synth.voiceGate (v));
        voiceNote[(size_t) v].store (synth.voiceNote (v));
    }
    outLevel.store (peak);
    lfoValue.store (synth.lfo());
}

// ================================================================ estado
// a memoria enteira: a fonte, o son, MANUAL, o chorus lembrado, a memoria interna e o último .syx
// aberto. A fonte gárdase pola súa pegada: os bancos da carpeta poden cambiar de orde.
juce::ValueTree BXProcessor::memoria() const
{
    juce::ValueTree m ("Memoria");
    auto b64 = [] (const std::vector<uint8_t>& d) { return juce::Base64::toBase64 (d.data(), d.size()); };
    m.setProperty ("source", source == INTERNAL ? juce::String ("int") : source == FILE_BANK ? juce::String ("file")
                             : juce::String (std::to_string (banks[(size_t) source].hash)), nullptr);
    m.setProperty ("patch", patchNum, nullptr);
    m.setProperty ("manual", manual, nullptr);
    m.setProperty ("chorusLevel", (int) chorusLevelBit, nullptr);
    m.setProperty ("internal", b64 (xuno::bankToSysex (std::vector<xuno::Patch> (internal.begin(), internal.end()))), nullptr);
    std::vector<uint8_t> play;   // dous bytes por son: (hai, porta aceso, asignación) e o tempo
    for (const auto& p : internalPlay) { play.push_back ((uint8_t) ((p.set ? 1 : 0) | (p.portaOn ? 2 : 0) | (p.mode << 2))); play.push_back ((uint8_t) p.porta); }
    m.setProperty ("play", b64 (play), nullptr);
    if (fileBankLoaded)
    {
        m.setProperty ("file", b64 (xuno::bankToSysex (std::vector<xuno::Patch> (fileBank.begin(), fileBank.end()))), nullptr);
        m.setProperty ("fileName", fileBankName, nullptr);
    }
    return m;
}

void BXProcessor::setMemoria (const juce::ValueTree& m)
{
    if (! m.isValid()) return;
    auto unb64 = [] (const juce::String& s)
    {
        juce::MemoryOutputStream out;
        if (! juce::Base64::convertFromBase64 (out, s)) return std::vector<uint8_t>();
        const auto* p = static_cast<const uint8_t*> (out.getData());
        return std::vector<uint8_t> (p, p + out.getDataSize());
    };
    const auto in = xuno::sysexToBank (xuno::parseSysex (unb64 (m.getProperty ("internal").toString())));
    if (in.size() >= 128) for (size_t k = 0; k < 128; ++k) internal[k] = in[k];
    const auto play = unb64 (m.getProperty ("play").toString());
    for (size_t k = 0; k < 128; ++k)
        internalPlay[k] = play.size() == 256 ? Play { (play[2 * k] & 1) != 0, (play[2 * k] & 2) != 0, play[2 * k + 1], juce::jlimit (0, 2, play[2 * k] >> 2) } : Play {};
    fileBankLoaded = false;
    if (m.hasProperty ("file"))
    {
        const auto fb = xuno::sysexToBank (xuno::parseSysex (unb64 (m.getProperty ("file").toString())));
        if (fb.size() >= 2)
        {
            for (size_t k = 0; k < 128; ++k) fileBank[k] = k < fb.size() ? fb[k] : xuno::initPatch();
            fileBankLoaded = true;
            fileBankName = m.getProperty ("fileName").toString().substring (0, 10);
        }
    }
    const auto key = m.getProperty ("source").toString();
    if (key == "int") source = INTERNAL;
    else if (key == "file") source = fileBankLoaded ? FILE_BANK : INTERNAL;
    else source = juce::jmax (0, sourceForHash ((uint64_t) std::strtoull (key.toRawUTF8(), nullptr, 10)));
    patchNum = juce::jlimit (0, 127, (int) m.getProperty ("patch", 0));
    manual = (bool) m.getProperty ("manual", false);
    chorusLevelBit = (uint8_t) ((int) m.getProperty ("chorusLevel", 0x40) & 0x40);
    publishBank();
}

escoita::Estado::Opcions BXProcessor::opcionsEstado()
{
    escoita::Estado::Opcions o;
    o.extension = "bxpreset";
    o.extra = [this] { return memoria(); };
    o.aplicarExtra = [this] (const juce::ValueTree& m)
    {
        setMemoria (m);
        updateHostDisplay (ChangeDetails().withProgramChanged (true));
    };
    return o;
}

// gárdanse o antes e o despois; «facer» non se repite a primeira vez (o cambio xa está feito), si ao refacer
void BXProcessor::cambiarMemoria (const juce::String& nome, const std::function<void()>& f)
{
    const auto antes = memoria();
    f();
    const auto despois = memoria();
    if (antes.isEquivalentTo (despois)) return;
    auto primeira = std::make_shared<bool> (true);
    estado.cambio (nome, [this, despois, primeira] { if (*primeira) *primeira = false; else { setMemoria (despois); updateHostDisplay (ChangeDetails().withProgramChanged (true)); } },
                         [this, antes] { setMemoria (antes); updateHostDisplay (ChangeDetails().withProgramChanged (true)); });
}

void BXProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto st = apvts.copyState();
    st.removeChild (st.getChildWithName ("Memoria"), nullptr);
    st.appendChild (memoria(), nullptr);
    st.setProperty ("uiScale", uiScale, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, destData);
}

void BXProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return;
    auto tree = juce::ValueTree::fromXml (*xml);
    uiScale = juce::jlimit (0.5f, 2.0f, (float) tree.getProperty ("uiScale", 1.0f));
    const auto m = tree.getChildWithName ("Memoria");
    setMemoria (m);
    tree.removeChild (m, nullptr);
    apvts.replaceState (tree);   // os parámetros son o panel (e o desfacer empeza de novo)
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

juce::AudioProcessorEditor* BXProcessor::createEditor() { return new BXEditor (*this); }

#ifndef BARULLO_NO_PLUGIN_ENTRY
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BXProcessor(); }
#endif
