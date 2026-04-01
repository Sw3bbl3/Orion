#include "MarkdownViewer.h"
#include "../OrionLookAndFeel.h"
#include "../OrionIcons.h"
#include <regex>

namespace Orion {



    class TextBlockComponent : public juce::Component
    {
    public:
        TextBlockComponent(const juce::String& md)
        {
            // Defer parsing until we have a parent or lookandfeel?
            // No, we can just use default colours or expect to be repainted if lookandfeel changes.
            // But we need the colours now. We'll use defaults if not available, or hardcoded Orion colours.
            parse(md);
        }

        void parse(const juce::String& md)
        {
            attrString.clear();
            attrString.setJustification(juce::Justification::topLeft);

            juce::Font baseFont(juce::FontOptions(15.0f));
            juce::Font header1Font(juce::FontOptions(32.0f, juce::Font::bold));
            juce::Font header2Font(juce::FontOptions(26.0f, juce::Font::bold));
            juce::Font header3Font(juce::FontOptions(21.0f, juce::Font::bold));
            juce::Font header4Font(juce::FontOptions(18.0f, juce::Font::bold));

            juce::Colour textCol = juce::Colours::white.withAlpha(0.85f);
            juce::Colour headerCol = juce::Colours::white;
            juce::Colour accentCol = juce::Colour(0xFF0A84FF);
            juce::Colour quoteCol = juce::Colours::white.withAlpha(0.6f);

            juce::StringArray lines;
            lines.addLines(md);

            for (int i = 0; i < lines.size(); ++i)
            {
                juce::String line = lines[i];
                juce::String trim = line.trim();

                if (trim.isEmpty()) {
                    if (attrString.getText().isNotEmpty() && !attrString.getText().endsWith("\n\n"))
                        attrString.append("\n\n", baseFont.withHeight(10.0f), textCol);
                    continue;
                }

                // Headers
                if (trim.startsWith("# ")) {
                    attrString.append(trim.substring(2) + "\n", header1Font, headerCol);
                    if (i < lines.size() - 1) attrString.append("\n", baseFont.withHeight(8.0f), headerCol);
                }
                else if (trim.startsWith("## ")) {
                    attrString.append(trim.substring(3) + "\n", header2Font, headerCol);
                    if (i < lines.size() - 1) attrString.append("\n", baseFont.withHeight(6.0f), headerCol);
                }
                else if (trim.startsWith("### ")) {
                    attrString.append(trim.substring(4) + "\n", header3Font, headerCol);
                    if (i < lines.size() - 1) attrString.append("\n", baseFont.withHeight(4.0f), headerCol);
                }
                else if (trim.startsWith("#### ")) {
                    attrString.append(trim.substring(5) + "\n", header4Font, headerCol);
                }
                // Horizontal Rule
                else if (trim == "---" || trim == "***" || trim == "___") {
                    attrString.append("\n", baseFont.withHeight(8.0f), textCol);
                    attrString.append("________________________________________________________________________________\n", 
                                       baseFont.withHeight(10.0f), juce::Colours::white.withAlpha(0.15f));
                    attrString.append("\n", baseFont.withHeight(8.0f), textCol);
                }
                // Unordered List
                else if (trim.startsWith("- ") || trim.startsWith("* ") || trim.startsWith("+ ")) {
                    attrString.append("  \u2022  ", baseFont, accentCol);
                    parseInline(trim.substring(2).trimEnd(), baseFont, textCol);
                    attrString.append("\n", baseFont, textCol);
                }
                // Blockquote
                else if (trim.startsWith("> ")) {
                    attrString.append("    ", baseFont, textCol);
                    parseInline(trim.substring(2).trimEnd(), baseFont.italicised(), quoteCol);
                    attrString.append("\n", baseFont, textCol);
                }
                // Ordered List
                else {
                    std::string s = trim.toStdString();
                    static const std::regex orderedListPattern(R"(^(\d+)\.\s+(.*))");
                    std::smatch match;
                    if (std::regex_match(s, match, orderedListPattern)) {
                        juce::String num = match[1].str();
                        juce::String content = match[2].str();
                        attrString.append("  " + num + ".  ", baseFont, accentCol);
                        parseInline(content.trimEnd(), baseFont, textCol);
                        attrString.append("\n", baseFont, textCol);
                    } else {
                        // Regular text - join lines if they don't look like new paragraphs
                        parseInline(line.trimEnd(), baseFont, textCol);
                        if (i < lines.size() - 1 && !lines[i+1].trim().isEmpty())
                            attrString.append(" ", baseFont, textCol);
                        else
                            attrString.append("\n", baseFont, textCol);
                    }
                }
            }
            
            updateLayout();
        }

        void updateLayout()
        {
            textLayout.createLayout(attrString, (float)juce::jmax(100, lastWidth));
        }

        void parseInline(const juce::String& text, juce::Font font, juce::Colour colour)
        {
            parseMarkdownInline(text, font, colour, attrString);
        }

        void paint(juce::Graphics& g) override
        {
            textLayout.draw(g, getLocalBounds().toFloat());
        }

        void resized() override
        {
            if (lastWidth != getWidth() || textLayout.getHeight() == 0) {
                lastWidth = getWidth();
                updateLayout();
                setSize(getWidth(), (int)textLayout.getHeight() + 4);
            }
        }

        void mouseUp(const juce::MouseEvent& e) override
        {
            juce::ignoreUnused(e);
        }

        juce::AttributedString attrString;
        juce::TextLayout textLayout;
        int lastWidth = 0;

        static void parseMarkdownInline(const juce::String& text, juce::Font font, juce::Colour colour, juce::AttributedString& target)
        {
            if (text.isEmpty()) return;
            std::string s = text.toStdString();
            
            static const std::regex bold1(R"(\*\*(.*?)\*\*)"), bold2(R"(__(.*?)__)");
            static const std::regex italic1(R"(\*(.*?)\*)"), italic2(R"(_(.*?)_)");
            static const std::regex code(R"(`(.*?)`)"), link(R"(\[(.*?)\]\((.*?)\))"), strike(R"(~~(.*?)~~)");

            struct Match { size_t pos, len; int type; std::smatch sm; };
            std::vector<Match> matches;
            auto check = [&](const std::regex& reg, int type) {
                std::smatch m;
                if (std::regex_search(s, m, reg)) matches.push_back({(size_t)m.position(), (size_t)m.length(), type, m});
            };

            check(bold1, 0); check(bold2, 0); check(italic1, 1); check(italic2, 1);
            check(code, 2); check(link, 3); check(strike, 4);

            if (matches.empty()) { target.append(text, font, colour); return; }

            auto best = std::min_element(matches.begin(), matches.end(), [](const Match& a, const Match& b) {
                return a.pos != b.pos ? a.pos < b.pos : a.len > b.len;
            });

            if (best->pos > 0) target.append(s.substr(0, best->pos), font, colour);

            std::string content = best->sm[1].str();
            std::string suffix = best->sm.suffix().str();

            if (best->type == 0) parseMarkdownInline(juce::String(content), font.boldened(), colour, target);
            else if (best->type == 1) parseMarkdownInline(juce::String(content), font.italicised(), colour, target);
            else if (best->type == 2) target.append(juce::String(content), juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), font.getHeight(), juce::Font::plain)), juce::Colour(0xFFFFA500));
            else if (best->type == 3) parseMarkdownInline(juce::String(content), font, juce::Colour(0xFF4CA1FF), target);
            else if (best->type == 4) parseMarkdownInline(juce::String(content), font, colour.withAlpha(0.5f), target);

            parseMarkdownInline(juce::String(suffix), font, colour, target);
        }
    };

    class AlertBlockComponent : public juce::Component
    {
    public:
        AlertBlockComponent(const juce::String& type, const juce::String& content)
        {
            this->type = type.toUpperCase();
            this->content = content;

            if (this->type == "NOTE") { bgCol = juce::Colour(0xFF0D1117); borderCol = juce::Colour(0xFF30363D); accentCol = juce::Colour(0xFF388BFD); label = "Note"; }
            else if (this->type == "TIP") { bgCol = juce::Colour(0xFF0D1117); borderCol = juce::Colour(0xFF238636).withAlpha(0.3f); accentCol = juce::Colour(0xFF3FB950); label = "Tip"; }
            else if (this->type == "IMPORTANT") { bgCol = juce::Colour(0xFF0D1117); borderCol = juce::Colour(0xFF8957E5).withAlpha(0.3f); accentCol = juce::Colour(0xFFA371F7); label = "Important"; }
            else if (this->type == "WARNING") { bgCol = juce::Colour(0xFF0D1117); borderCol = juce::Colour(0xFF9E6A03).withAlpha(0.3f); accentCol = juce::Colour(0xFFD29922); label = "Warning"; }
            else if (this->type == "CAUTION") { bgCol = juce::Colour(0xFF0D1117); borderCol = juce::Colour(0xFFDA3633).withAlpha(0.3f); accentCol = juce::Colour(0xFFF85149); label = "Caution"; }
            else { bgCol = juce::Colour(0xFF161B22); borderCol = juce::Colour(0xFF30363D); accentCol = juce::Colours::white; label = "Info"; }

            textComp = std::make_unique<TextBlockComponent>(content);
            addAndMakeVisible(textComp.get());
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            g.setColour(bgCol);
            g.fillRoundedRectangle(bounds, 6.0f);
            
            g.setColour(borderCol);
            g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

            auto accentLine = bounds.removeFromLeft(4.0f);
            g.setColour(accentCol);
            g.fillRect(accentLine.reduced(0, 4.0f));

            g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
            g.drawText(label, juce::Rectangle<int>(12, 8, 100, 20), juce::Justification::topLeft);
        }

        void resized() override
        {
            if (textComp) {
                textComp->setBounds(12, 32, getWidth() - 24, textComp->getHeight());
                setSize(getWidth(), textComp->getHeight() + 44);
            }
        }

    private:
        juce::String type, content, label;
        juce::Colour bgCol, borderCol, accentCol;
        std::unique_ptr<TextBlockComponent> textComp;
    };

    class TableBlockComponent : public juce::Component
    {
    public:
        TableBlockComponent(const juce::StringArray& rows)
        {
            juce::Font font(juce::FontOptions(15.0f));
            juce::Colour textCol = juce::Colours::white;

            for (auto& row : rows) {
                juce::Array<juce::AttributedString> rowData;
                auto parts = juce::StringArray::fromTokens(row, "|", "");

                // Skip separator rows (|---| |:---| etc)
                bool isSeparator = true;
                for (auto& p : parts) {
                    juce::String s = p.trim();
                    if (s.isEmpty()) continue;
                    if (s.containsOnly("-") || s.containsOnly(":") || s.containsOnly("|")) continue;
                    isSeparator = false;
                    break;
                }
                if (isSeparator && data.size() > 0) continue;

                for (auto& p : parts) {
                    juce::String s = p.trim();
                    if (s.isEmpty() && parts.indexOf(p) == 0) continue;
                    
                    juce::AttributedString cell;
                    cell.setJustification(juce::Justification::centredLeft);
                    TextBlockComponent::parseMarkdownInline(s, font, textCol, cell);
                    rowData.add(cell);
                }
                if (rowData.size() > 0) data.add(rowData);
            }
        }

        void paint(juce::Graphics& g) override
        {
            g.setColour(juce::Colours::white.withAlpha(0.05f));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);

            if (data.size() == 0) return;

            int numCols = 0;
            for (auto& r : data) numCols = juce::jmax(numCols, r.size());
            if (numCols == 0) return;

            float colW = (float)getWidth() / (float)numCols;
            float curY = 4.0f;

            for (int r = 0; r < data.size(); ++r) {
                float maxRowH = 24.0f;
                
                // First pass: find max height in row
                for (int c = 0; c < data[r].size(); ++c) {
                    juce::TextLayout layout;
                    layout.createLayout(data[r][c], colW - 12.0f);
                    maxRowH = juce::jmax(maxRowH, layout.getHeight() + 8.0f);
                }

                for (int c = 0; c < data[r].size(); ++c) {
                    juce::TextLayout layout;
                    layout.createLayout(data[r][c], colW - 12.0f);
                    
                    auto area = juce::Rectangle<float>((float)c * colW, curY, colW, maxRowH).reduced(6.0f, 4.0f);
                    layout.draw(g, area);
                    
                    g.setColour(juce::Colours::white.withAlpha(0.08f));
                    g.drawRect((float)c * colW, curY, colW, maxRowH, 1.0f);
                }
                curY += maxRowH;
            }
        }

        void resized() override
        {
            if (data.isEmpty()) return;

            int numCols = 0;
            for (auto& r : data) numCols = juce::jmax(numCols, r.size());
            float colW = (float)getWidth() / (float)numCols;
            
            float totalH = 8.0f;
            for (int r = 0; r < data.size(); ++r) {
                float maxRowH = 24.0f;
                for (int c = 0; c < data[r].size(); ++c) {
                    juce::TextLayout layout;
                    layout.createLayout(data[r][c], colW - 12.0f);
                    maxRowH = juce::jmax(maxRowH, layout.getHeight() + 8.0f);
                }
                totalH += maxRowH;
            }
            
            setSize(getWidth(), (int)totalH);
        }

    private:
        juce::Array<juce::Array<juce::AttributedString>> data;
    };

    class CodeBlockComponent : public juce::Component
    {
    public:
        CodeBlockComponent(const juce::String& code, const juce::String& lang)
        {
            this->code = code;
            language = lang.trim().isEmpty() ? "text" : lang.trim();

            addAndMakeVisible(copyBtn);
            copyBtn.setButtonText("Copy");
            copyBtn.onClick = [this] { juce::SystemClipboard::copyTextToClipboard(this->code); };
            copyBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x44FFFFFF));
            copyBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

            langLabel.setText(language.toUpperCase(), juce::dontSendNotification);
            langLabel.setJustificationType(juce::Justification::centredLeft);
            langLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.75f));
            addAndMakeVisible(langLabel);

            document.replaceAllContent(code);

            auto langLower = language.toLowerCase();
            if (langLower == "cpp" || langLower == "c++" || langLower == "cc" || langLower == "c")
                tokeniser = std::make_unique<juce::CPlusPlusCodeTokeniser>();
            else if (langLower == "lua")
                tokeniser = std::make_unique<juce::LuaTokeniser>();
            else if (langLower == "xml" || langLower == "html")
                tokeniser = std::make_unique<juce::XmlTokeniser>();

            codeEditor = std::make_unique<juce::CodeEditorComponent>(document, tokeniser.get());
            codeEditor->setColour(juce::CodeEditorComponent::backgroundColourId, juce::Colours::transparentBlack);
            codeEditor->setColour(juce::CodeEditorComponent::lineNumberBackgroundId, juce::Colour(0x22000000));
            codeEditor->setColour(juce::CodeEditorComponent::lineNumberTextId, juce::Colours::white.withAlpha(0.5f));
            codeEditor->setReadOnly(true);
            codeEditor->setTabSize(4, true);
            codeEditor->loadContent(code);
            // Keep wheel scrolling routed to the outer documentation viewport.
            codeEditor->setInterceptsMouseClicks(false, false);
            addAndMakeVisible(codeEditor.get());
        }

        void paint(juce::Graphics& g) override
        {
            g.setColour(juce::Colour(0xFF161A22));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

            g.setColour(juce::Colours::white.withAlpha(0.12f));
            g.drawRoundedRectangle(getLocalBounds().toFloat(), 6.0f, 1.0f);

            g.setColour(juce::Colours::black.withAlpha(0.25f));
            g.fillRoundedRectangle(juce::Rectangle<float>(1.0f, 1.0f, (float)getWidth() - 2.0f, 28.0f), 6.0f);
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced(12);

            auto top = area.removeFromTop(24);
            langLabel.setBounds(top.removeFromLeft(120));
            copyBtn.setBounds(top.removeFromRight(60));
            area.removeFromTop(8);

            if (codeEditor)
                codeEditor->setBounds(area);

            // Calculate height based on editor content
            int numLines = juce::StringArray::fromLines(code).size();
            int contentHeight = (numLines * 18) + 60;
            
            if (getHeight() != contentHeight) {
                setSize(getWidth(), contentHeight);
            }
        }

    private:
        juce::String code;
        juce::String language;
        juce::TextButton copyBtn;
        juce::Label langLabel;
        juce::CodeDocument document;
        std::unique_ptr<juce::CodeTokeniser> tokeniser;
        std::unique_ptr<juce::CodeEditorComponent> codeEditor;
    };

    class ImageBlockComponent : public juce::Component
    {
    public:
        ImageBlockComponent(const juce::String& url, const juce::File& baseDir)
        {

            juce::File imgFile;

            if (juce::File::isAbsolutePath(url)) {
                imgFile = juce::File(url);
            } else {
                imgFile = baseDir.getChildFile(url);
            }

            if (imgFile.existsAsFile()) {
                image = juce::ImageFileFormat::loadFrom(imgFile);
            }
        }

        void paint(juce::Graphics& g) override
        {
            if (image.isValid()) {
                g.drawImage(image, getLocalBounds().toFloat(), juce::RectanglePlacement::xMid);
            } else {
                g.setColour(juce::Colours::red);
                g.drawRect(getLocalBounds());
                g.drawText("Image not found", getLocalBounds(), juce::Justification::centred, true);
            }
        }

        void resized() override
        {
            if (image.isValid()) {
                float ratio = (float)image.getHeight() / (float)image.getWidth();
                int h = (int)(getWidth() * ratio);
                if (getHeight() != h) setSize(getWidth(), h);
            } else {
                if (getHeight() != 50) setSize(getWidth(), 50);
            }
        }

    private:
        juce::Image image;
    };




    MarkdownViewer::MarkdownViewer()
    {
        setOpaque(true);
        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&contentComp, false);
        viewport.setScrollBarsShown(true, false, true, true);
        viewport.getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, juce::Colours::white.withAlpha(0.1f));
        viewport.getVerticalScrollBar().setColour(juce::ScrollBar::backgroundColourId, juce::Colours::transparentBlack);

        addAndMakeVisible(closeBtn);
        closeBtn.setButtonText("Back");
        closeBtn.onClick = [this] { if (onClose) onClose(); };
        closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::white.withAlpha(0.1f));

        addAndMakeVisible(titleLabel);
        titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
        titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    }

    MarkdownViewer::~MarkdownViewer() {}

    void MarkdownViewer::setEmbedded(bool embedded)
    {
        isEmbedded = embedded;
        closeBtn.setVisible(!embedded);
        titleLabel.setVisible(!embedded);
        resized();
    }

    void MarkdownViewer::loadFile(const juce::File& file)
    {
        if (file.existsAsFile()) {
            titleLabel.setText(file.getFileNameWithoutExtension(), juce::dontSendNotification);
            contentComp.setMarkdown(file.loadFileAsString(), file.getParentDirectory());
        } else {
            titleLabel.setText("File Not Found", juce::dontSendNotification);
            contentComp.setMarkdown("The requested documentation file could not be found.");
        }
        resized();
        viewport.setViewPosition(0, 0);
    }

    void MarkdownViewer::loadContent(const juce::String& text)
    {
        contentComp.setMarkdown(text);
        resized();
        viewport.setViewPosition(0, 0);
    }

    void MarkdownViewer::paint(juce::Graphics& g)
    {
        auto bg = findColour(OrionLookAndFeel::dashboardBackgroundColourId);
        g.fillAll(bg);

        // Subtle content area background with a premium gradient
        auto contentArea = getLocalBounds().toFloat().reduced(12.0f, 4.0f);
        
        juce::ColourGradient grad(bg.brighter(0.05f), contentArea.getX(), contentArea.getY(),
                                bg.darker(0.05f), contentArea.getX(), contentArea.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(contentArea, 10.0f);

        g.setColour(juce::Colours::white.withAlpha(0.03f));
        g.drawRoundedRectangle(contentArea, 10.0f, 1.2f);
    }

    void MarkdownViewer::resized()
    {
        auto area = getLocalBounds();

        viewport.setBounds(area.reduced(4, 0));

        int w = viewport.getWidth();
        if (viewport.getVerticalScrollBar().isVisible())
            w -= viewport.getScrollBarThickness();
        
        w -= 8; // Smaller margin
        
        contentComp.setSize(juce::jmax(200, w), contentComp.getHeight());
    }



    MarkdownViewer::MarkdownComponent::MarkdownComponent()
    {
        setOpaque(true);
    }
    MarkdownViewer::MarkdownComponent::~MarkdownComponent() { blocks.clear(); }

    void MarkdownViewer::MarkdownComponent::setMarkdown(const juce::String& md, const juce::File& baseDir)
    {
        blocks.clear();
        removeAllChildren();
        currentBaseDir = baseDir;

        juce::StringArray lines;
        lines.addLines(md);

        juce::String currentText;
        bool inCodeBlock = false;
        juce::String currentCode;
        juce::String codeLang;
        
        juce::StringArray currentTable;
        bool inTable = false;

        static const std::regex imagePattern(R"(^!\[(.*?)\]\((.*?)\)$)");
        static const std::regex alertPattern(R"(^>\s+\[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\](.*)$)", std::regex_constants::icase);

        for (int i = 0; i < lines.size(); ++i)
        {
            juce::String line = lines[i];
            juce::String trim = line.trim();

            // Table check
            if (trim.startsWith("|") && !inCodeBlock) {
                if (currentText.isNotEmpty()) {
                    addTextBlock(currentText);
                    currentText.clear();
                }
                inTable = true;
                currentTable.add(trim);
                continue;
            } else if (inTable) {
                inTable = false;
                addTableBlock(currentTable);
                currentTable.clear();
            }

            if (trim.startsWith("```")) {
                if (inCodeBlock) {
                    inCodeBlock = false;
                    addCodeBlock(currentCode, codeLang);
                    currentCode.clear();
                } else {
                    if (currentText.isNotEmpty()) {
                        addTextBlock(currentText);
                        currentText.clear();
                    }
                    inCodeBlock = true;
                    codeLang = trim.substring(3).trim();
                }
                continue;
            }

            if (inCodeBlock) {
                currentCode += line + "\n";
                continue;
            }

            // Alert check
            std::string s = trim.toStdString();
            std::smatch alertMatch;
            if (std::regex_match(s, alertMatch, alertPattern)) {
                if (currentText.isNotEmpty()) {
                    addTextBlock(currentText);
                    currentText.clear();
                }
                
                juce::String type = alertMatch[1].str();
                juce::String content = alertMatch[2].str();
                
                // Collect following blockquote lines as content
                while (i + 1 < lines.size() && lines[i+1].trim().startsWith(">")) {
                    i++;
                    juce::String nextLine = lines[i].trim();
                    content += "\n" + nextLine.substring(1).trim();
                }
                
                addAlertBlock(type, content);
                continue;
            }

            // Image check
            std::smatch imgMatch;
            if (std::regex_match(s, imgMatch, imagePattern)) {
                if (currentText.isNotEmpty()) {
                    addTextBlock(currentText);
                    currentText.clear();
                }

                juce::String alt = imgMatch[1].str();
                juce::String url = imgMatch[2].str();
                addImageBlock(alt, url, baseDir);
                continue;
            }

            currentText += line + "\n";
        }

        if (currentText.isNotEmpty()) addTextBlock(currentText);
        if (currentTable.size() > 0) addTableBlock(currentTable);

        resized();
    }

    void MarkdownViewer::MarkdownComponent::addTextBlock(const juce::String& text)
    {
        auto* b = new TextBlockComponent(text);
        blocks.add(b);
        addAndMakeVisible(b);
    }

    void MarkdownViewer::MarkdownComponent::addCodeBlock(const juce::String& code, const juce::String& lang)
    {
        auto* b = new CodeBlockComponent(code, lang);
        blocks.add(b);
        addAndMakeVisible(b);
    }

    void MarkdownViewer::MarkdownComponent::addAlertBlock(const juce::String& type, const juce::String& content)
    {
        auto* b = new AlertBlockComponent(type, content);
        blocks.add(b);
        addAndMakeVisible(b);
    }

    void MarkdownViewer::MarkdownComponent::addTableBlock(const juce::StringArray& rows)
    {
        auto* b = new TableBlockComponent(rows);
        blocks.add(b);
        addAndMakeVisible(b);
    }

    void MarkdownViewer::MarkdownComponent::addImageBlock(const juce::String& alt, const juce::String& url, const juce::File& baseDir)
    {
        juce::ignoreUnused(alt);
        auto* b = new ImageBlockComponent(url, baseDir);
        blocks.add(b);
        addAndMakeVisible(b);
    }

    void MarkdownViewer::MarkdownComponent::resized()
    {
        const int w = getWidth() - ((int)padding * 2);
        if (w < 100) return;

        int y = (int)padding;
        
        for (auto* b : blocks)
        {
            b->setSize(w, b->getHeight());
            b->setTopLeftPosition((int)padding, y);
            
            int spacing = 16;
            if (dynamic_cast<CodeBlockComponent*>(b) || dynamic_cast<ImageBlockComponent*>(b) || 
                dynamic_cast<TableBlockComponent*>(b) || dynamic_cast<AlertBlockComponent*>(b))
                spacing = 24;

            y += b->getHeight() + spacing;
        }

        int totalH = y + (int)padding + 40;
        if (getHeight() != totalH) {
            setSize(getWidth(), totalH);
        }
    }

}
