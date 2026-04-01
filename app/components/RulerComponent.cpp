#include "RulerComponent.h"
#include "MarkerListComponent.h"
#include "../OrionLookAndFeel.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

RulerComponent::RulerComponent(Orion::AudioEngine& e) : engine(e)
{
    startTimerHz(30);
}

RulerComponent::~RulerComponent()
{
}

void RulerComponent::setPixelsPerSecond(double pps)
{
    pixelsPerSecond = pps;
    repaint();
}

double RulerComponent::getXForFrame(uint64_t targetFrame) const
{
    double sampleRate = engine.getSampleRate();
    if (sampleRate <= 0) return 0;

    auto tempos = engine.getTempoMarkers();
    auto sigs = engine.getTimeSigMarkers();

    uint64_t currentFrame = 0;
    double currentX = 0;


    float currentBpm = engine.getBpm();
    int currentNum = 4;
    int currentDen = 4;
    engine.getTimeSignature(currentNum, currentDen);

    size_t tIdx = 0;
    size_t sIdx = 0;


    while (tIdx < tempos.size() && tempos[tIdx].position <= 0) {
        currentBpm = tempos[tIdx].bpm;
        tIdx++;
    }
    while (sIdx < sigs.size() && sigs[sIdx].position <= 0) {
        currentNum = sigs[sIdx].numerator;
        currentDen = sigs[sIdx].denominator;
        sIdx++;
    }


    while (currentFrame < targetFrame)
    {

        while (tIdx < tempos.size() && tempos[tIdx].position <= currentFrame) {
            currentBpm = tempos[tIdx].bpm;
            tIdx++;
        }
        while (sIdx < sigs.size() && sigs[sIdx].position <= currentFrame) {
            currentNum = sigs[sIdx].numerator;
            currentDen = sigs[sIdx].denominator;
            sIdx++;
        }

        double quartersPerBar = (double)currentNum * (4.0 / (double)currentDen);
        double secondsPerBar = quartersPerBar * (60.0 / currentBpm);
        uint64_t framesPerBar = (uint64_t)(secondsPerBar * sampleRate);
        if (framesPerBar == 0) framesPerBar = 1;

        if (currentFrame + framesPerBar > targetFrame) {

            double fraction = (double)(targetFrame - currentFrame) / (double)framesPerBar;
            return currentX + (fraction * (secondsPerBar * pixelsPerSecond));
        }

        currentX += secondsPerBar * pixelsPerSecond;
        currentFrame += framesPerBar;
    }

    return currentX;
}

uint64_t RulerComponent::getFrameForX(double targetX) const
{
    double sampleRate = engine.getSampleRate();
    if (sampleRate <= 0) return 0;

    auto tempos = engine.getTempoMarkers();
    auto sigs = engine.getTimeSigMarkers();

    uint64_t currentFrame = 0;
    double currentX = 0;

    float currentBpm = engine.getBpm();
    int currentNum = 4;
    int currentDen = 4;
    engine.getTimeSignature(currentNum, currentDen);

    size_t tIdx = 0;
    size_t sIdx = 0;


    while (tIdx < tempos.size() && tempos[tIdx].position <= 0) {
        currentBpm = tempos[tIdx].bpm;
        tIdx++;
    }
    while (sIdx < sigs.size() && sigs[sIdx].position <= 0) {
        currentNum = sigs[sIdx].numerator;
        currentDen = sigs[sIdx].denominator;
        sIdx++;
    }



    int safety = 0;
    while (currentX < targetX && safety++ < 100000)
    {
        while (tIdx < tempos.size() && tempos[tIdx].position <= currentFrame) {
            currentBpm = tempos[tIdx].bpm;
            tIdx++;
        }
        while (sIdx < sigs.size() && sigs[sIdx].position <= currentFrame) {
            currentNum = sigs[sIdx].numerator;
            currentDen = sigs[sIdx].denominator;
            sIdx++;
        }

        double quartersPerBar = (double)currentNum * (4.0 / (double)currentDen);
        double secondsPerBar = quartersPerBar * (60.0 / currentBpm);
        double width = secondsPerBar * pixelsPerSecond;
        uint64_t framesPerBar = (uint64_t)(secondsPerBar * sampleRate);
        if (framesPerBar == 0) framesPerBar = 1;

        if (currentX + width >= targetX) {
            double offset = targetX - currentX;
            double fraction = offset / width;
            return currentFrame + (uint64_t)(fraction * framesPerBar);
        }

        currentX += width;
        currentFrame += framesPerBar;
    }

    return currentFrame;
}

void RulerComponent::paint(juce::Graphics& g)
{

    g.fillAll(findColour(OrionLookAndFeel::timelineRulerBackgroundColourId));
    g.setColour(findColour(OrionLookAndFeel::timelineGridColourId));
    g.drawRect(getLocalBounds().removeFromBottom(1));

    double sampleRate = engine.getSampleRate();
    if (sampleRate <= 0) return;


    auto tempos = engine.getTempoMarkers();
    auto sigs = engine.getTimeSigMarkers();

    uint64_t currentFrame = 0;
    double currentX = 0;

    float currentBpm = engine.getBpm();
    int currentNum = 4;
    int currentDen = 4;
    engine.getTimeSignature(currentNum, currentDen);

    size_t tIdx = 0;
    size_t sIdx = 0;





    auto clip = g.getClipBounds();
    double visibleLeft = clip.getX();
    double visibleRight = clip.getRight();

    int barIndex = 1;


    while (tIdx < tempos.size() && tempos[tIdx].position <= 0) {
        currentBpm = tempos[tIdx].bpm;
        tIdx++;
    }
    while (sIdx < sigs.size() && sigs[sIdx].position <= 0) {
        currentNum = sigs[sIdx].numerator;
        currentDen = sigs[sIdx].denominator;
        sIdx++;
    }


    if (engine.isLooping()) {
        uint64_t start = engine.getLoopStart();
        uint64_t end = engine.getLoopEnd();
        if (end > start) {
            double x1 = getXForFrame(start);
            double x2 = getXForFrame(end);


            g.setColour(juce::Colours::yellow.withAlpha(0.2f));
            g.fillRect((float)x1, 0.0f, (float)(x2 - x1), (float)getHeight());


            g.setColour(juce::Colours::yellow);
            g.drawLine((float)x1, 0.0f, (float)x1, (float)getHeight(), 1.0f);
            g.drawLine((float)x2, 0.0f, (float)x2, (float)getHeight(), 1.0f);


            juce::Path p;
            p.addTriangle((float)x1, 0.0f, (float)x1 + 6.0f, 0.0f, (float)x1, 6.0f);
            g.fillPath(p);

            juce::Path p2;
            p2.addTriangle((float)x2, 0.0f, (float)x2 - 6.0f, 0.0f, (float)x2, 6.0f);
            g.fillPath(p2);
        }
    }






    int safety = 0;
    while (currentX < visibleRight + 100 && safety++ < 10000)
    {

        bool changed = false;
        while (tIdx < tempos.size() && tempos[tIdx].position <= currentFrame) {
            currentBpm = tempos[tIdx].bpm;
            tIdx++;
            changed = true;
        }
        while (sIdx < sigs.size() && sigs[sIdx].position <= currentFrame) {
            currentNum = sigs[sIdx].numerator;
            currentDen = sigs[sIdx].denominator;
            sIdx++;
            changed = true;
        }

        double quartersPerBar = (double)currentNum * (4.0 / (double)currentDen);
        double secondsPerBar = quartersPerBar * (60.0 / currentBpm);
        double barWidth = secondsPerBar * pixelsPerSecond;
        uint64_t framesPerBar = (uint64_t)(secondsPerBar * sampleRate);
        if (framesPerBar == 0) framesPerBar = 1;


        int drawInterval = 1;
        if (barWidth < 10.0) drawInterval = 16;
        else if (barWidth < 20.0) drawInterval = 8;
        else if (barWidth < 40.0) drawInterval = 4;
        else if (barWidth < 80.0) drawInterval = 2;

        bool shouldDrawBar = ((barIndex - 1) % drawInterval == 0);

        if (currentX + barWidth >= visibleLeft) {
            if (shouldDrawBar) {

                g.setColour(findColour(juce::Label::textColourId).withAlpha(0.5f));
                g.drawLine((float)currentX, 0.0f, (float)currentX, (float)getHeight(), 1.0f);


                g.setColour(findColour(juce::Label::textColourId));
                g.setFont(10.0f);
                g.drawText(juce::String(barIndex), (int)currentX + 4, 0, 40, 15, juce::Justification::left, false);


                if (changed || barIndex == 1) {
                    juce::String info = juce::String(currentBpm, 1) + " bpm  " + juce::String(currentNum) + "/" + juce::String(currentDen);
                    g.setColour(juce::Colours::cyan);
                    g.drawText(info, (int)currentX + 4, 15, 100, 15, juce::Justification::left, false);
                }
            }


            if (barWidth > 20.0) {
                double pixelsPerBeat = barWidth / (double)currentNum;
                g.setColour(findColour(juce::Label::textColourId).withAlpha(0.2f));
                for (int b = 1; b < currentNum; ++b) {
                    float bx = (float)(currentX + b * pixelsPerBeat);
                    if (bx > visibleRight) break;
                    if (bx >= visibleLeft) {
                        g.drawLine(bx, 15.0f, bx, (float)getHeight(), 1.0f);
                    }
                }
            }
        }

        currentX += barWidth;
        currentFrame += framesPerBar;
        barIndex++;
    }


    auto regions = engine.getRegionMarkers();
    for (int i = 0; i < (int)regions.size(); ++i) {
        if (i == draggedMarkerIndex) continue;

        const auto& r = regions[i];
        double x = getXForFrame(r.position);
        if (x >= visibleLeft - 100 && x <= visibleRight + 100) {
             g.setColour(juce::Colours::orange);
             g.drawLine((float)x, 0.0f, (float)x, (float)getHeight(), 2.0f);


             g.setColour(juce::Colours::orange);
             g.fillRect((float)x, 0.0f, 60.0f, 14.0f);

             g.setColour(juce::Colours::white);
             g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
             g.drawText(r.name, (int)x + 2, 0, 56, 14, juce::Justification::centredLeft, false);
        }
    }


    if (draggedMarkerIndex != -1) {
        double x = getXForFrame(draggedMarkerPos);
        g.setColour(juce::Colours::orange.withAlpha(0.6f));
        g.drawLine((float)x, 0.0f, (float)x, (float)getHeight(), 2.0f);
        g.fillRect((float)x, 0.0f, 60.0f, 14.0f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(draggedMarkerName, (int)x + 2, 0, 56, 14, juce::Justification::centredLeft, false);
    }
}

int RulerComponent::hitTestMarker(int x) {
    auto regions = engine.getRegionMarkers();
    for (int i = 0; i < (int)regions.size(); ++i) {
        double mx = getXForFrame(regions[i].position);
        if (x >= mx && x <= mx + 60) {


            return i;
        }
    }
    return -1;
}

void RulerComponent::resized()
{
}

void RulerComponent::timerCallback()
{
    repaint();
}

void RulerComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    int hit = hitTestMarker(e.x);
    if (hit != -1) {
        auto markers = engine.getRegionMarkers();
        if (hit < (int)markers.size()) {
            auto* w = new juce::AlertWindow("Rename Marker", "Enter new name:", juce::AlertWindow::NoIcon);
            w->addTextEditor("name", markers[hit].name);
            w->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
            w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

            w->enterModalState(true, juce::ModalCallbackFunction::create([this, hit, w](int result) {
                if (result == 1) {
                    std::string newName = w->getTextEditorContents("name").toStdString();

                    auto current = engine.getRegionMarkers();
                    if (hit < (int)current.size()) {
                        engine.updateRegionMarker(hit, current[hit].position, newName);
                        repaint();
                    }
                }
            }), true);
        }
        return;
    }


    if (e.y < getHeight() / 2) {
        bool loop = engine.isLooping();
        engine.setLooping(!loop);
        repaint();
    }
}

void RulerComponent::mouseDown(const juce::MouseEvent& e)
{

    int hit = hitTestMarker(e.x);

    if (e.mods.isRightButtonDown()) {
        juce::PopupMenu m;

        if (hit != -1) {
            m.addItem(1, "Delete Marker");

        } else {
            m.addItem(10, "Insert Tempo Change (140 bpm)");
            m.addItem(11, "Insert Time Signature Change (3/4)");
            m.addItem(12, "Insert Time Signature Change (4/4)");
            m.addSeparator();
            m.addItem(13, "Add Marker (Verse)");
            m.addItem(14, "Add Marker (Chorus)");
            m.addSeparator();
            m.addItem(20, "Show Marker List");
        }

        juce::Component::SafePointer<RulerComponent> safeThis(this);
        m.showMenuAsync(juce::PopupMenu::Options(), [safeThis, e, hit](int result) {
            if (result == 0 || safeThis == nullptr) return;
            uint64_t frame = safeThis->getFrameForX(e.position.x);

            if (result == 1 && hit != -1) safeThis->engine.removeRegionMarker(hit);
            else if (result == 10) safeThis->engine.addTempoMarker(frame, 140.0f);
            else if (result == 11) safeThis->engine.addTimeSigMarker(frame, 3, 4);
            else if (result == 12) safeThis->engine.addTimeSigMarker(frame, 4, 4);
            else if (result == 13) safeThis->engine.addRegionMarker(frame, "Verse");
            else if (result == 14) safeThis->engine.addRegionMarker(frame, "Chorus");
            else if (result == 20) {
                juce::DialogWindow::LaunchOptions opt;
                opt.dialogTitle = "Markers";
                opt.content.setOwned(new MarkerListComponent(safeThis->engine));
                opt.content->setSize(300, 400);
                opt.resizable = true;
                opt.launchAsync();
            }

            safeThis->repaint();
        });
        return;
    }

    if (hit != -1) {
        draggedMarkerIndex = hit;
        auto markers = engine.getRegionMarkers();
        if (hit < (int)markers.size()) {
            draggedMarkerPos = markers[hit].position;
            draggedMarkerName = markers[hit].name;
        }
        return;
    }

    bool isLoopTool = e.mods.isCtrlDown();

    if (isLoopTool) {
        uint64_t startFrame = getFrameForX(e.position.x);
        if (snapFunction) startFrame = snapFunction(startFrame);
        engine.setLoopPoints(startFrame, startFrame);
        engine.setLooping(true);
    }
    else {
        mouseDrag(e);
    }

    repaint();
}

void RulerComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggedMarkerIndex != -1) {
        uint64_t frame = getFrameForX(e.position.x);
        if (snapFunction && !e.mods.isAltDown()) frame = snapFunction(frame);
        draggedMarkerPos = frame;
        repaint();
        return;
    }

    double sampleRate = engine.getSampleRate();
    if (sampleRate <= 0) return;

    bool isLoopTool = e.mods.isCtrlDown();

    if (isLoopTool) {
        uint64_t startFrame = getFrameForX(e.getMouseDownX());
        uint64_t currentFrame = getFrameForX(e.position.x);
        if (snapFunction) {
            startFrame = snapFunction(startFrame);
            currentFrame = snapFunction(currentFrame);
        }
        uint64_t loopStart = std::min(startFrame, currentFrame);
        uint64_t loopEnd = std::max(startFrame, currentFrame);
        if (loopEnd == loopStart) loopEnd += (uint64_t)(sampleRate * 0.1);
        engine.setLoopPoints(loopStart, loopEnd);
        engine.setLooping(true);
    }
    else {
        uint64_t frame = getFrameForX(e.position.x);
        if (snapFunction && !e.mods.isAltDown()) frame = snapFunction(frame);
        engine.setCursor(frame);
        if (onCursorMove) onCursorMove();
    }

    repaint();
}

void RulerComponent::mouseUp(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    if (draggedMarkerIndex != -1) {
        engine.updateRegionMarker(draggedMarkerIndex, draggedMarkerPos, draggedMarkerName);
        draggedMarkerIndex = -1;
        repaint();
    }
}
