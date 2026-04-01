#pragma once

#include <JuceHeader.h>
#include "orion/MidiClip.h"
#include "orion/AudioEngine.h"
#include "orion/InstrumentTrack.h"
#include "orion/CommandHistory.h"
#include "RulerComponent.h"

enum class PianoRollTool { Select, Draw, Erase };

class PianoKeysComponent : public juce::Component
{
public:
    PianoKeysComponent();
    ~PianoKeysComponent() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    float getKeyHeight() const { return keyHeight; }
    void setKeyHeight(float h) { keyHeight = h; repaint(); }

    std::function<void(int)> onNoteOn;
    std::function<void(int)> onNoteOff;
    std::function<void(const juce::MouseWheelDetails&)> onMouseWheelRelay;

private:
    float keyHeight = 16.0f;
    int lastClickedNote = -1;

    bool isBlackKey(int note) const;
    int getNoteAt(int y) const;
};

class MidiGridComponent : public juce::Component
{
public:
    MidiGridComponent(Orion::AudioEngine& engine, Orion::CommandHistory& history);
    ~MidiGridComponent() override;

    void setClips(const std::vector<std::shared_ptr<Orion::MidiClip>>& clips,
                  std::shared_ptr<Orion::MidiClip> activeClip);
    void setTool(PianoRollTool tool) { currentTool = tool; }
    std::shared_ptr<Orion::MidiClip> getActiveClip() const { return activeClip; }

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    void duplicateSelected();
    void copySelected();
    void paste();
    void deleteSelected();

    void setKeyHeight(float h) { keyHeight = h; repaint(); }
    void setPixelsPerSecond(double pps);
    int getPlayheadX() const;

    std::function<void(float)> onKeyHeightChanged;
    std::function<void(double)> onPixelsPerSecondChanged;
    std::function<void(std::shared_ptr<Orion::MidiClip>)> onActiveClipChanged;


    void setSnap(int division) { snapDivision = division; }
    void setSingleClickDrawEnabled(bool enabled) { singleClickDrawEnabled = enabled; }

    const std::vector<int>& getSelectedIndices() const { return selectedIndices; }

private:
    Orion::AudioEngine& engine;
    Orion::CommandHistory& commandHistory;
    std::shared_ptr<Orion::MidiClip> activeClip;

    struct ClipView {
        std::shared_ptr<Orion::MidiClip> clip;
        uint64_t startFrame = 0;
        uint64_t lengthFrames = 0;
        uint64_t offsetFrames = 0;
        float color[3] = { 0.0f, 0.8f, 0.0f };
    };
    std::vector<ClipView> clipViews;

    float keyHeight = 16.0f;
    double pixelsPerSecond = 100.0;
    int snapDivision = 16;


    std::vector<int> selectedIndices;
    std::shared_ptr<Orion::MidiClip> selectedClip;
    bool isDragging = false;
    bool isResizing = false;
    bool isSelecting = false;
    juce::Rectangle<int> selectionRect;


    static std::vector<Orion::MidiNote> clipboard;

    int dragStartX = 0;
    int dragStartY = 0;
    int leadNoteIndex = -1;
    bool showHoverGuides = false;
    bool singleClickDrawEnabled = false;
    int previewNote = -1;
    uint64_t previewStartFrame = 0;
    uint64_t previewLengthFrames = 0;

    struct DragState {
        std::shared_ptr<Orion::MidiClip> clip;
        int index;
        uint64_t startFrame;
        uint64_t lengthFrames;
        int noteNumber;
    };
    std::vector<DragState> dragStates;
    PianoRollTool currentTool = PianoRollTool::Select;

    struct NoteRef {
        std::shared_ptr<Orion::MidiClip> clip;
        int index = -1;
    };

    NoteRef getNoteAt(int x, int y);
    uint64_t xToFrame(int x);
    int frameToX(uint64_t frame) const;
    int noteToY(int note);
    int yToNote(int y);
    uint64_t getDefaultNoteLengthFrames() const;
    const ClipView* getClipForFrame(uint64_t frame) const;
    const ClipView* getClipView(const std::shared_ptr<Orion::MidiClip>& clip) const;
    bool isNoteSelected(const std::shared_ptr<Orion::MidiClip>& clip, int index) const;
    void setActiveClip(std::shared_ptr<Orion::MidiClip> clip);

    uint64_t snapFrame(uint64_t frame);
};

class VelocityLaneComponent : public juce::Component
{
public:
    VelocityLaneComponent(Orion::AudioEngine& engine);
    ~VelocityLaneComponent() override;

    void setClips(const std::vector<std::shared_ptr<Orion::MidiClip>>& clips,
                  std::shared_ptr<Orion::MidiClip> activeClip);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void setPixelsPerSecond(double pps) { pixelsPerSecond = pps; repaint(); }

private:
    Orion::AudioEngine& engine;
    std::shared_ptr<Orion::MidiClip> activeClip;
    struct ClipView {
        std::shared_ptr<Orion::MidiClip> clip;
        uint64_t startFrame = 0;
        uint64_t lengthFrames = 0;
        uint64_t offsetFrames = 0;
        float color[3] = { 0.0f, 0.8f, 0.0f };
    };
    std::vector<ClipView> clipViews;
    double pixelsPerSecond = 100.0;

    int getNoteIndexAt(int x);
    int frameToX(uint64_t frame);
    const ClipView* getClipView(const std::shared_ptr<Orion::MidiClip>& clip) const;
};

class PianoRollComponent : public juce::Component, public juce::Timer, public juce::ComponentListener
{
public:
    PianoRollComponent(Orion::AudioEngine& engine, Orion::CommandHistory& history);
    ~PianoRollComponent() override;

    void setClip(std::shared_ptr<Orion::MidiClip> clip, std::shared_ptr<Orion::InstrumentTrack> track = nullptr);

    std::shared_ptr<Orion::MidiClip> getCurrentClip() const { return currentClip; }
    std::shared_ptr<Orion::InstrumentTrack> getCurrentTrack() const { return currentTrack; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    bool keyPressed(const juce::KeyPress& key) override;

    void componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) override;

    const MidiGridComponent& getGrid() const { return grid; }

private:
    Orion::AudioEngine& engine;
    std::shared_ptr<Orion::MidiClip> currentClip;
    std::shared_ptr<Orion::InstrumentTrack> currentTrack;

    PianoKeysComponent keys;
    MidiGridComponent grid;
    VelocityLaneComponent velocityLane;
    RulerComponent ruler;

    juce::Viewport keysViewport;
    juce::Viewport rulerViewport;
    juce::Viewport gridViewport;
    juce::Viewport velocityViewport;

    juce::ComboBox snapComboBox;
    juce::Label snapLabel;
    juce::Label clipInfoLabel;
    juce::TextButton quantizeButton;
    juce::TextButton fitClipButton;
    juce::TextButton resetZoomButton;
    juce::ToggleButton followButton;
    juce::ToggleButton singleClickDrawToggle;
    juce::TextButton swingButton;

    juce::ToggleButton selectToolButton;
    juce::ToggleButton drawToolButton;
    juce::ToggleButton eraseToolButton;
    PianoRollTool currentTool = PianoRollTool::Select;

    void setTool(PianoRollTool tool);
    void refreshClipInfo();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};
