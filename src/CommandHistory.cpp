#include "orion/CommandHistory.h"

namespace Orion {

    void CommandHistory::push(std::unique_ptr<Command> cmd) {

        cmd->execute();


        if (!undoStack.empty()) {
            if (undoStack.back()->mergeWith(cmd.get())) {

                currentStateId = ++nextStateId;
                undoStateIds.back() = currentStateId;
                return;
            }
        }

        undoStack.push_back(std::move(cmd));
        currentStateId = ++nextStateId;
        undoStateIds.push_back(currentStateId);

        redoStack.clear();
        redoStateIds.clear();
    }

    void CommandHistory::undo() {
        if (undoStack.empty()) return;

        auto cmd = std::move(undoStack.back());
        undoStack.pop_back();

        long long stateId = undoStateIds.back();
        undoStateIds.pop_back();

        cmd->undo();

        redoStack.push_back(std::move(cmd));
        redoStateIds.push_back(stateId);

        currentStateId = undoStateIds.empty() ? 0 : undoStateIds.back();
    }

    void CommandHistory::redo() {
        if (redoStack.empty()) return;

        auto cmd = std::move(redoStack.back());
        redoStack.pop_back();

        long long stateId = redoStateIds.back();
        redoStateIds.pop_back();

        cmd->execute();

        undoStack.push_back(std::move(cmd));
        undoStateIds.push_back(stateId);

        currentStateId = stateId;
    }

    bool CommandHistory::canUndo() const {
        return !undoStack.empty();
    }

    bool CommandHistory::canRedo() const {
        return !redoStack.empty();
    }

    std::string CommandHistory::getUndoName() const {
        if (undoStack.empty()) return "";
        return undoStack.back()->getName();
    }

    std::string CommandHistory::getRedoName() const {
        if (redoStack.empty()) return "";
        return redoStack.back()->getName();
    }

    void CommandHistory::clear() {
        undoStack.clear();
        redoStack.clear();
        undoStateIds.clear();
        redoStateIds.clear();
        currentStateId = 0;
        nextStateId = 1;
        savedStateId = 0;
    }

    void CommandHistory::setSavePoint() {
        savedStateId = currentStateId;
    }

    bool CommandHistory::isDirty() const {
        return currentStateId != savedStateId;
    }

}
