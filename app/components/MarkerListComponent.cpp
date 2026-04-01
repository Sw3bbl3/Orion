#include "MarkerListComponent.h"

MarkerListComponent::MarkerListComponent(Orion::AudioEngine& e) : engine(e)
{
    addAndMakeVisible(listBox);
    listBox.setModel(this);
    listBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::black.withAlpha(0.5f));
    listBox.setOutlineThickness(1);

    startTimer(500);
}

MarkerListComponent::~MarkerListComponent() {}

void MarkerListComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::grey);
}

void MarkerListComponent::resized()
{
    listBox.setBounds(getLocalBounds());
}

int MarkerListComponent::getNumRows()
{
    return (int)markers.size();
}

void MarkerListComponent::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (row >= (int)markers.size()) return;

    if (selected) g.fillAll(juce::Colours::cyan.withAlpha(0.3f));

    g.setColour(juce::Colours::white);
    g.setFont(14.0f);

    auto& m = markers[row];




    double seconds = (double)m.position / engine.getSampleRate();
    juce::String timeStr = juce::String(seconds, 2) + "s";

    g.drawText(timeStr, 5, 0, 80, height, juce::Justification::centredLeft, true);
    g.drawText(m.name, 90, 0, width - 95, height, juce::Justification::centredLeft, true);
}

void MarkerListComponent::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    if (row < (int)markers.size()) {
        engine.setCursor(markers[row].position);
    }
}

void MarkerListComponent::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row < (int)markers.size()) {
        auto* w = new juce::AlertWindow("Rename Marker", "Enter new name:", juce::AlertWindow::NoIcon);
        w->addTextEditor("name", markers[row].name);
        w->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
        w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));


        uint64_t pos = markers[row].position;
        int idx = row;

        w->enterModalState(true, juce::ModalCallbackFunction::create([this, idx, pos, w](int result) {
            if (result == 1) {
                std::string newName = w->getTextEditorContents("name").toStdString();


                engine.updateRegionMarker(idx, pos, newName);
                updateContent();
            }
        }), true);
    }
}

void MarkerListComponent::deleteKeyPressed(int row)
{
    if (row < (int)markers.size()) {
        engine.removeRegionMarker(row);
        listBox.selectRow(row);
        updateContent();
    }
}

void MarkerListComponent::timerCallback()
{
    auto newMarkers = engine.getRegionMarkers();


    bool changed = false;
    if (newMarkers.size() != markers.size()) changed = true;
    else {
        for(size_t i=0; i<markers.size(); ++i) {
            if (markers[i].position != newMarkers[i].position || markers[i].name != newMarkers[i].name) {
                changed = true;
                break;
            }
        }
    }

    if (changed) {
        markers = newMarkers;
        listBox.updateContent();
        repaint();
    }
}

void MarkerListComponent::updateContent()
{
    markers = engine.getRegionMarkers();
    listBox.updateContent();
    repaint();
}
