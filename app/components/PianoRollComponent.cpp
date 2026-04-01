#include "PianoRollComponent.h"
#include "orion/EditorCommands.h"
#include "orion/SettingsManager.h"
#include "../OrionLookAndFeel.h"
#include <cmath>
#include <algorithm>

std::vector<Orion::MidiNote> MidiGridComponent::clipboard;

namespace
{
    double getSafeSampleRate(const Orion::AudioEngine& engine)
    {
        const auto sr = engine.getSampleRate();
        return sr > 1.0 ? sr : 44100.0;
    }

    int getSnapDivisionForId(int id)
    {
        switch (id)
        {
            case 2: return 4;
            case 3: return 8;
            case 4: return 16;
            case 5: return 32;
            default: return 0;
        }
    }
}





PianoKeysComponent::PianoKeysComponent()
{
    setSize(60, 128 * (int)keyHeight);
}

PianoKeysComponent::~PianoKeysComponent() {}

bool PianoKeysComponent::isBlackKey(int note) const
{
    int n = note % 12;
    return (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
}

int PianoKeysComponent::getNoteAt(int y) const
{
    int i = y / (int)keyHeight;
    int note = 127 - i;
    return juce::jlimit(0, 127, note);
}

void PianoKeysComponent::paint(juce::Graphics& g)
{

    g.fillAll(juce::Colours::white);

    for (int i = 0; i < 128; ++i)
    {
        int note = 127 - i;
        float y = (float)i * keyHeight;

        if (isBlackKey(note))
        {
            g.setColour(juce::Colours::black);
            g.fillRect(0.0f, y, (float)getWidth(), keyHeight);
            g.setColour(juce::Colours::grey);
            g.drawRect(0.0f, y, (float)getWidth(), keyHeight, 1.0f);
        }
        else
        {
            g.setColour(juce::Colours::white);
            g.fillRect(0.0f, y, (float)getWidth(), keyHeight);
            g.setColour(juce::Colours::black);
            g.drawRect(0.0f, y, (float)getWidth(), keyHeight, 1.0f);

            if (note % 12 == 0)
            {
                g.setColour(juce::Colours::black);
                g.setFont(10.0f);
                g.drawText("C" + juce::String(note / 12 - 1), 2, (int)y, getWidth() - 4, (int)keyHeight, juce::Justification::centredRight, false);
            }
        }
    }


    if (lastClickedNote != -1) {
         int i = 127 - lastClickedNote;
         float y = (float)i * keyHeight;
         g.setColour(juce::Colours::yellow.withAlpha(0.5f));
         g.fillRect(0.0f, y, (float)getWidth(), keyHeight);
    }
}

void PianoKeysComponent::mouseDown(const juce::MouseEvent& e)
{
    int note = getNoteAt(e.y);
    if (note != lastClickedNote) {
        lastClickedNote = note;
        repaint();
        if (onNoteOn) onNoteOn(note);
    }
}

void PianoKeysComponent::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    if (lastClickedNote != -1) {
        if (onNoteOff) onNoteOff(lastClickedNote);
        lastClickedNote = -1;
        repaint();
    }
}

void PianoKeysComponent::mouseExit(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    if (lastClickedNote != -1) {
        if (onNoteOff) onNoteOff(lastClickedNote);
        lastClickedNote = -1;
        repaint();
    }
}

void PianoKeysComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused(e);
    if (onMouseWheelRelay) onMouseWheelRelay(wheel);
}






MidiGridComponent::MidiGridComponent(Orion::AudioEngine& e, Orion::CommandHistory& history)
    : engine(e), commandHistory(history)
{
    setSize(2000, 128 * (int)keyHeight);
}

MidiGridComponent::~MidiGridComponent() {}

void MidiGridComponent::setClips(const std::vector<std::shared_ptr<Orion::MidiClip>>& clips,
                                 std::shared_ptr<Orion::MidiClip> active)
{
    clipViews.clear();
    selectedIndices.clear();
    showHoverGuides = false;

    uint64_t maxEnd = 0;
    for (const auto& c : clips) {
        if (!c) continue;
        ClipView view;
        view.clip = c;
        view.startFrame = c->getStartFrame();
        view.lengthFrames = c->getLengthFrames();
        view.offsetFrames = c->getOffsetFrames();
        const float* col = c->getColor();
        view.color[0] = col[0];
        view.color[1] = col[1];
        view.color[2] = col[2];
        clipViews.push_back(view);

        uint64_t end = view.startFrame + view.lengthFrames;
        if (end > maxEnd) maxEnd = end;
    }

    if (active) {
        activeClip = active;
    } else if (!clipViews.empty()) {
        activeClip = clipViews.front().clip;
    } else {
        activeClip.reset();
    }
    selectedClip = activeClip;

    double sr = engine.getSampleRate();
    if (sr <= 0.0) sr = 44100.0;
    if (maxEnd == 0) maxEnd = (uint64_t)(sr * 8.0);
    double lenSeconds = (double)maxEnd / sr;
    int w = (int)(lenSeconds * pixelsPerSecond) + 200;
    setSize(std::max(getParentWidth(), w), getHeight());

    repaint();
}

void MidiGridComponent::setPixelsPerSecond(double pps)
{
    pixelsPerSecond = juce::jlimit(10.0, 2000.0, pps);
    uint64_t maxEnd = 0;
    for (const auto& view : clipViews) {
        uint64_t end = view.startFrame + view.lengthFrames;
        if (end > maxEnd) maxEnd = end;
    }

    const double sr = getSafeSampleRate(engine);
    if (maxEnd == 0) maxEnd = (uint64_t)(sr * 8.0);
    double lenSeconds = (double)maxEnd / sr;
    int w = (int)(lenSeconds * pixelsPerSecond) + 200;
    setSize(std::max(getParentWidth(), w), getHeight());

    if (onPixelsPerSecondChanged) onPixelsPerSecondChanged(pixelsPerSecond);
    repaint();
}

int MidiGridComponent::yToNote(int y)
{
    int i = y / (int)keyHeight;
    int note = 127 - i;
    return juce::jlimit(0, 127, note);
}

int MidiGridComponent::noteToY(int note)
{
    int i = 127 - note;
    return i * (int)keyHeight;
}

uint64_t MidiGridComponent::xToFrame(int x)
{
    if (pixelsPerSecond <= 0.0)
        return 0;

    const double s = (double)x / pixelsPerSecond;
    return (uint64_t)(s * getSafeSampleRate(engine));
}

int MidiGridComponent::frameToX(uint64_t frame) const
{
    double s = (double)frame / getSafeSampleRate(engine);
    return (int)(s * pixelsPerSecond);
}

uint64_t MidiGridComponent::getDefaultNoteLengthFrames() const
{
    double bpm = engine.getBpm();
    if (bpm <= 0.1) bpm = 120.0;

    const double sr = getSafeSampleRate(engine);

    double samplesPerBeat = (60.0 / bpm) * sr;
    if (snapDivision > 0) {
        return (uint64_t)(samplesPerBeat * (4.0 / snapDivision));
    }
    return (uint64_t)samplesPerBeat;
}

const MidiGridComponent::ClipView* MidiGridComponent::getClipForFrame(uint64_t frame) const
{
    for (const auto& view : clipViews) {
        if (frame >= view.startFrame && frame < view.startFrame + view.lengthFrames)
            return &view;
    }
    return nullptr;
}

const MidiGridComponent::ClipView* MidiGridComponent::getClipView(const std::shared_ptr<Orion::MidiClip>& clip) const
{
    if (!clip) return nullptr;
    for (const auto& view : clipViews) {
        if (view.clip == clip) return &view;
    }
    return nullptr;
}

bool MidiGridComponent::isNoteSelected(const std::shared_ptr<Orion::MidiClip>& clip, int index) const
{
    if (!clip || index < 0) return false;
    return (clip == selectedClip) && (std::find(selectedIndices.begin(), selectedIndices.end(), index) != selectedIndices.end());
}

void MidiGridComponent::setActiveClip(std::shared_ptr<Orion::MidiClip> clip)
{
    if (clip == activeClip) return;
    activeClip = clip;
    selectedIndices.clear();
    selectedClip = clip;
    if (onActiveClipChanged) onActiveClipChanged(activeClip);
    repaint();
}

int MidiGridComponent::getPlayheadX() const
{
    uint64_t playCursor = engine.getCursor();
    return frameToX(playCursor);
}

uint64_t MidiGridComponent::snapFrame(uint64_t frame)
{
    if (snapDivision <= 0) return frame;

    double bpm = engine.getBpm();
    if (bpm <= 0.1) bpm = 120.0;

    const double sr = getSafeSampleRate(engine);

    double secondsPerBeat = 60.0 / bpm;
    double samplesPerBeat = secondsPerBeat * sr;
    double samplesPerSnap = samplesPerBeat * (4.0 / (double)snapDivision);

    if (samplesPerSnap < 1.0) return frame;

    double snapped = std::round((double)frame / samplesPerSnap) * samplesPerSnap;
    return (uint64_t)snapped;
}

void MidiGridComponent::paint(juce::Graphics& g)
{

    juce::Colour base = findColour(OrionLookAndFeel::timelineBackgroundColourId);
    g.fillAll(base);


    for (int i = 0; i < 128; ++i)
    {
        int note = 127 - i;
        float y = (float)i * keyHeight;


        int n = note % 12;
        bool isBlack = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);

        if (isBlack)
            g.setColour(juce::Colours::black.withAlpha(0.15f));
        else
            g.setColour(base.brighter(0.02f));

        g.fillRect(0.0f, y, (float)getWidth(), keyHeight);
        g.setColour(findColour(OrionLookAndFeel::timelineGridColourId).withAlpha(0.1f));
        g.drawHorizontalLine((int)y, 0.0f, (float)getWidth());
    }


    double bpm = engine.getBpm();
    if (bpm <= 0.1) bpm = 120.0;

    double secondsPerBeat = 60.0 / bpm;
    double pixelsPerBeat = secondsPerBeat * pixelsPerSecond;


    g.setColour(findColour(OrionLookAndFeel::timelineGridColourId));
    for (double x = 0; x < getWidth(); x += pixelsPerBeat) {
        g.drawVerticalLine((int)x, 0.0f, (float)getHeight());
    }


    if (snapDivision > 0 && pixelsPerBeat > 20) {
        double snapSpacing = pixelsPerBeat * (4.0 / (double)snapDivision);
        g.setColour(findColour(OrionLookAndFeel::timelineGridColourId).withAlpha(0.5f));
        for (double x = 0; x < getWidth(); x += snapSpacing) {
             g.drawVerticalLine((int)x, 0.0f, (float)getHeight());
        }
    }

    if (showHoverGuides && previewNote >= 0) {
        const int rowY = noteToY(previewNote);
        const int previewX = frameToX(previewStartFrame);
        int previewW = frameToX(previewLengthFrames);
        if (previewW < 10) previewW = 10;

        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.fillRect(0, rowY, getWidth(), (int)keyHeight);

        g.setColour(juce::Colours::white.withAlpha(0.28f));
        g.drawVerticalLine(previewX, 0.0f, (float)getHeight());

        g.setColour(juce::Colours::cyan.withAlpha(0.20f));
        g.fillRoundedRectangle((float)previewX, (float)rowY + 1.0f, (float)previewW, keyHeight - 2.0f, 3.0f);
        g.setColour(juce::Colours::cyan.withAlpha(0.7f));
        g.drawRoundedRectangle((float)previewX, (float)rowY + 1.0f, (float)previewW, keyHeight - 2.0f, 3.0f, 1.0f);
    }

    auto clipBounds = g.getClipBounds();
    int clipX = clipBounds.getX();
    int clipRight = clipBounds.getRight();

    for (const auto& view : clipViews)
    {
        if (!view.clip) continue;

        int clipStartX = frameToX(view.startFrame);
        int clipEndX = frameToX(view.startFrame + view.lengthFrames);
        int clipWidth = clipEndX - clipStartX;
        if (clipWidth < 1) clipWidth = 1;

        juce::Colour clipColor = juce::Colour::fromFloatRGBA(view.color[0], view.color[1], view.color[2], 1.0f);
        bool isActive = (view.clip == activeClip);
        juce::Colour fill = isActive ? clipColor.withAlpha(0.08f) : clipColor.withAlpha(0.04f);
        g.setColour(fill);
        g.fillRect(clipStartX, 0, clipWidth, getHeight());
        g.setColour(clipColor.withAlpha(isActive ? 0.4f : 0.2f));
        g.drawVerticalLine(clipStartX, 0.0f, (float)getHeight());
        g.drawVerticalLine(clipStartX + clipWidth, 0.0f, (float)getHeight());

        const auto& notes = view.clip->getNotes();
        for (int idx = 0; idx < (int)notes.size(); ++idx)
        {
            const auto& note = notes[idx];
            int64_t globalStart = (int64_t)view.startFrame + (int64_t)note.startFrame - (int64_t)view.offsetFrames;
            int64_t globalEnd = globalStart + (int64_t)note.lengthFrames;
            if (globalEnd <= (int64_t)view.startFrame || globalStart >= (int64_t)(view.startFrame + view.lengthFrames))
                continue;
            if (globalStart < 0) continue;

            int x = frameToX((uint64_t)globalStart);
            int w = frameToX(note.lengthFrames);
            if (w < 4) w = 4;

            if (x > clipRight || x + w < clipX) continue;

            int y = noteToY(note.noteNumber);

            bool isSelected = isNoteSelected(view.clip, idx);
            if (isSelected)
                g.setColour(juce::Colours::cyan);
            else
                g.setColour(isActive ? clipColor.brighter(0.3f) : clipColor.withAlpha(0.45f));

            g.fillRoundedRectangle((float)x, (float)y + 1.0f, (float)w, (float)keyHeight - 2.0f, 3.0f);

            g.setColour(juce::Colours::white.withAlpha(isActive ? 0.45f : 0.2f));
            g.fillRect(x + w - 8, y + 1, 8, (int)keyHeight - 2);

            g.setColour(juce::Colours::white.withAlpha(isActive ? 0.9f : 0.35f));
            g.drawRoundedRectangle((float)x, (float)y + 1.0f, (float)w, (float)keyHeight - 2.0f, 3.0f, 1.0f);
        }
    }

    if (isSelecting) {
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.fillRect(selectionRect);
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawRect(selectionRect);
    }

    const int playheadX = getPlayheadX();
    if (playheadX >= 0 && playheadX <= getWidth()) {
        g.setColour(juce::Colours::orange.withAlpha(0.95f));
        g.drawLine((float)playheadX, 0.0f, (float)playheadX, (float)getHeight(), 2.0f);
    }
}

MidiGridComponent::NoteRef MidiGridComponent::getNoteAt(int x, int y)
{
    NoteRef ref;
    if (clipViews.empty()) return ref;

    for (const auto& view : clipViews) {
        if (!view.clip) continue;
        const auto& notes = view.clip->getNotes();
        for (int i = 0; i < (int)notes.size(); ++i) {
            const auto& note = notes[i];
            int64_t globalStart = (int64_t)view.startFrame + (int64_t)note.startFrame - (int64_t)view.offsetFrames;
            if (globalStart < 0) continue;
            int nx = frameToX((uint64_t)globalStart);
            int nw = frameToX(note.lengthFrames);
            if (nw < 4) nw = 4;
            int ny = noteToY(note.noteNumber);

            if (x >= nx && x < nx + nw && y >= ny && y < ny + (int)keyHeight) {
                ref.clip = view.clip;
                ref.index = i;
                return ref;
            }
        }
    }
    return ref;
}

void MidiGridComponent::mouseDown(const juce::MouseEvent& e)
{
    if (clipViews.empty()) return;

    auto hit = getNoteAt(e.x, e.y);

    if (e.mods.isRightButtonDown()) {
        if (hit.index != -1 && hit.clip) {
            setActiveClip(hit.clip);
            auto notes = hit.clip->getNotes();
            if (hit.index < (int)notes.size()) {
                commandHistory.push(std::make_unique<Orion::RemoveMidiNoteCommand>(hit.clip, notes[hit.index]));
                selectedIndices.clear();
                selectedClip = hit.clip;
                repaint();
            }
        }
        return;
    }

    if (currentTool == PianoRollTool::Erase) {
        if (hit.index != -1 && hit.clip) {
            setActiveClip(hit.clip);
            auto notes = hit.clip->getNotes();
            if (hit.index < (int)notes.size()) {
                commandHistory.push(std::make_unique<Orion::RemoveMidiNoteCommand>(hit.clip, notes[hit.index]));
                selectedIndices.clear();
                selectedClip = hit.clip;
                repaint();
            }
        }
        return;
    }

    if (currentTool == PianoRollTool::Draw || singleClickDrawEnabled) {
        if (!activeClip) return;
        const ClipView* view = getClipView(activeClip);
        if (!view) return;
        uint64_t globalFrame = xToFrame(e.x);
        if (globalFrame < view->startFrame || globalFrame >= view->startFrame + view->lengthFrames) {
            return;
        }
        Orion::MidiNote n;
        n.noteNumber = yToNote(e.y);
        n.velocity = 0.8f;
        n.startFrame = globalFrame - view->startFrame + view->offsetFrames;
        if (snapDivision > 0) {
            uint64_t snapped = snapFrame(globalFrame);
            if (snapped < view->startFrame) snapped = view->startFrame;
            n.startFrame = snapped - view->startFrame + view->offsetFrames;
        }
        n.lengthFrames = getDefaultNoteLengthFrames();
        commandHistory.push(std::make_unique<Orion::AddMidiNoteCommand>(activeClip, n));
        repaint();
        return;
    }

    if (hit.index != -1 && hit.clip) {
        if (hit.clip != activeClip) {
            setActiveClip(hit.clip);
        }

        if (selectedClip != hit.clip) {
            selectedIndices.clear();
            selectedClip = hit.clip;
        }

        if (e.mods.isShiftDown()) {
            auto it = std::find(selectedIndices.begin(), selectedIndices.end(), hit.index);
            if (it != selectedIndices.end()) {
                 selectedIndices.erase(it);
            } else {
                 selectedIndices.push_back(hit.index);
            }
        } else {
            bool alreadySelected = std::find(selectedIndices.begin(), selectedIndices.end(), hit.index) != selectedIndices.end();
            if (!alreadySelected) {
                selectedIndices.clear();
                selectedIndices.push_back(hit.index);
            }
        }

        isDragging = true;
        hit.clip->setDirty(true);
        dragStartX = e.x;
        dragStartY = e.y;
        leadNoteIndex = hit.index;

        auto notes = hit.clip->getNotes();
        if (hit.index < (int)notes.size()) {
            const auto& note = notes[hit.index];
            int64_t globalStart = (int64_t)hit.clip->getStartFrame() + (int64_t)note.startFrame - (int64_t)hit.clip->getOffsetFrames();
            int nx = frameToX((uint64_t)std::max<int64_t>(0, globalStart));
            int nw = frameToX(note.lengthFrames);
            if (nw < 4) nw = 4;
            if (e.x > nx + nw - 10) isResizing = true;
            else isResizing = false;
        }

        dragStates.clear();
        for (int i : selectedIndices) {
             if (i < (int)notes.size()) {
                 dragStates.push_back({hit.clip, i, notes[i].startFrame, notes[i].lengthFrames, notes[i].noteNumber});
             }
        }
    } else {
        uint64_t clickFrame = xToFrame(e.x);
        if (const ClipView* view = getClipForFrame(clickFrame)) {
            if (view->clip && view->clip != activeClip) {
                setActiveClip(view->clip);
            }
        }

        isSelecting = true;
        dragStartX = e.x;
        dragStartY = e.y;
        selectionRect = juce::Rectangle<int>(e.x, e.y, 0, 0);

        if (!e.mods.isShiftDown()) {
            selectedIndices.clear();
        }
        selectedClip = activeClip;
        isDragging = false;
    }

    repaint();
}

void MidiGridComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (clipViews.empty()) return;

    if (isSelecting) {
        int x = std::min(dragStartX, e.x);
        int y = std::min(dragStartY, e.y);
        int w = std::abs(e.x - dragStartX);
        int h = std::abs(e.y - dragStartY);
        selectionRect = juce::Rectangle<int>(x, y, w, h);


        selectedIndices.clear();
        selectedClip = activeClip;
        if (activeClip) {
            const ClipView* view = getClipView(activeClip);
            auto notes = activeClip->getNotes();
            for (int i = 0; i < (int)notes.size(); ++i) {
                const auto& note = notes[i];
                int64_t globalStart = (int64_t)(view ? view->startFrame : 0) + (int64_t)note.startFrame - (int64_t)(view ? view->offsetFrames : 0);
                if (globalStart < 0) continue;
                int nx = frameToX((uint64_t)globalStart);
                int nw = frameToX(note.lengthFrames);
                if (nw < 4) nw = 4;
                int ny = noteToY(note.noteNumber);

                juce::Rectangle<int> r(nx, ny, nw, (int)keyHeight);
                if (selectionRect.intersects(r)) {
                    selectedIndices.push_back(i);
                }
            }
        }
        repaint();
        return;
    }

    if (selectedIndices.empty() || !isDragging || dragStates.empty()) return;
    if (!selectedClip) return;

    int dx = e.x - dragStartX;
    int dy = e.y - dragStartY;

    const double sr = getSafeSampleRate(engine);
    if (pixelsPerSecond <= 0.0)
        return;

    int64_t frameDeltaRaw = (int64_t)((double)dx / pixelsPerSecond * sr);

    auto& notes = selectedClip->getNotesMutable();


    DragState leadState = {0};
    bool foundLead = false;
    for (const auto& s : dragStates) {
        if (s.index == leadNoteIndex && s.clip == selectedClip) {
            leadState = s;
            foundLead = true;
            break;
        }
    }
    if (!foundLead && !dragStates.empty()) leadState = dragStates[0];

    const ClipView* view = getClipView(selectedClip);
    if (!view) return;
    int64_t clipStart = (int64_t)view->startFrame;
    int64_t clipLen = (int64_t)view->lengthFrames;
    int64_t clipOffset = (int64_t)view->offsetFrames;

    if (isResizing) {

        int64_t leadGlobalStart = clipStart + (int64_t)leadState.startFrame - clipOffset;
        int64_t originalEnd = leadGlobalStart + (int64_t)leadState.lengthFrames;
        int64_t frameDelta = frameDeltaRaw;

        if (snapDivision > 0) {
            uint64_t newEnd = snapFrame((uint64_t)std::max<int64_t>(0, originalEnd + frameDelta));
            frameDelta = (int64_t)newEnd - originalEnd;
        }

        for (const auto& state : dragStates) {
            if (state.clip != selectedClip || state.index >= (int)notes.size()) continue;
            Orion::MidiNote& note = notes[state.index];
            int64_t localStart = (int64_t)state.startFrame;
            int64_t maxLen = clipLen - (localStart - clipOffset);
            if (maxLen < 1) maxLen = 1;

            int64_t newLen = (int64_t)state.lengthFrames + frameDelta;
            if (newLen < 100) newLen = 100;
            if (newLen > maxLen) newLen = maxLen;
            note.lengthFrames = (uint64_t)newLen;
        }

    } else {


        int64_t frameDelta = frameDeltaRaw;
        if (snapDivision > 0) {
            int64_t leadGlobalStart = clipStart + (int64_t)leadState.startFrame - clipOffset;
            int64_t newStart = leadGlobalStart + frameDelta;
            if (newStart < clipStart) newStart = clipStart;
            newStart = (int64_t)snapFrame((uint64_t)newStart);
            frameDelta = newStart - leadGlobalStart;
        }


        int64_t minDelta = frameDelta;
        int64_t maxDelta = frameDelta;
        for (const auto& state : dragStates) {
            if (state.clip != selectedClip) continue;
            int64_t minLocal = clipOffset;
            int64_t maxLocal = clipOffset + clipLen - (int64_t)state.lengthFrames;
            int64_t minD = minLocal - (int64_t)state.startFrame;
            int64_t maxD = maxLocal - (int64_t)state.startFrame;
            minDelta = std::max(minDelta, minD);
            maxDelta = std::min(maxDelta, maxD);
        }
        frameDelta = std::clamp(frameDelta, minDelta, maxDelta);

        int semitoneDelta = -(int)std::round((float)dy / keyHeight);

        for (const auto& state : dragStates) {
            if (state.clip != selectedClip || state.index >= (int)notes.size()) continue;
            Orion::MidiNote& note = notes[state.index];

            int64_t newStart = (int64_t)state.startFrame + frameDelta;
            if (newStart < clipOffset) newStart = clipOffset;
            if (newStart > clipOffset + clipLen) newStart = clipOffset + clipLen;
            note.startFrame = (uint64_t)newStart;

            int newNote = state.noteNumber + semitoneDelta;
            if (newNote < 0) newNote = 0;
            if (newNote > 127) newNote = 127;
            note.noteNumber = newNote;
        }
    }

    repaint();
}

void MidiGridComponent::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    if (isDragging && selectedClip && !dragStates.empty()) {

        auto macro = std::make_unique<Orion::MacroCommand>(isResizing ? "Resize Notes" : "Move Notes");
        auto& notes = selectedClip->getNotesMutable();


        for (const auto& state : dragStates) {
            if (state.clip == selectedClip && state.index < (int)notes.size()) {
                Orion::MidiNote newNote = notes[state.index];


                notes[state.index].startFrame = state.startFrame;
                notes[state.index].lengthFrames = state.lengthFrames;
                notes[state.index].noteNumber = state.noteNumber;

                Orion::MidiNote oldNote = notes[state.index];

                if (isResizing) {
                    macro->addCommand(std::make_unique<Orion::ResizeMidiNoteCommand>(selectedClip, oldNote, newNote));
                } else {
                    macro->addCommand(std::make_unique<Orion::MoveMidiNoteCommand>(selectedClip, oldNote, newNote));
                }
            }
        }


        commandHistory.push(std::move(macro));




    }
    isDragging = false;
    isResizing = false;
    isSelecting = false;
    selectionRect = {};
    repaint();
}

void MidiGridComponent::mouseMove(const juce::MouseEvent& e)
{
    if (!activeClip) return;
    const ClipView* view = getClipView(activeClip);
    if (!view) return;

    uint64_t frame = xToFrame(e.x);
    if (frame < view->startFrame || frame >= view->startFrame + view->lengthFrames) {
        showHoverGuides = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
        return;
    }

    previewNote = yToNote(e.y);
    previewStartFrame = frame;
    if (snapDivision > 0) previewStartFrame = snapFrame(previewStartFrame);
    previewLengthFrames = getDefaultNoteLengthFrames();
    showHoverGuides = true;

    auto hit = getNoteAt(e.x, e.y);
    if (hit.index != -1 && hit.clip == activeClip) {
        auto notes = activeClip->getNotes();
        if (hit.index < (int)notes.size()) {
            const auto& note = notes[hit.index];
            int64_t globalStart = (int64_t)view->startFrame + (int64_t)note.startFrame - (int64_t)view->offsetFrames;
            int nx = frameToX((uint64_t)std::max<int64_t>(0, globalStart));
            int nw = frameToX(note.lengthFrames);
            if (nw < 4) nw = 4;

            if (e.x > nx + nw - 10) {
                setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
                repaint();
                return;
            }
        }
    }
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

void MidiGridComponent::mouseExit(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    showHoverGuides = false;
    repaint();
}

void MidiGridComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    if (e.mods.isCommandDown() || e.mods.isCtrlDown()) {

        double zoom = (wheel.deltaY > 0) ? 1.1 : 0.9;
        double nextPps = pixelsPerSecond * zoom;
        if (nextPps < 10.0) nextPps = 10.0;
        if (nextPps > 2000.0) nextPps = 2000.0;
        setPixelsPerSecond(nextPps);
    } else if (e.mods.isAltDown()) {

        float zoom = (wheel.deltaY > 0) ? 1.1f : 0.9f;
        keyHeight *= zoom;
        if (keyHeight < 4.0f) keyHeight = 4.0f;
        if (keyHeight > 40.0f) keyHeight = 40.0f;

        setSize(getWidth(), 128 * (int)keyHeight);
        if (onKeyHeightChanged) onKeyHeightChanged(keyHeight);

        repaint();
    } else {




    }
}

void MidiGridComponent::duplicateSelected() {
    if (!selectedClip || selectedIndices.empty()) return;


    uint64_t minStart = UINT64_MAX;
    uint64_t maxEnd = 0;

    auto notes = selectedClip->getNotes();
    std::vector<Orion::MidiNote> toDup;

    for (int idx : selectedIndices) {
        if (idx < (int)notes.size()) {
            const auto& n = notes[idx];
            if (n.startFrame < minStart) minStart = n.startFrame;
            uint64_t end = n.startFrame + n.lengthFrames;
            if (end > maxEnd) maxEnd = end;
            toDup.push_back(n);
        }
    }

    if (toDup.empty()) return;

    uint64_t span = maxEnd - minStart;

    if (snapDivision > 0) {
        double bpm = engine.getBpm();
        if (bpm <= 0.1) bpm = 120.0;
        double sr = getSafeSampleRate(engine);
        double samplesPerBeat = (60.0 / bpm) * sr;
        double gridFrames = samplesPerBeat * (4.0 / snapDivision);

        double spans = (double)span / gridFrames;
        span = (uint64_t)(std::ceil(spans) * gridFrames);
        if (span == 0) span = (uint64_t)gridFrames;
    }

    auto macro = std::make_unique<Orion::MacroCommand>("Duplicate Notes");



    selectedIndices.clear();

    for (auto n : toDup) {
        n.startFrame += span;
        macro->addCommand(std::make_unique<Orion::AddMidiNoteCommand>(selectedClip, n));
    }

    commandHistory.push(std::move(macro));
    repaint();
}

void MidiGridComponent::copySelected() {
    if (!selectedClip || selectedIndices.empty()) return;
    clipboard.clear();
    auto notes = selectedClip->getNotes();
    for (int idx : selectedIndices) {
        if (idx < (int)notes.size()) {
            clipboard.push_back(notes[idx]);
        }
    }

    if (!clipboard.empty()) {
        uint64_t minStart = UINT64_MAX;
        for (const auto& n : clipboard) if (n.startFrame < minStart) minStart = n.startFrame;
        for (auto& n : clipboard) n.startFrame -= minStart;
    }
}

void MidiGridComponent::paste() {
    if (!selectedClip || clipboard.empty()) return;

    uint64_t startFrame = xToFrame(dragStartX);
    if (snapDivision > 0) startFrame = snapFrame(startFrame);

    auto macro = std::make_unique<Orion::MacroCommand>("Paste Notes");

    for (auto n : clipboard) {
        n.startFrame += startFrame;
        macro->addCommand(std::make_unique<Orion::AddMidiNoteCommand>(selectedClip, n));
    }
    commandHistory.push(std::move(macro));
    repaint();
}

void MidiGridComponent::deleteSelected() {
    if (!selectedClip || selectedIndices.empty()) return;

    auto macro = std::make_unique<Orion::MacroCommand>("Delete Notes");
    auto notes = selectedClip->getNotes();

    for (int idx : selectedIndices) {
        if (idx < (int)notes.size()) {
            macro->addCommand(std::make_unique<Orion::RemoveMidiNoteCommand>(selectedClip, notes[idx]));
        }
    }

    commandHistory.push(std::move(macro));
    selectedIndices.clear();
    repaint();
}

void MidiGridComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!activeClip) return;

    auto hit = getNoteAt(e.x, e.y);
    if (hit.index == -1) {
        const ClipView* view = getClipView(activeClip);
        if (!view) return;

        int noteNum = yToNote(e.y);
        uint64_t start = xToFrame(e.x);
        if (start < view->startFrame || start >= view->startFrame + view->lengthFrames) return;

        if (snapDivision > 0) {
            start = snapFrame(start);
        }

        Orion::MidiNote n;
        n.noteNumber = noteNum;
        n.velocity = 0.8f;
        n.startFrame = start - view->startFrame + view->offsetFrames;
        n.lengthFrames = getDefaultNoteLengthFrames();

        commandHistory.push(std::make_unique<Orion::AddMidiNoteCommand>(activeClip, n));
        repaint();
    }
}






VelocityLaneComponent::VelocityLaneComponent(Orion::AudioEngine& e) : engine(e)
{
    setSize(2000, 100);
}

VelocityLaneComponent::~VelocityLaneComponent() {}

void VelocityLaneComponent::setClips(const std::vector<std::shared_ptr<Orion::MidiClip>>& clips,
                                     std::shared_ptr<Orion::MidiClip> active)
{
    clipViews.clear();
    activeClip = active;

    uint64_t maxEnd = 0;
    for (const auto& c : clips) {
        if (!c) continue;
        ClipView view;
        view.clip = c;
        view.startFrame = c->getStartFrame();
        view.lengthFrames = c->getLengthFrames();
        view.offsetFrames = c->getOffsetFrames();
        const float* col = c->getColor();
        view.color[0] = col[0];
        view.color[1] = col[1];
        view.color[2] = col[2];
        clipViews.push_back(view);

        uint64_t end = view.startFrame + view.lengthFrames;
        if (end > maxEnd) maxEnd = end;
    }

    double sr = engine.getSampleRate();
    if (sr <= 0.0) sr = 44100.0;
    if (maxEnd == 0) maxEnd = (uint64_t)(sr * 8.0);
    double lenSeconds = (double)maxEnd / sr;
    int w = (int)(lenSeconds * pixelsPerSecond) + 200;
    setSize(std::max(getParentWidth(), w), getHeight());
    repaint();
}

int VelocityLaneComponent::frameToX(uint64_t frame)
{
    double s = (double)frame / getSafeSampleRate(engine);
    return (int)(s * pixelsPerSecond);
}

const VelocityLaneComponent::ClipView* VelocityLaneComponent::getClipView(const std::shared_ptr<Orion::MidiClip>& clip) const
{
    if (!clip) return nullptr;
    for (const auto& view : clipViews) {
        if (view.clip == clip) return &view;
    }
    return nullptr;
}

void VelocityLaneComponent::paint(juce::Graphics& g)
{
    g.fillAll(findColour(OrionLookAndFeel::timelineBackgroundColourId).darker(0.2f));
    g.setColour(findColour(OrionLookAndFeel::timelineGridColourId));
    g.drawLine(0, 0, (float)getWidth(), 0);

    for (const auto& view : clipViews)
    {
        if (!view.clip) continue;
        int clipStartX = frameToX(view.startFrame);
        int clipEndX = frameToX(view.startFrame + view.lengthFrames);
        int clipWidth = clipEndX - clipStartX;
        if (clipWidth < 1) clipWidth = 1;

        juce::Colour clipColor = juce::Colour::fromFloatRGBA(view.color[0], view.color[1], view.color[2], 1.0f);
        bool isActive = (view.clip == activeClip);
        g.setColour(clipColor.withAlpha(isActive ? 0.08f : 0.04f));
        g.fillRect(clipStartX, 0, clipWidth, getHeight());
        g.setColour(clipColor.withAlpha(isActive ? 0.4f : 0.2f));
        g.drawVerticalLine(clipStartX, 0.0f, (float)getHeight());
        g.drawVerticalLine(clipStartX + clipWidth, 0.0f, (float)getHeight());

        auto notes = view.clip->getNotes();

        for (const auto& note : notes)
        {
            int64_t globalStart = (int64_t)view.startFrame + (int64_t)note.startFrame - (int64_t)view.offsetFrames;
            if (globalStart < 0) continue;
            int x = frameToX((uint64_t)globalStart);
            int w = frameToX(note.lengthFrames);
            if (w < 4) w = 4;

            float vel = note.velocity;
            float h = (float)getHeight() * vel;
            float y = (float)getHeight() - h;

            juce::Colour base = juce::Colour::fromFloatRGBA(view.color[0], view.color[1], view.color[2], 1.0f);
            g.setColour(base.withAlpha(isActive ? 0.8f : 0.35f));
            g.fillRect((float)x, y, (float)w, h);

            g.setColour(juce::Colours::white.withAlpha(isActive ? 0.3f : 0.15f));
            g.drawRect(x, (int)y, w, (int)h);

            g.setColour(juce::Colours::white.withAlpha(isActive ? 0.9f : 0.3f));
            g.fillRect(x, (int)y, w, 2);
        }
    }

    uint64_t playCursor = engine.getCursor();
    int playheadX = frameToX(playCursor);
    if (playheadX >= 0 && playheadX <= getWidth()) {
        g.setColour(juce::Colours::orange.withAlpha(0.95f));
        g.drawLine((float)playheadX, 0.0f, (float)playheadX, (float)getHeight(), 2.0f);
    }
}

int VelocityLaneComponent::getNoteIndexAt(int ) { return -1; }

void VelocityLaneComponent::mouseDown(const juce::MouseEvent& e)
{
    mouseDrag(e);
}

void VelocityLaneComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!activeClip) return;
    const ClipView* view = getClipView(activeClip);
    if (!view) return;

    auto& notes = activeClip->getNotesMutable();
    bool changed = false;

    for (auto& note : notes) {
        int64_t globalStart = (int64_t)view->startFrame + (int64_t)note.startFrame - (int64_t)view->offsetFrames;
        if (globalStart < 0) continue;
        int x = frameToX((uint64_t)globalStart);
        int w = frameToX(note.lengthFrames);
        if (w < 4) w = 4;

        if (e.x >= x && e.x <= x + w) {
            float newVel = 1.0f - ((float)e.y / (float)getHeight());
            newVel = juce::jlimit(0.0f, 1.0f, newVel);
            note.velocity = newVel;
            changed = true;
        }
    }

    if (changed) repaint();
}





PianoRollComponent::PianoRollComponent(Orion::AudioEngine& e, Orion::CommandHistory& history)
    : engine(e), grid(e, history), velocityLane(e), ruler(e)
{
    addAndMakeVisible(keysViewport);
    keysViewport.setViewedComponent(&keys, false);
    keysViewport.setScrollBarsShown(false, false);

    addAndMakeVisible(rulerViewport);
    rulerViewport.setViewedComponent(&ruler, false);
    rulerViewport.setScrollBarsShown(false, false);

    addAndMakeVisible(gridViewport);
    gridViewport.setViewedComponent(&grid, false);
    gridViewport.setScrollBarsShown(true, true);

    addAndMakeVisible(velocityViewport);
    velocityViewport.setViewedComponent(&velocityLane, false);
    velocityViewport.setScrollBarsShown(false, false);


    addAndMakeVisible(snapLabel);
    snapLabel.setText("Snap", juce::dontSendNotification);
    snapLabel.setJustificationType(juce::Justification::centredRight);
    snapLabel.setTooltip("Grid quantization used for draw/drag/quantize");

    addAndMakeVisible(clipInfoLabel);
    clipInfoLabel.setJustificationType(juce::Justification::centredLeft);
    clipInfoLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.78f));
    clipInfoLabel.setFont(juce::FontOptions(12.0f, juce::Font::plain));
    clipInfoLabel.setText("No clip selected", juce::dontSendNotification);
    clipInfoLabel.setTooltip("Current MIDI clip context");

    addAndMakeVisible(snapComboBox);
    snapComboBox.addItem("Off", 1);
    snapComboBox.addItem("1/4", 2);
    snapComboBox.addItem("1/8", 3);
    snapComboBox.addItem("1/16", 4);
    snapComboBox.addItem("1/32", 5);
    snapComboBox.setSelectedId(4);

    snapComboBox.onChange = [this] {
        int div = getSnapDivisionForId(snapComboBox.getSelectedId());
        grid.setSnap(div);

        juce::String label = "Snap";
        if (div == 0) label = "Snap Off";
        else label = "Snap 1/" + juce::String(div);
        snapLabel.setText(label, juce::dontSendNotification);
    };
    snapComboBox.onChange();

    addAndMakeVisible(quantizeButton);
    quantizeButton.setButtonText("Q");
    quantizeButton.setTooltip("Quantize selected clip to current snap");
    quantizeButton.onClick = [this] {
        if (!currentClip)
            return;

        const int div = getSnapDivisionForId(snapComboBox.getSelectedId());
        if (div <= 0)
            return;

        double bpm = engine.getBpm();
        if (bpm <= 0.1)
            bpm = 120.0;

        const double samplesPerBeat = (60.0 / bpm) * getSafeSampleRate(engine);
        const double gridFrames = samplesPerBeat * (4.0 / (double) div);
        if (gridFrames <= 0.0)
            return;

        currentClip->quantize(gridFrames);
        grid.repaint();
        velocityLane.repaint();
        refreshClipInfo();
    };

    addAndMakeVisible(fitClipButton);
    fitClipButton.setButtonText("Fit");
    fitClipButton.setTooltip("Fit current clip to editor width");
    fitClipButton.onClick = [this] {
        if (!currentClip)
            return;

        const double sr = getSafeSampleRate(engine);
        const int viewportWidth = juce::jmax(180, gridViewport.getViewWidth());
        const double clipSeconds = (double) juce::jmax<uint64_t>(1, currentClip->getLengthFrames()) / sr;
        const double targetPps = juce::jlimit(20.0, 2000.0, (double) (viewportWidth - 80) / clipSeconds);
        grid.setPixelsPerSecond(targetPps);

        const int startX = juce::jmax(0, (int) (((double) currentClip->getStartFrame() / sr) * targetPps) - 40);
        gridViewport.setViewPosition(startX, gridViewport.getViewPositionY());
    };

    addAndMakeVisible(resetZoomButton);
    resetZoomButton.setButtonText("Reset");
    resetZoomButton.setTooltip("Reset piano roll zoom");
    resetZoomButton.onClick = [this] {
        grid.setPixelsPerSecond(100.0);
        grid.setKeyHeight(16.0f);
        keys.setKeyHeight(16.0f);
        keys.setSize(60, 128 * 16);
    };

    addAndMakeVisible(followButton);
    followButton.setButtonText("Follow");
    followButton.setToggleState(true, juce::dontSendNotification);
    followButton.setTooltip("Keep the playhead visible while playback is running");

    addAndMakeVisible(singleClickDrawToggle);
    singleClickDrawToggle.setButtonText("Click Draw");
    singleClickDrawToggle.setToggleState(false, juce::dontSendNotification);
    singleClickDrawToggle.setTooltip("Single-click in grid to insert notes");
    singleClickDrawToggle.onClick = [this] {
        grid.setSingleClickDrawEnabled(singleClickDrawToggle.getToggleState());
    };

    addAndMakeVisible(selectToolButton);
    selectToolButton.setButtonText("Select");
    selectToolButton.setToggleState(true, juce::dontSendNotification);
    selectToolButton.setTooltip("Select notes (S)");
    selectToolButton.onClick = [this] { setTool(PianoRollTool::Select); };

    addAndMakeVisible(drawToolButton);
    drawToolButton.setButtonText("Draw");
    drawToolButton.setToggleState(false, juce::dontSendNotification);
    drawToolButton.setTooltip("Draw notes (D)");
    drawToolButton.onClick = [this] { setTool(PianoRollTool::Draw); };

    addAndMakeVisible(eraseToolButton);
    eraseToolButton.setButtonText("Erase");
    eraseToolButton.setToggleState(false, juce::dontSendNotification);
    eraseToolButton.setTooltip("Erase notes (E)");
    eraseToolButton.onClick = [this] { setTool(PianoRollTool::Erase); };

    grid.setSingleClickDrawEnabled(singleClickDrawToggle.getToggleState());

    addAndMakeVisible(swingButton);
    swingButton.setButtonText("Swing (Soon)");
    swingButton.setEnabled(false);
    swingButton.setTooltip("Swing quantize is planned but not yet available");


    keys.onNoteOn = [this](int note) {
        if (currentTrack) currentTrack->startPreviewNote(note, 0.8f);
    };
    keys.onNoteOff = [this](int note) {
        if (currentTrack) currentTrack->stopPreviewNote(note);
    };

    keys.onMouseWheelRelay = [this](const juce::MouseWheelDetails& wheel) {

        double currentY = gridViewport.getViewPositionY();
        double delta = wheel.deltaY * 500.0;
        gridViewport.setViewPosition(gridViewport.getViewPositionX(), (int)(currentY - delta));
    };

    grid.onKeyHeightChanged = [this](float h) {
        keys.setKeyHeight(h);
        keys.setSize(60, 128 * (int)h);
    };

    grid.onPixelsPerSecondChanged = [this](double pps) {
        velocityLane.setPixelsPerSecond(pps);
        ruler.setPixelsPerSecond(pps);
    };

    grid.onActiveClipChanged = [this](std::shared_ptr<Orion::MidiClip> clip) {
        currentClip = clip;
        std::vector<std::shared_ptr<Orion::MidiClip>> clips;
        if (currentTrack) {
            auto shared = currentTrack->getMidiClips();
            if (shared) clips = *shared;
        }
        velocityLane.setClips(clips, currentClip);
        refreshClipInfo();
    };

    grid.setPixelsPerSecond(100.0);

    grid.addComponentListener(this);

    setWantsKeyboardFocus(true);
    startTimer(30);
    refreshClipInfo();
}

PianoRollComponent::~PianoRollComponent() {}

void PianoRollComponent::setClip(std::shared_ptr<Orion::MidiClip> c, std::shared_ptr<Orion::InstrumentTrack> t)
{
    currentTrack = t;

    std::vector<std::shared_ptr<Orion::MidiClip>> clips;
    if (currentTrack) {
        auto shared = currentTrack->getMidiClips();
        if (shared) clips = *shared;
    } else if (c) {
        clips.push_back(c);
    }

    grid.setClips(clips, c);
    currentClip = grid.getActiveClip();
    velocityLane.setClips(clips, currentClip);

    if (!currentClip && !clips.empty()) {
        currentClip = clips.front();
    }

    refreshClipInfo();
    grid.repaint();
    velocityLane.repaint();
    repaint();
}

void PianoRollComponent::paint(juce::Graphics& g)
{
    g.fillAll(findColour(OrionLookAndFeel::mixerChassisColourId));

    if (!currentClip && Orion::SettingsManager::get().hintPolicy.contextualHintsEnabled) {
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
        g.drawText("No MIDI clip selected", getLocalBounds().removeFromTop(28), juce::Justification::centred, true);
        g.setColour(juce::Colours::white.withAlpha(0.28f));
        g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
        g.drawText("Double-click an instrument clip in Timeline, then use Draw or Click Draw.",
                   getLocalBounds().withTop(26), juce::Justification::centredTop, true);
    }
}

void PianoRollComponent::resized()
{
    auto area = getLocalBounds();

    auto toolbar = area.removeFromTop(34);

    auto toolArea = toolbar.removeFromLeft(220);
    selectToolButton.setBounds(toolArea.removeFromLeft(70).reduced(2));
    drawToolButton.setBounds(toolArea.removeFromLeft(70).reduced(2));
    eraseToolButton.setBounds(toolArea.removeFromLeft(70).reduced(2));

    auto rightControls = toolbar.removeFromRight(juce::jmin(toolbar.getWidth(), 540));
    followButton.setBounds(rightControls.removeFromRight(72).reduced(2));
    swingButton.setBounds(rightControls.removeFromRight(104).reduced(2));
    resetZoomButton.setBounds(rightControls.removeFromRight(64).reduced(2));
    fitClipButton.setBounds(rightControls.removeFromRight(56).reduced(2));
    singleClickDrawToggle.setBounds(rightControls.removeFromRight(90).reduced(2));
    snapComboBox.setBounds(rightControls.removeFromRight(78).reduced(2));
    snapLabel.setBounds(rightControls.removeFromRight(74).reduced(2));
    quantizeButton.setBounds(rightControls.removeFromRight(34).reduced(2));
    clipInfoLabel.setBounds(toolbar.reduced(8, 2));

    auto rulerArea = area.removeFromTop(24);
    auto velocityArea = area.removeFromBottom(100);
    auto keysArea = area.removeFromLeft(60);

    keysViewport.setBounds(keysArea);
    gridViewport.setBounds(area);

    auto rulerSpacer = rulerArea.removeFromLeft(60);
    juce::ignoreUnused(rulerSpacer);
    rulerViewport.setBounds(rulerArea);

    auto velocitySpacer = velocityArea.removeFromLeft(60);
    juce::ignoreUnused(velocitySpacer);
    velocityViewport.setBounds(velocityArea);

    velocityLane.setSize(grid.getWidth(), 100);
    ruler.setSize(grid.getWidth(), rulerViewport.getHeight());
    keys.setSize(60, 128 * (int)keys.getKeyHeight());
}

void PianoRollComponent::timerCallback()
{

    if (velocityLane.getWidth() != grid.getWidth())
        velocityLane.setSize(grid.getWidth(), 100);

    keysViewport.setViewPosition(keysViewport.getViewPositionX(), gridViewport.getViewPositionY());
    velocityViewport.setViewPosition(gridViewport.getViewPositionX(), 0);
    rulerViewport.setViewPosition(gridViewport.getViewPositionX(), 0);

    if (ruler.getWidth() != grid.getWidth())
        ruler.setSize(grid.getWidth(), rulerViewport.getHeight());

    refreshClipInfo();

    auto transportState = engine.getTransportState();
    const bool isRunning = (transportState == Orion::TransportState::Playing
                         || transportState == Orion::TransportState::Recording);
    if (isRunning) {
        if (followButton.getToggleState()) {
            int playheadX = grid.getPlayheadX();
            if (playheadX >= 0) {
                int viewX = gridViewport.getViewPositionX();
                int viewW = gridViewport.getViewWidth();
                const int rightLimit = viewX + viewW - juce::jmax(60, viewW / 4);
                if (playheadX < viewX || playheadX > rightLimit) {
                    int newX = juce::jmax(0, playheadX - viewW / 3);
                    gridViewport.setViewPosition(newX, gridViewport.getViewPositionY());
                }
            }
        }

        grid.repaint();
        velocityLane.repaint();
    }
}

void PianoRollComponent::refreshClipInfo()
{
    if (!currentClip)
    {
        clipInfoLabel.setText("No MIDI clip selected", juce::dontSendNotification);
        return;
    }

    juce::String trackName = currentTrack ? juce::String(currentTrack->getName()) : "Standalone Clip";
    const auto notesCount = currentClip->getNotes().size();
    juce::String info;
    info << trackName << " / " << currentClip->getName()
         << "  (" << (int)notesCount << " notes)";
    clipInfoLabel.setText(info, juce::dontSendNotification);
}

void PianoRollComponent::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized)
{
    juce::ignoreUnused(wasResized);
    if (&component == &grid && wasMoved) {

        keysViewport.setViewPosition(keysViewport.getViewPositionX(), gridViewport.getViewPositionY());

        velocityViewport.setViewPosition(gridViewport.getViewPositionX(), 0);
        rulerViewport.setViewPosition(gridViewport.getViewPositionX(), 0);
    }
}

bool PianoRollComponent::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
        grid.deleteSelected();
        return true;
    }

    if (!key.getModifiers().isCommandDown()
        && !key.getModifiers().isCtrlDown()
        && !key.getModifiers().isAltDown()) {
        auto ch = juce::CharacterFunctions::toLowerCase(key.getTextCharacter());
        if (ch == 's') {
            setTool(PianoRollTool::Select);
            return true;
        }
        if (ch == 'd') {
            setTool(PianoRollTool::Draw);
            return true;
        }
        if (ch == 'e') {
            setTool(PianoRollTool::Erase);
            return true;
        }
    }

    if (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown()) {
        if (key.getKeyCode() == 'D') {
            grid.duplicateSelected();
            return true;
        }
        if (key.getKeyCode() == 'C') {
            grid.copySelected();
            return true;
        }
        if (key.getKeyCode() == 'V') {
            grid.paste();
            return true;
        }
    }
    return false;
}

void PianoRollComponent::setTool(PianoRollTool tool)
{
    currentTool = tool;
    selectToolButton.setToggleState(tool == PianoRollTool::Select, juce::dontSendNotification);
    drawToolButton.setToggleState(tool == PianoRollTool::Draw, juce::dontSendNotification);
    eraseToolButton.setToggleState(tool == PianoRollTool::Erase, juce::dontSendNotification);
    grid.setTool(tool);
}
