#include "ClipComponent.h"
#include "TimelineComponent.h"
#include "orion/ClipCommands.h"
#include "../OrionLookAndFeel.h"

ClipComponent::ClipComponent(std::shared_ptr<Orion::AudioClip> c) : clip(c)
{
    if (clip) {
        const float* col = clip->getColor();
        lastColor[0] = col[0]; lastColor[1] = col[1]; lastColor[2] = col[2];
    }
    startTimerHz(15);
}

ClipComponent::~ClipComponent() {}

void ClipComponent::timerCallback()
{
    if (clip) {
        const float* col = clip->getColor();
        if (std::abs(col[0] - lastColor[0]) > 0.001f ||
            std::abs(col[1] - lastColor[1]) > 0.001f ||
            std::abs(col[2] - lastColor[2]) > 0.001f)
        {
            lastColor[0] = col[0]; lastColor[1] = col[1]; lastColor[2] = col[2];
            repaint();
        }
    }
}

void ClipComponent::paint(juce::Graphics& g)
{
    if (!clip)
    {
        g.fillAll(findColour(OrionLookAndFeel::timelineBackgroundColourId).darker(0.3f));
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.drawText("Missing audio clip", getLocalBounds(), juce::Justification::centred, true);
        return;
    }

    auto bounds = getLocalBounds().toFloat();
    const float* color = clip->getColor();
    juce::Colour clipColor = juce::Colour::fromFloatRGBA(color[0], color[1], color[2], 1.0f);

    // Main background with vertical gradient for 3D effect
    juce::ColourGradient bgGrad(clipColor.withAlpha(0.15f), 0, bounds.getY(),
                                clipColor.withAlpha(0.35f), 0, bounds.getBottom(), false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds, 6.0f);

    float headerHeight = 20.0f;
    juce::Rectangle<float> headerRect = bounds.withHeight(headerHeight);

    // Header gradient
    juce::ColourGradient headerGrad(clipColor.brighter(0.1f), 0, headerRect.getY(),
                                    clipColor.darker(0.1f), 0, headerRect.getBottom(), false);
    g.setGradientFill(headerGrad);
    g.fillRoundedRectangle(headerRect, 6.0f);

    // Square off the bottom of the header
    g.fillRect(headerRect.removeFromBottom(4.0f));

    // Header subtle highlight
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawHorizontalLine((int)headerRect.getY(), headerRect.getX() + 2.0f, headerRect.getRight() - 2.0f);

    g.setColour(juce::Colours::white);
    if (clipColor.getBrightness() > 0.7f) g.setColour(juce::Colours::black.withAlpha(0.8f));

    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(clip->getName(), headerRect.reduced(6, 0), juce::Justification::centredLeft, true);

    auto waveBounds = bounds.withTrimmedTop(headerHeight).reduced(2);

    if (clip && clip->getChannels() > 0)
    {
        auto* data = clip->getChannelData(0);
        auto numSamples = clip->getLengthFrames();
        auto sourceOffset = clip->getSourceStartFrame();
        size_t maxFrames = clip->getFile() ? clip->getFile()->getLengthFrames() : 0;

        auto w = waveBounds.getWidth();
        auto h = waveBounds.getHeight();
        auto startX = waveBounds.getX();
        auto startY = waveBounds.getY();

        if (data && numSamples > 0 && w > 0)
        {
            double ratio = (double)numSamples / w;
            bool resizing = (dragMode == ResizeLeft || dragMode == ResizeRight);
            if (resizing && cachedFramesPerPixel > 0.0) {
                ratio = cachedFramesPerPixel;
            }

            juce::Colour bgColor = findColour(OrionLookAndFeel::timelineBackgroundColourId);
            bool isLightTheme = bgColor.getBrightness() > 0.5f;

            juce::Colour waveColor = isLightTheme ? juce::Colours::black.withAlpha(0.7f) : juce::Colours::white.withAlpha(0.9f);
            if (clipColor.getBrightness() > 0.8f && !isLightTheme) waveColor = juce::Colours::black.withAlpha(0.6f);
            else if (clipColor.getBrightness() < 0.2f && isLightTheme) waveColor = juce::Colours::white.withAlpha(0.7f);

            // Subtle center line
            g.setColour(waveColor.withAlpha(0.1f));
            g.drawHorizontalLine((int)(startY + h * 0.5f), startX, startX + w);

            // Path-based waveform for smoother rendering
            juce::Path wavePath;
            bool first = true;

            for (int x = 0; x < w; ++x)
            {
                int startSample = (int)(x * ratio);
                int endSample = (int)((x + 1) * ratio);
                if (endSample <= startSample) endSample = startSample + 1;

                if (startSample >= (int64_t)numSamples) break;
                if (endSample > (int64_t)numSamples) endSample = (int)numSamples;

                float maxVal = 0.0f;
                int innerStep = juce::jmax(1, (endSample - startSample) / 32);

                for (int i = startSample; i < endSample; i += innerStep) {
                    if (i + sourceOffset < maxFrames) {
                        float s = std::abs(data[i + sourceOffset]);
                        if (s > maxVal) maxVal = s;
                    }
                }

                if (maxVal > 0.0f) {
                    float halfH = maxVal * h * 0.45f;
                    float yTopVal = startY + (h * 0.5f) - halfH;
                    float yBottomVal = startY + (h * 0.5f) + halfH;

                    if (first) {
                        wavePath.startNewSubPath(startX + x, yTopVal);
                        wavePath.lineTo(startX + x, yBottomVal);
                        first = false;
                    } else {
                        wavePath.lineTo(startX + x, yTopVal);
                        wavePath.lineTo(startX + x, yBottomVal);
                    }
                }
            }

            // Draw wave with a slight glow
            g.setColour(waveColor.withAlpha(0.3f));
            g.strokePath(wavePath, juce::PathStrokeType(2.0f));
            g.setColour(waveColor);
            g.strokePath(wavePath, juce::PathStrokeType(0.8f));
        }
    }

    // Selection / Border
    if (selected)
    {
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(bounds, 6.0f, 2.5f);
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.drawRoundedRectangle(bounds.reduced(1.5f), 5.0f, 1.0f);
    }
    else
    {
        g.setColour(clipColor.darker(0.3f).withAlpha(0.8f));
        g.drawRoundedRectangle(bounds, 6.0f, 1.2f);
    }

    // Fades and Gain
    double currentRatio = (cachedFramesPerPixel > 0.0)
        ? cachedFramesPerPixel
        : ((double)clip->getLengthFrames() / (double)juce::jmax(1, getWidth()));
    if (currentRatio > 0.0001) {
        float fadeInX = (float)(clip->getFadeIn() / currentRatio);
        float fadeOutX = (float)getWidth() - (float)(clip->getFadeOut() / currentRatio);
        float h_f = (float)getHeight();

        g.setColour(juce::Colours::white.withAlpha(0.5f));

        if (clip->getFadeIn() > 0) {
            juce::Path p;
            p.startNewSubPath(0, h_f);
            p.quadraticTo(fadeInX * 0.5f, h_f * 0.5f, fadeInX, headerHeight);
            g.strokePath(p, juce::PathStrokeType(1.5f));
            g.fillEllipse(fadeInX - 4, headerHeight - 4, 8, 8);
        } else {
             g.drawEllipse(fadeInX - 3, headerHeight - 3, 6, 6, 1.0f);
        }

        if (clip->getFadeOut() > 0) {
            juce::Path p;
            p.startNewSubPath(fadeOutX, headerHeight);
            p.quadraticTo(fadeOutX + (getWidth() - fadeOutX) * 0.5f, h_f * 0.5f, (float)getWidth(), h_f);
            g.strokePath(p, juce::PathStrokeType(1.5f));
            g.fillEllipse(fadeOutX - 4, headerHeight - 4, 8, 8);
        } else {
             g.drawEllipse(fadeOutX - 3, headerHeight - 3, 6, 6, 1.0f);
        }
    }

    float h_g = (float)getHeight();
    float gainY = h_g - (clip->getGain() * 0.5f * (h_g - headerHeight)) - (h_g - headerHeight) * 0.25f;
    gainY = juce::jlimit(headerHeight + 10.0f, h_g - 10.0f, gainY);

    g.setColour(juce::Colours::white.withAlpha(0.4f));
    float dashes[] = { 4.0f, 4.0f };
    g.drawDashedLine(juce::Line<float>(0, gainY, (float)getWidth(), gainY), dashes, 2, 1.5f);

    if (clip->isLooping()) {
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        if (clipColor.getBrightness() > 0.7f) g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText("∞", (int)(bounds.getRight() - 20), (int)bounds.getY(), 18, (int)headerHeight, juce::Justification::centred, false);
    }


    if (splitX >= 0) {
        g.setColour(juce::Colours::white);
        g.drawLine((float)splitX, 0.0f, (float)splitX, (float)getHeight(), 1.5f);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawLine((float)splitX - 1.0f, 0.0f, (float)splitX - 1.0f, (float)getHeight(), 1.0f);
        g.drawLine((float)splitX + 1.0f, 0.0f, (float)splitX + 1.0f, (float)getHeight(), 1.0f);
    }
}

void ClipComponent::resized() {}

void ClipComponent::mouseMove(const juce::MouseEvent& e)
{
    if (!clip)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    lastMousePos = e.position.toInt();
    isMouseInside = true;

    if (isSplitToolActive && isSplitToolActive()) {
        if (splitX != e.x) {
            splitX = e.x;
            repaint();
        }
        setMouseCursor(juce::MouseCursor::IBeamCursor);
        return;
    } else {
        if (splitX != -1) {
            splitX = -1;
            repaint();
        }
    }


    double ratio = (double)clip->getLengthFrames() / (double)juce::jmax(1, getWidth());
    if (cachedFramesPerPixel > 0) ratio = cachedFramesPerPixel;

    float fadeInX = (float)(clip->getFadeIn() / ratio);
    float fadeOutX = (float)getWidth() - (float)(clip->getFadeOut() / ratio);
    float h = (float)getHeight();
    float gainY = h - (clip->getGain() * 0.75f * h);

    bool onFadeIn = (std::abs(e.x - fadeInX) < 6 && e.y < 30);
    bool onFadeOut = (std::abs(e.x - fadeOutX) < 6 && e.y < 30);
    bool onGain = (std::abs(e.y - gainY) < 5);

    if (onFadeIn || onFadeOut) setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else if (onGain) setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    else if (e.x < 6 || e.x > getWidth() - 6) setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else setMouseCursor(juce::MouseCursor::NormalCursor);
}

void ClipComponent::mouseExit(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    isMouseInside = false;
    if (splitX != -1) {
        splitX = -1;
        repaint();
    }
}

juce::String ClipComponent::getTooltip()
{
    if (!isMouseInside)
        return {};
    if (!clip)
        return {};

    if (isSplitToolActive && isSplitToolActive())
        return "Split at cursor (click)";

    auto w = (double)getWidth();
    if (w <= 0.0)
        return {};

    double ratio = (double)clip->getLengthFrames() / w;
    if (cachedFramesPerPixel > 0) ratio = cachedFramesPerPixel;
    if (ratio <= 0.000001)
        return "Move clip (drag). Alt = slip";

    float fadeInX = (float)(clip->getFadeIn() / ratio);
    float fadeOutX = (float)getWidth() - (float)(clip->getFadeOut() / ratio);
    float h = (float)getHeight();
    float gainY = h - (clip->getGain() * 0.75f * h);

    bool onFadeIn = (std::abs(lastMousePos.x - fadeInX) < 6 && lastMousePos.y < 30);
    bool onFadeOut = (std::abs(lastMousePos.x - fadeOutX) < 6 && lastMousePos.y < 30);
    bool onGain = (std::abs(lastMousePos.y - gainY) < 5);
    bool left = (lastMousePos.x < 6);
    bool right = (lastMousePos.x > getWidth() - 6);

    if (onFadeIn) return "Fade In (drag)";
    if (onFadeOut) return "Fade Out (drag)";
    if (onGain) return "Clip gain (drag)";
    if (left || right) return "Trim clip (drag)";

    if (juce::ModifierKeys::getCurrentModifiers().isAltDown())
        return "Slip edit (drag)";

    return "Move clip (drag). Alt = slip";
}

void ClipComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!clip)
        return;

    if (e.mods.isMiddleButtonDown()) {
        if (auto* timeline = findParentComponentOfClass<TimelineComponent>()) {
            timeline->startPanning(e.getScreenPosition());
        }
        return;
    }

    if (auto* timeline = findParentComponentOfClass<TimelineComponent>()) {
        if (timeline->getTool() == TimelineTool::Listen) {
            isListeningGesture = true;
            timeline->beginListenAt(e);
            return;
        }
    }

    bool left = (e.x < 6);
    bool right = (e.x > getWidth() - 6);


    if (isSplitToolActive && isSplitToolActive()) {
        if (onSplit) onSplit(this, e);
        return;
    }


    double ratio = (double)clip->getLengthFrames() / (double)juce::jmax(1, getWidth());
    if (cachedFramesPerPixel > 0) ratio = cachedFramesPerPixel;
    float fadeInX = (float)(clip->getFadeIn() / ratio);
    float fadeOutX = (float)getWidth() - (float)(clip->getFadeOut() / ratio);
    float h = (float)getHeight();
    float gainY = h - (clip->getGain() * 0.75f * h);

    bool onFadeIn = (std::abs(e.x - fadeInX) < 6 && e.y < 30);
    bool onFadeOut = (std::abs(e.x - fadeOutX) < 6 && e.y < 30);
    bool onGain = (std::abs(e.y - gainY) < 5);

    cachedFramesPerPixel = (double)clip->getLengthFrames() / (double)juce::jmax(1, getWidth());
    originalStart = clip->getStartFrame();
    originalLength = clip->getLengthFrames();
    originalSourceStart = clip->getSourceStartFrame();
    originalFadeIn = clip->getFadeIn();
    originalFadeOut = clip->getFadeOut();
    originalGain = clip->getGain();
    dragStartX = e.getScreenX();
    dragStartY = e.getScreenY();

    if (onFadeIn) {
        dragMode = FadeIn;
    } else if (onFadeOut) {
        dragMode = FadeOut;
    } else if (left) {
        dragMode = ResizeLeft;
    } else if (right) {
        dragMode = ResizeRight;
    } else if (onGain) {
        dragMode = Gain;
    } else if (e.mods.isAltDown()) {
        dragMode = Slip;
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    } else {
        dragMode = None;
    }

    if (dragMode != None) return;

    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu m;
        m.addItem("Rename", [this] {
             auto* l = new juce::Label("rename", clip->getName());
             l->setColour(juce::Label::backgroundColourId, juce::Colours::white);
             l->setColour(juce::Label::textColourId, juce::Colours::black);
             l->setEditable(true);
             addAndMakeVisible(l);
             l->setBounds(0, 0, getWidth(), 20);
             l->onEditorHide = [this, l] {
                 clip->setName(l->getText().toStdString());
                 if (onRename) onRename(this, l->getText());
                 repaint();
                 if (auto* p = l->getParentComponent())
                     p->removeChildComponent(l);
                 juce::MessageManager::callAsync([l]{ delete l; });
             };
             l->showEditor();
        });

        juce::PopupMenu colorMenu;
        colorMenu.addItem("Green", [this] { clip->setColor(0.0f, 0.8f, 0.0f); repaint(); });
        colorMenu.addItem("Blue", [this] { clip->setColor(0.0f, 0.4f, 0.8f); repaint(); });
        colorMenu.addItem("Red", [this] { clip->setColor(0.8f, 0.2f, 0.2f); repaint(); });
        colorMenu.addItem("Purple", [this] { clip->setColor(0.6f, 0.2f, 0.8f); repaint(); });
        colorMenu.addItem("Orange", [this] { clip->setColor(1.0f, 0.6f, 0.0f); repaint(); });
        colorMenu.addItem("Teal", [this] { clip->setColor(0.0f, 0.8f, 0.8f); repaint(); });
        m.addSubMenu("Color", colorMenu);

        juce::PopupMenu transposeMenu;
        auto formatPitch = [](float p) {
            juce::String s;
            if (p >= 0.0f) s << "+";
            s << juce::String(p, 2) << " st";
            return s;
        };
        transposeMenu.addItem(juce::PopupMenu::Item("Current: " + formatPitch(clip->getPitch())).setEnabled(false));
        transposeMenu.addSeparator();
        auto clampPitch = [](float p) { return juce::jlimit(-24.0f, 24.0f, p); };
        transposeMenu.addItem("Transpose +1", [this, clampPitch] {
            clip->setPitch(clampPitch(clip->getPitch() + 1.0f));
            repaint();
        });
        transposeMenu.addItem("Transpose -1", [this, clampPitch] {
            clip->setPitch(clampPitch(clip->getPitch() - 1.0f));
            repaint();
        });
        transposeMenu.addItem("Octave +12", [this, clampPitch] {
            clip->setPitch(clampPitch(clip->getPitch() + 12.0f));
            repaint();
        });
        transposeMenu.addItem("Octave -12", [this, clampPitch] {
            clip->setPitch(clampPitch(clip->getPitch() - 12.0f));
            repaint();
        });
        transposeMenu.addSeparator();
        transposeMenu.addItem("Fine +10 ct", [this, clampPitch] {
            clip->setPitch(clampPitch(clip->getPitch() + 0.10f));
            repaint();
        });
        transposeMenu.addItem("Fine -10 ct", [this, clampPitch] {
            clip->setPitch(clampPitch(clip->getPitch() - 0.10f));
            repaint();
        });
        transposeMenu.addSeparator();
        transposeMenu.addItem("Reset Transpose", [this] {
            clip->setPitch(0.0f);
            repaint();
        });
        juce::PopupMenu::Item preserveItem("Preserve Length");
        preserveItem.setTicked(clip->isPreservePitch());
        preserveItem.action = [this] {
            clip->setPreservePitch(!clip->isPreservePitch());
            repaint();
        };
        transposeMenu.addItem(preserveItem);
        m.addSubMenu("Transpose", transposeMenu);

        m.addSeparator();
        m.addItem("Normalize", [this] {
            clip->normalize();
            repaint();
        });

        m.addItem("Properties...", [this] {
            juce::String msg;
            msg << "Name: " << clip->getName() << "\n";
            msg << "Start: " << clip->getStartFrame() << " frames\n";
            msg << "Length: " << clip->getLengthFrames() << " frames\n";
            msg << "Source Offset: " << clip->getSourceStartFrame() << " frames\n";
            msg << "Gain: " << clip->getGain() << "\n";
            msg << "Pitch: " << clip->getPitch() << " semitones\n";
            msg << "Preserve Length: " << (clip->isPreservePitch() ? "Yes" : "No") << "\n";
            msg << "Sync Mode: ";
            switch (clip->getSyncMode()) {
                case Orion::SyncMode::Off: msg << "Off"; break;
                case Orion::SyncMode::Resample: msg << "Resample"; break;
                case Orion::SyncMode::Stretch: msg << "Stretch"; break;
            }
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Clip Properties", msg);
        });

        m.addSeparator();

        juce::PopupMenu::Item loopItem("Loop");
        loopItem.action = [this] {
            clip->setLooping(!clip->isLooping());
            repaint();
        };
        loopItem.setTicked(clip->isLooping());
        m.addItem(loopItem);

        juce::PopupMenu::Item reverseItem("Reverse");
        reverseItem.action = [this] {
            clip->setReverse(!clip->isReverse());
            repaint();
        };
        reverseItem.setTicked(clip->isReverse());
        m.addItem(reverseItem);

        m.addSeparator();
        m.addItem("Split at Cursor", [this, e] {
            if (onSplit) onSplit(this, e);
        });

        m.addSeparator();
        m.addItem("Delete", [this] { if (onDelete) onDelete(this); });

        m.showMenuAsync(juce::PopupMenu::Options());
        return;
    }

    lastMouseX = e.getScreenX();

    bool isMultiSelect = e.mods.isCommandDown() || e.mods.isShiftDown();
    if (!selected || isMultiSelect) {
         if (onSelectionChanged) onSelectionChanged(this);
    } else {
         shouldNotifySelectionOnMouseUp = true;
    }
}

void ClipComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!clip)
    {
        dragMode = None;
        return;
    }

    if (e.mods.isMiddleButtonDown()) {
        if (auto* timeline = findParentComponentOfClass<TimelineComponent>()) {
            timeline->dragPanning(e.getScreenPosition());
        }
        return;
    }

    if (isListeningGesture) {
        if (auto* timeline = findParentComponentOfClass<TimelineComponent>()) {
            timeline->updateListenAt(e);
        }
        return;
    }

    shouldNotifySelectionOnMouseUp = false;

    if (dragMode != None) {
        int deltaX = e.getScreenX() - dragStartX;
        int deltaY = e.getScreenY() - dragStartY;

        double framesPerPixel = (cachedFramesPerPixel > 0.0)
            ? cachedFramesPerPixel
            : ((double)originalLength / (double)juce::jmax(1, getWidth()));
        int64_t frameDelta = (int64_t)(deltaX * framesPerPixel);

        int64_t fileFrames = 0;
        if (clip->getFile()) fileFrames = (int64_t)clip->getFile()->getLengthFrames();
        else fileFrames = (int64_t)clip->getLengthFrames();

        if (dragMode == ResizeRight) {
             int64_t newLen = (int64_t)originalLength + frameDelta;


             if (snapFunction && !e.mods.isAltDown()) {
                 int64_t currentEnd = (int64_t)originalStart + newLen;
                 if (currentEnd < 0) currentEnd = 0;
                 int64_t snappedEnd = (int64_t)snapFunction((uint64_t)currentEnd);
                 newLen = snappedEnd - (int64_t)originalStart;
             }

             if (newLen < 1000) newLen = 1000;

             if (!clip->isLooping() && (int64_t)originalSourceStart + newLen > fileFrames) {
                 newLen = fileFrames - (int64_t)originalSourceStart;
             }

             clip->setLengthFrames(newLen);
        } else if (dragMode == ResizeLeft) {
             int64_t newStart = (int64_t)originalStart + frameDelta;


             if (snapFunction && !e.mods.isAltDown()) {
                 if (newStart < 0) newStart = 0;
                 newStart = (int64_t)snapFunction((uint64_t)newStart);
             }


             frameDelta = newStart - (int64_t)originalStart;

             int64_t newLen = (int64_t)originalLength - frameDelta;
             int64_t newSource = (int64_t)originalSourceStart + frameDelta;

             if (newLen < 1000) {
                 newLen = 1000;
                 newStart = (int64_t)originalStart + ((int64_t)originalLength - 1000);
                 newSource = (int64_t)originalSourceStart + ((int64_t)originalLength - 1000);
             }

             if (newStart < 0) {
                 newStart = 0;
                 int64_t diff = 0 - (int64_t)originalStart;
                 newLen = (int64_t)originalLength - diff;
                 newSource = (int64_t)originalSourceStart + diff;
             }

             if (newSource < 0) {
                 newSource = 0;
                 int64_t diff = 0 - (int64_t)originalSourceStart;
                 newStart = (int64_t)originalStart + diff;
                 newLen = (int64_t)originalLength - diff;
             }

             clip->setStartFrame(newStart);
             clip->setLengthFrames(newLen);
             clip->setSourceStartFrame(newSource);
        } else if (dragMode == FadeIn) {
            if (e.mods.isAltDown()) {

                float sensitivity = 0.005f;
                float newCurve = clip->getFadeInCurve() + (float)deltaX * sensitivity;
                newCurve = std::clamp(newCurve, -1.0f, 1.0f);
                clip->setFadeInCurve(newCurve);
            } else {
                int64_t newFade = (int64_t)originalFadeIn + frameDelta;
                if (newFade < 0) newFade = 0;
                if (newFade > (int64_t)clip->getLengthFrames()) newFade = clip->getLengthFrames();
                clip->setFadeIn(newFade);
            }
        } else if (dragMode == FadeOut) {
            if (e.mods.isAltDown()) {

                float sensitivity = -0.005f;
                float newCurve = clip->getFadeOutCurve() + (float)deltaX * sensitivity;
                newCurve = std::clamp(newCurve, -1.0f, 1.0f);
                clip->setFadeOutCurve(newCurve);
            } else {
                int64_t newFade = (int64_t)originalFadeOut - frameDelta;
                if (newFade < 0) newFade = 0;
                if (newFade > (int64_t)clip->getLengthFrames()) newFade = clip->getLengthFrames();
                clip->setFadeOut(newFade);
            }
        } else if (dragMode == Gain) {
            float sensitivity = 0.01f;
            float newGain = originalGain - deltaY * sensitivity;
            if (newGain < 0.0f) newGain = 0.0f;
            if (newGain > 4.0f) newGain = 4.0f;
            clip->setGain(newGain);
        } else if (dragMode == Slip) {

            int64_t newSource = (int64_t)originalSourceStart - frameDelta;

            if (!clip->isLooping()) {
                if (newSource < 0) newSource = 0;

                if (clip->getFile()) {
                    int64_t fileLen = (int64_t)clip->getFile()->getLengthFrames();
                    if (newSource + (int64_t)originalLength > fileLen) {
                        newSource = fileLen - (int64_t)originalLength;
                    }
                    if (newSource < 0) newSource = 0;
                }
            }












            if (clip->isLooping() && clip->getFile()) {
                int64_t fileLen = (int64_t)clip->getFile()->getLengthFrames();
                if (fileLen > 0) {
                     while (newSource < 0) newSource += fileLen;
                     while (newSource >= fileLen) newSource -= fileLen;
                }
            } else {
                 if (newSource < 0) newSource = 0;
            }

            clip->setSourceStartFrame((uint64_t)newSource);
        }

        if (onResizing) onResizing(this);
        repaint();
        return;
    }

    if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        if (!container->isDragAndDropActive())
        {
            juce::Image transparentImage(juce::Image::ARGB, 1, 1, true);
            container->startDragging("AudioClip", this, juce::ScaledImage(transparentImage, 1.0f));
        }
    }
}

void ClipComponent::mouseUp(const juce::MouseEvent& e)
{
    if (!clip)
    {
        dragMode = None;
        return;
    }

    if (e.mods.isMiddleButtonDown()) {
        if (auto* timeline = findParentComponentOfClass<TimelineComponent>()) {
            timeline->stopPanning();
        }
        return;
    }

    if (isListeningGesture) {
        if (auto* timeline = findParentComponentOfClass<TimelineComponent>()) {
            timeline->endListen();
        }
        isListeningGesture = false;
        return;
    }

    juce::ignoreUnused(e);
    if (shouldNotifySelectionOnMouseUp) {
         if (onSelectionChanged) onSelectionChanged(this);
         shouldNotifySelectionOnMouseUp = false;
    }

    if (dragMode == ResizeLeft || dragMode == ResizeRight) {
        if (auto* timeline = findParentComponentOfClass<TimelineComponent>()) {
             auto cmd = std::make_unique<Orion::ResizeClipCommand>(
                 clip,
                 originalStart, clip->getStartFrame(),
                 originalLength, clip->getLengthFrames(),
                 originalSourceStart, clip->getSourceStartFrame()
             );
             timeline->pushCommand(std::move(cmd));
        }
        if (onDragEnd) onDragEnd(this);
    }
    dragMode = None;
}

void ClipComponent::setSelected(bool s)
{
    if (selected != s)
    {
        selected = s;
        repaint();
    }
}
