#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace ui
{
    // A familia Barullo: o azul mariño da noite de Vigo, a crema da letra de Escoita e a luz laranxa,
    // nun aparello de estudio (paneis en relevo, pantallas de cristal, visores dourados). Por dentro,
    // a orde do panel dun polisintetizador de 1984: LFO, DCO, HPF, VCF, VCA, ENV e CHORUS.
    namespace col
    {
        const juce::Colour noite    { 0xff0c111f };
        const juce::Colour crema    { 0xfffff7c1 };
        const juce::Colour crema2   { 0xffbdb792 };
        const juce::Colour crema3   { 0xff857f67 };
        const juce::Colour laranxa  { 0xffe08b16 };
        const juce::Colour ouro     { 0xffedba23 };
        const juce::Colour verde    { 0xff9dc6a0 };
        const juce::Colour panel1   { 0xff1c2540 }, panel2 { 0xff111830 };
        const juce::Colour tecla1   { 0xff27314f }, tecla2 { 0xff1a2239 };
        const juce::Colour visor    { 0xff070b16 };
        const juce::Colour cristal1 { 0xff142046 }, cristal2 { 0xff070c1c };
        const juce::Colour lina     { 0xff2a3350 };
    }

    juce::Font body (float size);    // Inter
    juce::Font semi (float size);    // Montserrat SemiBold
    juce::Font cond (float size);    // Montserrat Bold
    juce::Font caps (float size, float tracking = 0.18f);
    juce::String upper (const juce::String&);
    void drawCaps (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Justification, float size, float tracking = 0.18f);
    void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Justification, juce::Colour);

    void paintPanel (juce::Graphics&, juce::Rectangle<float>, float radius);
    void paintWell (juce::Graphics&, juce::Rectangle<float>, float radius);
    void paintGlass (juce::Graphics&, juce::Rectangle<float>, float radius);
    void paintGlassShine (juce::Graphics&, juce::Rectangle<float>, float radius);
    void paintKey (juce::Graphics&, juce::Rectangle<float>, bool over, bool down);
    void paintLed (juce::Graphics&, juce::Point<float>, bool on);

    struct Look : juce::LookAndFeel_V4
    {
        Look();
        juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
        void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
        juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
        juce::Font getPopupMenuFont() override { return body (15.0f); }
        void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    };

    // un deslizador do panel: a pista afundida co LED laranxa ata o capuchón, o valor nun visor e o nome
    struct Fader : juce::Component
    {
        Fader();
        void resized() override;
        void paint (juce::Graphics&) override;
        juce::Slider slider { juce::Slider::LinearVertical, juce::Slider::NoTextBox };
        juce::String name;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
        static constexpr int readoutH = 20, labelH = 16;
    };

    // mando co anel de LED e o seu valor debaixo (os do lado do teclado)
    struct Knob : juce::Component
    {
        Knob();
        void resized() override { slider.setBounds (getLocalBounds().withTrimmedBottom (15)); }
        void paint (juce::Graphics&) override;
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        juce::String name;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    // a panca: aos lados é o bend (volve ao centro ao soltala), cara arriba é o LFO
    struct Lever : juce::Component, juce::SettableTooltipClient
    {
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        float x = 0.0f; bool pushed = false;
        std::function<void (float, bool)> onMove;
    };

    // teclado de cinco oitavas, coas teclas da familia
    struct Keys : juce::MidiKeyboardComponent
    {
        using MidiKeyboardComponent::MidiKeyboardComponent;
        void drawWhiteNote (int note, juce::Graphics&, juce::Rectangle<float>, bool down, bool over, juce::Colour, juce::Colour) override;
        void drawBlackNote (int note, juce::Graphics&, juce::Rectangle<float>, bool down, bool over, juce::Colour) override;
    };

    // a fila de pantallas: voces, o son, a onda do DCO, a envolvente e o LFO
    class Screens : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        explicit Screens (BXProcessor& p) : proc (p) {}
        void paint (juce::Graphics&) override;
        void resized() override;
        void showParam (const juce::String& name, const juce::String& value);
        void setMessage (const juce::String& m);
        void tick();
        juce::Rectangle<float> voicesBox, mainBox, waveBox, envBox, lfoBox;
    private:
        void paintVoices (juce::Graphics&, juce::Rectangle<float>);
        void paintMain (juce::Graphics&, juce::Rectangle<float>);
        void paintWave (juce::Graphics&, juce::Rectangle<float>);
        void paintEnv (juce::Graphics&, juce::Rectangle<float>);
        void paintLfo (juce::Graphics&, juce::Rectangle<float>);
        BXProcessor& proc;
        juce::String paramName, paramValue, message;
        juce::uint32 paramTime = 0, messageTime = 0;
        std::array<float, xuno::MAXV> levels {};
        float lfoShown = 0.0f;
    };

    class MainView;

    // a táboa dos sons: os 64 do grupo en 8 bancos × 8 números
    class Chart : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        Chart (BXProcessor&, MainView&);
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
        void refresh();
        int group = 0;
    private:
        int cellAt (juce::Point<int>) const;
        juce::Rectangle<float> cell (int i) const;
        BXProcessor& proc;
        MainView& main;
        juce::TextButton groupA { "Grupo A" }, groupB { "Grupo B" }, close { "Pechar" };
        juce::Rectangle<int> grid;
        int hover = -1;
    };

    class MainView : public juce::Component, private juce::Timer
    {
    public:
        static constexpr int W = 1300, H = 900;
        explicit MainView (BXProcessor&);
        ~MainView() override;
        void paint (juce::Graphics&) override;
        void resized() override;
        void showChart (bool);
        bool chartShown() const { return chart.isVisible(); }
        void showParam (const juce::String& name, const juce::String& value) { screens.showParam (name, value); }
        void showMessage (const juce::String& m) { screens.setMessage (m); }
        void chooseBank();
        void timerCallback() override;

    private:
        void openSyx();
        void saveMenu();
        void functionMenu();
        void transposeMenu();
        void chooseFolder();
        void pressPatch (int number);
        void pressBank (int bank);
        void pressGroup (int group);
        void refreshKeys();

        BXProcessor& proc;
        Look look;
        juce::TooltipWindow tooltips { this, 500 };

        juce::OwnedArray<Fader> faders;
        Fader* addFader (const char* id, const juce::String& name, const juce::String& tip, std::function<juce::String (int)> fmt);
        std::array<juce::Rectangle<int>, 7> sections;

        // teclas do panel (cada unha atada ao seu parámetro)
        juce::TextButton range16 { "16'" }, range8 { "8'" }, range4 { "4'" };
        juce::TextButton pwmLfo { "LFO" }, pwmMan { "Man" };
        juce::TextButton pulseBtn { "Pulso" }, sawBtn { "Serra" };
        juce::TextButton envPos { "+" }, envNeg { escoita::U ("−") };
        juce::TextButton vcaEnv { "Env" }, vcaGate { "Gate" };
        juce::TextButton chorusOff { "Off" }, chorusI { "I" }, chorusII { "II" };
        std::unique_ptr<juce::ParameterAttachment> rangeAtt, pulseAtt, sawAtt, chorusAtt, pwmAtt, envAtt, vcaAtt, polyAtt, transpAtt, portaOnAtt;

        // cabeceira
        juce::TextButton bankBtn, chartBtn { "Sons" }, openBtn { "Abrir" }, saveBtn { "Gardar" };
        juce::TextButton poly1 { "Poly 1" }, poly2 { "Poly 2" }, unison { escoita::U ("Unísono") };
        juce::TextButton transpBtn, funcBtn { escoita::U ("Función") };

        // fila dos sons
        juce::TextButton groupBtn[2];
        juce::OwnedArray<juce::TextButton> bankBtns, patchBtns;
        juce::TextButton manualBtn { "Manual" }, writeBtn { "Write" };
        bool writeArmed = false;

        // á esquerda do teclado
        Knob volume, tune, porta, bendDco, bendVcf, lfoMod;
        juce::TextButton portaOn { "Porta" };
        Lever lever;

        juce::TextButton newVersion;
        Screens screens;
        Keys keyboard;
        Chart chart;

        juce::Rectangle<int> headerArea, screensArea, controlsArea, patchArea, ctrlArea, keysArea;
        uint32_t seenChanges = 0;
        int slowTick = 0;
        std::unique_ptr<juce::FileChooser> chooser;

        juce::SharedResourcePointer<escoita::Actualizacions> versions;
    };
}

class BXEditor : public juce::AudioProcessorEditor
{
public:
    explicit BXEditor (BXProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (ui::col::noite); }
    ui::MainView& view() { return main; }
private:
    BXProcessor& proc;
    ui::MainView main;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BXEditor)
};
