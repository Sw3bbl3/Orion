#include "PluginWindowManager.h"
#include "components/InternalPluginEditor.h"
#include "components/FluxShaperEditor.h"
#include "components/PrismStackEditor.h"
#include "components/WindowWrapper.h"
#include "components/NativeHostingComponent.h"
#include "ui/OrionDesignSystem.h"
#include "orion/FluxShaperNode.h"
#include "orion/PrismStackNode.h"
#include "orion/AudioEngine.h"

class PluginWindow : public juce::DocumentWindow
{
public:
    PluginWindow(const juce::String& name, std::shared_ptr<Orion::EffectNode> node, PluginWindowManager& mgr, std::unique_ptr<juce::Component> customEditor = nullptr)
        : DocumentWindow(name, Orion::UI::DesignSystem::instance().currentTheme().colors.appBg, DocumentWindow::allButtons),
          effectNode(node), manager(mgr)
    {

        setUsingNativeTitleBar(false);
        setTitleBarHeight(0);
        setResizable(true, true);


        setResizeLimits(100, 50, 4000, 3000);

        effectNode->onSizeChanged = [this](int w, int h) {
            juce::Component::SafePointer<PluginWindow> safeThis(this);
            juce::MessageManager::callAsync([safeThis, w, h] {
                if (safeThis) {
                    safeThis->setContentComponentSize(w, h + 30);
                }
            });
        };

        int w = 500, h = 400;
        bool hasCustomEditor = (customEditor != nullptr);
        std::unique_ptr<juce::Component> contentToWrap;
        NativeHostingComponent* nativeHost = nullptr;

        if (hasCustomEditor)
        {
            w = customEditor->getWidth();
            h = customEditor->getHeight();
            contentToWrap = std::move(customEditor);
        }
        else
        {

            if (effectNode->getEditorSize(w, h)) {

            }

            auto host = std::make_unique<NativeHostingComponent>();
            nativeHost = host.get();
            host->setSize(w, h);
            contentToWrap = std::move(host);
        }


        auto wrapper = std::make_unique<WindowWrapper>(contentToWrap.release(), name, false, true);


        wrapper->getTitleBar().setShowPin(true);
        wrapper->getTitleBar().setOnPinToggled([this](bool pinned){
            setAlwaysOnTop(pinned);
        });


        h += 30;

        wrapper->setSize(w, h);
        setContentOwned(wrapper.release(), true);


        setContentComponentSize(w, h);

        centreWithSize(getWidth(), getHeight());
        setVisible(true);


        if (!hasCustomEditor && nativeHost) {


            void* handle = nativeHost->getNativeHandle();
            if (handle) {
                effectNode->openEditor(handle);
            }
        }
    }

    ~PluginWindow() override
    {
        effectNode->onSizeChanged = nullptr;

        effectNode->closeEditor();
    }

    void closeButtonPressed() override
    {
        manager.handleWindowClosed(effectNode.get());
    }

private:
    std::shared_ptr<Orion::EffectNode> effectNode;
    PluginWindowManager& manager;
};

PluginWindowManager::~PluginWindowManager() = default;

PluginWindowManager& PluginWindowManager::getInstance()
{
    static PluginWindowManager instance;
    return instance;
}

void PluginWindowManager::showEditor(std::shared_ptr<Orion::EffectNode> node)
{
    auto it = windows.find(node.get());
    if (it != windows.end())
    {
        it->second->toFront(true);
    }
    else
    {

        juce::String title = node->getName();
        if (title.isEmpty()) title = "Plugin Editor";


        if (node->isExternal())
        {

            windows[node.get()] = std::make_unique<PluginWindow>(title, node, *this);
        }
        else
        {
            std::unique_ptr<juce::Component> editor;
            Orion::AudioTrack* ownerTrack = nullptr;
            if (audioEngine) {
                auto tracks = audioEngine->getTracks();
                for (const auto& t : tracks) {
                    if (!t) continue;
                    auto effects = t->getEffects();
                    if (!effects) continue;
                    for (const auto& fx : *effects) {
                        if (fx.get() == node.get()) {
                            ownerTrack = t.get();
                            break;
                        }
                    }
                    if (ownerTrack) break;
                }
            }

            if (auto flux = std::dynamic_pointer_cast<Orion::FluxShaperNode>(node)) {
                editor = std::make_unique<FluxShaperEditor>(flux);
            } else if (auto prism = std::dynamic_pointer_cast<Orion::PrismStackNode>(node)) {
                editor = std::make_unique<PrismStackEditor>(prism);
            } else {
                editor = std::make_unique<InternalPluginEditor>(node, audioEngine, ownerTrack);
            }

            windows[node.get()] = std::make_unique<PluginWindow>(title, node, *this, std::move(editor));
        }
    }
}

void PluginWindowManager::closeEditor(Orion::EffectNode* node)
{
    auto it = windows.find(node);
    if (it != windows.end())
    {
        windows.erase(it);
    }
}

void PluginWindowManager::handleWindowClosed(Orion::EffectNode* node)
{

    juce::MessageManager::callAsync([this, node] {
        closeEditor(node);
    });
}
