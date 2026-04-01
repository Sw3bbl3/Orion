#include "DocumentationViewComponent.h"
#include "../OrionLookAndFeel.h"
#include "../OrionIcons.h"

namespace Orion {

    DocumentationViewComponent::DocumentationViewComponent()
    {
        setOpaque(true);

        addAndMakeVisible(headerLabel);
        headerLabel.setText("Documentation", juce::dontSendNotification);
        headerLabel.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        headerLabel.setJustificationType(juce::Justification::centredLeft);
        headerLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        
        addAndMakeVisible(subtitleLabel);
        subtitleLabel.setText("Search guides, API notes, and examples", juce::dontSendNotification);
        subtitleLabel.setFont(juce::FontOptions(12.0f));
        subtitleLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));

        addAndMakeVisible(searchBox);
        searchBox.setTextToShowWhenEmpty("Search docs...", juce::Colours::grey);
        searchBox.onTextChange = [this] {
            filterText = searchBox.getText().trim().toLowerCase();
            updateList();
        };

        addAndMakeVisible(list);
        list.setModel(this);
        list.setOpaque(true);
        list.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xFF14161B));
        list.setRowHeight(54); // Increased for snippets

        addAndMakeVisible(viewer);
        viewer.setEmbedded(true);

        addAndMakeVisible(currentDocLabel);
        currentDocLabel.setJustificationType(juce::Justification::centredLeft);
        currentDocLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.75f));

        addAndMakeVisible(resizer);
        resizer.setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);

        addAndMakeVisible(toggleSidebarBtn);
        toggleSidebarBtn.setButtonText("Toggle Sidebar");
        toggleSidebarBtn.onClick = [this] { toggleSidebar(); };
        
        // Load toggle icon
        auto toggleIconPath = OrionIcons::getMenuIcon();
        toggleIcon = std::make_unique<juce::DrawablePath>();
        auto* dp = static_cast<juce::DrawablePath*>(toggleIcon.get());
        dp->setPath(toggleIconPath);
        dp->setFill(juce::FillType(juce::Colours::white.withAlpha(0.6f)));
        toggleSidebarBtn.setImages(toggleIcon.get(), nullptr, nullptr);

        scanDocumentation();

        if (!items.empty()) {
            for (int i=0; i<(int)items.size(); ++i) {
                if (!items[i].isCategory) {
                    list.selectRow(i);
                    viewer.loadFile(items[i].file);
                    currentDocLabel.setText("Viewing: " + items[i].title, juce::dontSendNotification);
                    break;
                }
            }
        }
    }

    DocumentationViewComponent::~DocumentationViewComponent() {}

    void DocumentationViewComponent::mouseDown(const juce::MouseEvent& e)
    {
        if (e.eventComponent == &resizer)
            startSidebarWidth = sidebarWidth;
    }

    void DocumentationViewComponent::mouseDrag(const juce::MouseEvent& e)
    {
        if (e.eventComponent == &resizer)
        {
            auto delta = e.getOffsetFromDragStart().getX();
            sidebarWidth = juce::jlimit(150, 600, startSidebarWidth + delta);
            resized();
        }
    }
    void DocumentationViewComponent::scanDocumentation()
    {
        docMap.clear();
        items.clear();
        fileContentCache.clear();

        juce::File docsDir;
        const auto& exec = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        const auto& cwd = juce::File::getCurrentWorkingDirectory();


        std::vector<juce::File> candidates = {
            cwd.getChildFile("assets/documentation"),
            exec.getParentDirectory().getChildFile("assets/documentation"),
            exec.getParentDirectory().getChildFile("../assets/documentation"),
            exec.getParentDirectory().getChildFile("../../assets/documentation"),
            exec.getParentDirectory().getChildFile("../../../assets/documentation")
        };

        for (const auto& d : candidates) {
            if (d.exists() && d.isDirectory()) {
                docsDir = d;
                break;
            }
        }

        if (docsDir.exists() && docsDir.isDirectory()) {
            auto dirs = docsDir.findChildFiles(juce::File::findDirectories, false);
            dirs.sort();

            for (const auto& dir : dirs) {
                auto files = dir.findChildFiles(juce::File::findFiles, false, "*.md");
                files.sort();
                if (!files.isEmpty()) {
                    std::vector<juce::File> fileList;
                    for (auto const& f : files) {
                        fileList.push_back(f);
                        // Index content for search and snippets
                        auto content = f.loadFileAsString();
                        fileContentCache[f.getFullPathName()] = content;
                    }
                    docMap[dir.getFileName()] = fileList;
                    if (categoryExpanded.find(dir.getFileName()) == categoryExpanded.end())
                        categoryExpanded[dir.getFileName()] = true;
                }
            }


            auto rootFiles = docsDir.findChildFiles(juce::File::findFiles, false, "*.md");
            rootFiles.sort();
            if (!rootFiles.isEmpty()) {
                std::vector<juce::File> fileList;
                for (auto const& f : rootFiles) {
                    fileList.push_back(f);
                    auto content = f.loadFileAsString();
                    fileContentCache[f.getFullPathName()] = content;
                }
                docMap["General"] = fileList;
                if (categoryExpanded.find("General") == categoryExpanded.end())
                    categoryExpanded["General"] = true;
            }
        }

        updateList();
    }

    void DocumentationViewComponent::updateList()
    {
        items.clear();
        for (auto const& [category, files] : docMap) {
            bool categoryMatches = false;
            if (!filterText.isEmpty() && category.toLowerCase().contains(filterText))
                categoryMatches = true;

            std::vector<DocItem> tempFiles;
            for (const auto& f : files) {
                auto title = f.getFileNameWithoutExtension();
                auto const& content = fileContentCache[f.getFullPathName()];
                
                bool matches = categoryMatches || title.toLowerCase().contains(filterText) || content.toLowerCase().contains(filterText);
                
                if (filterText.isEmpty() || matches) {
                    DocItem fileItem;
                    fileItem.title = title;
                    fileItem.file = f;
                    fileItem.isCategory = false;
                    fileItem.level = 1;
                    
                    // Generate basic snippet (skip frontmatter if exists)
                    juce::String snippetData = content.substring(0, 300).replace("\n", " ").replace("\r", " ").trim();
                    if (snippetData.length() > 80)
                        snippetData = snippetData.substring(0, 77) + "...";
                    fileItem.snippet = snippetData;
                    
                    tempFiles.push_back(fileItem);
                }
            }

            if (!tempFiles.empty()) {
                DocItem cat;
                cat.title = category;
                cat.isCategory = true;
                cat.level = 0;
                cat.expanded = categoryExpanded[category];
                items.push_back(cat);

                if (cat.expanded || !filterText.isEmpty()) {
                    for (auto const& tf : tempFiles)
                        items.push_back(tf);
                }
            }
        }
        list.updateContent();
        list.repaint();
    }

    void DocumentationViewComponent::toggleSidebar()
    {
        sidebarVisible = !sidebarVisible;
        resized();
        repaint();
    }

    void DocumentationViewComponent::resized()
    {
        auto area = getLocalBounds();
        
        if (sidebarVisible) {
            auto sidebar = area.removeFromLeft(sidebarWidth);
            
            resizer.setBounds(sidebar.withWidth(4).withX(sidebar.getRight() - 2));
            resizer.setVisible(true);

            auto header = sidebar.removeFromTop(44);
            headerLabel.setBounds(header.removeFromLeft(header.getWidth() - 40).reduced(10, 0));
            toggleSidebarBtn.setBounds(header.reduced(8));

            subtitleLabel.setBounds(sidebar.removeFromTop(20).reduced(10, 0));
            searchBox.setBounds(sidebar.removeFromTop(34).reduced(10, 2));
            sidebar.removeFromTop(6);

            list.setBounds(sidebar.reduced(6, 0));
        } else {
            resizer.setVisible(false);
            auto header = area.removeFromTop(44);
            toggleSidebarBtn.setBounds(header.removeFromLeft(44).reduced(8));
        }

        auto contentHeader = area.removeFromTop(44);
        currentDocLabel.setBounds(contentHeader.reduced(10, 0));
        viewer.setBounds(area.reduced(2));
    }

    void DocumentationViewComponent::paint(juce::Graphics& g)
    {
        auto bg = findColour(OrionLookAndFeel::dashboardBackgroundColourId).darker(0.15f);
        g.fillAll(bg);

        if (sidebarVisible) {
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.fillRect(0, 0, sidebarWidth, getHeight());

            g.setColour(juce::Colours::white.withAlpha(0.05f));
            g.drawVerticalLine(sidebarWidth, 0.0f, (float)getHeight());
        }
    }

    int DocumentationViewComponent::getNumRows()
    {
        return (int)items.size();
    }

    void DocumentationViewComponent::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool rowIsSelected)
    {
        if (row >= (int)items.size()) return;

        const auto& item = items[row];
        auto accentColor = findColour(OrionLookAndFeel::accentColourId);

        g.fillAll(juce::Colour(0xFF14161B));

        if (rowIsSelected) {
            g.setColour(accentColor.withAlpha(0.15f));
            g.fillRoundedRectangle(juce::Rectangle<float>(8, 2, (float)width - 16, (float)height - 4), 6.0f);

            g.setColour(accentColor);
            g.fillRect(2.0f, 6.0f, 3.0f, (float)height - 12.0f);
        }

        if (item.isCategory) {
            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
            const bool expanded = categoryExpanded[item.title];

            // Draw a more modern arrow
            juce::Path p;
            float arrowSize = 6.0f;
            float centerY = height * 0.5f;
            if (expanded) {
                p.startNewSubPath(12, centerY - arrowSize*0.3f);
                p.lineTo(12 + arrowSize, centerY + arrowSize*0.3f);
                p.lineTo(12 + arrowSize*2, centerY - arrowSize*0.3f);
            } else {
                p.startNewSubPath(14, centerY - arrowSize);
                p.lineTo(14 + arrowSize, centerY);
                p.lineTo(14, centerY + arrowSize);
            }
            g.strokePath(p, juce::PathStrokeType(1.5f));

            auto icon = OrionIcons::getFolderIcon();
            OrionIcons::drawIcon(g, icon, juce::Rectangle<float>(30, (height-16)/2.0f, 16, 16), juce::Colours::white.withAlpha(0.5f));
            g.drawText(item.title.toUpperCase(), 54, 0, width - 54, height, juce::Justification::centredLeft);
        } else {
            auto titleArea = juce::Rectangle<int>(64, 8, width - 74, 20);
            auto snippetArea = juce::Rectangle<int>(64, 28, width - 74, 18);

            g.setColour(juce::Colours::white.withAlpha(rowIsSelected ? 1.0f : 0.85f));
            g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
            g.drawText(item.title, titleArea, juce::Justification::bottomLeft);

            g.setColour(juce::Colours::white.withAlpha(rowIsSelected ? 0.7f : 0.45f));
            g.setFont(juce::FontOptions(12.0f));
            g.drawText(item.snippet, snippetArea, juce::Justification::topLeft, true);

            auto icon = OrionIcons::getDocumentationIcon();
            OrionIcons::drawIcon(g, icon, juce::Rectangle<float>(42, (height - 14) / 2.0f, 14, 14),
                               juce::Colours::white.withAlpha(rowIsSelected ? 0.8f : 0.4f));
        }
    }

    void DocumentationViewComponent::listBoxItemClicked(int row, const juce::MouseEvent&)
    {
        if (row >= 0 && row < (int)items.size()) {
            const auto& item = items[row];
            if (item.isCategory) {
                categoryExpanded[item.title] = !categoryExpanded[item.title];
                updateList();
            } else {
                viewer.loadFile(item.file);
                currentDocLabel.setText("Viewing: " + item.title, juce::dontSendNotification);
            }
        }
    }

}
