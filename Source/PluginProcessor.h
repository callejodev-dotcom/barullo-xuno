#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <escoita_nucleo/escoita_nucleo.h>
#include "xuno/Synth.h"

// Como no Juno-106: o panel é o son (os deslizadores son parámetros do DAW), unha memoria de 128 sons
// (grupos A e B, bancos 1–8, números 1–8) que se pode gravar, o banco propio e os bancos .syx dunha
// carpeta. O son que se toca é sempre o do panel; escoller unha memoria move o panel a ela.
class BXProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    static constexpr int INTERNAL = -1;    // fontes: −1 a memoria, −2 o último .syx aberto,
    static constexpr int FILE_BANK = -2;   // 0..numBanks()−1 os bancos (o propio e os da carpeta)

    BXProcessor();
    ~BXProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Barullo Xuno"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override { return 128; }
    int getCurrentProgram() override { return patchNum; }
    void setCurrentProgram (int index) override { selectPatch (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    // identificadores dos 16 deslizadores (na orde do SysEx)
    static const char* const sliderIds[16];

    // ---- memoria (só desde o fío da interface) ----
    int getSource() const { return source; }
    int getPatchNum() const { return patchNum; }
    bool isManual() const { return manual; }
    const xuno::Patch& patchAt (int src, int i) const;
    juce::String sourceName (int src) const;      // «MEMORIA», «XUNO», …
    juce::String sourceTitle (int src) const;
    juce::String patchName (int src, int i) const;   // nome (os propios e os de fábrica coñecidos)
    juce::String patchDesc (int src, int i) const;   // descrición (só os propios)
    bool hasFileBank() const { return fileBankLoaded; }
    int numBanks() const { return (int) banks.size(); }
    int numFolderBanks() const { return (int) banks.size() - 1; }
    static juce::String patchLabel (int i);          // «A11» … «B88»

    juce::File getBankFolder() const { return bankFolder; }
    void setBankFolder (const juce::File&);
    void rescanBankFolder();

    void selectSource (int src);                  // cambia de banco e carga o son co mesmo número
    void selectPatch (int i);                     // move o panel a un son
    void setManual();                             // MANUAL: o panel queda solto, sen memoria
    xuno::Patch panelPatch() const;               // o son do panel (os parámetros)
    void setPanel (const xuno::Patch&);           // move os parámetros a un son
    bool isEdited() const;
    void writeTo (int slot);                      // WRITE: garda o panel na memoria
    void copySourceToInternal();
    void initPanel();

    bool loadSyxFile (const juce::File&, juce::String& message);
    bool saveBankFile (const juce::File&, int src) const;
    bool savePatchFile (const juce::File&) const;

    // cambia cada vez que muda a memoria ou a selección (a interface mira isto)
    std::atomic<uint32_t> changes { 1 };
    juce::String notice; juce::uint32 noticeTime = 0;

    // ---- a panca (desde a interface; o fío de audio cóllea en cada bloque)
    std::atomic<float> lever { 0.0f };
    std::atomic<bool> leverPushed { false };

    // ---- para a pantalla (escríbese desde o fío de audio) ----
    std::atomic<int> activeVoices { 0 };
    std::array<std::atomic<float>, xuno::MAXV> voiceLevel {};
    std::array<std::atomic<bool>, xuno::MAXV> voiceGate {};
    std::atomic<float> outLevel { 0 }, lfoValue { 0 };
    std::atomic<uint32_t> midiActivity { 0 };

    float uiScale = 1.0f;
    std::array<std::atomic<int>, xuno::MAXV> voiceNote {};
    void serviceMessages() { timerCallback(); }   // o mesmo que fai o temporizador (para as probas)

private:
    xuno::Synth synth;

    struct Bank
    {
        juce::String name, title;
        uint64_t hash = 0;
        bool own = false;              // o banco propio (ten nomes e descricións)
        std::array<xuno::Patch, 128> patches {};
        int size = 128;
    };
    std::vector<Bank> banks;           // primeiro o propio, despois os da carpeta
    juce::File bankFolder;
    std::unique_ptr<juce::PropertiesFile> settings;
    int sourceForHash (uint64_t h) const;
    std::array<xuno::Patch, 128> internal {}, fileBank {};
    juce::String fileBankName;
    bool fileBankLoaded = false;
    int source = 0, patchNum = 0;
    bool manual = false;
    std::atomic<int> chorusLevelBit { 0x40 };   // co chorus apagado lémbrase se era I ou II (lese no fío de audio)

    std::array<xuno::Patch, 128>& bankOf (int src);
    const std::array<xuno::Patch, 128>& bankOf (int src) const;

    // fío de audio
    xuno::Patch lastParamPatch {};
    std::array<xuno::Patch, 128> audioBank {};
    juce::SpinLock bankLock;
    std::array<xuno::Patch, 128> sharedBank {};
    bool bankDirty = false;
    void publishBank();

    // do fío de audio á interface
    std::atomic<int> pendingProgram { -1 };
    juce::AbstractFifo sysexFifo { 1 << 14 };
    std::vector<uint8_t> sysexBuf = std::vector<uint8_t> (1 << 14);
    void pushSysex (const uint8_t* data, int size);
    void timerCallback() override;
    void handleSysex (const std::vector<uint8_t>&);
    void setParamValue (const char* id, float value);

    std::array<std::atomic<float>*, 16> sliderParams {};
    std::atomic<float>* rangeParam = nullptr;
    std::atomic<float>* pulseParam = nullptr;
    std::atomic<float>* sawParam = nullptr;
    std::atomic<float>* chorusParam = nullptr;
    std::atomic<float>* pwmParam = nullptr;
    std::atomic<float>* envPolParam = nullptr;
    std::atomic<float>* vcaParam = nullptr;
    std::atomic<float>* hpfParam = nullptr;
    std::atomic<float>* volumeParam = nullptr;
    std::atomic<float>* tuneParam = nullptr;
    std::atomic<float>* portaParam = nullptr;
    std::atomic<float>* portaOnParam = nullptr;
    std::atomic<float>* bendDcoParam = nullptr;
    std::atomic<float>* bendVcfParam = nullptr;
    std::atomic<float>* lfoModParam = nullptr;
    std::atomic<float>* polyParam = nullptr;
    std::atomic<float>* transposeParam = nullptr;
    std::atomic<float>* instantParam = nullptr;
    std::atomic<float>* varianceParam = nullptr;
    std::atomic<float>* noiseParam = nullptr;
    std::atomic<float>* voices12Param = nullptr;

    xuno::Patch patchFromParams() const;
    void applyPerformance();

    double sr = 48000, volS = 1;
    std::vector<float> right;

    void handleMidi (const juce::MidiMessage&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BXProcessor)
};
