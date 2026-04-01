#pragma once
#include <JuceHeader.h>
#include "orion/AudioEngine.h"
#include "orion/AudioTrack.h"
#include "orion/AudioClip.h"
#include "orion/MidiClip.h"
#include "../CommandManager.h"
#include "orion/CommandHistory.h"
#include "RulerComponent.h"


class TrackHeaderComponent;
class TimelineLaneComponent;
class TimelineEmptyStateOverlay;

enum class TimelineTool { Select, Draw, Erase, Split, Glue, Zoom, Mute, Listen };

class GlobalPlayheadOverlay : public juce::Component
{
public:
    GlobalPlayheadOverlay(Orion::AudioEngine& e) : engine(e)
    {
        setInterceptsMouseClicks(false, false);
    }

    void setParams(double pps, int scrollX)
    {
        pixelsPerSecond = pps;
        currentScrollX = scrollX;
    }

    void paint(juce::Graphics& g) override;

private:
    double frameToX(uint64_t frame) const;

    Orion::AudioEngine& engine;
    double pixelsPerSecond = 50.0;
    int currentScrollX = 0;
};




class TimelineContentComponent : public juce::Component
{
public:
    TimelineContentComponent(Orion::AudioEngine& engine);
    ~TimelineContentComponent() override;

    void mouseDown(const juce::MouseEvent& e) override;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void updatePlayhead() {}

    void updateLanes();
    void setPixelsPerSecond(double pps);
    double getPixelsPerSecond() const { return pixelsPerSecond; }

    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;


    void setSnapFunction(std::function<uint64_t(uint64_t)> fn) { snapFunc = fn; }


    void setToolGetter(std::function<TimelineTool()> getter) { toolGetter = getter; }

    void setTrackHeight(int h);
    int getTrackHeight() const { return trackHeight; }
    std::function<void(int)> onTrackHeightChanged;

    void setSelectionState(const std::function<bool(std::shared_ptr<Orion::AudioClip>)>& isSelectedAudio,
                           const std::function<bool(std::shared_ptr<Orion::MidiClip>)>& isSelectedMidi);

    std::function<void(std::shared_ptr<Orion::AudioClip>, bool)> onClipSelected;
    std::function<void(std::shared_ptr<Orion::MidiClip>, std::shared_ptr<Orion::InstrumentTrack>, bool)> onMidiClipSelected;
    std::function<void(std::shared_ptr<Orion::MidiClip>, std::shared_ptr<Orion::InstrumentTrack>)> onMidiClipOpened;

    std::function<void(const juce::MouseEvent&)> onBackgroundMouseDown;
    std::function<void(const juce::MouseEvent&)> onBackgroundMouseDrag;
    std::function<void(const juce::MouseEvent&)> onBackgroundMouseUp;

private:
    Orion::AudioEngine& engine;
    std::vector<std::unique_ptr<TimelineLaneComponent>> lanes;

    double pixelsPerSecond = 50.0;
    int trackHeight = 100;

    std::function<uint64_t(uint64_t)> snapFunc;
    std::function<TimelineTool()> toolGetter;

    friend class TimelineComponent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineContentComponent)
};




class TrackListComponent : public juce::Component
{
public:
    TrackListComponent(Orion::AudioEngine& engine, Orion::CommandHistory& history);
    ~TrackListComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateHeaders();
    void setTrackHeight(int h);
    void setSelectedTrack(std::shared_ptr<Orion::AudioTrack> track);
    void duplicateTrack(std::shared_ptr<Orion::AudioTrack> track);

    std::function<void(std::shared_ptr<Orion::AudioTrack>)> onTrackSelected;
    std::function<void(std::shared_ptr<Orion::AudioTrack>)> onTrackDeleted;
    std::function<void(std::shared_ptr<Orion::AudioTrack>)> onTrackSelectAllClips;
    std::function<void(std::shared_ptr<Orion::AudioTrack>, int)> onTrackDragToIndex;
    std::function<void()> onLayoutChanged;
    std::function<void(int)> onTrackHeightChanged;

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    Orion::AudioEngine& engine;
    Orion::CommandHistory& commandHistory;
    std::vector<std::unique_ptr<TrackHeaderComponent>> headers;
    int trackHeight = 100;
    bool isReorderingTrack = false;
    std::shared_ptr<Orion::AudioTrack> reorderTrack;
    int reorderTargetIndex = -1;

    int getTrackHeightAtIndex(int index) const;
    int getTrackIndexForTrack(const std::shared_ptr<Orion::AudioTrack>& track) const;
    int getDropIndexForY(int y) const;
    int getTrackTopForIndex(int index) const;
    void clearTrackReorderPreview();

    friend class TimelineComponent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackListComponent)
};





class SyncViewport : public juce::Viewport
{
public:
    std::function<void(int, int)> positionMoved;

    void visibleAreaChanged(const juce::Rectangle<int>& newVisibleArea) override
    {
        if (positionMoved)
            positionMoved(newVisibleArea.getX(), newVisibleArea.getY());

        juce::Viewport::visibleAreaChanged(newVisibleArea);
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {

        if (e.mods.isShiftDown() && wheel.deltaX == 0 && wheel.deltaY != 0)
        {

            juce::MouseWheelDetails wheel2 = wheel;
            wheel2.deltaX = wheel.deltaY;
            wheel2.deltaY = 0;



            juce::MouseEvent e2 (e.source, e.position, e.mods.withoutFlags(juce::ModifierKeys::shiftModifier), e.pressure, e.orientation, e.rotation, e.tiltX, e.tiltY, e.eventComponent, e.originalComponent, e.eventTime, e.mouseDownPosition, e.mouseDownTime, e.getNumberOfClicks(), e.mouseWasDraggedSinceMouseDown());

            juce::Viewport::mouseWheelMove(e2, wheel2);
            return;
        }
        juce::Viewport::mouseWheelMove(e, wheel);
    }
};

class TimelineComponent : public juce::Component,
                          public juce::Timer,
                          public juce::FileDragAndDropTarget,
                          public juce::DragAndDropTarget
{
public:
    TimelineComponent(Orion::AudioEngine& engine, Orion::CommandHistory& history);
    ~TimelineComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void lookAndFeelChanged() override;


    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragMove(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;


    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    bool keyPressed(const juce::KeyPress& key) override;

    void paintOverChildren(juce::Graphics& g) override;

    std::function<void(std::shared_ptr<Orion::AudioClip>)> onClipSelected;
    std::function<void(std::shared_ptr<Orion::MidiClip>, std::shared_ptr<Orion::InstrumentTrack>, bool)> onMidiClipSelected;
    std::function<void(std::shared_ptr<Orion::MidiClip>, std::shared_ptr<Orion::InstrumentTrack>)> onMidiClipOpened;
    std::function<void(std::shared_ptr<Orion::AudioTrack>)> onTrackSelected;

    // Lets other views (e.g. Mixer) keep selection in sync.
    void setSelectedTrack(std::shared_ptr<Orion::AudioTrack> track);
    std::shared_ptr<Orion::AudioTrack> getSelectedTrack() const { return selection.track; }


    void setTool(TimelineTool t);
    TimelineTool getTool() const { return currentTool; }
    void deleteSelection();
    void duplicateSelection();
    void toggleMute();


    void setTrackHeight(int h);

    double getPixelsPerSecond() const { return timelineContent.getPixelsPerSecond(); }
    void setPixelsPerSecond(double pps) { timelineContent.setPixelsPerSecond(pps); }
    int getTrackHeight() const { return trackHeight; }
    int getSnapDivision() const { return snapDivision; }
    void setSnapDivision(int div);
    bool hasAnyMidiClip() const;
    bool createStarterMidiClip();

    int getScrollX() const { return timelineViewport.getViewPositionX(); }
    int getScrollY() const { return timelineViewport.getViewPositionY(); }
    void setViewPosition(int x, int y) { timelineViewport.setViewPosition(x, y); }

    void startPanning(juce::Point<int> screenPos);
    void dragPanning(juce::Point<int> screenPos);
    void stopPanning();
    void beginListenAt(const juce::MouseEvent& e);
    void updateListenAt(const juce::MouseEvent& e);
    void endListen();

    void pushCommand(std::unique_ptr<Orion::Command> cmd);

    void refresh();

    std::function<void(int)> onInvokeCommand;
    std::function<void()> onShowGuidedStart;

private:
    Orion::AudioEngine& engine;
    Orion::CommandHistory& commandHistory;

public:

    struct Selection {
        std::set<std::shared_ptr<Orion::AudioClip>> clips;
        std::set<std::shared_ptr<Orion::MidiClip>> midiClips;
        std::shared_ptr<Orion::AudioTrack> track;

        void clear() { clips.clear(); midiClips.clear(); track = nullptr; }
        bool isEmpty() const { return clips.empty() && midiClips.empty(); }
        bool contains(std::shared_ptr<Orion::AudioClip> c) const { return clips.find(c) != clips.end(); }
        bool contains(std::shared_ptr<Orion::MidiClip> c) const { return midiClips.find(c) != midiClips.end(); }
        void add(std::shared_ptr<Orion::AudioClip> c) { clips.insert(c); }
        void add(std::shared_ptr<Orion::MidiClip> c) { midiClips.insert(c); }
        void remove(std::shared_ptr<Orion::AudioClip> c) { clips.erase(c); }
        void remove(std::shared_ptr<Orion::MidiClip> c) { midiClips.erase(c); }
        void toggle(std::shared_ptr<Orion::AudioClip> c) {
            if (contains(c)) remove(c); else add(c);
        }
        void toggle(std::shared_ptr<Orion::MidiClip> c) {
            if (contains(c)) remove(c); else add(c);
        }
    } selection;

    const Selection& getSelection() const { return selection; }

private:
    TimelineTool currentTool = TimelineTool::Select;


    bool isRubberBandSelecting = false;
    juce::Rectangle<int> rubberBandRect;
    juce::Point<int> rubberBandStart;

    bool isListening = false;
    bool listenWasPlaying = false;


    bool isPanning = false;
    juce::Point<int> lastPanPos;

    bool isDragging = false;
    int dragX = 0;
    int dragY = 0;
    int dragTargetTrackIndex = -1;


    std::shared_ptr<Orion::AudioClip> draggedAudioClip;
    std::shared_ptr<Orion::MidiClip> draggedMidiClip;
    int dragOffsetPixels = 0;

    struct DragOriginal {
        uint64_t startFrame;
        std::shared_ptr<Orion::AudioTrack> track;
    };
    std::map<void*, DragOriginal> dragOriginals;


    SyncViewport headerViewport;
    SyncViewport timelineViewport;
    juce::Viewport rulerViewport;


    TrackListComponent trackList;
    TimelineContentComponent timelineContent;
    RulerComponent ruler;
    GlobalPlayheadOverlay playheadOverlay{ engine };
    std::unique_ptr<TimelineEmptyStateOverlay> emptyStateOverlay;


    juce::ComboBox snapComboBox;
    juce::Label snapLabel;


    juce::TextButton addTrackButton;

    int snapDivision = 16;
    int trackHeight = 100;


    uint64_t snapFrame(uint64_t frame);
    void updateSelectionState();
    void updateEmptyStateOverlay();
    bool hasAnyClip() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineComponent)
};
