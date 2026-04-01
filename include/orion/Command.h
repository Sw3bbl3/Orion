#pragma once
#include <string>
#include <vector>
#include <memory>

namespace Orion {

    class Command {
    public:
        virtual ~Command() = default;
        virtual void execute() = 0;
        virtual void undo() = 0;
        virtual std::string getName() const = 0;
        virtual bool mergeWith(const Command* ) { return false; }
    };

    class MacroCommand : public Command {
    public:
        MacroCommand(const std::string& name) : name(name) {}
        void addCommand(std::unique_ptr<Command> cmd) { commands.push_back(std::move(cmd)); }
        void execute() override { for(auto& c : commands) c->execute(); }
        void undo() override { for(auto it = commands.rbegin(); it != commands.rend(); ++it) (*it)->undo(); }
        std::string getName() const override { return name; }
    private:
        std::string name;
        std::vector<std::unique_ptr<Command>> commands;
    };

}
