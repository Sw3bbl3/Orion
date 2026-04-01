#include <iostream>
#include <memory>
#include <vector>
#include <cassert>
#include "orion/Command.h"
#include "orion/CommandHistory.h"

// Mock Command
class MockCommand : public Orion::Command {
public:
    MockCommand(std::string n, bool mergeable = false) : name(n), mergeable(mergeable) {}
    void execute() override { std::cout << "Exec " << name << "\n"; }
    void undo() override { std::cout << "Undo " << name << "\n"; }
    std::string getName() const override { return name; }

    bool mergeWith(const Orion::Command* other) override {
        if (mergeable && other->getName() == name) {
            std::cout << "Merged " << name << "\n";
            return true;
        }
        return false;
    }

private:
    std::string name;
    bool mergeable;
};

int main() {
    Orion::CommandHistory history;

    // 1. Initial State
    if (history.isDirty()) { std::cerr << "Fail 1\n"; return 1; }
    std::cout << "Test 1 Passed: Initial Clean\n";

    // 2. Push Command
    history.push(std::make_unique<MockCommand>("Cmd1"));
    if (!history.isDirty()) { std::cerr << "Fail 2\n"; return 1; }
    std::cout << "Test 2 Passed: Push Dirty\n";

    // 3. Save
    history.setSavePoint();
    if (history.isDirty()) { std::cerr << "Fail 3\n"; return 1; }
    std::cout << "Test 3 Passed: Save Clean\n";

    // 4. Push Command 2
    history.push(std::make_unique<MockCommand>("Cmd2"));
    if (!history.isDirty()) { std::cerr << "Fail 4\n"; return 1; }
    std::cout << "Test 4 Passed: Push Dirty 2\n";

    // 5. Undo
    history.undo();
    if (history.isDirty()) { std::cerr << "Fail 5\n"; return 1; }
    std::cout << "Test 5 Passed: Undo Clean\n";

    // 6. Redo
    history.redo();
    if (!history.isDirty()) { std::cerr << "Fail 6\n"; return 1; }
    std::cout << "Test 6 Passed: Redo Dirty\n";

    // 7. Undo again
    history.undo();
    if (history.isDirty()) { std::cerr << "Fail 7\n"; return 1; }

    // 8. Undo past save point
    history.undo(); // Undoing Cmd1
    if (!history.isDirty()) { std::cerr << "Fail 8\n"; return 1; } // Empty state != Saved state (Cmd1)
    std::cout << "Test 8 Passed: Undo past save Dirty\n";

    // 9. Redo back to save point
    history.redo();
    if (history.isDirty()) { std::cerr << "Fail 9\n"; return 1; }
    std::cout << "Test 9 Passed: Redo to save Clean\n";

    // 10. Merge Test
    // Create mergeable command
    history.push(std::make_unique<MockCommand>("Slider", true));
    // Saved state is Cmd1. Current state is Cmd1 + Slider.
    history.setSavePoint(); // Save at Cmd1 + Slider
    if (history.isDirty()) { std::cerr << "Fail 10a\n"; return 1; }

    // Push mergeable command
    history.push(std::make_unique<MockCommand>("Slider", true));
    // Should merge. Stack size same. But state changed.
    if (!history.isDirty()) { std::cerr << "Fail 10b\n"; return 1; }
    std::cout << "Test 10 Passed: Merge Dirty\n";

    std::cout << "All Tests Passed!\n";
    return 0;
}
