#include "PluginEditor.h"
#include "BinaryData.h"
#include "xuno/Rom.h"

namespace
{
    using escoita::U;
    using escoita::numero;

    constexpr double LOOP_MS = 4.2335;   // o bucle da CPU de voces

    juce::String tempo (double ms)
    {
        if (ms < 995.0) return juce::String (juce::roundToInt (ms)) + " ms";
        return numero (ms / 1000.0, ms < 9950.0 ? 1 : 0) + " s";
    }
    juce::String percent (int v) { return juce::String (juce::roundToInt (v * 100.0 / 127.0)) + " %"; }

    // o que significa cada deslizador, coas táboas da ROM
    juce::String lfoRateText (int v)
    {
        const int half = (8192 + xuno::rom::LFO_SPEED[v] - 1) / xuno::rom::LFO_SPEED[v];
        const double hz = 1000.0 / (4.0 * half * LOOP_MS);
        return numero (hz, hz < 1.0 ? 2 : 1) + " Hz";
    }
    juce::String lfoDelayText (int v)
    {
        const int inc = xuno::rom::ENV_ATTACK[v], ramp = xuno::rom::LFO_DELAY_RAMP[(v >> 4) & 7];
        const int ticks = (0x4000 + inc - 1) / inc + (ramp >= 0xffff ? 0 : (65536 + ramp - 1) / ramp);
        return v == 0 ? juce::String ("0 ms") : tempo (ticks * LOOP_MS);
    }
    juce::String dcoLfoText (int v) { return U ("±") + juce::String (juce::roundToInt (xuno::rom::LFO_DEPTH[v] / 255.0 * 398.0)) + " c"; }
    juce::String pwmText (int v)
    {
        const double pw = 16383.0 - (2.0 * v) * 16383.0 / 256.0;
        return juce::String (juce::roundToInt ((1.0 - xuno::pulseThreshold ((uint16_t) pw)) * 100.0)) + " %";
    }
    juce::String vcfFreqText (int v)
    {
        const double hz = xuno::vcfHz ((double) ((v << 7) >> 2));
        return hz < 1000.0 ? juce::String (juce::roundToInt (hz)) + " Hz" : numero (hz / 1000.0, hz < 10000.0 ? 2 : 1) + " kHz";
    }
    juce::String octText (double o, bool both) { return (both ? U ("±") : juce::String ("+")) + numero (o, 1) + " oit"; }
    juce::String levelText (int v) { const double db = -10.0 + 20.0 * (double) (v << 7) / 16256.0; return (db >= 0.05 ? "+" : "") + numero (db, 1) + " dB"; }
    juce::String attackText (int v) { const int inc = xuno::rom::ENV_ATTACK[v]; return tempo ((double) ((0x3fff + inc - 1) / inc) * LOOP_MS); }
    juce::String decayText (int v)
    {
        const double c = xuno::rom::ENV_DECAY[v] / 65536.0;
        return tempo (std::ceil (std::log (0.1) / std::log (c)) * LOOP_MS);   // ata −20 dB
    }
    juce::String noteName (int midi)
    {
        static const char* n[12] = { "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" };
        return juce::String (n[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 2);
    }
}

namespace ui
{
// ================================================================ tipografía
// Montserrat (títulos e rótulos, en maiúsculas espaciadas coma na web de Escoita) e Inter
// (texto corrido): as da familia Barullo. As dúas, licenza OFL.
static juce::Typeface::Ptr tf (int which)
{
    static juce::Typeface::Ptr t[3] = {
        juce::Typeface::createSystemTypefaceFor (BinaryData::InterMedium_ttf, BinaryData::InterMedium_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize) };
    return t[which];
}
juce::Font body (float s) { return juce::Font (juce::FontOptions (tf (0)).withHeight (s)); }
juce::Font semi (float s) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s)); }
juce::Font cond (float s) { return juce::Font (juce::FontOptions (tf (2)).withHeight (s)); }
juce::Font caps (float s, float tracking) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s).withKerningFactor (tracking)); }
juce::String upper (const juce::String& t) { return escoita::maiusculas (t); }

void drawCaps (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, float size, float tracking)
{
    g.setFont (caps (size, tracking));
    g.drawText (upper (t), r, j, false);
}

void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, juce::Colour c)
{
    g.setColour (c.withAlpha (0.16f));
    for (const auto d : { juce::Point<float> (-0.8f, 0.0f), { 0.8f, 0.0f }, { 0.0f, -0.8f }, { 0.0f, 0.8f } })
        g.drawText (t, r.translated (d.x, d.y), j, false);
    g.setColour (c);
    g.drawText (t, r, j, false);
}

// ================================================================ pezas do aparello
void paintPanel (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.11f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f * (float) i).expanded ((float) i), rad + (float) i);
    }
    g.setGradientFill (juce::ColourGradient (col::panel1, 0.0f, r.getY(), col::panel2, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRect (r.getX() + rad, r.getY() + 0.5f, r.getWidth() - 2.0f * rad, 1.0f);
}

void paintWell (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), rad);
    g.setColour (col::visor.brighter (0.06f));
    g.fillRoundedRectangle (r, rad);
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState();
    g.reduceClipRegion (p);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.65f), 0.0f, r.getY(), juce::Colours::transparentBlack, 0.0f, r.getY() + 5.0f, false));
    g.fillRect (r.withHeight (6.0f));
    g.restoreState();
}

void paintGlass (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), rad);
    juce::ColourGradient gr (col::cristal1, r.getCentreX(), r.getY(), col::cristal2, r.getCentreX(), r.getBottom(), false);
    gr.addColour (0.55, juce::Colour (0xff0a1129));
    g.setGradientFill (gr);
    g.fillRoundedRectangle (r, rad);
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState();
    g.reduceClipRegion (p);
    for (int i = 0; i < 5; ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.16f - 0.03f * (float) i));
        g.drawRoundedRectangle (r.reduced ((float) i + 0.5f), rad, 1.0f);
    }
    g.restoreState();
}

void paintGlassShine (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState();
    g.reduceClipRegion (p);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.06f), r.getX(), r.getY(),
                                             juce::Colours::transparentWhite, r.getX() + r.getWidth() * 0.32f, r.getY() + r.getHeight() * 0.55f, false));
    g.fillRect (r);
    g.restoreState();
}

void paintKey (juce::Graphics& g, juce::Rectangle<float> r, bool over, bool down)
{
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 7.0f);
    if (down) r.translate (0.0f, 1.0f);
    const float lift = over && ! down ? 0.10f : 0.0f;
    g.setGradientFill (juce::ColourGradient (col::tecla1.brighter (lift), 0.0f, r.getY(), col::tecla2.brighter (lift * 0.6f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (juce::Colours::white.withAlpha (down ? 0.04f : 0.10f));
    g.fillRect (r.getX() + 6.0f, r.getY() + 0.5f, r.getWidth() - 12.0f, 1.0f);
}

void paintLed (juce::Graphics& g, juce::Point<float> c, bool on)
{
    if (on)
    {
        g.setColour (col::laranxa.withAlpha (0.16f)); g.fillEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f);
        g.setColour (col::laranxa.withAlpha (0.34f)); g.fillEllipse (c.x - 4.6f, c.y - 4.6f, 9.2f, 9.2f);
        g.setColour (col::laranxa); g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f)); g.fillEllipse (c.x - 1.3f, c.y - 2.0f, 2.2f, 2.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff3a3326)); g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f, 0.8f);
    }
}

// rótulo dun grupo: laranxa e unha liña debaixo
static void groupTitle (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t)
{
    g.setColour (col::laranxa);
    drawCaps (g, t, r.withHeight (14.0f), juce::Justification::centred, 9.0f, 0.28f);
    g.setColour (col::laranxa.withAlpha (0.25f));
    g.fillRect (r.getX() + 8.0f, r.getY() + 18.0f, r.getWidth() - 16.0f, 1.0f);
}

// ================================================================ aspecto
Look::Look()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::noite);
    setColour (juce::Label::textColourId, col::crema);
    setColour (juce::TextEditor::backgroundColourId, col::visor);
    setColour (juce::TextEditor::textColourId, col::ouro);
    setColour (juce::TextEditor::outlineColourId, col::lina);
    setColour (juce::TextEditor::focusedOutlineColourId, col::laranxa);
    setColour (juce::PopupMenu::backgroundColourId, col::panel2);
    setColour (juce::PopupMenu::textColourId, col::crema);
    setColour (juce::PopupMenu::headerTextColourId, col::laranxa);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::laranxa);
    setColour (juce::PopupMenu::highlightedTextColourId, col::noite);
    setColour (juce::TextButton::buttonColourId, col::tecla2);
    setColour (juce::TextButton::textColourOffId, col::crema2);
    setColour (juce::TextButton::textColourOnId, col::crema);
    setColour (juce::AlertWindow::backgroundColourId, col::panel2);
    setColour (juce::AlertWindow::textColourId, col::crema);
    setColour (juce::AlertWindow::outlineColourId, col::lina);
}

juce::Typeface::Ptr Look::getTypefaceForFont (const juce::Font& f) { return tf (f.isBold() ? 1 : 0); }

// as teclas: en relevo, cun LED (aceso se está escollida) e o texto en maiúsculas espaciadas;
// «gold» = un visor dourado (o selector do banco)
void Look::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (1.5f);
    if (b.getProperties().contains ("gold"))
    {
        paintWell (g, r, 8.0f);
        if (over) { g.setColour (col::laranxa.withAlpha (0.08f)); g.fillRoundedRectangle (r, 8.0f); }
        return;
    }
    paintKey (g, r, over, down);
    if (down) r.translate (0.0f, 1.0f);
    if (! b.getProperties().contains ("noLed"))
        paintLed (g, { r.getX() + 12.0f, r.getCentreY() }, b.getToggleState());
}

void Look::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (1.5f);
    if (b.getProperties().contains ("gold"))
    {
        g.setFont (caps (12.0f, 0.1f));
        glowText (g, upper (b.getButtonText()), r.reduced (14.0f, 0.0f), juce::Justification::centredLeft, col::ouro);
        juce::Path arrow;
        const float ax = r.getRight() - 18.0f, ay = r.getCentreY();
        arrow.addTriangle (ax - 5.0f, ay - 2.5f, ax + 5.0f, ay - 2.5f, ax, ay + 3.5f);
        g.setColour (col::ouro); g.fillPath (arrow);
        return;
    }
    if (down) r.translate (0.0f, 1.0f);
    if (! b.getProperties().contains ("noLed")) r.removeFromLeft (14.0f);
    juce::Colour c = b.getToggleState() || over ? col::crema : col::crema2;
    if (! b.isEnabled()) c = col::crema3.withAlpha (0.6f);
    g.setColour (c);
    g.setFont (caps ((float) b.getProperties().getWithDefault ("fontSize", 9.5f), 0.15f));
    g.drawFittedText (upper (b.getButtonText()), r.toNearestInt().reduced (3, 0), juce::Justification::centred, 1, 0.7f);
}

// os mandos: disco mecanizado, anel de LED laranxa e o valor nun visor debaixo
void Look::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    auto area = juce::Rectangle<int> (x, y, w, h).toFloat();
    auto lcd = area.removeFromBottom (18.0f);
    lcd = lcd.withSizeKeepingCentre (juce::jmin (70.0f, area.getWidth() - 2.0f), 18.0f);
    area.removeFromBottom (4.0f);
    const float d = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.withSizeKeepingCentre (d, d).getCentre();
    const float ring = d * 0.5f - 4.0f;

    juce::Path track; track.addCentredArc (c.x, c.y, ring, ring, 0.0f, a0, a1, true);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const bool bip = s.getMinimum() < 0 && s.getMaximum() > 0;
    const double zeroAt = bip ? 0.0 : s.getMinimum();
    const float zero = a0 + (float) s.valueToProportionOfLength (zeroAt) * (a1 - a0);
    const float ang = a0 + pos * (a1 - a0);
    if (std::abs (ang - zero) > 0.01f)
    {
        juce::Path val; val.addCentredArc (c.x, c.y, ring, ring, 0.0f, juce::jmin (zero, ang), juce::jmax (zero, ang), true);
        const auto lit = s.isEnabled() ? col::laranxa : col::crema3;
        g.setColour (lit.withAlpha (0.12f)); g.strokePath (val, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (lit.withAlpha (0.26f)); g.strokePath (val, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (lit); g.strokePath (val, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float kr = ring - 8.0f;
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - kr, c.y - kr + 2.5f, kr * 2.0f, kr * 2.0f);
    juce::ColourGradient disc (juce::Colour (0xff4a5577), c.x - kr * 0.3f, c.y - kr * 0.45f, juce::Colour (0xff0e1323), c.x + kr * 0.55f, c.y + kr * 1.05f, true);
    disc.addColour (0.55, juce::Colour (0xff232b45));
    g.setGradientFill (disc);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);
    const juce::Point<float> tip (c.x + kr * 0.82f * std::sin (ang), c.y - kr * 0.82f * std::cos (ang));
    g.setColour (col::crema.withAlpha (0.92f));
    g.drawLine ({ c, tip }, 2.2f);

    paintWell (g, lcd, 5.0f);
    g.setFont (caps (9.5f, 0.06f));
    glowText (g, upper (s.getTextFromValue (s.getValue())), lcd, juce::Justification::centred, col::ouro);
}

// os deslizadores: a pista afundida, a luz laranxa desde abaixo ata o capuchón e o capuchón mecanizado
void Look::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    const float cx = (float) x + (float) w * 0.5f;
    const float top = (float) y, bot = (float) (y + h);
    auto slot = juce::Rectangle<float> (cx - 5.0f, top - 8.0f, 10.0f, bot - top + 16.0f);
    paintWell (g, slot, 5.0f);

    // marcas a cada lado (10 pasos; o HPF, as súas catro posicións)
    const int steps = s.getMaximum() <= 3.5 ? 3 : 10;
    g.setColour (col::crema3.withAlpha (0.55f));
    for (int t = 0; t <= steps; ++t)
    {
        const float ty = bot - (bot - top) * (float) t / (float) steps;
        const float len = (t % 5 == 0 || steps == 3) ? 6.0f : 3.0f;
        g.fillRect (cx - 10.0f - len, ty - 0.5f, len, 1.0f);
        g.fillRect (cx + 10.0f, ty - 0.5f, len, 1.0f);
    }

    // a luz
    if (pos < bot - 0.5f)
    {
        auto lit = juce::Rectangle<float> (cx - 1.5f, pos, 3.0f, bot + 4.0f - pos);
        g.setColour (col::laranxa.withAlpha (0.14f)); g.fillRoundedRectangle (lit.expanded (3.0f, 0.0f), 3.0f);
        g.setColour (col::laranxa.withAlpha (0.30f)); g.fillRoundedRectangle (lit.expanded (1.5f, 0.0f), 2.0f);
        g.setColour (col::laranxa); g.fillRoundedRectangle (lit, 1.5f);
    }

    // o capuchón
    auto cap = juce::Rectangle<float> (cx - 15.0f, pos - 8.0f, 30.0f, 16.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (cap.translated (0.0f, 2.5f), 4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a5577), cap.getX(), cap.getY(), juce::Colour (0xff141a2e), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.drawRoundedRectangle (cap.reduced (0.5f), 4.0f, 1.0f);
    g.setColour (col::crema); g.fillRect (cap.getX() + 4.0f, pos - 0.8f, cap.getWidth() - 8.0f, 1.6f);
}

void Look::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    auto r = juce::Rectangle<int> (w, h).toFloat();
    g.setColour (col::panel2); g.fillRoundedRectangle (r, 6.0f);
    g.setColour (col::lina); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    juce::AttributedString s; s.setJustification (juce::Justification::topLeft);
    s.append (text, body (13.5f), col::crema);
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    tl.draw (g, r.reduced (10.0f, 7.0f));
}

juce::Rectangle<int> Look::getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    juce::AttributedString s; s.append (text, body (13.5f));
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    const int w = (int) tl.getWidth() + 22, h = (int) tl.getHeight() + 16;
    return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 12,
                                 pos.y > parent.getCentreY() ? pos.y - (h + 8) : pos.y + 18, w, h).constrainedWithin (parent);
}

void Look::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::panel2);
    g.setColour (col::lina); g.drawRect (0, 0, w, h);
}

// ================================================================ deslizador, mando e panca
Fader::Fader()
{
    slider.setSliderSnapsToMousePosition (false);
    slider.setVelocityBasedMode (false);
    slider.setMouseDragSensitivity (200);
    slider.setDoubleClickReturnValue (false, 0.0);
    addAndMakeVisible (slider);
}

void Fader::resized() { slider.setBounds (getLocalBounds().withTrimmedBottom (readoutH + labelH + 6).withTrimmedTop (8)); }

void Fader::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto label = r.removeFromBottom ((float) labelH);
    r.removeFromBottom (3.0f);
    auto lcd = r.removeFromBottom ((float) readoutH).withSizeKeepingCentre (juce::jmin (64.0f, r.getWidth()), (float) readoutH);
    paintWell (g, lcd, 5.0f);
    g.setFont (caps (9.0f, 0.04f));
    glowText (g, upper (slider.getTextFromValue (slider.getValue())), lcd, juce::Justification::centred, col::ouro);
    g.setColour (col::crema2);
    drawCaps (g, name, label, juce::Justification::centred, 8.5f, 0.2f);
}

Knob::Knob()
{
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (180);
    addAndMakeVisible (slider);
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (col::crema2);
    drawCaps (g, name, getLocalBounds().removeFromBottom (13).toFloat(), juce::Justification::centred, 8.0f, 0.18f);
}

void Lever::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    paintWell (g, r, 9.0f);
    auto slot = r.reduced (12.0f, r.getHeight() * 0.40f);
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (slot, 3.0f);
    g.setColour (col::crema3.withAlpha (0.6f)); g.fillRect (r.getCentreX() - 0.5f, r.getY() + 5.0f, 1.0f, 5.0f);
    const float half = r.getWidth() * 0.5f - 22.0f;
    const float cx = r.getCentreX() + x * half;
    if (std::abs (x) > 0.01f)
    {
        auto lit = juce::Rectangle<float> (juce::jmin (cx, r.getCentreX()), slot.getCentreY() - 1.5f, std::abs (cx - r.getCentreX()), 3.0f);
        g.setColour (col::laranxa.withAlpha (0.3f)); g.fillRoundedRectangle (lit.expanded (0.0f, 2.0f), 2.0f);
        g.setColour (col::laranxa); g.fillRoundedRectangle (lit, 1.5f);
    }
    const float cy = r.getCentreY() - (pushed ? 5.0f : 0.0f);
    auto handle = juce::Rectangle<float> (cx - 13.0f, cy - 13.0f, 26.0f, 26.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (handle.translated (0.0f, 2.5f), 5.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a5577), handle.getX(), handle.getY(), juce::Colour (0xff141a2e), handle.getX(), handle.getBottom(), false));
    g.fillRoundedRectangle (handle, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.drawRoundedRectangle (handle.reduced (0.5f), 5.0f, 1.0f);
    for (int i = 0; i < 4; ++i) { g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRect (handle.getX() + 6.0f, handle.getY() + 7.0f + (float) i * 3.5f, handle.getWidth() - 12.0f, 1.0f); }
    g.setColour (col::crema); g.fillRect (cx - 0.8f, handle.getY() + 3.0f, 1.6f, 4.0f);
    paintLed (g, { r.getRight() - 10.0f, r.getY() + 10.0f }, pushed);
}

void Lever::mouseDown (const juce::MouseEvent& e) { mouseDrag (e); }

void Lever::mouseDrag (const juce::MouseEvent& e)
{
    const float half = (float) getWidth() * 0.5f - 22.0f;
    x = juce::jlimit (-1.0f, 1.0f, (e.position.x - (float) getWidth() * 0.5f) / half);
    pushed = e.position.y < (float) getHeight() * 0.3f || e.getDistanceFromDragStartY() < -10;
    repaint();
    if (onMove) onMove (x, pushed);
}

void Lever::mouseUp (const juce::MouseEvent&)
{
    x = 0.0f; pushed = false; repaint();   // a panca volve ao centro
    if (onMove) onMove (x, pushed);
}

// ================================================================ pantallas
void Screens::resized()
{
    auto r = getLocalBounds().toFloat();
    const float gap = 10.0f;
    voicesBox = r.removeFromLeft (156.0f); r.removeFromLeft (gap);
    lfoBox = r.removeFromRight (168.0f); r.removeFromRight (gap);
    envBox = r.removeFromRight (226.0f); r.removeFromRight (gap);
    waveBox = r.removeFromRight (226.0f); r.removeFromRight (gap);
    mainBox = r;
}

void Screens::showParam (const juce::String& name, const juce::String& value)
{
    paramName = name; paramValue = value; paramTime = juce::Time::getMillisecondCounter();
    repaint (mainBox.toNearestInt());
}

void Screens::setMessage (const juce::String& m)
{
    message = m; messageTime = juce::Time::getMillisecondCounter();
    repaint (mainBox.toNearestInt());
}

void Screens::tick()
{
    bool v = false;
    for (int i = 0; i < xuno::MAXV; ++i)
    {
        const float l = proc.voiceLevel[(size_t) i].load();
        if (std::abs (l - levels[(size_t) i]) > 0.01f) { levels[(size_t) i] = l; v = true; }
    }
    if (v) { repaint (voicesBox.toNearestInt()); repaint (envBox.toNearestInt()); }
    const float lf = proc.lfoValue.load();
    if (std::abs (lf - lfoShown) > 0.01f) { lfoShown = lf; repaint (lfoBox.toNearestInt()); }
    const auto now = juce::Time::getMillisecondCounter();
    if ((paramTime != 0 && now - paramTime > 2200 && now - paramTime < 2400) || (message.isNotEmpty() && now - messageTime > 7000))
    {
        if (now - messageTime > 7000) message = {};
        repaint (mainBox.toNearestInt());
    }
}

static void screenTitle (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t)
{
    g.setColour (col::crema3);
    drawCaps (g, t, r.reduced (14.0f, 10.0f).withHeight (12.0f), juce::Justification::centredLeft, 8.0f, 0.26f);
}

void Screens::paintVoices (juce::Graphics& g, juce::Rectangle<float> r)
{
    paintGlass (g, r, 14.0f);
    screenTitle (g, r, "Voces");
    const int nv = proc.apvts.getRawParameterValue ("voices12")->load() > 0.5f ? 12 : 6;
    auto in = r.reduced (12.0f, 0.0f).withTrimmedTop (30.0f).withTrimmedBottom (26.0f);
    const float cw = in.getWidth() / (float) nv;
    const int segs = 14;
    for (int v = 0; v < nv; ++v)
    {
        auto bar = juce::Rectangle<float> (in.getX() + (float) v * cw, in.getY(), cw, in.getHeight()).reduced (cw * 0.22f, 0.0f);
        const float lv = levels[(size_t) v];
        const bool gate = proc.voiceGate[(size_t) v].load();
        const float sh = bar.getHeight() / (float) segs;
        for (int s = 0; s < segs; ++s)
        {
            auto seg = juce::Rectangle<float> (bar.getX(), bar.getBottom() - (float) (s + 1) * sh, bar.getWidth(), sh - 1.5f);
            const bool on = lv > ((float) s + 0.5f) / (float) segs;
            const auto c = s >= segs - 2 ? col::laranxa : col::verde;
            g.setColour (on ? c : c.withAlpha (0.08f));
            g.fillRoundedRectangle (seg, 1.0f);
        }
        g.setColour (gate ? col::crema : col::crema3);
        const int note = proc.voiceNote[(size_t) v].load();
        drawCaps (g, nv == 6 ? noteName (note) : juce::String (v + 1), juce::Rectangle<float> (in.getX() + (float) v * cw - 4.0f, in.getBottom() + 6.0f, cw + 8.0f, 12.0f),
                  juce::Justification::centred, nv == 6 ? 7.5f : 7.0f, 0.04f);
    }
    paintGlassShine (g, r, 14.0f);
}

void Screens::paintMain (juce::Graphics& g, juce::Rectangle<float> r)
{
    paintGlass (g, r, 14.0f);
    auto in = r.reduced (20.0f, 14.0f);
    const auto now = juce::Time::getMillisecondCounter();
    const int src = proc.getSource(), n = proc.getPatchNum();
    if (paramTime != 0 && now - paramTime < 2200)
    {
        g.setColour (col::crema2);
        drawCaps (g, paramName, in.withHeight (16.0f), juce::Justification::centredLeft, 10.0f, 0.26f);
        g.setFont (cond (38.0f));
        glowText (g, upper (paramValue), in.withTrimmedTop (22.0f).withHeight (52.0f), juce::Justification::centredLeft, col::ouro);
    }
    else
    {
        // o número do son, grande e dourado, e o nome en crema
        const auto label = proc.isManual() ? juce::String ("--") : BXProcessor::patchLabel (n);
        g.setFont (cond (42.0f));
        auto numR = in.withWidth (98.0f).withHeight (56.0f);
        glowText (g, label, numR, juce::Justification::centredLeft, col::ouro);
        if (proc.isEdited()) { g.setColour (col::laranxa); g.fillEllipse (numR.getRight() - 6.0f, numR.getY() + 12.0f, 7.0f, 7.0f); }
        auto right = in.withTrimmedLeft (110.0f);
        const auto name = proc.isManual() ? U ("Manual") : proc.patchName (src, n);
        g.setColour (col::crema);
        g.setFont (cond (24.0f).withExtraKerningFactor (0.02f));
        g.drawFittedText (upper (name.isNotEmpty() ? name : U ("Sen nome")), right.withHeight (32.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
        g.setColour (col::crema2);
        drawCaps (g, proc.isManual() ? U ("O panel, sen memoria") : (U ("Banco ") + proc.sourceName (src) + (proc.isEdited() ? U (" · editado") : juce::String())),
                  right.withY (right.getY() + 34.0f).withHeight (14.0f), juce::Justification::centredLeft, 8.5f, 0.24f);
        const auto desc = proc.isManual() || message.isNotEmpty() ? juce::String() : proc.patchDesc (src, n);
        if (desc.isNotEmpty())
        {
            g.setColour (col::crema2);
            g.setFont (body (12.5f));
            g.drawFittedText (desc, in.withTrimmedTop (62.0f).withHeight (36.0f).toNearestInt(), juce::Justification::topLeft, 2, 0.9f);
        }
    }
    if (message.isNotEmpty())
    {
        g.setColour (col::laranxa);
        g.setFont (body (12.5f));
        g.drawFittedText (message, in.withTop (in.getBottom() - 16.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.85f);
    }
    paintGlassShine (g, r, 14.0f);
}

// a forma de onda do DCO tal como a mestura o panel: serra, pulso e sub (dous ciclos, para que se vexa o sub)
void Screens::paintWave (juce::Graphics& g, juce::Rectangle<float> r)
{
    paintGlass (g, r, 14.0f);
    screenTitle (g, r, "DCO");
    const auto p = proc.panelPatch();
    const bool saw = p[xuno::SW1] & 0x10, pulse = p[xuno::SW1] & 0x08;
    double pw = 16383.0;
    if (p[xuno::SW2] & 1) pw = 16383.0 - (2.0 * p[xuno::DCO_PWM]) * 16383.0 / 256.0;   // manual
    else pw = 16383.0 - (2.0 * p[xuno::DCO_PWM]) * (0.5 + 0.5 * std::abs (lfoShown)) * 16383.0 / 256.0;
    const double theta = xuno::pulseThreshold ((uint16_t) juce::jlimit (0.0, 16383.0, pw));
    const double sub = xuno::subGain (p[xuno::DCO_SUB] / 127.0) * 1.19;
    auto box = r.reduced (16.0f, 0.0f).withTrimmedTop (30.0f).withTrimmedBottom (16.0f);
    g.setColour (col::lina.withAlpha (0.6f));
    g.fillRect (box.getX(), box.getCentreY(), box.getWidth(), 1.0f);
    juce::Path path;
    const int N = 240;
    double peak = 0.0;
    std::vector<double> ys ((size_t) N + 1);
    for (int i = 0; i <= N; ++i)
    {
        const double t = 2.0 * i / N;            // dous ciclos
        const double ph = t - std::floor (t);
        double y = 0.0;
        if (saw) y += ph - 0.5;
        if (pulse) y += ph > theta ? 0.5 : -0.5;
        y += (t < 1.0 ? 0.5 : -0.5) * sub;
        ys[(size_t) i] = y; peak = std::max (peak, std::abs (y));
    }
    const double sc = peak > 1e-6 ? 0.44 / std::max (peak, 0.6) : 0.0;
    for (int i = 0; i <= N; ++i)
    {
        const float x = box.getX() + box.getWidth() * (float) i / (float) N;
        const float y = box.getCentreY() - (float) (ys[(size_t) i] * sc) * box.getHeight();
        if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
    }
    g.setColour (col::laranxa.withAlpha (0.18f)); g.strokePath (path, juce::PathStrokeType (5.0f));
    g.setColour (col::crema); g.strokePath (path, juce::PathStrokeType (1.6f));
    if (p[xuno::DCO_NOISE] > 0)
    {
        g.setColour (col::crema3);
        drawCaps (g, U ("+ ruído ") + percent (p[xuno::DCO_NOISE]), r.reduced (14.0f, 8.0f).withTop (r.getBottom() - 20.0f), juce::Justification::centredRight, 7.5f, 0.2f);
    }
    g.setColour (col::crema3);
    static const char* ranges[3] = { "16'", "8'", "4'" };
    drawCaps (g, ranges[xuno::rangeOf (p)], r.reduced (14.0f, 10.0f).withHeight (12.0f), juce::Justification::centredRight, 8.0f, 0.2f);
    paintGlassShine (g, r, 14.0f);
}

// a envolvente: os catro tramos cos tempos de verdade (en escala comprimida) e o nivel actual
void Screens::paintEnv (juce::Graphics& g, juce::Rectangle<float> r)
{
    paintGlass (g, r, 14.0f);
    screenTitle (g, r, "Envolvente");
    const auto p = proc.panelPatch();
    const int inc = xuno::rom::ENV_ATTACK[p[xuno::ENV_A]];
    const double aMs = (double) ((0x3fff + inc - 1) / inc) * LOOP_MS;
    auto tc = [] (int v) { const double c = xuno::rom::ENV_DECAY[v] / 65536.0; return -LOOP_MS / std::log (c); };   // constante de tempo (ms)
    const double dTau = tc (p[xuno::ENV_D]), rTau = tc (p[xuno::ENV_R]);
    const double s = p[xuno::ENV_S] / 127.0;
    auto sq = [] (double ms) { return std::sqrt (std::max (ms, 1.0)); };   // escala comprimida
    const double wa = sq (aMs), wd = sq (dTau * 3.0), ws = 18.0, wr = sq (rTau * 3.0);
    auto box = r.reduced (16.0f, 0.0f).withTrimmedTop (30.0f).withTrimmedBottom (16.0f);
    const double total = wa + wd + ws + wr;
    auto X = [&] (double u) { return box.getX() + (float) (u / total) * box.getWidth(); };
    auto Y = [&] (double l) { return box.getBottom() - (float) l * box.getHeight(); };
    juce::Path path;
    path.startNewSubPath (X (0), Y (0));
    path.lineTo (X (wa), Y (1.0));
    for (int i = 1; i <= 40; ++i) { const double f = i / 40.0; path.lineTo (X (wa + wd * f), Y (s + (1.0 - s) * std::exp (-3.0 * f))); }
    path.lineTo (X (wa + wd + ws), Y (s + (1.0 - s) * std::exp (-3.0)));
    const double s0 = s + (1.0 - s) * std::exp (-3.0);
    for (int i = 1; i <= 40; ++i) { const double f = i / 40.0; path.lineTo (X (wa + wd + ws + wr * f), Y (s0 * std::exp (-3.0 * f))); }
    juce::Path fill (path); fill.lineTo (X (total), Y (0)); fill.closeSubPath();
    g.setColour (col::laranxa.withAlpha (0.08f)); g.fillPath (fill);
    g.setColour (col::laranxa.withAlpha (0.18f)); g.strokePath (path, juce::PathStrokeType (5.0f));
    g.setColour (col::crema); g.strokePath (path, juce::PathStrokeType (1.6f));
    float mx = 0.0f;
    for (float l : levels) mx = std::max (mx, l);
    if (mx > 0.005f)
    {
        g.setColour (col::laranxa.withAlpha (0.8f));
        g.fillRect (box.getX(), Y (mx) - 0.75f, box.getWidth(), 1.5f);
    }
    g.setColour (col::crema3);
    drawCaps (g, (p[xuno::SW2] & 4) ? U ("VCA: porta") : U ("VCA: env"), r.reduced (14.0f, 10.0f).withHeight (12.0f), juce::Justification::centredRight, 7.5f, 0.18f);
    paintGlassShine (g, r, 14.0f);
}

void Screens::paintLfo (juce::Graphics& g, juce::Rectangle<float> r)
{
    paintGlass (g, r, 14.0f);
    screenTitle (g, r, "LFO");
    const auto p = proc.panelPatch();
    auto box = r.reduced (16.0f, 0.0f).withTrimmedTop (30.0f).withTrimmedBottom (36.0f).withTrimmedRight (12.0f);
    juce::Path tri;
    tri.startNewSubPath (box.getX(), box.getCentreY());
    tri.lineTo (box.getX() + box.getWidth() * 0.25f, box.getY());
    tri.lineTo (box.getX() + box.getWidth() * 0.75f, box.getBottom());
    tri.lineTo (box.getRight(), box.getCentreY());
    g.setColour (col::lina.withAlpha (0.6f)); g.fillRect (box.getX(), box.getCentreY(), box.getWidth(), 1.0f);
    g.setColour (col::crema.withAlpha (0.5f)); g.strokePath (tri, juce::PathStrokeType (1.4f));
    // o valor de agora: un punto que sobe e baixa
    const float y = box.getCentreY() - lfoShown * box.getHeight() * 0.5f;
    const float x = box.getRight() + 8.0f;
    g.setColour (col::laranxa.withAlpha (0.3f)); g.fillEllipse (x - 6.0f, y - 6.0f, 12.0f, 12.0f);
    g.setColour (col::laranxa); g.fillEllipse (x - 3.5f, y - 3.5f, 7.0f, 7.0f);
    g.setFont (caps (11.0f, 0.06f));
    glowText (g, upper (lfoRateText (p[xuno::LFO_RATE])), r.reduced (14.0f, 6.0f).withTop (r.getBottom() - 28.0f), juce::Justification::centredLeft, col::ouro);
    paintGlassShine (g, r, 14.0f);
}

void Screens::paint (juce::Graphics& g)
{
    paintVoices (g, voicesBox);
    paintMain (g, mainBox);
    paintWave (g, waveBox);
    paintEnv (g, envBox);
    paintLfo (g, lfoBox);
}

// ================================================================ táboa dos sons
Chart::Chart (BXProcessor& p, MainView& m) : proc (p), main (m)
{
    for (auto* b : { &groupA, &groupB, &close }) { b->setWantsKeyboardFocus (false); addAndMakeVisible (b); }
    close.getProperties().set ("noLed", true);
    groupA.onClick = [this] { group = 0; refresh(); };
    groupB.onClick = [this] { group = 1; refresh(); };
    close.onClick = [this] { main.showChart (false); };
}

void Chart::refresh()
{
    groupA.setToggleState (group == 0, juce::dontSendNotification);
    groupB.setToggleState (group == 1, juce::dontSendNotification);
    repaint();
}

void Chart::resized()
{
    auto r = getLocalBounds().reduced (20, 14);
    auto top = r.removeFromTop (30);
    close.setBounds (top.removeFromRight (96));
    top.removeFromRight (10);
    groupB.setBounds (top.removeFromRight (100)); top.removeFromRight (8);
    groupA.setBounds (top.removeFromRight (100));
    r.removeFromTop (12);
    grid = r;
}

juce::Rectangle<float> Chart::cell (int i) const
{
    const int bank = i / 8, num = i % 8;
    const float cw = (float) (grid.getWidth() - 70) / 8.0f, ch = (float) grid.getHeight() / 8.0f;
    return { (float) grid.getX() + 70.0f + (float) num * cw, (float) grid.getY() + (float) bank * ch, cw, ch };
}

int Chart::cellAt (juce::Point<int> p) const
{
    for (int i = 0; i < 64; ++i) if (cell (i).contains (p.toFloat())) return i;
    return -1;
}

void Chart::paint (juce::Graphics& g)
{
    paintPanel (g, getLocalBounds().toFloat(), 16.0f);
    auto head = getLocalBounds().toFloat().reduced (20.0f, 14.0f).removeFromTop (30.0f);
    g.setColour (col::crema);
    g.setFont (cond (15.0f).withExtraKerningFactor (0.08f));
    g.drawText (upper (U ("Táboa de sons")), head, juce::Justification::centredLeft, false);
    g.setColour (col::crema2);
    drawCaps (g, U ("Banco ") + proc.sourceName (proc.getSource()) + U (" · ") + proc.sourceTitle (proc.getSource()),
              head.withTrimmedLeft (190.0f).withTrimmedRight (330.0f), juce::Justification::centredLeft, 8.5f, 0.2f);
    paintWell (g, grid.toFloat().expanded (6.0f), 10.0f);
    const int src = proc.getSource();
    for (int b = 0; b < 8; ++b)
    {
        auto c = cell (b * 8);
        g.setColour (col::laranxa);
        drawCaps (g, U ("Banco ") + juce::String (b + 1), c.withX ((float) grid.getX() + 4.0f).withWidth (62.0f), juce::Justification::centredLeft, 8.0f, 0.2f);
    }
    for (int i = 0; i < 64; ++i)
    {
        const int idx = group * 64 + i;
        auto c = cell (i).reduced (3.0f);
        const bool sel = ! proc.isManual() && idx == proc.getPatchNum();
        paintKey (g, c, i == hover, false);
        paintLed (g, { c.getX() + 11.0f, c.getCentreY() }, sel);
        g.setColour (sel ? col::ouro : col::crema3);
        drawCaps (g, BXProcessor::patchLabel (idx), c.withTrimmedLeft (20.0f).withWidth (28.0f), juce::Justification::centredLeft, 8.0f, 0.06f);
        const auto name = proc.patchName (src, idx);
        g.setColour (name.isEmpty() ? col::crema3 : (sel ? col::crema : col::crema2));
        g.setFont (body (12.5f));
        g.drawFittedText (name.isEmpty() ? U ("—") : name, c.withTrimmedLeft (50.0f).reduced (2.0f, 0.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
    }
}

void Chart::mouseMove (const juce::MouseEvent& e)
{
    const int h = cellAt (e.getPosition());
    if (h == hover) return;
    hover = h; repaint();
    if (h >= 0)
    {
        const int idx = group * 64 + h;
        const auto d = proc.patchDesc (proc.getSource(), idx);
        setTooltip (BXProcessor::patchLabel (idx) + "  " + proc.patchName (proc.getSource(), idx) + (d.isNotEmpty() ? "\n" + d : juce::String()));
    }
    else setTooltip ({});
}

void Chart::mouseDown (const juce::MouseEvent& e)
{
    const int h = cellAt (e.getPosition());
    if (h < 0) return;
    proc.selectPatch (group * 64 + h);
    refresh();
}

// ================================================================ vista principal
Fader* MainView::addFader (const char* id, const juce::String& name, const juce::String& tip, std::function<juce::String (int)> fmt)
{
    auto* f = faders.add (new Fader());
    f->name = name;
    f->slider.setTooltip (tip);
    f->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, f->slider);
    f->slider.textFromValueFunction = [fmt] (double v) { return fmt (juce::roundToInt (v)); };
    f->slider.onValueChange = [this, f] { f->repaint(); showParam (f->name, f->slider.getTextFromValue (f->slider.getValue())); };
    addAndMakeVisible (f);
    return f;
}

MainView::MainView (BXProcessor& p)
    : proc (p), screens (p), chart (p, *this)
{
    setLookAndFeel (&look);

    // ---- os deslizadores, na orde do panel
    auto vcfEnv = [] (int v) { return octText (v * 254.0 / 256.0 * 32.0 / 285.5, false); };
    addFader ("lfoRate", "Rate", U ("Velocidade do LFO (triangular): de 0,04 a 30 Hz."), lfoRateText);
    addFader ("lfoDelay", "Delay", U ("Canto tarda o LFO en chegar despois da primeira nota (se non soaba ningunha)."), lfoDelayText);
    addFader ("dcoLfo", "LFO", U ("Vibrato: canto move o LFO o ton (ata ±4 semitóns)."), dcoLfoText);
    addFader ("dcoPwm", "PWM", U ("A anchura do pulso (manual) ou canto a move o LFO. O deslizador chega a 105, coma no orixinal."), pwmText);
    addFader ("dcoSub", "Sub", U ("O sub-oscilador: unha cadrada unha oitava por debaixo."), percent);
    addFader ("dcoNoise", U ("Ruído"), U ("O ruído (un só xerador para as seis voces)."), percent);
    addFader ("hpf", "Freq", U ("Filtro paso alto: 0 realza os graves, 1 plano, 2 corta por debaixo de 236 Hz e 3 de 754 Hz."),
              [] (int v) { static const char* n[4] = { "Graves +", "Plano", "236 Hz", "754 Hz" }; return U (n[juce::jlimit (0, 3, v)]); });
    addFader ("vcfFreq", "Freq", U ("O corte do filtro de catro polos (no Do3). Vai en 128 pasos, coma no orixinal."), vcfFreqText);
    addFader ("vcfRes", "Res", U ("Resonancia: preto do máximo o filtro oscila só."), percent);
    addFader ("vcfEnv", "Env", U ("Canto move a envolvente o corte (cara arriba ou abaixo, segundo + e −)."), vcfEnv);
    addFader ("vcfLfo", "LFO", U ("Canto move o LFO o corte."), [] (int v) { return octText (v * 254.0 / 256.0 * 8.0 / 285.5, true); });
    addFader ("vcfKybd", "Kybd", U ("Canto segue o corte ás notas (ao máximo, unha oitava por oitava)."), percent);
    addFader ("vcaLevel", "Level", U ("O volume do son: ±10 dB arredor do medio."), levelText);
    addFader ("envA", "A", U ("Ataque: de inmediato a 3,3 s."), attackText);
    addFader ("envD", "D", U ("Caída ata o sustain (o tempo ata −20 dB)."), decayText);
    addFader ("envS", "S", U ("Nivel de sustain."), percent);
    addFader ("envR", "R", U ("Relaxación ao soltar a tecla (o tempo ata −20 dB)."), decayText);
    faders[3]->slider.setRange (0, 105, 1);   // PWM: o panel do orixinal non pasa de 105
    faders[6]->slider.setMouseDragSensitivity (80);

    // ---- teclas do panel, atadas aos parámetros
    auto bind = [this] (const char* id, std::unique_ptr<juce::ParameterAttachment>& att, std::function<void (int)> show)
    {
        auto* prm = proc.apvts.getParameter (id);
        att = std::make_unique<juce::ParameterAttachment> (*prm, [prm, show] (float v) { show ((int) std::lround (prm->convertFrom0to1 (prm->convertTo0to1 (v)))); });
        att->sendInitialUpdate();
    };
    auto set = [] (std::unique_ptr<juce::ParameterAttachment>& att, float v) { att->setValueAsCompleteGesture (v); };
    auto key = [this] (juce::TextButton& b, const juce::String& tip)
    {
        b.setTooltip (tip);
        b.setWantsKeyboardFocus (false);
        addAndMakeVisible (b);
    };

    bind ("range", rangeAtt, [this] (int v) { range16.setToggleState (v == 0, juce::dontSendNotification); range8.setToggleState (v == 1, juce::dontSendNotification); range4.setToggleState (v == 2, juce::dontSendNotification); });
    range16.onClick = [this, set] { set (rangeAtt, 0); showParam ("Rexistro", "16'"); };
    range8.onClick = [this, set] { set (rangeAtt, 1); showParam ("Rexistro", "8'"); };
    range4.onClick = [this, set] { set (rangeAtt, 2); showParam ("Rexistro", "4'"); };
    for (auto* b : { &range16, &range8, &range4 }) key (*b, U ("O rexistro do DCO: 16', 8' ou 4' (o reloxo dos contadores a 1, 2 ou 4 MHz)."));
    bind ("pwmMode", pwmAtt, [this] (int v) { pwmLfo.setToggleState (v == 0, juce::dontSendNotification); pwmMan.setToggleState (v == 1, juce::dontSendNotification); });
    pwmLfo.onClick = [this, set] { set (pwmAtt, 0); showParam ("PWM con", "LFO"); };
    pwmMan.onClick = [this, set] { set (pwmAtt, 1); showParam ("PWM con", "Manual"); };
    key (pwmLfo, U ("O LFO move a anchura do pulso.")); key (pwmMan, U ("A anchura do pulso é a do deslizador."));
    bind ("pulse", pulseAtt, [this] (int v) { pulseBtn.setToggleState (v != 0, juce::dontSendNotification); });
    bind ("saw", sawAtt, [this] (int v) { sawBtn.setToggleState (v != 0, juce::dontSendNotification); });
    pulseBtn.onClick = [this, set] { set (pulseAtt, pulseBtn.getToggleState() ? 0.0f : 1.0f); showParam ("Pulso", pulseBtn.getToggleState() ? U ("Aceso") : U ("Apagado")); };
    sawBtn.onClick = [this, set] { set (sawAtt, sawBtn.getToggleState() ? 0.0f : 1.0f); showParam ("Serra", sawBtn.getToggleState() ? U ("Acesa") : U ("Apagada")); };
    key (pulseBtn, U ("A onda de pulso (cadrada co PWM ao mínimo).")); key (sawBtn, U ("A onda de serra."));
    bind ("envPol", envAtt, [this] (int v) { envPos.setToggleState (v == 0, juce::dontSendNotification); envNeg.setToggleState (v == 1, juce::dontSendNotification); });
    envPos.onClick = [this, set] { set (envAtt, 0); showParam ("Envolvente no VCF", "+"); };
    envNeg.onClick = [this, set] { set (envAtt, 1); showParam ("Envolvente no VCF", U ("−")); };
    key (envPos, U ("A envolvente abre o filtro.")); key (envNeg, U ("A envolvente pecha o filtro."));
    bind ("vcaMode", vcaAtt, [this] (int v) { vcaEnv.setToggleState (v == 0, juce::dontSendNotification); vcaGate.setToggleState (v == 1, juce::dontSendNotification); });
    vcaEnv.onClick = [this, set] { set (vcaAtt, 0); showParam ("VCA", "Envolvente"); };
    vcaGate.onClick = [this, set] { set (vcaAtt, 1); showParam ("VCA", "Porta"); };
    key (vcaEnv, U ("O VCA segue a envolvente.")); key (vcaGate, U ("O VCA abre e pecha coa tecla (porta)."));
    bind ("chorus", chorusAtt, [this] (int v) { chorusOff.setToggleState (v == 0, juce::dontSendNotification); chorusI.setToggleState (v == 1, juce::dontSendNotification); chorusII.setToggleState (v == 2, juce::dontSendNotification); });
    chorusOff.onClick = [this, set] { set (chorusAtt, 0); showParam ("Chorus", "Apagado"); };
    chorusI.onClick = [this, set] { set (chorusAtt, 1); showParam ("Chorus", "I"); };
    chorusII.onClick = [this, set] { set (chorusAtt, 2); showParam ("Chorus", "II"); };
    key (chorusOff, U ("Sen chorus.")); key (chorusI, U ("Chorus I: dous BBD en contrafase, lento (0,5 Hz)."));
    key (chorusII, U ("Chorus II: máis rápido (0,86 Hz)."));

    // ---- cabeceira
    bankBtn.getProperties().set ("gold", true);
    bankBtn.onClick = [this] { chooseBank(); };
    key (bankBtn, U ("O banco: a memoria, os sons propios, os .syx da carpeta…"));
    chartBtn.onClick = [this] { showChart (! chart.isVisible()); };
    key (chartBtn, U ("A táboa cos nomes dos 64 sons de cada grupo."));
    openBtn.onClick = [this] { openSyx(); };
    key (openBtn, U ("Abrir un .syx de Juno-106 (un son ou un banco enteiro)."));
    saveBtn.onClick = [this] { saveMenu(); };
    key (saveBtn, U ("Gardar o banco ou o son do panel como .syx de Juno-106."));
    for (auto* b : { &openBtn, &saveBtn }) b->getProperties().set ("noLed", true);
    bind ("poly", polyAtt, [this] (int v) { poly1.setToggleState (v == 0, juce::dontSendNotification); poly2.setToggleState (v == 1, juce::dontSendNotification); unison.setToggleState (v == 2, juce::dontSendNotification); });
    poly1.onClick = [this, set] { set (polyAtt, 0); showParam (U ("Asignación"), "Poly 1"); };
    poly2.onClick = [this, set] { set (polyAtt, 1); showParam (U ("Asignación"), "Poly 2"); };
    unison.onClick = [this, set] { set (polyAtt, 2); showParam (U ("Asignación"), U ("Unísono")); };
    key (poly1, U ("POLY 1: cada nota colle a voz que leva máis tempo calada; as colas seguen soando."));
    key (poly2, U ("POLY 2: sempre a primeira voz libre; corta as colas (bo co portamento)."));
    key (unison, U ("Unísono (no orixinal, POLY 1 e POLY 2 á vez): as seis voces nunha nota."));
    bind ("transpose", transpAtt, [this] (int v) { transpBtn.setButtonText (U ("Transp. ") + (v > 0 ? "+" : "") + numero (v, 0)); transpBtn.setToggleState (v != 0, juce::dontSendNotification); });
    transpBtn.onClick = [this] { transposeMenu(); };
    key (transpBtn, U ("KEY TRANSPOSE: sobe ou baixa todas as notas ata unha oitava."));
    funcBtn.onClick = [this] { functionMenu(); };
    funcBtn.getProperties().set ("noLed", true);
    key (funcBtn, U ("Opcións: tempo das notas, tolerancias das voces, ruído, doce voces, avisos…"));

    // ---- fila dos sons
    for (int g2 = 0; g2 < 2; ++g2)
    {
        groupBtn[g2].setButtonText (g2 == 0 ? "A" : "B");
        groupBtn[g2].onClick = [this, g2] { pressGroup (g2); };
        key (groupBtn[g2], U ("Grupo ") + (g2 == 0 ? "A" : "B"));
    }
    for (int i = 0; i < 8; ++i)
    {
        auto* b = bankBtns.add (new juce::TextButton (juce::String (i + 1)));
        b->onClick = [this, i] { pressBank (i); };
        key (*b, U ("Banco ") + juce::String (i + 1));
        auto* n = patchBtns.add (new juce::TextButton (juce::String (i + 1)));
        n->onClick = [this, i] { pressPatch (i); };
        key (*n, U ("Número ") + juce::String (i + 1) + U (". Con WRITE aceso, garda aquí o panel."));
    }
    manualBtn.onClick = [this] { proc.setManual(); showMessage (U ("Manual: o son é o que marca o panel, sen memoria.")); };
    key (manualBtn, U ("MANUAL: o son é o do panel, sen ningunha memoria."));
    writeBtn.onClick = [this]
    {
        writeArmed = ! writeArmed;
        showMessage (writeArmed ? U ("WRITE: escolle grupo e banco, e preme o número onde gardar o panel (na memoria).") : juce::String());
        refreshKeys();
    };
    key (writeBtn, U ("WRITE: garda o panel na memoria (despois, preme un número)."));

    // ---- á esquerda do teclado
    auto knob = [this] (Knob& k, const char* id, const juce::String& name, const juce::String& tip, std::function<juce::String (double)> fmt)
    {
        k.name = name;
        k.slider.setTooltip (tip);
        k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
        k.slider.textFromValueFunction = fmt;
        addAndMakeVisible (k);
    };
    knob (volume, "volume", "Volume", U ("O volume de saída."), [] (double v) { return (v >= 0.05 ? "+" : "") + numero (v, 1) + " dB"; });
    knob (tune, "tune", U ("Afinación"), U ("±½ semitón. No medio, o La3 dá 440 Hz."),
          [] (double v) { const double c = (v + 8.0) * 100.0 / 256.0; return (c >= 0.05 ? "+" : "") + numero (c, 1) + " c"; });
    knob (porta, "porta", "Portamento", U ("O tempo que tarda o ton en escorregar unha oitava."),
          [] (double v) { const int a = (int) v; if (a == 0) return U ("0 ms"); const int c = xuno::rom::PORTA[a >> 1]; return tempo (12.0 * 256.0 / std::max (1, c) * LOOP_MS); });
    knob (bendDco, "bendDco", "DCO", U ("Canto move a panca (e a roda de afinación do MIDI) o ton."),
          [] (double v) { return U ("±") + numero (0.754 * 255.0 * v / 4096.0, 1) + " st"; });
    knob (bendVcf, "bendVcf", "VCF", U ("Canto move a panca o corte do filtro."),
          [] (double v) { return U ("±") + numero (255.0 * v / 16.0 / 4.0 / 285.5, 1) + " oit"; });
    knob (lfoMod, "lfoMod", "LFO", U ("Canto vibrato dá a panca empurrada (ou a roda de modulación do MIDI)."),
          [] (double v) { return U ("±") + juce::String (juce::roundToInt (v / 255.0 * 398.0)) + " c"; });
    volume.slider.setDoubleClickReturnValue (true, 0.0);
    tune.slider.setDoubleClickReturnValue (true, -8.0);
    for (auto* k : { &volume, &tune, &porta, &bendDco, &bendVcf, &lfoMod })
        k->slider.onValueChange = [this, k] { k->slider.repaint(); showParam (k->name, k->slider.getTextFromValue (k->slider.getValue())); };
    bind ("portaOn", portaOnAtt, [this] (int v) { portaOn.setToggleState (v != 0, juce::dontSendNotification); });
    portaOn.onClick = [this, set] { set (portaOnAtt, portaOn.getToggleState() ? 0.0f : 1.0f); showParam ("Portamento", portaOn.getToggleState() ? U ("Aceso") : U ("Apagado")); };
    key (portaOn, U ("O portamento: aceso ou apagado."));
    lever.onMove = [this] (float x, bool pushed) { proc.lever.store (x); proc.leverPushed.store (pushed); };
    lever.setTooltip (U ("A panca: aos lados move o ton (e o filtro); cara arriba dá o vibrato do LFO."));
    addAndMakeVisible (lever);

    newVersion.getProperties().set ("noLed", true);
    newVersion.onClick = [] { juce::URL (escoita::Actualizacions::downloadsUrl()).launchInDefaultBrowser(); };
    newVersion.setWantsKeyboardFocus (false);
    addChildComponent (newVersion);

    addAndMakeVisible (screens);

    listenBtn.onStateChange = [this]
    {
        const bool down = listenBtn.isDown();
        if (down == listening) return;
        listening = down;
        for (int n : { 48, 60, 64, 67 })
            if (down) proc.keyboardState.noteOn (1, n, 1.0f);
            else proc.keyboardState.noteOff (1, n, 0.0f);
    };
    key (listenBtn, U ("Mentres o tes premido, soa un acorde co son do panel (para probar sen teclado MIDI)."));

    addChildComponent (chart);

    setSize (W, H);
    chart.group = proc.getPatchNum() / 64;
    seenChanges = proc.changes.load();
    refreshKeys();
    startTimerHz (30);
    timerCallback();
}

MainView::~MainView() { stopTimer(); setLookAndFeel (nullptr); }

void MainView::showChart (bool s)
{
    chart.setVisible (s);
    chartBtn.setToggleState (s, juce::dontSendNotification);
    if (s) { chart.group = proc.getPatchNum() / 64; chart.refresh(); chart.toFront (false); }
}

void MainView::refreshKeys()
{
    const int n = proc.getPatchNum();
    const bool man = proc.isManual();
    groupBtn[0].setToggleState (! man && n < 64, juce::dontSendNotification);
    groupBtn[1].setToggleState (! man && n >= 64, juce::dontSendNotification);
    for (int i = 0; i < 8; ++i)
    {
        bankBtns[i]->setToggleState (! man && (n % 64) / 8 == i, juce::dontSendNotification);
        patchBtns[i]->setToggleState (! man && n % 8 == i, juce::dontSendNotification);
    }
    manualBtn.setToggleState (man, juce::dontSendNotification);
    writeBtn.setToggleState (writeArmed && (juce::Time::getMillisecondCounter() / 300) % 2 == 0, juce::dontSendNotification);
    bankBtn.setButtonText (U ("Banco: ") + proc.sourceName (proc.getSource()));
}

void MainView::pressGroup (int group)
{
    const int n = proc.getPatchNum();
    const int to = group * 64 + n % 64;
    if (writeArmed) { proc.writeTo (to); writeArmed = false; showMessage (U ("Gardado en ") + BXProcessor::patchLabel (to) + U (" (memoria).")); }
    else proc.selectPatch (to);
    refreshKeys();
}

void MainView::pressBank (int bank)
{
    const int n = proc.getPatchNum();
    const int to = (n / 64) * 64 + bank * 8 + n % 8;
    if (writeArmed) { proc.writeTo (to); writeArmed = false; showMessage (U ("Gardado en ") + BXProcessor::patchLabel (to) + U (" (memoria).")); }
    else proc.selectPatch (to);
    refreshKeys();
}

void MainView::pressPatch (int number)
{
    const int to = (proc.getPatchNum() / 8) * 8 + number;
    if (writeArmed) { proc.writeTo (to); writeArmed = false; showMessage (U ("Gardado en ") + BXProcessor::patchLabel (to) + U (" (memoria).")); }
    else proc.selectPatch (to);
    refreshKeys();
}

void MainView::chooseBank()
{
    juce::PopupMenu m;
    m.addSectionHeader (U ("De onde son os sons"));
    const int cur = proc.getSource();
    m.addItem (1, U ("MEMORIA — os teus sons gardados"), true, cur == BXProcessor::INTERNAL);
    for (int b = 0; b < proc.numBanks(); ++b)
        m.addItem (100 + b, proc.sourceName (b) + U (" — ") + proc.sourceTitle (b), true, cur == b);
    m.addItem (2, (proc.hasFileBank() ? proc.sourceName (BXProcessor::FILE_BANK) : juce::String ("FICHEIRO")) + U (" — o último .syx aberto"), proc.hasFileBank(), cur == BXProcessor::FILE_BANK);
    m.addSeparator();
    m.addItem (3, U ("Copiar este banco á memoria"), cur != BXProcessor::INTERNAL);
    m.addItem (4, U ("Carpeta de bancos .syx…"));
    if (proc.getBankFolder().isDirectory()) m.addItem (5, U ("Volver ler a carpeta (") + proc.getBankFolder().getFileName() + ")");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&bankBtn), [this] (int r)
    {
        if (r == 1) proc.selectSource (BXProcessor::INTERNAL);
        else if (r == 2) proc.selectSource (BXProcessor::FILE_BANK);
        else if (r == 3) { proc.copySourceToInternal(); showMessage (U ("Banco copiado na memoria.")); }
        else if (r == 4) chooseFolder();
        else if (r == 5) { proc.rescanBankFolder(); showMessage (juce::String (proc.numFolderBanks()) + U (" bancos na carpeta.")); }
        else if (r >= 100) proc.selectSource (r - 100);
        chart.refresh();
        refreshKeys();
    });
}

void MainView::chooseFolder()
{
    chooser = std::make_unique<juce::FileChooser> (U ("Carpeta cos bancos .syx de Juno-106"), proc.getBankFolder());
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories, [this] (const juce::FileChooser& fc)
    {
        const auto f = fc.getResult();
        if (! f.isDirectory()) return;
        proc.setBankFolder (f);
        showMessage (juce::String (proc.numFolderBanks()) + U (" bancos atopados en ") + f.getFileName() + ".");
        chart.refresh();
    });
}

void MainView::openSyx()
{
    chooser = std::make_unique<juce::FileChooser> (U ("Abrir un .syx de Juno-106"), proc.getBankFolder(), "*.syx;*.SYX");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
    {
        const auto f = fc.getResult();
        if (! f.existsAsFile()) return;
        juce::String msg;
        proc.loadSyxFile (f, msg);
        showMessage (msg);
        chart.refresh();
        refreshKeys();
    });
}

void MainView::saveMenu()
{
    juce::PopupMenu m;
    m.addItem (1, U ("Gardar o banco (") + proc.sourceName (proc.getSource()) + U (", 128 sons)…"));
    m.addItem (2, U ("Gardar o son do panel…"));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&saveBtn), [this] (int r)
    {
        if (r <= 0) return;
        const bool bank = r == 1;
        const auto name = bank ? proc.sourceName (proc.getSource()).toLowerCase() : BXProcessor::patchLabel (proc.getPatchNum());
        chooser = std::make_unique<juce::FileChooser> (bank ? U ("Gardar o banco") : U ("Gardar o son"),
                                                       proc.getBankFolder().getChildFile (name + ".syx"), "*.syx");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this, bank] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            if (! f.hasFileExtension ("syx")) f = f.withFileExtension ("syx");
            const bool ok = bank ? proc.saveBankFile (f, proc.getSource()) : proc.savePatchFile (f);
            showMessage (ok ? U ("Gardado: ") + f.getFileName() : U ("Non se puido gardar."));
        });
    });
}

void MainView::transposeMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader (U ("Transposición das teclas"));
    const int cur = (int) std::lround (proc.apvts.getRawParameterValue ("transpose")->load());
    for (int t = 12; t >= -12; --t)
        m.addItem (t + 100, (t > 0 ? "+" : "") + numero (t, 0) + (t == 0 ? U (" (sen transpor)") : juce::String()), true, t == cur);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&transpBtn), [this] (int r)
    {
        if (r < 88) return;
        transpAtt->setValueAsCompleteGesture ((float) (r - 100));
        showParam (U ("Transposición"), (r > 100 ? "+" : "") + numero (r - 100, 0));
    });
}

void MainView::functionMenu()
{
    auto& a = proc.apvts;
    auto get = [&a] (const char* id) { return a.getRawParameterValue (id)->load() > 0.5f; };
    auto flip = [&a] (const char* id)
    {
        if (auto* p = a.getParameter (id)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); p->endChangeGesture(); }
    };
    juce::PopupMenu m;
    m.addSectionHeader (U ("Función"));
    m.addItem (1, U ("Notas ao instante (se non, co tempo do orixinal: porto serie e bucle de 4,2 ms)"), true, get ("instant"));
    m.addItem (2, U ("Voces con tolerancias (cada voz filtra e soa un pouco distinto, coma nun aparello)"), true, get ("variance"));
    m.addItem (3, U ("Ruído do chorus (o sopro dos BBD)"), true, get ("analogNoise"));
    m.addItem (4, U ("Doce voces (coma dous Juno-106 xuntos)"), true, get ("voices12"));
    m.addSeparator();
    m.addItem (6, U ("Avisar das versións novas"), true, versions->enabled());
    m.addItem (7, U ("escoita.link"));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&funcBtn), [this, flip] (int r)
    {
        if (r == 1) flip ("instant");
        else if (r == 2) flip ("variance");
        else if (r == 3) flip ("analogNoise");
        else if (r == 4) flip ("voices12");
        else if (r == 6) versions->setEnabled (! versions->enabled());
        else if (r == 7) juce::URL ("https://www.escoita.link").launchInDefaultBrowser();
    });
}

void MainView::timerCallback()
{
    const auto ch = proc.changes.load();
    if (ch != seenChanges)
    {
        seenChanges = ch;
        if (chart.isVisible()) chart.refresh();
        screens.repaint();
    }
    refreshKeys();
    screens.tick();
    if (++slowTick % 6 == 0) screens.repaint (screens.mainBox.toNearestInt());   // o «editado» e os nomes
    if (slowTick % 3 == 0) screens.repaint (screens.waveBox.toNearestInt());

    const auto nv = versions->newVersion();
    if (nv.isNotEmpty() != newVersion.isVisible())
    {
        newVersion.setButtonText (U ("Hai unha versión nova: ") + nv + U (" · descargar"));
        newVersion.setVisible (nv.isNotEmpty());
        repaint();
    }
}

// ---------------------------------------------------------------- o aparello
void MainView::paint (juce::Graphics& g)
{
    // o fondo: degradado e un cepillado vertical moi fino
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff161e33), 0.0f, 0.0f, juce::Colour (0xff0b101e), 0.0f, (float) H, false));
    g.fillAll();
    {
        static const juce::Image brush = []
        {
            juce::Image im (juce::Image::ARGB, 3, 1, true);
            im.setPixelAt (0, 0, juce::Colours::white.withAlpha (0.018f));
            return im;
        }();
        g.setFillType (juce::FillType (brush, juce::AffineTransform()));
        g.fillRect (0, 0, W, H);
    }

    // cabeceira: o panel, a placa co nome e os pozos das teclas
    paintPanel (g, headerArea.toFloat(), 14.0f);
    {
        const auto plate = juce::Rectangle<float> ((float) headerArea.getX() + 10.0f, (float) headerArea.getY() + 6.0f, 188.0f, (float) headerArea.getHeight() - 12.0f);
        paintWell (g, plate, 9.0f);
        g.setColour (col::crema);
        g.setFont (cond (14.5f).withExtraKerningFactor (0.07f));
        g.drawText ("BARULLO XUNO", plate.reduced (14.0f, 0.0f).withTrimmedBottom (14.0f).withTrimmedTop (4.0f), juce::Justification::centredLeft, false);
        auto sub = plate.reduced (14.0f, 0.0f).withTop (plate.getBottom() - 17.0f).withHeight (11.0f);
        g.setFont (caps (7.5f, 0.36f));
        const float we = juce::GlyphArrangement::getStringWidth (caps (7.5f, 0.36f), "ESCOITA");
        g.setColour (col::crema2); g.drawText ("ESCOITA", sub, juce::Justification::centredLeft, false);
        g.setColour (col::laranxa); g.drawText (".", sub.withTrimmedLeft (we - 1.5f), juce::Justification::centredLeft, false);
        g.setColour (col::crema2); g.drawText (U (" · FERRAMENTAS"), sub.withTrimmedLeft (we + 4.0f), juce::Justification::centredLeft, false);
    }
    auto well = [&] (juce::Component& a, juce::Component& b) { paintWell (g, a.getBounds().getUnion (b.getBounds()).expanded (4).toFloat(), 11.0f); };
    well (chartBtn, saveBtn);
    well (poly1, unison);

    // o panel dos mandos
    paintPanel (g, controlsArea.toFloat(), 16.0f);
    static const char* names[7] = { "LFO", "DCO", "HPF", "VCF", "VCA", "ENV", "Chorus" };
    for (int i = 0; i < 7; ++i)
    {
        const auto r = sections[(size_t) i].toFloat();
        groupTitle (g, r.withHeight (20.0f), names[i]);
        if (i > 0) { g.setColour (col::lina.withAlpha (0.7f)); g.fillRect (r.getX() - 1.0f, r.getY() + 30.0f, 1.0f, r.getHeight() - 40.0f); }
    }
    // pozos das columnas de teclas e os seus rótulos
    auto keyWell = [&] (juce::Component& first, juce::Component& last, const juce::String& t)
    {
        auto b = first.getBounds().getUnion (last.getBounds()).expanded (4).toFloat();
        paintWell (g, b, 10.0f);
        g.setColour (col::crema3);
        drawCaps (g, t, b.withY (b.getY() - 16.0f).withHeight (12.0f).expanded (10.0f, 0.0f), juce::Justification::centred, 7.5f, 0.2f);
    };
    keyWell (range16, range4, "Rexistro");
    keyWell (pwmLfo, pwmMan, "PWM");
    keyWell (pulseBtn, sawBtn, "Ondas");
    keyWell (envPos, envNeg, "Env");
    keyWell (vcaEnv, vcaGate, "Modo");
    keyWell (chorusOff, chorusII, {});

    // a fila dos sons
    paintPanel (g, patchArea.toFloat(), 14.0f);
    groupTitle (g, juce::Rectangle<float> ((float) patchArea.getX() + 10.0f, (float) patchArea.getY() + 8.0f, (float) patchArea.getWidth() - 20.0f, 20.0f), "Memoria");
    auto rowLabel = [&] (juce::Component& first, juce::Component& last, const juce::String& t)
    {
        auto b = first.getBounds().getUnion (last.getBounds()).expanded (4).toFloat();
        paintWell (g, b, 10.0f);
        g.setColour (col::crema3);
        drawCaps (g, t, b.withX (b.getX() - 76.0f).withWidth (70.0f), juce::Justification::centredRight, 8.0f, 0.24f);
    };
    rowLabel (groupBtn[0], groupBtn[1], "Grupo");
    rowLabel (*bankBtns[0], *bankBtns[7], "Banco");
    rowLabel (*patchBtns[0], *patchBtns[7], U ("Número"));
    paintWell (g, writeBtn.getBounds().getUnion (manualBtn.getBounds()).expanded (4).toFloat(), 10.0f);
    paintWell (g, listenBtn.getBounds().expanded (4).toFloat(), 10.0f);

    // á esquerda do teclado
    paintPanel (g, ctrlArea.toFloat(), 14.0f);
    {
        auto c = ctrlArea.toFloat();
        groupTitle (g, juce::Rectangle<float> ((float) volume.getX(), c.getY() + 8.0f, (float) (porta.getX() - volume.getX()), 20.0f), U ("Xeral"));
        groupTitle (g, juce::Rectangle<float> ((float) bendDco.getX(), c.getY() + 8.0f, c.getRight() - 10.0f - (float) bendDco.getX(), 20.0f), "Bender");
        g.setColour (col::crema3);
        drawCaps (g, "LFO trig", lever.getBounds().toFloat().translated (0.0f, -16.0f).withHeight (12.0f), juce::Justification::centred, 7.5f, 0.2f);
    }

    // o pé: Escoita, a axuda e as coordenadas
    {
        const auto r = juce::Rectangle<float> (22.0f, (float) H - 22.0f, (float) W - 44.0f, 14.0f);
        g.setColour (col::crema3);
        const float we = juce::GlyphArrangement::getStringWidth (caps (8.0f, 0.3f), "ESCOITA");
        drawCaps (g, "ESCOITA", r, juce::Justification::centredLeft, 8.0f, 0.3f);
        g.setColour (col::laranxa);
        drawCaps (g, ".", r.withTrimmedLeft (we - 2.0f), juce::Justification::centredLeft, 8.0f, 0.3f);
        g.setColour (col::crema3);
        drawCaps (g, U (" Laboratorio creativo · v") + escoita::Actualizacions::currentVersion(), r.withTrimmedLeft (we + 3.0f), juce::Justification::centredLeft, 8.0f, 0.3f);
        if (! newVersion.isVisible())
            drawCaps (g, U ("Dobre clic no volume e na afinación: o valor de partida"), r, juce::Justification::centred, 8.0f, 0.2f);
        drawCaps (g, U ("42°14′N 8°43′W"), r, juce::Justification::centredRight, 8.0f, 0.3f);
    }
}

void MainView::resized()
{
    const int x0 = 16, w = W - 32;
    headerArea = { x0, 14, w, 52 };
    screensArea = { x0, 76, w, 142 };
    controlsArea = { x0, 228, w, 282 };
    ctrlArea = { x0, 520, 548, H - 520 - 30 };
    patchArea = { x0 + 558, 520, w - 558, H - 520 - 30 };

    // cabeceira
    {
        int x = headerArea.getX() + 210;
        const int y = headerArea.getY() + 10, h = 32;
        bankBtn.setBounds (x, y, 230, h); x += 246;
        chartBtn.setBounds (x, y, 86, h); openBtn.setBounds (x + 90, y, 78, h); saveBtn.setBounds (x + 172, y, 78, h);
        int r = headerArea.getRight() - 10;
        funcBtn.setBounds (r - 104, y, 104, h); r -= 116;
        transpBtn.setBounds (r - 122, y, 122, h); r -= 140;
        unison.setBounds (r - 96, y, 96, h); poly2.setBounds (r - 184, y, 84, h); poly1.setBounds (r - 272, y, 84, h);
    }

    screens.setBounds (screensArea);
    chart.setBounds (screensArea.getX(), screensArea.getY(), screensArea.getWidth(), controlsArea.getBottom() - screensArea.getY());

    // seccións do panel
    const int sx = controlsArea.getX() + 14, sy = controlsArea.getY() + 12, sh = controlsArea.getHeight() - 22;
    const int widths[6] = { 104, 368, 58, 282, 112, 190 };
    int x = sx;
    for (int i = 0; i < 6; ++i) { sections[(size_t) i] = { x, sy, widths[i], sh }; x += widths[i]; }
    sections[6] = { x, sy, controlsArea.getRight() - 14 - x, sh };

    const int fy = sy + 34, fh = sh - 40, fw = 46;
    auto place = [&] (int idx, int cx) { faders[idx]->setBounds (cx - fw / 2, fy, fw, fh); };
    auto stack = [&] (std::initializer_list<juce::TextButton*> keys, int cx, int width)
    {
        int ky = fy + 26;
        for (auto* k : keys) { k->setBounds (cx - width / 2, ky, width, 28); ky += 34; }
    };
    // LFO
    place (0, sections[0].getX() + 28); place (1, sections[0].getX() + 74);
    // DCO
    {
        const int d = sections[1].getX();
        stack ({ &range16, &range8, &range4 }, d + 32, 50);
        place (2, d + 86); place (3, d + 132);
        stack ({ &pwmLfo, &pwmMan }, d + 184, 50);
        stack ({ &pulseBtn, &sawBtn }, d + 244, 62);
        place (4, d + 300); place (5, d + 346);
    }
    // HPF
    place (6, sections[2].getX() + 30);
    // VCF
    {
        const int v = sections[3].getX();
        place (7, v + 28); place (8, v + 74);
        stack ({ &envPos, &envNeg }, v + 120, 40);
        place (9, v + 166); place (10, v + 212); place (11, v + 258);
    }
    // VCA
    stack ({ &vcaEnv, &vcaGate }, sections[4].getX() + 32, 54);
    place (12, sections[4].getX() + 86);
    // ENV
    for (int i = 0; i < 4; ++i) place (13 + i, sections[5].getX() + 28 + i * 46);
    // CHORUS
    stack ({ &chorusOff, &chorusI, &chorusII }, sections[6].getCentreX(), juce::jmin (72, sections[6].getWidth() - 16));

    // a fila dos sons
    {
        // dúas filas: grupo e banco (e WRITE); escoitar, número (e MANUAL)
        const int h = 30, y1 = patchArea.getY() + 24, y2 = y1 + 50;
        const int gx = patchArea.getX() + 78, bx = gx + 96 + 92;
        groupBtn[0].setBounds (gx, y1, 46, h); groupBtn[1].setBounds (gx + 50, y1, 46, h);
        listenBtn.setBounds (gx, y2, 96, h);
        for (int i = 0; i < 8; ++i) { bankBtns[i]->setBounds (bx + i * 42, y1, 38, h); patchBtns[i]->setBounds (bx + i * 42, y2, 38, h); }
        const int wx = bx + 8 * 42 + 14;
        writeBtn.setBounds (wx, y1, patchArea.getRight() - 12 - wx, h);
        manualBtn.setBounds (wx, y2, patchArea.getRight() - 12 - wx, h);
    }

    // á esquerda do teclado
    {
        // unha soa fila: xeral (volume, afinación, portamento) e bender (tres mandos e a panca)
        const int cx = ctrlArea.getX() + 10, kw = 66, kh = 96, y = ctrlArea.getY() + 28;
        volume.setBounds (cx, y, kw, kh); tune.setBounds (cx + kw, y, kw, kh); porta.setBounds (cx + 2 * kw, y, kw, kh);
        const int bx = cx + 3 * kw + 18;
        bendDco.setBounds (bx, y, kw, kh); bendVcf.setBounds (bx + kw, y, kw, kh); lfoMod.setBounds (bx + 2 * kw, y, kw, kh);
        portaOn.setBounds (porta.getX() + 2, ctrlArea.getY() + 6, kw - 4, 20);
        lever.setBounds (bx + 3 * kw + 8, y + 18, ctrlArea.getRight() - 10 - (bx + 3 * kw + 8), 48);
    }
    newVersion.setBounds (W / 2 - 170, H - 27, 340, 22);
}
} // namespace ui

// ================================================================ xanela (escalable)
BXEditor::BXEditor (BXProcessor& p) : AudioProcessorEditor (&p), proc (p), main (p)
{
    addAndMakeVisible (main);
    setResizable (true, true);
    setResizeLimits (ui::MainView::W * 5 / 10, ui::MainView::H * 5 / 10, ui::MainView::W * 2, ui::MainView::H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) ui::MainView::W / ui::MainView::H);
    float s = proc.uiScale;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        const auto ua = d->userArea;
        const float fit = juce::jmin ((float) (ua.getWidth() - 40) / (float) ui::MainView::W, (float) (ua.getHeight() - 90) / (float) ui::MainView::H);
        if (s > fit && fit > 0.4f) s = fit;
    }
    setSize (juce::roundToInt (ui::MainView::W * s), juce::roundToInt (ui::MainView::H * s));
}

void BXEditor::resized()
{
    const float s = (float) getWidth() / (float) ui::MainView::W;
    main.setBounds (0, 0, ui::MainView::W, ui::MainView::H);
    main.setTransform (juce::AffineTransform::scale (s));
    proc.uiScale = s;
}
