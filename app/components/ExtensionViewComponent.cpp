#include "ExtensionViewComponent.h"
#include "../ExtensionManager.h"
#include "../../include/orion/SettingsManager.h"
#include "../OrionLookAndFeel.h"
#include "../OrionIcons.h"

namespace Orion {

    ExtensionViewComponent::ExtensionViewComponent()
        : newBtn    ("New",     juce::Colours::grey, juce::Colours::grey, juce::Colours::grey),
          refreshBtn("Refresh", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey),
          importBtn ("Import",  juce::Colours::grey, juce::Colours::grey, juce::Colours::grey),
          exportBtn ("Export",  juce::Colours::grey, juce::Colours::grey, juce::Colours::grey),
          deleteBtn ("Delete",  juce::Colours::grey, juce::Colours::grey, juce::Colours::grey)
    {
        Orion::ExtensionManager::getInstance().scanExtensions();

        // Header label — colour driven by theme, not hardcoded
        addAndMakeVisible(headerLabel);
        headerLabel.setText("EXTENSIONS", juce::dontSendNotification);
        headerLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        headerLabel.setJustificationType(juce::Justification::centredLeft);

        // List box
        addAndMakeVisible(list);
        list.setModel(this);
        list.setRowHeight(52);           // taller rows for name + subtitle
        list.setOutlineThickness(0);
        list.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);

        // Icon buttons — initial colour set via updateIconColours
        auto setupIconBtn = [this](juce::ShapeButton& b, juce::Path p, std::function<void()> cb) {
            b.setShape(p, true, true, false);
            b.onClick = cb;
            addAndMakeVisible(b);
        };

        setupIconBtn(newBtn,     OrionIcons::getPlusIcon(),    [this] { createNew(); });
        setupIconBtn(refreshBtn, OrionIcons::getRefreshIcon(), [this] { refreshList(); });
        setupIconBtn(importBtn,  OrionIcons::getImportIcon(),  [this] { importExtension(); });
        setupIconBtn(exportBtn,  OrionIcons::getExportIcon(),  [this] { exportExtension(); });
        setupIconBtn(deleteBtn,  OrionIcons::getTrashIcon(),   [this] { deleteSelectedExtension(); });

        addAndMakeVisible(editor);

        if (!Orion::ExtensionManager::getInstance().getExtensions().empty()) {
            list.selectRow(0);
        }

        updateIconColours();
    }

    ExtensionViewComponent::~ExtensionViewComponent() {}

    void ExtensionViewComponent::paint(juce::Graphics& g)
    {
        auto area = getLocalBounds();
        auto sidebarBounds = area.removeFromLeft(240).toFloat();

        // Draw sidebar background (matching Dashboard sidebar)
        auto sidebarBg = findColour(OrionLookAndFeel::dashboardSidebarBackgroundColourId);
        g.setColour(sidebarBg);
        g.fillRect(sidebarBounds);

        // Sidebar separator with subtle gradient (matching Dashboard)
        juce::Colour separatorColor = juce::Colours::black.withAlpha(0.3f);
        juce::ColourGradient sepGrad(separatorColor.withAlpha(0.0f), 240.0f, 0.0f,
                                     separatorColor, 240.0f, (float)getHeight() * 0.1f, false);
        sepGrad.addColour(0.9f, separatorColor);
        sepGrad.addColour(1.0f, separatorColor.withAlpha(0.0f));
        g.setGradientFill(sepGrad);
        g.fillRect(240.0f, 0.0f, 1.0f, (float)getHeight());
    }

    void ExtensionViewComponent::lookAndFeelChanged()
    {
        updateIconColours();
        repaint();
    }

    void ExtensionViewComponent::updateIconColours()
    {
        auto textColor = findColour(OrionLookAndFeel::dashboardTextColourId);
        auto accentColor = findColour(OrionLookAndFeel::dashboardAccentColourId);
        
        auto iconCol  = textColor.withAlpha(0.6f);
        auto iconOver = textColor.withAlpha(0.9f);
        auto iconDown = accentColor;

        newBtn.setColours    (iconCol, iconOver, iconDown);
        refreshBtn.setColours(iconCol, iconOver, iconDown);
        importBtn.setColours (iconCol, iconOver, iconDown);
        exportBtn.setColours (iconCol, iconOver, iconDown);
        deleteBtn.setColours (juce::Colours::red.withAlpha(0.4f),
                              juce::Colours::red.withAlpha(0.8f),
                              juce::Colours::red);

        // Also update header colour
        headerLabel.setColour(juce::Label::textColourId, textColor.withAlpha(0.5f));
    }

    void ExtensionViewComponent::resized()
    {
        auto area = getLocalBounds();
        auto sidebar = area.removeFromLeft(240);

        // Toolbar area in sidebar
        auto toolbar = sidebar.removeFromTop(50);
        headerLabel.setBounds(toolbar.removeFromLeft(100).reduced(12, 0));

        const int btnW = 28;
        deleteBtn .setBounds(toolbar.removeFromRight(btnW).reduced(4));
        exportBtn .setBounds(toolbar.removeFromRight(btnW).reduced(4));
        importBtn .setBounds(toolbar.removeFromRight(btnW).reduced(4));
        refreshBtn.setBounds(toolbar.removeFromRight(btnW).reduced(4));
        newBtn    .setBounds(toolbar.removeFromRight(btnW).reduced(4));

        list.setBounds(sidebar.reduced(0, 4));
        editor.setBounds(area);
    }

    int ExtensionViewComponent::getNumRows()
    {
        return (int)Orion::ExtensionManager::getInstance().getExtensions().size();
    }

    void ExtensionViewComponent::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool rowIsSelected)
    {
        auto accentColor = findColour(OrionLookAndFeel::dashboardAccentColourId);
        auto textColor = findColour(OrionLookAndFeel::dashboardTextColourId);
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(12.0f, 2.0f);

        if (rowIsSelected)
        {
            // Background highlight (matching Dashboard sidebar)
            juce::ColourGradient grad(accentColor.withAlpha(0.15f), bounds.getTopLeft(),
                                      accentColor.withAlpha(0.00f), bounds.getBottomRight(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(bounds, 8.0f);

            // Left indicator bar (matching Dashboard sidebar)
            g.setColour(accentColor);
            g.fillRoundedRectangle(bounds.getX(), bounds.getY() + 8.0f, 4.0f, bounds.getHeight() - 16.0f, 2.0f);
        }
        else
        {
            // Hover state handled by juce::ListBox? No, we should check if mouse is over this row.
            // For now, simpler selection focus.
        }

        auto exts = Orion::ExtensionManager::getInstance().getExtensions();
        if (row >= (int)exts.size()) return;

        const auto& ext = exts[row];

        auto contentArea = bounds.reduced(18.0f, 0.0f);
        
        // Extension name
        g.setColour(rowIsSelected ? textColor : textColor.withAlpha(0.85f));
        g.setFont(juce::FontOptions(14.0f, rowIsSelected ? juce::Font::bold : juce::Font::plain));
        
        auto nameArea = contentArea.removeFromTop(height * 0.55f);
        g.drawText(ext.name, nameArea, juce::Justification::centredLeft, true);

        // Subtitle — description or author/version
        std::string sub = ext.description.empty()
            ? (ext.author.empty() ? "" : "by " + ext.author + (ext.version.empty() ? "" : "  v" + ext.version))
            : ext.description;
            
        if (!sub.empty()) {
            g.setColour(textColor.withAlpha(0.7f));
            g.setFont(juce::FontOptions(11.0f));
            g.drawText(sub, contentArea, juce::Justification::centredLeft, true);
        }
    }

    void ExtensionViewComponent::listBoxItemClicked(int row, const juce::MouseEvent&)
    {
        auto exts = Orion::ExtensionManager::getInstance().getExtensions();
        if (row >= 0 && row < (int)exts.size())
        {
            juce::File mainScript(exts[row].mainScript);
            if (mainScript.existsAsFile())
                editor.loadExtension(mainScript.getParentDirectory());
        }
    }

    void ExtensionViewComponent::refreshList()
    {
        Orion::ExtensionManager::getInstance().scanExtensions();
        list.updateContent();
        list.repaint();
    }

    void ExtensionViewComponent::createNew()
    {
        auto w = std::make_shared<juce::AlertWindow>("New Extension", "Enter extension name:", juce::AlertWindow::NoIcon);
        w->addTextEditor("name", "MyExtension");
        w->addButton("Create", 1);
        w->addButton("Cancel", 0);

        juce::Component::SafePointer<ExtensionViewComponent> safeThis(this);

        w->enterModalState(true, juce::ModalCallbackFunction::create(
            [safeThis, w](int result) {
                if (result == 1 && safeThis != nullptr) {
                    auto name = w->getTextEditorContents("name");
                    if (name.isEmpty()) return;

                    auto root = juce::File(Orion::SettingsManager::get().rootDataPath);
                    if (!root.exists()) root = juce::File::getCurrentWorkingDirectory();

                    auto extDir = root.getChildFile("Extensions").getChildFile(name);
                    if (extDir.exists()) {
                         juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "Extension already exists!");
                         return;
                    }

                    extDir.createDirectory();

                    // Create main.lua with a useful template
                    auto main = extDir.getChildFile("main.lua");
                    main.create();
                    main.replaceWithText(
                        "-- " + name + "\n"
                        "-- Extension for Orion DAW\n\n"
                        "local bpm = orion.project.getBpm()\n"
                        "local tracks = orion.track.getCount()\n\n"
                        "orion.log('Hello from " + name + "!')\n"
                        "orion.log('BPM: ' .. bpm .. '  Tracks: ' .. tracks)\n"
                    );

                    // Create manifest.lua
                    auto manifest = extDir.getChildFile("manifest.lua");
                    manifest.create();
                    manifest.replaceWithText(
                        "name        = \"" + name + "\"\n"
                        "description = \"A new Orion extension.\"\n"
                        "author      = \"You\"\n"
                        "version     = \"1.0.0\"\n"
                    );

                    safeThis->refreshList();
                }
            }
        ));
    }

    void ExtensionViewComponent::importExtension()
    {
        auto fc = std::make_shared<juce::FileChooser>("Import Extension",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.oex");

        juce::Component::SafePointer<ExtensionViewComponent> safeThis(this);
        fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safeThis, fc](const juce::FileChooser& chooser) {
                if (safeThis == nullptr) return;
                auto result = chooser.getResult();
                if (result.existsAsFile()) {
                    if (Orion::ExtensionManager::getInstance().importExtension(result)) {
                        safeThis->refreshList();
                    }
                }
            });
    }

    void ExtensionViewComponent::exportExtension()
    {
        int row = list.getSelectedRow();
        auto exts = Orion::ExtensionManager::getInstance().getExtensions();
        if (row < 0 || row >= (int)exts.size()) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Export", "Please select an extension to export.");
            return;
        }

        auto name = exts[row].name;
        auto fc = std::make_shared<juce::FileChooser>("Export Extension",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(name + ".oex"), "*.oex");

        fc->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
            [name, fc](const juce::FileChooser& chooser) {
                auto result = chooser.getResult();
                if (result != juce::File()) {
                    Orion::ExtensionManager::getInstance().exportExtension(name, result);
                }
            });
    }

    void ExtensionViewComponent::deleteSelectedExtension()
    {
        int row = list.getSelectedRow();
        auto exts = Orion::ExtensionManager::getInstance().getExtensions();
        if (row < 0 || row >= (int)exts.size()) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Delete", "Please select an extension to delete.");
            return;
        }

        auto name = exts[row].name;
        juce::Component::SafePointer<ExtensionViewComponent> safeThis(this);

        juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::WarningIcon,
            "Delete Extension",
            "Are you sure you want to permanently delete \"" + name + "\"?\nThis cannot be undone.",
            "Delete", "Cancel", this,
            juce::ModalCallbackFunction::create([safeThis, name](int res) {
                if (res != 0 && safeThis != nullptr) {
                    Orion::ExtensionManager::getInstance().deleteExtension(name);
                    safeThis->refreshList();
                }
            })
        );
    }

}
