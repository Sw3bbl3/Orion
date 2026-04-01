#include <iostream>

#include "orion/AudioEngine.h"
#include "CommandManager.h"

int main()
{
    Orion::AudioEngine engine;
    Orion::CommandManager manager(engine);

    juce::Array<juce::CommandID> commands;
    manager.getAllCommands(commands);

    if (!commands.contains(Orion::CommandManager::ShowGuidedStartWizard)) {
        std::cerr << "Expected ShowGuidedStartWizard command to be registered\n";
        return 1;
    }

    juce::ApplicationCommandInfo info(Orion::CommandManager::ShowGuidedStartWizard);
    manager.getCommandInfo(Orion::CommandManager::ShowGuidedStartWizard, info);

    if (info.shortName.isEmpty()) {
        std::cerr << "Expected guided start command to have a name\n";
        return 1;
    }

    if (!info.shortName.containsIgnoreCase("guided")) {
        std::cerr << "Expected guided start command name to include 'guided'\n";
        return 1;
    }

    std::cout << "TestGuidedStartCommand passed\n";
    return 0;
}

