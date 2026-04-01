#include "HeaderComponent.h"
#include "../OrionIcons.h"
#include "../OrionLookAndFeel.h"
#include "../UiHints.h"
#include "../ui/OrionDesignSystem.h"

#include <array>

#if JUCE_LINUX || defined(__linux__)
#include <fstream>
#include <unistd.h>
#elif JUCE_WINDOWS || defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif JUCE_MAC || defined(__APPLE__)
#include <mach/mach.h>
#endif

// Helper for RAM usage (Cross-platform)
static long getRamUsageMB() {
#if JUCE_LINUX || defined(__linux__)
    long rss = 0;
    std::ifstream statm("/proc/self/statm");
    if (statm.is_open()) {
        long ignore;
        statm >> ignore >> rss;
        statm.close();
        long pageSize = sysconf(_SC_PAGESIZE);
        return (rss * pageSize) / (1024 * 1024);
    }
#elif JUCE_WINDOWS || defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return (long)(pmc.WorkingSetSize / (1024 * 1024));
    }
#elif JUCE_MAC || defined(__APPLE__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS) {
        return (long)(info.resident_size / (1024 * 1024));
    }
#endif
    return 0;
}

class HeaderComponent::LiveWaveformComponent : public juce::Component, public juce::Timer, public juce::TooltipClient
{
public:
    LiveWaveformComponent(Orion::AudioEngine& e) : engine(e)
    {
        startTimerHz(60);
    }

    juce::String getTooltip() override
    {
        return tooltipText;
    }

    void setTooltip(const juce::String& text)
    {
        tooltipText = text;
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        showMeter = !showMeter;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.saveState();
        g.reduceClipRegion(bounds.toNearestInt());


        juce::Colour lcdBg = findColour(OrionLookAndFeel::lcdBackgroundColourId);

        // Complex LCD background gradient
        juce::ColourGradient bgGrad(lcdBg.brighter(0.08f), 0, bounds.getY(),
                                    lcdBg.darker(0.15f), 0, bounds.getBottom(), false);
        bgGrad.addColour(0.5, lcdBg);
        g.setGradientFill(bgGrad);
        g.fillRoundedRectangle(bounds, 4.0f);

        // Scanlines
        g.setColour(juce::Colours::black.withAlpha(0.22f));
        for (float y = 0; y < bounds.getHeight(); y += 2.0f) {
            g.drawHorizontalLine((int)y, 0.0f, bounds.getWidth());
        }

        // Inner bezel / glow
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.5f);

        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 3.5f, 1.0f);

        if (showMeter)
        {
            drawMeter(g, bounds);
        }
        else
        {
            drawWaveform(g, bounds);
        }


        juce::ColourGradient glass(juce::Colours::white.withAlpha(0.05f), 0, bounds.getY(),
                                   juce::Colours::transparentBlack, 0, bounds.getCentreY(), false);
        g.setGradientFill(glass);
        g.fillRoundedRectangle(bounds, 4.0f);

        g.restoreState();
    }

    void drawWaveform(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        auto textColor = findColour(OrionLookAndFeel::lcdTextColourId);

        // Grid
        g.setColour(textColor.withAlpha(0.08f));
        for (float x = 0; x < bounds.getWidth(); x += 20.0f)
            g.drawVerticalLine((int)x, 0.0f, bounds.getHeight());

        g.setColour(textColor.withAlpha(0.15f));
        g.drawHorizontalLine((int)bounds.getCentreY(), 0.0f, bounds.getWidth());

        if (!scopeData.empty())
        {
            juce::Path p;
            float centerY = bounds.getCentreY();
            float height = bounds.getHeight();
            float xInc = bounds.getWidth() / (float)scopeData.size();
            float yScale = height * 0.45f;

            p.startNewSubPath(0, centerY);
            for (size_t i = 0; i < scopeData.size(); ++i) {
                p.lineTo((float)i * xInc, centerY - (scopeData[i] * yScale));
            }

            // Glow effect
            g.setColour(textColor.withAlpha(0.25f));
            g.strokePath(p, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            g.setColour(textColor.withAlpha(0.5f));
            g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            g.setColour(textColor);
            g.strokePath(p, juce::PathStrokeType(0.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Fill area below waveform for a modern look
            juce::Path fillP = p;
            fillP.lineTo(bounds.getRight(), bounds.getBottom());
            fillP.lineTo(bounds.getX(), bounds.getBottom());
            fillP.closeSubPath();

            juce::ColourGradient fillGrad(textColor.withAlpha(0.15f), 0, bounds.getY(),
                                          juce::Colours::transparentBlack, 0, bounds.getBottom(), false);
            g.setGradientFill(fillGrad);
            g.fillPath(fillP);
        }
    }

    void drawMeter(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        float levelL = 0.0f;
        float levelR = 0.0f;

        if (auto* master = engine.getMasterNode()) {
            levelL = master->getLevelL();
            levelR = master->getLevelR();
        }

        float h = bounds.getHeight();
        float w = bounds.getWidth();
        float barH = h * 0.28f;
        float gap = h * 0.08f;
        float yL = h * 0.15f;
        float yR = yL + barH + gap;

        // Theme colors
        juce::Colour meterBg = findColour(OrionLookAndFeel::meterBackgroundColourId);
        juce::Colour cStart = findColour(OrionLookAndFeel::meterGradientStartColourId);
        juce::Colour cMid = findColour(OrionLookAndFeel::meterGradientMidColourId);
        juce::Colour cEnd = findColour(OrionLookAndFeel::meterGradientEndColourId);

        auto drawBar = [&](float y, float level) {
             float db = juce::Decibels::gainToDecibels(level);
             if (db < -60.0f) db = -60.0f;
             float norm = juce::jmap(db, -60.0f, 6.0f, 0.0f, 1.0f);
             norm = juce::jlimit(0.0f, 1.0f, norm);

             float width = w * norm;

             g.setColour(meterBg.withAlpha(0.6f));
             g.fillRoundedRectangle(0, y, w, barH, 3.0f);

             if (width > 0) {
                 juce::ColourGradient grad(cStart, 0, y, cEnd, w, y, false);
                 grad.addColour(0.7, cMid);
                 g.setGradientFill(grad);
                 g.fillRoundedRectangle(0, y, width, barH, 3.0f);

                 // Highlight
                 g.setColour(juce::Colours::white.withAlpha(0.2f));
                 g.fillRect(0.0f, y, width, barH * 0.3f);
             }
        };

        drawBar(yL, levelL);
        drawBar(yR, levelR);

        // Labels and dB ticks
        g.setColour(findColour(OrionLookAndFeel::lcdTextColourId).withAlpha(0.8f));
        g.setFont(Orion::UI::DesignSystem::instance().fonts().mono(9.0f, juce::Font::plain));

        g.drawText("L", 2, (int)yL, 10, (int)barH, juce::Justification::centredLeft, false);
        g.drawText("R", 2, (int)yR, 10, (int)barH, juce::Justification::centredLeft, false);

        float dbMarks[] = { 0.0f, -12.0f, -24.0f, -48.0f };
        for (float db : dbMarks) {
            float norm = juce::jmap(db, -60.0f, 6.0f, 0.0f, 1.0f);
            float x = norm * w;
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.drawVerticalLine((int)x, yL, yR + barH);

            g.setColour(findColour(OrionLookAndFeel::lcdLabelColourId).withAlpha(0.6f));
            g.drawText(juce::String((int)db), (int)x - 10, (int)bounds.getBottom() - 10, 20, 10, juce::Justification::centred, false);
        }
    }

    void timerCallback() override
    {
        if (!showMeter) {
            engine.getScopeData(scopeData);
        }
        repaint();
    }

private:
    Orion::AudioEngine& engine;
    std::vector<float> scopeData;
    bool showMeter = false;
    juce::String tooltipText;
};

class HeaderComponent::PerformanceDisplayComponent : public juce::Component, public juce::Timer
{
public:
    PerformanceDisplayComponent(Orion::AudioEngine& e) : engine(e)
    {
        startTimerHz(10);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto textColor = findColour(OrionLookAndFeel::lcdTextColourId);
        auto labelColor = findColour(OrionLookAndFeel::lcdLabelColourId);

        auto drawMetricCard = [&](juce::Rectangle<float> card,
                                  const juce::String& label,
                                  const juce::String& value,
                                  float normalized,
                                  juce::Colour baseColour,
                                  bool warn)
        {
            auto radius = 3.0f;
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.fillRoundedRectangle(card, radius);

            juce::Colour accent = warn ? juce::Colour(0xFFFF453A) : baseColour;
            auto body = card.reduced(3.0f, 2.0f);

            auto topArea = body.removeFromTop(body.getHeight() * 0.5f);
            auto titleArea = topArea.removeFromLeft(topArea.getWidth() * 0.4f);
            auto valueArea = topArea;

            // Label
            g.setColour(labelColor.withAlpha(0.6f));
            g.setFont(Orion::UI::DesignSystem::instance().fonts().mono(9.0f, juce::Font::plain));
            g.drawText(label, titleArea.toNearestInt(), juce::Justification::centredLeft, false);

            // Value
            g.setColour(warn ? juce::Colour(0xFFFF6B64) : textColor.withAlpha(0.9f));
            g.setFont(Orion::UI::DesignSystem::instance().fonts().mono(9.0f, juce::Font::bold));
            g.drawText(value, valueArea.toNearestInt(), juce::Justification::centredRight, false);

            // Bar
            normalized = juce::jlimit(0.0f, 1.0f, normalized);
            auto track = body.reduced(0.0f, 2.5f);
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.fillRoundedRectangle(track, 1.0f);

            auto fill = track.withWidth(track.getWidth() * normalized);
            if (!fill.isEmpty())
            {
                juce::ColourGradient barGrad(accent.brighter(0.3f), fill.getX(), fill.getY(), accent.darker(0.1f), fill.getRight(), fill.getBottom(), false);
                g.setGradientFill(barGrad);
                g.fillRoundedRectangle(fill, 1.0f);
            }
        };

        double avgLoad = engine.getCpuLoad();
        double rtLoad = engine.getRtLoad();
        long ram = getRamUsageMB();

        juce::String srStr = juce::String((int)(engine.getSampleRate() / 1000)) + "k";
        juce::String bufStr = juce::String((int)engine.getBlockSize());
        float diskLevel = (float)diskIndicator / 10.0f;

        struct Metric { juce::String label; juce::String value; float meter; juce::Colour colour; bool warn; };
        std::array<Metric, 6> metrics{{
            { "RT",  juce::String((int)(rtLoad * 100)) + "%", (float)rtLoad, juce::Colour(0xFF0A84FF), rtLoad > 0.9 },
            { "CPU", juce::String((int)(avgLoad * 100)) + "%", (float)avgLoad, juce::Colour(0xFF5E5CE6), avgLoad > 0.8 },
            { "RAM", juce::String(ram) + "MB", juce::jlimit(0.0f, 1.0f, (float)ram / 4096.0f), juce::Colour(0xFFFF9F0A), ram > 3072 },
            { "DSK", diskIndicator > 0 ? "ACT" : "IDL", diskLevel, juce::Colour(0xFF30D158), false },
            { "SR",  srStr, 1.0f, juce::Colour(0xFFBF5AF2), false },
            { "BUF", bufStr, 1.0f, juce::Colour(0xFFFF375F), false }
        }};

        auto content = bounds.reduced(4.0f, 4.0f);
        float gapX = 4.0f;
        float gapY = 4.0f;
        float cardW = (content.getWidth() - gapX) / 2.0f;
        float cardH = (content.getHeight() - gapY * 2.0f) / 3.0f;

        for (int i = 0; i < 6; ++i)
        {
            int col = i / 3;
            int row = i % 3;
            auto card = juce::Rectangle<float>(content.getX() + col * (cardW + gapX),
                                               content.getY() + row * (cardH + gapY),
                                               cardW, cardH);
            drawMetricCard(card, metrics[(size_t)i].label, metrics[(size_t)i].value, metrics[(size_t)i].meter, metrics[(size_t)i].colour, metrics[(size_t)i].warn);
        }

        if (diskIndicator > 0)
            --diskIndicator;

        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawRoundedRectangle(bounds.reduced(0.6f), 5.0f, 1.0f);
    }

    void timerCallback() override
    {
        int currentDisk = Orion::gDiskActivityCounter.load();
        if (currentDisk != lastDiskCounter) {
            diskIndicator = 10;
            lastDiskCounter = currentDisk;
        }
        repaint();
    }

private:
    Orion::AudioEngine& engine;
    int lastDiskCounter = 0;
    int diskIndicator = 0;
};


class TransportButton : public juce::Button
{
public:
    TransportButton(const juce::String& name, const juce::Path& iconPath, juce::Colour iconColour)
        : juce::Button(name), path(iconPath), baseIconColour(iconColour)
    {
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsMouseOver, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.5f);
        bool isDown = shouldDrawButtonAsDown || getToggleState();
        bool isHover = shouldDrawButtonAsMouseOver;

        juce::Colour base = findColour(OrionLookAndFeel::transportButtonBackgroundColourId);
        float corner = 6.0f;

        // Button Body
        juce::ColourGradient grad(base.brighter(isHover ? 0.08f : 0.04f), 0, bounds.getY(),
                                   base.darker(isDown ? 0.25f : 0.1f), 0, bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, corner);

        // Outer Shadow/Border
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.drawRoundedRectangle(bounds, corner, 1.0f);

        // Inner Highlight
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawRoundedRectangle(bounds.reduced(1.0f), corner - 1.0f, 1.0f);

        auto iconBounds = isDown ? bounds.reduced(9.5f).translated(0, 0.5f) : bounds.reduced(9.5f);
        juce::Colour iconC = findColour(OrionLookAndFeel::transportButtonSymbolColourId);

        if (!isEnabled()) iconC = iconC.withAlpha(0.3f);
        else if (getToggleState()) {
             iconC = baseIconColour;
             // Glow
             g.setColour(iconC.withAlpha(0.25f));
             g.fillEllipse(iconBounds.expanded(5.0f));
        }
        else if (isHover) {
             iconC = iconC.brighter(0.4f);
        }

        g.setColour(iconC);

        if (iconBounds.getWidth() > 0 && iconBounds.getHeight() > 0) {
            auto pathBounds = path.getBounds();
            float scale = juce::jmin(iconBounds.getWidth() / pathBounds.getWidth(), iconBounds.getHeight() / pathBounds.getHeight());

            auto transform = juce::AffineTransform::translation(-pathBounds.getX(), -pathBounds.getY())
                                            .scaled(scale)
                                            .translated(iconBounds.getX() + (iconBounds.getWidth() - pathBounds.getWidth() * scale) * 0.5f,
                                                        iconBounds.getY() + (iconBounds.getHeight() - pathBounds.getHeight() * scale) * 0.5f);

            g.fillPath(path, transform);
        }
    }

private:
    juce::Path path;
    juce::Colour baseIconColour;
};


class ToolButton : public juce::Button
{
public:
    ToolButton(const juce::String& name, const juce::Path& iconPath)
        : juce::Button(name), path(iconPath)
    {
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsMouseOver, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.5f);
        bool isDown = shouldDrawButtonAsDown || getToggleState();
        bool isHover = shouldDrawButtonAsMouseOver;

        juce::Colour base = findColour(OrionLookAndFeel::toolButtonBackgroundColourId);
        float corner = 5.0f;

        if (isDown) {
            juce::Colour activeCol = findColour(juce::Slider::rotarySliderFillColourId);
            juce::ColourGradient grad(activeCol.darker(0.1f), 0, bounds.getY(), activeCol.darker(0.4f), 0, bounds.getBottom(), false);
            g.setGradientFill(grad);
        } else {
            juce::ColourGradient grad(base.brighter(isHover ? 0.08f : 0.04f), 0, bounds.getY(), base.darker(0.12f), 0, bounds.getBottom(), false);
            g.setGradientFill(grad);
        }

        g.fillRoundedRectangle(bounds, corner);

        // Border
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.drawRoundedRectangle(bounds, corner, 1.0f);

        if (!isDown) {
            g.setColour(juce::Colours::white.withAlpha(0.06f));
            g.drawRoundedRectangle(bounds.reduced(1.0f), corner - 1.0f, 1.0f);
        }

        auto iconBounds = isDown ? bounds.reduced(7.5f).translated(0, 0.5f) : bounds.reduced(7.5f);

        juce::Colour iconC = isDown ? juce::Colours::white : findColour(OrionLookAndFeel::transportButtonSymbolColourId).withAlpha(0.8f);

        if (isDown) {
             // Already set to white
        } else if (isHover) {
             iconC = iconC.brighter(0.4f);
        }

        g.setColour(iconC);
        auto pathBounds = path.getBounds();
        float scale = juce::jmin(iconBounds.getWidth() / pathBounds.getWidth(), iconBounds.getHeight() / pathBounds.getHeight());
        auto transform = juce::AffineTransform::translation(-pathBounds.getX(), -pathBounds.getY())
                                        .scaled(scale)
                                        .translated(iconBounds.getX() + (iconBounds.getWidth() - pathBounds.getWidth() * scale) * 0.5f,
                                                    iconBounds.getY() + (iconBounds.getHeight() - pathBounds.getHeight() * scale) * 0.5f);

        g.fillPath(path, transform);
    }
private:
    juce::Path path;
};

HeaderComponent::HeaderComponent(Orion::AudioEngine& e, Orion::CommandManager& cm)
    : engine(e), commandManager(cm)
{

    waveform = std::make_unique<LiveWaveformComponent>(engine);
    addAndMakeVisible(waveform.get());
    waveform->setTooltip("Master Output Waveform / Level Meter (Click to Toggle)");



    createButton(playButton,
                 Orion::UiHints::withShortcut("Play or Pause", Orion::CommandManager::PlayStop, commandManager.getApplicationCommandManager()),
                 OrionIcons::getPlayIcon(),
                 Orion::CommandManager::PlayStop,
                 juce::Colour(0xFF30D158));
    createButton(stopButton,
                 Orion::UiHints::withShortcut("Stop Playback", Orion::CommandManager::Stop, commandManager.getApplicationCommandManager()),
                 OrionIcons::getStopIcon(),
                 Orion::CommandManager::Stop,
                 juce::Colour(0xFFAAAAAA));
    createButton(recButton,
                 Orion::UiHints::withShortcut("Record", Orion::CommandManager::Record, commandManager.getApplicationCommandManager()),
                 OrionIcons::getRecordIcon(),
                 Orion::CommandManager::Record,
                 juce::Colour(0xFFFF453A));
    createButton(loopButton,
                 Orion::UiHints::withShortcut("Toggle Loop", Orion::CommandManager::ToggleLoop, commandManager.getApplicationCommandManager()),
                 OrionIcons::getLoopIcon(),
                 Orion::CommandManager::ToggleLoop,
                 juce::Colour(0xFF0A84FF));


    createButton(rewindButton,
                 Orion::UiHints::withShortcut("Rewind", Orion::CommandManager::Rewind, commandManager.getApplicationCommandManager()),
                 OrionIcons::getRewindIcon(),
                 Orion::CommandManager::Rewind,
                 juce::Colours::grey);

    createButton(ffButton,
                 Orion::UiHints::withShortcut("Fast Forward", Orion::CommandManager::FastForward, commandManager.getApplicationCommandManager()),
                 OrionIcons::getFastForwardIcon(),
                 Orion::CommandManager::FastForward,
                 juce::Colours::grey);

    createButton(panicButton,
                 Orion::UiHints::withShortcut("MIDI Panic (Stop All Notes)", Orion::CommandManager::Panic, commandManager.getApplicationCommandManager()),
                 OrionIcons::getPanicIcon(),
                 Orion::CommandManager::Panic,
                 juce::Colour(0xFFFF9F0A));

    createButton(metroButton,
                 Orion::UiHints::withShortcut("Metronome", Orion::CommandManager::ToggleClick, commandManager.getApplicationCommandManager()),
                 OrionIcons::getClickIcon(),
                 Orion::CommandManager::ToggleClick,
                 juce::Colour(0xFFFFD60A));


    auto createTool = [&](std::unique_ptr<juce::Button>& btn, const juce::String& tip, const juce::Path& icon, int cmd) {
        btn = std::make_unique<ToolButton>("tool", icon);
        btn->setCommandToTrigger(&commandManager.getApplicationCommandManager(), cmd, true);
        btn->setClickingTogglesState(true);
        btn->setRadioGroupId(1001);
        btn->setTooltip(Orion::UiHints::withShortcut(tip, cmd, commandManager.getApplicationCommandManager()));
        addAndMakeVisible(btn.get());
    };

    createTool(toolSelect, "Select Tool", OrionIcons::getSelectIcon(), Orion::CommandManager::ToolSelect);
    createTool(toolDraw, "Draw Tool", OrionIcons::getDrawIcon(), Orion::CommandManager::ToolDraw);
    createTool(toolErase, "Erase Tool", OrionIcons::getEraseIcon(), Orion::CommandManager::ToolErase);
    createTool(toolSplit, "Split/Blade Tool", OrionIcons::getBladeIcon(), Orion::CommandManager::ToolSplit);
    createTool(toolGlue, "Glue Tool", OrionIcons::getGlueIcon(), Orion::CommandManager::ToolGlue);
    createTool(toolZoom, "Zoom Tool", OrionIcons::getZoomIcon(), Orion::CommandManager::ToolZoom);
    createTool(toolMute, "Mute Tool", OrionIcons::getMuteToolIcon(), Orion::CommandManager::ToolMute);

    createTool(toolListen, "Listen Tool", OrionIcons::getListenIcon(), Orion::CommandManager::ToolListen);

    // Initialize tool state
    if (toolSelect) toolSelect->setToggleState(true, juce::dontSendNotification);

    perfDisplay = std::make_unique<PerformanceDisplayComponent>(engine);
    addAndMakeVisible(perfDisplay.get());


    auto setupLCDLabel = [](juce::Label& lbl, float size, juce::Colour c, bool bold = true) {
        lbl.setFont(Orion::UI::DesignSystem::instance().fonts().mono(size, bold ? juce::Font::bold : juce::Font::plain));
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setColour(juce::Label::textColourId, c);
        lbl.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.25f));
        lbl.setColour(juce::Label::outlineColourId, juce::Colours::white.withAlpha(0.05f));
        lbl.setColour(juce::Label::textWhenEditingColourId, c);
    };


    addAndMakeVisible(timeDisplay);
    setupLCDLabel(timeDisplay, 22.0f, juce::Colours::white);
    timeDisplay.setText("00:00:00.000", juce::dontSendNotification);
    timeDisplay.setTooltip("Playback Time (MM:SS:ms)");


    addAndMakeVisible(keyDisplay);
    setupLCDLabel(keyDisplay, 14.0f, juce::Colours::grey, false);
    keyDisplay.setText(engine.getKeySignature(), juce::dontSendNotification);
    keyDisplay.setEditable(false);
    keyDisplay.setTooltip("Key Signature (Click to Change)");
    keyDisplay.addMouseListener(this, false);


    addAndMakeVisible(bpmDisplay);
    setupLCDLabel(bpmDisplay, 16.0f, juce::Colours::white);
    bpmDisplay.setText(juce::String(engine.getBpm()), juce::dontSendNotification);
    bpmDisplay.setEditable(true);
    bpmDisplay.onTextChange = [this] {
        float b = bpmDisplay.getText().getFloatValue();
        if (b > 10.0f && b < 500.0f) {
            engine.setBpm(b);
        } else {
            bpmDisplay.setText(juce::String(engine.getBpm()), juce::dontSendNotification);
        }
    };
    bpmDisplay.setTooltip("Tempo (BPM) (Drag to change, Click to Edit)");

    bpmDisplay.addMouseListener(this, false);

    addAndMakeVisible(bpmLabel);
    bpmLabel.setText("BPM", juce::dontSendNotification);
    setupLCDLabel(bpmLabel, 9.0f, juce::Colours::grey);

    startTimerHz(30);
}

HeaderComponent::~HeaderComponent()
{
}

void HeaderComponent::createButton(std::unique_ptr<juce::Button>& btn, const juce::String& tooltip, const juce::Path& icon, int commandID, juce::Colour colour)
{
    btn = std::make_unique<TransportButton>("btn", icon, colour);
    btn->setCommandToTrigger(&commandManager.getApplicationCommandManager(), commandID, true);
    btn->setClickingTogglesState(false);
    btn->setTooltip(tooltip);
    addAndMakeVisible(btn.get());
}

void HeaderComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::Colour base = findColour(OrionLookAndFeel::mixerChassisColourId).darker(0.2f);

    // Main background with subtle vertical gradient
    juce::ColourGradient bgGrad(base.brighter(0.02f), 0, 0, base.darker(0.05f), 0, bounds.getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillAll();

    // Top and bottom highlights
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawHorizontalLine(0, 0.0f, bounds.getWidth());
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.drawHorizontalLine((int)bounds.getHeight() - 1, 0.0f, bounds.getWidth());

    // Draw "Islands" backgrounds
    auto drawIsland = [&](juce::Rectangle<float> r) {
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.fillRoundedRectangle(r.reduced(2.0f, 4.0f), 6.0f);

        // Subtle Inner Shadow / Highlight
        g.setColour(juce::Colours::white.withAlpha(0.04f));
        g.drawRoundedRectangle(r.reduced(2.0f, 4.0f), 6.0f, 1.2f);

        g.setColour(juce::Colours::black.withAlpha(0.15f));
        g.drawRoundedRectangle(r.reduced(3.0f, 5.0f), 5.0f, 1.0f);
    };

    // Island Dimensions (Matching resized())
    const float toolsWidth = 280.0f;
    const float rightWidth = 320.0f;
    const float centerWidth = 480.0f;

    // Tools Island (Left)
    drawIsland(bounds.removeFromLeft(toolsWidth + 8).reduced(4, 0));

    // Right Island (Waveform & Perf)
    drawIsland(bounds.removeFromRight(rightWidth + 8).reduced(4, 0));

    // Center Island (Transport & LCD)
    float centerX = (getLocalBounds().getWidth() - centerWidth) * 0.5f;
    drawIsland(juce::Rectangle<float>(centerX, 0, centerWidth, bounds.getHeight()));
}

void HeaderComponent::resized()
{
    auto area = getLocalBounds().reduced(8, 0);
    const int toolsWidth = 280;
    const int rightWidth = 320;
    const int centerWidth = 480;

    // 1. Tools Area (Left)
    auto toolsArea = area.removeFromLeft(toolsWidth);
    juce::FlexBox toolsBox;
    toolsBox.justifyContent = juce::FlexBox::JustifyContent::center;
    toolsBox.alignItems = juce::FlexBox::AlignItems::center;

    int toolSize = 32;
    auto addTool = [&](juce::Button* b) {
        if (b) toolsBox.items.add(juce::FlexItem(*b).withWidth((float)toolSize).withHeight((float)toolSize).withMargin({0.0f, 3.0f, 0.0f, 0.0f}));
    };

    addTool(toolSelect.get());
    addTool(toolDraw.get());
    addTool(toolErase.get());
    addTool(toolSplit.get());
    addTool(toolGlue.get());
    addTool(toolZoom.get());
    addTool(toolMute.get());
    addTool(toolListen.get());
    toolsBox.performLayout(toolsArea.reduced(4, 0));

    // 2. Right Area (Waveform & Performance)
    auto rightArea = area.removeFromRight(rightWidth);
    juce::FlexBox rightBox;
    rightBox.justifyContent = juce::FlexBox::JustifyContent::center;
    rightBox.alignItems = juce::FlexBox::AlignItems::center;

    if (waveform)
        rightBox.items.add(juce::FlexItem(*waveform).withWidth(170.0f).withHeight((float)getHeight() - 12.0f).withMargin({0, 4, 0, 4}));

    if (perfDisplay)
        rightBox.items.add(juce::FlexItem(*perfDisplay).withWidth(130.0f).withHeight((float)getHeight() - 12.0f).withMargin({0, 4, 0, 4}));

    rightBox.performLayout(rightArea.reduced(4, 0));

    // 3. Center Area (Transport + LCD)
    auto centerArea = getLocalBounds().withSizeKeepingCentre(centerWidth, getHeight()).reduced(4, 0);

    juce::FlexBox centerBox;
    centerBox.justifyContent = juce::FlexBox::JustifyContent::center;
    centerBox.alignItems = juce::FlexBox::AlignItems::center;

    // Transport sub-group
    juce::FlexBox transBox;
    transBox.justifyContent = juce::FlexBox::JustifyContent::center;
    transBox.alignItems = juce::FlexBox::AlignItems::center;

    int tBtnSize = 34;
    auto addTrans = [&](juce::Component* c, int w) {
        if (c) transBox.items.add(juce::FlexItem(*c).withWidth((float)w).withHeight((float)tBtnSize).withMargin(1.0f));
    };

    addTrans(rewindButton.get(), tBtnSize);
    addTrans(stopButton.get(), tBtnSize);
    addTrans(playButton.get(), tBtnSize);
    addTrans(recButton.get(), tBtnSize);
    addTrans(ffButton.get(), tBtnSize);
    addTrans(loopButton.get(), tBtnSize);
    addTrans(metroButton.get(), tBtnSize);

    centerBox.items.add(juce::FlexItem(transBox).withWidth(260.0f).withHeight((float)getHeight()));

    // Separator
    centerBox.items.add(juce::FlexItem().withWidth(10.0f));

    // LCD sub-group
    juce::FlexBox lcdBox;
    lcdBox.flexDirection = juce::FlexBox::Direction::row;
    lcdBox.alignItems = juce::FlexBox::AlignItems::center;

    lcdBox.items.add(juce::FlexItem(timeDisplay).withWidth(110.0f).withHeight(30.0f));

    juce::FlexBox bpmKeyBox;
    bpmKeyBox.flexDirection = juce::FlexBox::Direction::column;
    bpmKeyBox.justifyContent = juce::FlexBox::JustifyContent::center;
    bpmKeyBox.items.add(juce::FlexItem(bpmDisplay).withWidth(50.0f).withHeight(16.0f));
    bpmKeyBox.items.add(juce::FlexItem(keyDisplay).withWidth(50.0f).withHeight(14.0f));

    lcdBox.items.add(juce::FlexItem(bpmKeyBox).withWidth(50.0f).withHeight(30.0f).withMargin({0, 4, 0, 4}));

    centerBox.items.add(juce::FlexItem(lcdBox).withWidth(170.0f).withHeight(30.0f));

    centerBox.performLayout(centerArea);
}

void HeaderComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.eventComponent == &bpmDisplay)
    {
        bpmAtDragStart = engine.getBpm();
    }
    else if (e.eventComponent == &keyDisplay)
    {
        auto wheel = std::make_unique<KeyWheelComponent>(engine, [this] {
            keyDisplay.setText(engine.getKeySignature(), juce::dontSendNotification);
        });

        juce::CallOutBox::launchAsynchronously(std::move(wheel), keyDisplay.getScreenBounds(), nullptr);
    }
}

void HeaderComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (e.eventComponent == &bpmDisplay)
    {
        float delta = (float)-e.getDistanceFromDragStartY();
        float newBpm = bpmAtDragStart + (delta * 0.1f);
        newBpm = juce::jlimit(10.0f, 500.0f, newBpm);
        engine.setBpm(newBpm);
        bpmDisplay.setText(juce::String(newBpm, 1), juce::dontSendNotification);
    }
}

void HeaderComponent::timerCallback()
{

    uint64_t frame = engine.getCursor();
    double sr = engine.getSampleRate();
    if (sr > 0) {
        double seconds = frame / sr;
        int min = (int)(seconds / 60.0);
        int sec = (int)(seconds) % 60;
        int ms = (int)((seconds - (int)seconds) * 1000.0);

        juce::String timeStr = juce::String::formatted("%02d:%02d:%03d", min, sec, ms);
        timeDisplay.setText(timeStr, juce::dontSendNotification);
    }


    timeDisplay.setColour(juce::Label::textColourId, findColour(OrionLookAndFeel::lcdTextColourId));
    keyDisplay.setColour(juce::Label::textColourId, findColour(OrionLookAndFeel::lcdLabelColourId).brighter(0.5f));
    bpmDisplay.setColour(juce::Label::textColourId, findColour(OrionLookAndFeel::lcdLabelColourId).brighter(0.8f));
    bpmLabel.setColour(juce::Label::textColourId, findColour(OrionLookAndFeel::lcdLabelColourId));

    if (!bpmDisplay.isBeingEdited()) {
         float engBpm = engine.getBpm();
         float dispBpm = bpmDisplay.getText().getFloatValue();
         if (std::abs(engBpm - dispBpm) > 0.01f) {
             bpmDisplay.setText(juce::String(engBpm, 2), juce::dontSendNotification);
         }
    }

    if (!keyDisplay.isBeingEdited()) {
        if (keyDisplay.getText() != juce::String(engine.getKeySignature())) {
            keyDisplay.setText(engine.getKeySignature(), juce::dontSendNotification);
        }
    }

    if (recButton) {
        bool rec = engine.getTransportState() == Orion::TransportState::Recording;
        if (recButton->getToggleState() != rec) recButton->setToggleState(rec, juce::dontSendNotification);
    }
    if (loopButton) {
        bool loop = engine.isLooping();
        if (loopButton->getToggleState() != loop) loopButton->setToggleState(loop, juce::dontSendNotification);
    }
    if (metroButton) {
        bool metro = engine.isMetronomeEnabled();
        if (metroButton->getToggleState() != metro) metroButton->setToggleState(metro, juce::dontSendNotification);
    }
    if (playButton) {
        bool play = engine.getTransportState() == Orion::TransportState::Playing;
        if (playButton->getToggleState() != play) playButton->setToggleState(play, juce::dontSendNotification);
    }

    if (getCurrentToolId) {
        int toolId = getCurrentToolId();
        auto updateTool = [&](juce::Button* b, int id) {
            if (b) {
                bool state = (toolId == id);
                if (b->getToggleState() != state) b->setToggleState(state, juce::dontSendNotification);
            }
        };
        updateTool(toolSelect.get(), 0);
        updateTool(toolDraw.get(), 1);
        updateTool(toolErase.get(), 2);
        updateTool(toolSplit.get(), 3);
        updateTool(toolGlue.get(), 4);
        updateTool(toolZoom.get(), 5);
        updateTool(toolMute.get(), 6);
        updateTool(toolListen.get(), 7);
    }

}
