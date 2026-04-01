#include "AutomationLaneComponent.h"

namespace {
    bool isVolume(Orion::AutomationParameter parameter)
    {
        return parameter == Orion::AutomationParameter::Volume;
    }
}

AutomationLaneComponent::AutomationLaneComponent(std::shared_ptr<Orion::AudioTrack> t, Orion::AutomationParameter p)
    : track(t), parameter(p), parameterSpec(Orion::getAutomationParameterSpec(parameter))
{
    setWantsKeyboardFocus(true);
}

AutomationLaneComponent::~AutomationLaneComponent()
{
}

int AutomationLaneComponent::findPointAtPosition(const Orion::AutomationLane& lane, int x, int y) const
{
    const auto& points = lane.getPoints();
    for (int i = 0; i < (int)points.size(); ++i) {
        int pointX = frameToX(points[i].frame);
        float pointY = valueToY(points[i].value);
        if (std::abs(x - pointX) < 6 && std::abs(y - pointY) < 6)
            return i;
    }

    return -1;
}

void AutomationLaneComponent::clearCurrentLane()
{
    commitLane(std::make_shared<Orion::AutomationLane>());
    draggedPointIndex = -1;
    selectedPointIndex = -1;
}

void AutomationLaneComponent::showContextMenu(const juce::MouseEvent& e)
{
    auto lane = getCurrentLane();
    const int hitIndex = findPointAtPosition(*lane, e.x, e.y);
    if (hitIndex != -1) selectedPointIndex = hitIndex;
    const int menuIndex = (selectedPointIndex >= 0 && selectedPointIndex < (int)lane->getPoints().size()) ? selectedPointIndex : hitIndex;

    juce::PopupMenu menu;
    if (menuIndex != -1) {
        menu.addItem("Delete Selected Point", [this, lane, menuIndex] {
            lane->removePoint(menuIndex);
            commitLane(lane);
            draggedPointIndex = -1;
            selectedPointIndex = -1;
        });
        menu.addItem("Set Selected Point to Default", [this, lane, menuIndex] {
            const auto& points = lane->getPoints();
            if (menuIndex < 0 || menuIndex >= (int)points.size()) return;

            const auto frame = points[(size_t)menuIndex].frame;
            selectedPointIndex = lane->updatePoint(menuIndex, frame, parameterSpec.defaultValue, 0.0f);
            commitLane(lane);
        });
    }

    menu.addItem("Add Point Here", [this, lane, e] {
        const auto frame = xToFrame(e.x);
        const auto value = yToValue((float)e.y);
        lane->addPoint(frame, value, 0.0f);
        selectedPointIndex = findPointAtPosition(*lane, e.x, e.y);
        commitLane(lane);
    });

    menu.addSeparator();
    menu.addItem("Clear Lane", [this] { clearCurrentLane(); });

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this));
}

void AutomationLaneComponent::setPixelsPerSecond(double pps)
{
    pixelsPerSecond = pps;
    repaint();
}

std::shared_ptr<Orion::AutomationLane> AutomationLaneComponent::getCurrentLane()
{
    std::shared_ptr<const Orion::AutomationLane> current;
    if (isVolume(parameter)) current = track->getVolumeAutomation();
    else current = track->getPanAutomation();

    if (current) return current->clone();
    return std::make_shared<Orion::AutomationLane>();
}

void AutomationLaneComponent::commitLane(std::shared_ptr<Orion::AutomationLane> lane)
{
    if (isVolume(parameter)) track->setVolumeAutomation(lane);
    else track->setPanAutomation(lane);
    repaint();
}

float AutomationLaneComponent::valueToY(float value) const
{
    const float h = (float)getHeight();
    const float normalized = Orion::automationValueToNormalized(parameter, value);
    return h - (normalized * h);
}

float AutomationLaneComponent::yToValue(float y) const
{
    const float h = (float)getHeight();
    if (h < 1.0f) return parameterSpec.defaultValue;

    const float normalized = 1.0f - (y / h);
    return Orion::normalizedToAutomationValue(parameter, normalized);
}

uint64_t AutomationLaneComponent::xToFrame(int x) const
{
    double seconds = (double)x / pixelsPerSecond;
    return (uint64_t)(seconds * sampleRate);
}

int AutomationLaneComponent::frameToX(uint64_t frame) const
{
    double seconds = (double)frame / sampleRate;
    return (int)(seconds * pixelsPerSecond);
}

void AutomationLaneComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.2f));

    if (parameterSpec.drawCenterLine) {
        float midY = valueToY(parameterSpec.defaultValue);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawHorizontalLine((int)midY, 0.0f, (float)getWidth());
    }

    std::shared_ptr<const Orion::AutomationLane> current;
    if (isVolume(parameter)) current = track->getVolumeAutomation();
    else current = track->getPanAutomation();

    if (!current) return;

    const auto& points = current->getPoints();
    if (points.empty()) return;

    g.setColour(juce::Colours::cyan.withAlpha(0.8f));
    juce::Path path;

    int startX = frameToX(0);
    float startY = valueToY(points.front().value);
    path.startNewSubPath((float)startX, startY);

    for (const auto& p : points) {
        int x = frameToX(p.frame);
        float y = valueToY(p.value);
        path.lineTo((float)x, y);
    }

    const float lastY = valueToY(points.back().value);
    path.lineTo((float)getWidth(), lastY);

    g.strokePath(path, juce::PathStrokeType(2.0f));

    g.setColour(juce::Colours::white);
    for (int i = 0; i < (int)points.size(); ++i) {
        const auto& p = points[(size_t)i];
        int x = frameToX(p.frame);
        float y = valueToY(p.value);

        const bool isSelected = (i == selectedPointIndex);
        const float radius = isSelected ? 5.0f : 3.0f;
        if (isSelected) {
            g.setColour(juce::Colours::orange.withAlpha(0.95f));
            g.fillEllipse((float)x - radius, y - radius, radius * 2.0f, radius * 2.0f);
            g.setColour(juce::Colours::black.withAlpha(0.7f));
            g.drawEllipse((float)x - radius, y - radius, radius * 2.0f, radius * 2.0f, 1.0f);
            g.setColour(juce::Colours::white.withAlpha(0.95f));
            g.fillEllipse((float)x - 2.0f, y - 2.0f, 4.0f, 4.0f);
        } else {
            g.setColour(juce::Colours::white);
            g.fillEllipse((float)x - radius, y - radius, radius * 2.0f, radius * 2.0f);
        }
    }
}

void AutomationLaneComponent::mouseDown(const juce::MouseEvent& e)
{
    grabKeyboardFocus();

    auto lane = getCurrentLane();
    const int hitIndex = findPointAtPosition(*lane, e.x, e.y);

    if (e.mods.isRightButtonDown()) {
        if (hitIndex != -1) selectedPointIndex = hitIndex;
        showContextMenu(e);
        return;
    }

    if (e.mods.isCommandDown()) {
        if (hitIndex != -1) {
            lane->removePoint(hitIndex);
            commitLane(lane);
            selectedPointIndex = -1;
        }
        return;
    }

    if (hitIndex != -1) {
        draggedPointIndex = hitIndex;
        selectedPointIndex = hitIndex;
        repaint();
    } else {
        uint64_t frame = xToFrame(e.x);
        float val = yToValue((float)e.y);
        lane->addPoint(frame, val, 0.0f);

        const auto& pts = lane->getPoints();
        for (int i = 0; i < (int)pts.size(); ++i) {
            if (pts[i].frame == frame && std::abs(pts[i].value - val) < 0.001f) {
                draggedPointIndex = i;
                selectedPointIndex = i;
                break;
            }
        }
        commitLane(lane);
    }
}

void AutomationLaneComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (draggedPointIndex != -1) {
        auto lane = getCurrentLane();

        uint64_t frame = xToFrame(e.x);
        float val = yToValue((float)e.y);

        draggedPointIndex = lane->updatePoint(draggedPointIndex, frame, val, 0.0f);
        selectedPointIndex = draggedPointIndex;

        commitLane(lane);
    }
}

void AutomationLaneComponent::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    draggedPointIndex = -1;
}

void AutomationLaneComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto lane = getCurrentLane();
    const int hitIndex = findPointAtPosition(*lane, e.x, e.y);

    if (hitIndex != -1) {
        lane->removePoint(hitIndex);
        selectedPointIndex = -1;
        draggedPointIndex = -1;
        commitLane(lane);
        return;
    }

    const auto frame = xToFrame(e.x);
    const auto value = yToValue((float)e.y);
    lane->addPoint(frame, value, 0.0f);
    selectedPointIndex = findPointAtPosition(*lane, e.x, e.y);
    commitLane(lane);
}

bool AutomationLaneComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
        if (selectedPointIndex == -1) return false;

        auto lane = getCurrentLane();
        if (selectedPointIndex >= 0 && selectedPointIndex < (int)lane->getPoints().size()) {
            lane->removePoint(selectedPointIndex);
            selectedPointIndex = -1;
            draggedPointIndex = -1;
            commitLane(lane);
            return true;
        }
    }

    return false;
}

void AutomationLaneComponent::resized()
{
    repaint();
}
