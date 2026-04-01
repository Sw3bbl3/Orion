#include "MidiClipComponent.h"
#include "TimelineComponent.h"
#include "orion/ClipCommands.h"

MidiClipComponent::MidiClipComponent(std::shared_ptr<Orion::MidiClip> c) : clip(c)
{
    setWantsKeyboardFocus(true);
    if (clip) {
        const float* col = clip->getColor();
        lastColor[0] = col[0]; lastColor[1] = col[1]; lastColor[2] = col[2];
    }
    startTimerHz(15);
}

MidiClipComponent::~MidiClipComponent()
{
}

void MidiClipComponent::timerCallback()
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

void MidiClipComponent::paint(juce::Graphics& g)
{
    if (!clip)
    {
        g.fillAll(juce::Colours::darkgrey.withAlpha(0.25f));
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.drawText("Missing MIDI clip", getLocalBounds(), juce::Justification::centred, true);
        return;
    }

    auto bounds = getLocalBounds().toFloat();
    const float* color = clip->getColor();
    juce::Colour clipColor = juce::Colour::fromFloatRGBA(color[0], color[1], color[2], 1.0f);


    g.setColour(clipColor.withAlpha(0.2f));
    g.fillRoundedRectangle(bounds, 4.0f);


    g.setColour(clipColor.withAlpha(0.3f));
    for(int i=0; i<getHeight(); i+=12) {
        g.drawHorizontalLine(i, 0.0f, (float)getWidth());
    }


    float headerHeight = 18.0f;
    juce::Rectangle<float> headerRect = bounds.withHeight(headerHeight);

    g.setColour(clipColor);
    g.fillRoundedRectangle(headerRect, 4.0f);


    g.setColour(juce::Colours::white);
    if (clipColor.getBrightness() > 0.8f) g.setColour(juce::Colours::black);

    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    juce::String name = clip->getName();
    if (isStretching) name += " (Stretch " + juce::String(currentStretchFactor, 2) + "x)";
    g.drawText(name, headerRect.reduced(4, 0), juce::Justification::centredLeft, true);


    const auto& notes = isStretching ? originalNotes : clip->getNotes();
    double clipLen = (double)clip->getLengthFrames();
    if (clipLen > 0) {
        double widthPerFrame = (double)getWidth() / clipLen;


        int minNote = 127;
        int maxNote = 0;

        if (notes.empty()) {
            minNote = 36; maxNote = 84;
        } else {
            for (const auto& n : notes) {
                if (n.noteNumber < minNote) minNote = n.noteNumber;
                if (n.noteNumber > maxNote) maxNote = n.noteNumber;
            }
            minNote = std::max(0, minNote - 5);
            maxNote = std::min(127, maxNote + 5);
        }

        double noteRange = (double)(maxNote - minNote + 1);
        double heightPerNote = (double)(getHeight() - headerHeight - 2) / noteRange;
        double startY = headerHeight + 1;

        uint64_t offset = clip->getOffsetFrames();
        uint64_t len = clip->getLengthFrames();

        for (const auto& note : notes) {
            double noteStart = (double)note.startFrame;
            double noteLen = (double)note.lengthFrames;

            if (isStretching) {
                noteStart *= currentStretchFactor;
                noteLen *= currentStretchFactor;
            }

            int64_t relStart = (int64_t)noteStart - (int64_t)offset;

            if (relStart + (int64_t)noteLen <= 0) continue;
            if (relStart >= (int64_t)len) continue;

            double x = (double)relStart * widthPerFrame;
            double w = (double)noteLen * widthPerFrame;

            if (relStart < 0) {
                double trim = (double)(-relStart) * widthPerFrame;
                x += trim;
                w -= trim;
            }
            if (relStart + (int64_t)noteLen > (int64_t)len) {
                int64_t overshoot = (relStart + (int64_t)noteLen) - (int64_t)len;
                w -= (double)overshoot * widthPerFrame;
            }

            if (w < 2) w = 2;

            double noteYVal = (double)(note.noteNumber - minNote);
            double y = (getHeight() - 2) - (noteYVal * heightPerNote) - heightPerNote;
            if (y < startY) y = startY;

            juce::Rectangle<float> noteRect((float)x, (float)y, (float)w, (float)std::max(2.0, heightPerNote - 1));

            g.setColour(clipColor.darker(0.2f));
            g.fillRoundedRectangle(noteRect, 2.0f);
        }
    }


    if (selected)
    {
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(bounds, 4.0f, 2.0f);

        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 4.0f, 1.0f);
    }
    else
    {
        g.setColour(clipColor.darker(0.4f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }


    if (splitX >= 0) {
        g.setColour(juce::Colours::white);
        g.drawLine((float)splitX, 0.0f, (float)splitX, (float)getHeight(), 1.5f);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawLine((float)splitX - 1.0f, 0.0f, (float)splitX - 1.0f, (float)getHeight(), 1.0f);
        g.drawLine((float)splitX + 1.0f, 0.0f, (float)splitX + 1.0f, (float)getHeight(), 1.0f);
    }
}

void MidiClipComponent::resized()
{
}

void MidiClipComponent::mouseMove(const juce::MouseEvent& e)
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

    bool left = (e.x < 6);
    bool right = (e.x > getWidth() - 6);
    if (left || right) setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else setMouseCursor(juce::MouseCursor::NormalCursor);
}

void MidiClipComponent::mouseExit(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    isMouseInside = false;
    if (splitX != -1) {
        splitX = -1;
        repaint();
    }
}

juce::String MidiClipComponent::getTooltip()
{
    if (!isMouseInside)
        return {};
    if (!clip)
        return {};

    if (isSplitToolActive && isSplitToolActive())
        return "Split at cursor (click)";

    bool left = (lastMousePos.x < 6);
    bool right = (lastMousePos.x > getWidth() - 6);
    if (left) return "Resize clip start (drag)";
    if (right) {
        if (juce::ModifierKeys::getCurrentModifiers().isAltDown())
            return "Resize clip (drag). Alt = stretch";
        return "Resize clip end (drag)";
    }

    return "Move clip (drag). Double-click to open piano roll";
}

void MidiClipComponent::mouseDown(const juce::MouseEvent& e)
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


    if (isSplitToolActive && isSplitToolActive()) {
        if (onSplit) onSplit(this, e);
        return;
    }

    bool left = (e.x < 6);
    bool right = (e.x > getWidth() - 6);

    if (left) {
        resizeMode = Left;
        originalStart = clip->getStartFrame();
        originalLength = clip->getLengthFrames();
        originalOffset = clip->getOffsetFrames();
        cachedFramesPerPixel = (double)originalLength / (double)juce::jmax(1, getWidth());
        dragStartX = e.getScreenX();
        isStretching = false;
        return;
    } else if (right) {
        resizeMode = Right;
        originalStart = clip->getStartFrame();
        originalLength = clip->getLengthFrames();
        originalOffset = clip->getOffsetFrames();
        cachedFramesPerPixel = (double)originalLength / (double)juce::jmax(1, getWidth());
        dragStartX = e.getScreenX();

        isStretching = e.mods.isAltDown();
        if (isStretching) {
            originalNotes = clip->getNotes();
            currentStretchFactor = 1.0;
        } else {
            isStretching = false;
        }
        return;
    }

    resizeMode = None;

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
        transposeMenu.addItem("Transpose +1", [this] { clip->transpose(1); repaint(); });
        transposeMenu.addItem("Transpose -1", [this] { clip->transpose(-1); repaint(); });
        transposeMenu.addItem("Octave +12", [this] { clip->transpose(12); repaint(); });
        transposeMenu.addItem("Octave -12", [this] { clip->transpose(-12); repaint(); });
        m.addSubMenu("Transpose", transposeMenu);

        m.addItem("Properties...", [this] {
            juce::String msg;
            msg << "Name: " << clip->getName() << "\n";
            msg << "Start: " << clip->getStartFrame() << " frames\n";
            msg << "Length: " << clip->getLengthFrames() << " frames\n";
            msg << "Offset: " << clip->getOffsetFrames() << " frames\n";
            msg << "Notes: " << clip->getNotes().size();
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "MIDI Clip Properties", msg);
        });

        m.addSeparator();

        m.addItem("Split at Cursor", [this, e] {
            if (onSplit) onSplit(this, e);
        });

        m.addSeparator();
        m.addItem("Delete", [this] { if (onDelete) onDelete(this); });

        m.showMenuAsync(juce::PopupMenu::Options());
        return;
    }


    if (onSplit && onSplit(this, e)) return;

    lastMouseX = e.getScreenX();

    bool isMultiSelect = e.mods.isCommandDown() || e.mods.isShiftDown();
    if (!selected || isMultiSelect) {
        if (onSelectionChanged) onSelectionChanged(this);
    } else {
        shouldNotifySelectionOnMouseUp = true;
    }
}

void MidiClipComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!clip)
    {
        resizeMode = None;
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

    if (resizeMode != None) {
        int deltaX = e.getScreenX() - dragStartX;
        double framesPerPixel = (cachedFramesPerPixel > 0.0)
            ? cachedFramesPerPixel
            : ((double)originalLength / (double)juce::jmax(1, getWidth()));
        int64_t frameDelta = (int64_t)(deltaX * framesPerPixel);

        if (resizeMode == Right) {
             int64_t newLen = (int64_t)originalLength + frameDelta;

             if (snapFunction && !e.mods.isAltDown()) {
                 int64_t currentEnd = (int64_t)originalStart + newLen;
                 if (currentEnd < 0) currentEnd = 0;
                 int64_t snappedEnd = (int64_t)snapFunction((uint64_t)currentEnd);
                 newLen = snappedEnd - (int64_t)originalStart;
             }

             if (newLen < 100) newLen = 100;
             clip->setLengthFrames(newLen);

             if (isStretching) {
                 const double baseLen = (double)juce::jmax<uint64_t>(1, originalLength);
                 currentStretchFactor = (double)newLen / baseLen;
                 if (currentStretchFactor < 0.01) currentStretchFactor = 0.01;
             }
        } else if (resizeMode == Left) {
             int64_t newStart = (int64_t)originalStart + frameDelta;

             if (snapFunction && !e.mods.isAltDown()) {
                 if (newStart < 0) newStart = 0;
                 newStart = (int64_t)snapFunction((uint64_t)newStart);
             }


             frameDelta = newStart - (int64_t)originalStart;

             int64_t newLen = (int64_t)originalLength - frameDelta;
             int64_t newOffset = (int64_t)originalOffset + frameDelta;

             if (newLen < 100) {
                 newLen = 100;
                 newStart = (int64_t)originalStart + ((int64_t)originalLength - 100);
                 int64_t actualDelta = newStart - (int64_t)originalStart;
                 newOffset = (int64_t)originalOffset + actualDelta;
             }

             if (newStart < 0) {
                 newStart = 0;
                 int64_t actualDelta = newStart - (int64_t)originalStart;
                 newLen = (int64_t)originalLength - actualDelta;
                 newOffset = (int64_t)originalOffset + actualDelta;
             }

             if (newOffset < 0) {
                 newOffset = 0;
                 int64_t actualDelta = newOffset - (int64_t)originalOffset;
                 newStart = (int64_t)originalStart + actualDelta;
                 newLen = (int64_t)originalLength - actualDelta;
             }

             clip->setStartFrame(newStart);
             clip->setLengthFrames(newLen);
             clip->setOffsetFrames(newOffset);
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
            container->startDragging("MidiClip", this, juce::ScaledImage(transparentImage, 1.0f));
        }
    }
}

void MidiClipComponent::mouseUp(const juce::MouseEvent& e)
{
    if (!clip)
    {
        resizeMode = None;
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

    if (resizeMode != None) {
        if (isStretching) {
            clip->stretch(currentStretchFactor);
            isStretching = false;
            originalNotes.clear();
        }

        if (auto* timeline = findParentComponentOfClass<TimelineComponent>()) {
             auto cmd = std::make_unique<Orion::ResizeMidiClipCommand>(
                 clip,
                 originalStart, clip->getStartFrame(),
                 originalLength, clip->getLengthFrames(),
                 originalOffset, clip->getOffsetFrames()
             );
             timeline->pushCommand(std::move(cmd));
        }
    }
    resizeMode = None;
}

void MidiClipComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    if (onDoubleClick) onDoubleClick(this);
}

void MidiClipComponent::setSelected(bool s)
{
    if (selected != s)
    {
        selected = s;
        repaint();
    }
}
