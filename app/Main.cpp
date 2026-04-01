#include <JuceHeader.h>
#include "MainComponent.h"
#include "components/SplashScreen.h"
#include "orion/Logger.h"
#include "orion/SettingsManager.h"
#include "OrionWindow.h"

#if JUCE_WINDOWS
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
  #include <dwmapi.h>
#endif

class OrionApplication  : public juce::JUCEApplication
{
public:
    OrionApplication() {}

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise (const juce::String& commandLine) override
    {

        auto logFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Orion")
            .getChildFile("Orion.log");

        logFile.getParentDirectory().createDirectory();

        Orion::Logger::init(logFile.getFullPathName().toStdString());
        Orion::Logger::info("Application Initialising...");

        Orion::SettingsManager::load();

        juce::StringArray args;
        args.addTokens(commandLine, " ", "\"'");
        int screenshotIndex = args.indexOf("--screenshot");
        juce::File screenshotFile;
        if (screenshotIndex != -1 && screenshotIndex + 1 < args.size()) {
            screenshotFile = juce::File::getCurrentWorkingDirectory().getChildFile(args[screenshotIndex + 1]);
        }


        splashWindow.reset(new SplashWindow());


        mainWindow.reset (new MainWindow (getApplicationName()));


        if (auto* content = dynamic_cast<MainComponent*>(mainWindow->getContentComponent()))
        {
            content->onInitProgress = [this](float progress, juce::String text) {
                if (splashWindow) {
                     auto* splash = splashWindow->getSplashScreen();
                     if (splash) {
                         splash->setProgress(progress);
                         splash->setStatus(text);
                     }
                }
            };

            content->onInitComplete = [this, screenshotFile]() {

                splashWindow.reset();


                if (mainWindow) {
                    mainWindow->setVisible(true);
                    mainWindow->toFront(true);

                    if (screenshotFile != juce::File()) {
                        juce::Timer::callAfterDelay(2000, [this, screenshotFile](){
                             if (mainWindow) {
                                 auto image = mainWindow->createComponentSnapshot(mainWindow->getLocalBounds());
                                 juce::File file = screenshotFile;
                                 if (file.exists()) file.deleteFile();

                                 juce::FileOutputStream stream(file);
                                 juce::PNGImageFormat pngWriter;
                                 pngWriter.writeImageToStream(image, stream);
                             }
                             quit();
                        });
                    }
                }
            };
        }
    }

    void shutdown() override
    {
        if (mainWindow) mainWindow->setVisible(false);
        Orion::Logger::info("Application Shutdown");
        mainWindow = nullptr;
        splashWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
    }


    class SplashWindow : public juce::DocumentWindow
    {
    public:
        SplashWindow()
            : DocumentWindow("Orion", juce::Colours::transparentBlack, DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(false);
            setTitleBarHeight(0);


            setBackgroundColour(juce::Colours::transparentBlack);
            setOpaque(false);

            setContentOwned(new OrionSplashScreen(), true);

            setResizable(false, false);
            centreWithSize(500, 300);
            setVisible(true);
            toFront(true);
        }

        OrionSplashScreen* getSplashScreen() {
            return dynamic_cast<OrionSplashScreen*>(getContentComponent());
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SplashWindow)
    };

    class MainWindow    : public juce::DocumentWindow
                        , public OrionWindow
    {
    public:
        MainWindow (juce::String name)
            : DocumentWindow (name,
                              juce::Desktop::getInstance().getDefaultLookAndFeel()
                                                          .findColour (juce::ResizableWindow::backgroundColourId),
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (false);
            setTitleBarHeight(0);
            setContentOwned (new MainComponent(), true);

            #if JUCE_IOS || JUCE_ANDROID
            setFullScreen (true);
            #else
            setResizable (true, false);
            centreWithSize (getWidth(), getHeight());
            #endif


            setVisible (false);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        void resized() override
        {
            juce::DocumentWindow::resized();
            updateWindowsCornerPreference();
        }

        void toggleWorkAreaMaximised() override
        {
            if (isFullScreen())
                setFullScreen(false);

            if (! workAreaMaximised)
            {
                restoreBounds = getBounds();

                auto& displays = juce::Desktop::getInstance().getDisplays();
                if (auto* d = displays.getDisplayForRect(getScreenBounds()))
                    setBounds(d->userArea);
                else
                    setBounds(displays.getPrimaryDisplay()->userArea);

                workAreaMaximised = true;
            }
            else
            {
                if (! restoreBounds.isEmpty())
                    setBounds(restoreBounds);

                workAreaMaximised = false;
            }

            updateWindowsCornerPreference();
        }

        bool isWorkAreaMaximised() const override
        {
            return workAreaMaximised;
        }

        juce::BorderSize<int> getBorderThickness() const override
        {
            if (isFullScreen() || isWorkAreaMaximised()) return {};
            return juce::DocumentWindow::getBorderThickness();
        }

        juce::BorderSize<int> getContentComponentBorder() const override
        {
            if (isFullScreen() || isWorkAreaMaximised()) return {};
            return juce::DocumentWindow::getContentComponentBorder();
        }

    private:
        void updateWindowsCornerPreference()
        {
           #if JUCE_WINDOWS
            if (auto* peer = getPeer())
            {
                if (auto hwnd = (HWND) peer->getNativeHandle())
                {
                    // Windows 11: when we "maximise" by resizing to the work area, DWM keeps rounded corners.
                    // Force square corners in that state so it looks like a real maximised window.
                    constexpr DWORD DWMWA_WINDOW_CORNER_PREFERENCE_ = 33;
                    constexpr DWORD DWMWCP_DEFAULT_ = 0;
                    constexpr DWORD DWMWCP_DONOTROUND_ = 1;

                    const DWORD pref = workAreaMaximised ? DWMWCP_DONOTROUND_ : DWMWCP_DEFAULT_;
                    ::DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE_, &pref, sizeof(pref));
                }
            }
           #endif
        }

        bool workAreaMaximised = false;
        juce::Rectangle<int> restoreBounds;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<SplashWindow> splashWindow;
};

START_JUCE_APPLICATION (OrionApplication)
