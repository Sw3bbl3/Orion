#include "LoadingWindow.h"
#include "../OrionLookAndFeel.h"
#include "../ui/OrionDesignSystem.h"
#include "../ui/OrionUIPrimitives.h"

class LoadingWindow::Content : public juce::Component
{
public:
    Content()
    {
        addAndMakeVisible(messageLabel);
        messageLabel.setText("Loading Project...", juce::dontSendNotification);
        messageLabel.setJustificationType(juce::Justification::centred);
        messageLabel.setFont(Orion::UI::DesignSystem::instance().fonts().sans(20.0f, juce::Font::bold));
        messageLabel.setColour(juce::Label::textColourId, Orion::UI::DesignSystem::instance().currentTheme().colors.textPrimary);
        setSize(300, 150);
    }

    void paint(juce::Graphics& g) override
    {
        Orion::UI::Primitives::paintPanelSurface(g, getLocalBounds().toFloat().reduced(2.0f), 10.0f, true);


        auto spinnerArea = getLocalBounds().reduced(20);
        spinnerArea.removeFromTop(40);

        getLookAndFeel().drawSpinningWaitAnimation(g, Orion::UI::DesignSystem::instance().currentTheme().colors.accentStrong,
                                                   spinnerArea.getX(), spinnerArea.getY(), spinnerArea.getWidth(), spinnerArea.getHeight());
    }

    void resized() override
    {
        messageLabel.setBounds(0, 30, getWidth(), 40);
    }

private:
    juce::Label messageLabel;
};

LoadingWindow::LoadingWindow()
    : DocumentWindow("Loading", juce::Colours::transparentBlack, DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(false);
    setTitleBarHeight(0);
    setResizable(false, false);
    setDropShadowEnabled(true);
    setAlwaysOnTop(true);

    setContentOwned(new Content(), true);
    centreWithSize(300, 150);
}

void LoadingWindow::show()
{
    setVisible(true);
    toFront(true);
}

void LoadingWindow::hide()
{
    setVisible(false);
}
