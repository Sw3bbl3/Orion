#include "CustomTitleBar.h"
#include "../OrionWindow.h"
#include "../OrionLookAndFeel.h"
#include "../OrionIcons.h"
#include "../ui/OrionDesignSystem.h"
#include "../ui/OrionUIPrimitives.h"

namespace {
    constexpr bool kShowTitleBarLogo = false;
}



CustomTitleBar::TitleBarButton::TitleBarButton(const juce::String& name, Type t)
    : juce::Button(name), type(t)
{
    if (type == Pin) setClickingTogglesState(true);
}

void CustomTitleBar::TitleBarButton::paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    auto area = getLocalBounds().toFloat();


    if (isMouseOverButton || isButtonDown)
    {
        if (type == Close)
            g.setColour(juce::Colours::red.withAlpha(0.8f));
        else
            g.setColour(findColour(juce::TextButton::textColourOffId).withAlpha(0.1f));

        g.fillAll();
    }


    g.setColour(findColour(juce::TextButton::textColourOffId));
    if (isMouseOverButton && type == Close) g.setColour(juce::Colours::white);

    juce::Path icon;
    if (type == Min) icon = OrionIcons::getMinusIcon();
    else if (type == Max) icon = OrionIcons::getMaximizeIcon();
    else if (type == Close) icon = OrionIcons::getCloseIcon();
    else if (type == Pin) icon = OrionIcons::getPinIcon();

    if (type == Pin && getToggleState())
        g.setColour(juce::Colours::cyan);

    auto iconBounds = area.reduced(type == Close ? 8.0f : 7.0f);
    g.fillPath(icon, icon.getTransformToScaleToFit(iconBounds, true));
}



CustomTitleBar::CustomTitleBar()
    : menuBar(nullptr),
      minButton("Min", TitleBarButton::Min),
      maxButton("Max", TitleBarButton::Max),
      closeButton("Close", TitleBarButton::Close),
      pinButton("Pin", TitleBarButton::Pin)
{
    setOpaque(true);

    addAndMakeVisible(menuBar);


    titleLabel.setText("Orion", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(Orion::UI::DesignSystem::instance().fonts().sans(14.0f, juce::Font::bold));
    titleLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(titleLabel);


    auto logoFile = juce::File::getCurrentWorkingDirectory().getChildFile("assets/orion/Orion_logo_transpant_bg.png");
    if (!logoFile.existsAsFile())
    {

        auto exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
        logoFile = exeDir.getChildFile("assets/orion/Orion_logo_transpant_bg.png");
    }

    if (logoFile.existsAsFile())
    {
        auto image = juce::ImageFileFormat::loadFrom(logoFile);
        if (image.isValid())
        {
            logoDrawable = std::make_unique<juce::DrawableImage>();
            static_cast<juce::DrawableImage*>(logoDrawable.get())->setImage(image);
            if (kShowTitleBarLogo)
                addAndMakeVisible(logoDrawable.get());
            else
                addChildComponent(logoDrawable.get());
        }
    }


    minButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    minButton.onClick = [this] {
        if (auto* w = getWindow()) w->setMinimised(true);
    };
    addAndMakeVisible(minButton);

    maxButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    maxButton.onClick = [this] {
        toggleMaximized();
    };
    addAndMakeVisible(maxButton);

    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    closeButton.onClick = [this] {
        if (onCloseCallback) {
            onCloseCallback();
        } else if (auto* w = getWindow()) {
            if (auto* dw = dynamic_cast<juce::DocumentWindow*>(w))
                dw->closeButtonPressed();
            else
                w->userTriedToCloseWindow();
        }
    };
    addAndMakeVisible(closeButton);

    pinButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    pinButton.setTooltip("Keep Window on Top");
    pinButton.onClick = [this] {
        if (onPinCallback) onPinCallback(pinButton.getToggleState());
    };
    addChildComponent(pinButton);

    auto configureViewButton = [this](juce::TextButton& button, const juce::String& tooltip, std::function<void()>* callback) {
        button.setTooltip(tooltip);
        button.setClickingTogglesState(false);
        button.onClick = [callback] { if (*callback) (*callback)(); };
        button.setColour(juce::TextButton::buttonColourId, juce::Colours::white.withAlpha(0.08f));
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white.withAlpha(0.22f));
        button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        button.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.85f));
        addChildComponent(button);
    };

    setInterceptsMouseClicks(true, true);
    menuBar.setInterceptsMouseClicks(true, true);
    menuBar.setOpaque(true);
}

CustomTitleBar::~CustomTitleBar()
{
}

void CustomTitleBar::toggleMaximized()
{
    if (auto* w = getWindow())
    {
        if (auto* orionWindow = dynamic_cast<OrionWindow*>(w))
        {
            orionWindow->toggleWorkAreaMaximised();
        }
        else
        {
            // Fallback: on windows that don't implement OrionWindow, keep existing behaviour.
            w->setFullScreen(!w->isFullScreen());
        }
    }
}

void CustomTitleBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    const auto& colors = Orion::UI::DesignSystem::instance().currentTheme().colors;
    auto bg = colors.chromeBg;

    // Subtle vertical gradient for a more professional look
    juce::ColourGradient grad(bg.brighter(0.05f), 0, 0, bg.darker(0.05f), 0, bounds.getHeight(), false);
    g.setGradientFill(grad);
    g.fillAll();

    // Top highlight
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawHorizontalLine(0, 0.0f, (float)getWidth());

    // Bottom shadow
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawHorizontalLine(getHeight() - 1, 0.0f, (float)getWidth());

    auto textCol = colors.textPrimary.withAlpha(0.9f);
    titleLabel.setColour(juce::Label::textColourId, textCol);

    minButton.setColour(juce::TextButton::textColourOffId, textCol);
    maxButton.setColour(juce::TextButton::textColourOffId, textCol);
    closeButton.setColour(juce::TextButton::textColourOffId, textCol);
    pinButton.setColour(juce::TextButton::textColourOffId, textCol);
}

void CustomTitleBar::resized()
{
    auto area = getLocalBounds();
    const int buttonWidth = 42;

    closeButton.setBounds(area.removeFromRight(buttonWidth));

    if (showMinMax)
    {
        maxButton.setVisible(true);
        minButton.setVisible(true);
        maxButton.setBounds(area.removeFromRight(buttonWidth));
        minButton.setBounds(area.removeFromRight(buttonWidth));
    }
    else
    {
        maxButton.setVisible(false);
        minButton.setVisible(false);
    }

    if (showPin)
    {
        pinButton.setVisible(true);
        pinButton.setBounds(area.removeFromRight(buttonWidth));
    }
    else
    {
        pinButton.setVisible(false);
    }

    auto leftArea = area.removeFromLeft(450);

    if (logoDrawable && kShowTitleBarLogo)
    {
        logoDrawable->setBounds(leftArea.removeFromLeft(40).reduced(8));
    }

    if (showMenuBar)
    {
        menuBar.setVisible(true);
        menuBar.setBounds(leftArea.reduced(0, 4));
    }
    else
    {
        menuBar.setVisible(false);
    }

    // Centered title: Try to center relative to the entire title bar, but avoid overlaps.
    int width = getWidth();
    int idealTitleWidth = 300;
    int titleX = (width - idealTitleWidth) / 2;

    int leftLimit = showMenuBar ? menuBar.getRight() : (logoDrawable ? logoDrawable->getRight() : 0);
    int rightLimit = pinButton.isVisible() ? pinButton.getX() : minButton.getX();

    // If perfectly centered would overlap left or right elements, push it.
    if (titleX < leftLimit + 10) titleX = leftLimit + 10;
    if (titleX + idealTitleWidth > rightLimit - 10) idealTitleWidth = rightLimit - 10 - titleX;

    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setBounds(titleX, 0, idealTitleWidth, getHeight());
}

void CustomTitleBar::setModel(juce::MenuBarModel* model)
{
    menuBar.setModel(model);
}

void CustomTitleBar::setTitle(const juce::String& title)
{
    titleLabel.setText(title, juce::dontSendNotification);
}

juce::ResizableWindow* CustomTitleBar::getWindow()
{
    return dynamic_cast<juce::ResizableWindow*>(getTopLevelComponent());
}

void CustomTitleBar::mouseDown(const juce::MouseEvent& event)
{
    if (auto* w = getWindow())
    {
        if (w->isFullScreen())
            return;

        if (auto* orionWindow = dynamic_cast<OrionWindow*>(w))
        {
            if (orionWindow->isWorkAreaMaximised())
                orionWindow->toggleWorkAreaMaximised();
        }

        dragger.startDraggingComponent(w, event);
    }
}

void CustomTitleBar::mouseDrag(const juce::MouseEvent& event)
{
    if (auto* w = getWindow())
    {
        if (!w->isFullScreen())
            dragger.dragComponent(w, event, nullptr);
    }
}

void CustomTitleBar::mouseDoubleClick(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    toggleMaximized();
}

void CustomTitleBar::setShowMenuBar(bool show)
{
    showMenuBar = show;
    menuBar.setVisible(show);
    resized();
}

void CustomTitleBar::setShowMinMax(bool show)
{
    showMinMax = show;
    minButton.setVisible(show);
    maxButton.setVisible(show);
    resized();
}

void CustomTitleBar::setShowPin(bool show)
{
    showPin = show;
    resized();
}


void CustomTitleBar::setShowViewButtons(bool show)
{
    showViewButtons = show;
    resized();
}

void CustomTitleBar::setViewButtonStates(bool showEditor, bool showMixer, bool showPianoRoll, bool showBrowser)
{
    juce::ignoreUnused(showEditor, showMixer, showPianoRoll, showBrowser);
}

void CustomTitleBar::setProjectName(const juce::String& name)
{
    if (name.isEmpty())
        titleLabel.setText("Orion", juce::dontSendNotification);
    else
        titleLabel.setText(name + " - Orion", juce::dontSendNotification);
}

void CustomTitleBar::setOnClose(std::function<void()> callback)
{
    onCloseCallback = callback;
}

void CustomTitleBar::setOnPinToggled(std::function<void(bool)> callback)
{
    onPinCallback = callback;
}

void CustomTitleBar::setOnToggleEditor(std::function<void()> callback)
{
    onToggleEditorCallback = callback;
}

void CustomTitleBar::setOnToggleMixer(std::function<void()> callback)
{
    onToggleMixerCallback = callback;
}

void CustomTitleBar::setOnTogglePianoRoll(std::function<void()> callback)
{
    onTogglePianoRollCallback = callback;
}

void CustomTitleBar::setOnToggleBrowser(std::function<void()> callback)
{
    onToggleBrowserCallback = callback;
}
