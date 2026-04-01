#pragma once
#include "Command.h"
#include <vector>
#include <memory>
#include <stack>

namespace Orion {

    class CommandHistory {
    public:
        void push(std::unique_ptr<Command> cmd);
        void undo();
        void redo();
        bool canUndo() const;
        bool canRedo() const;
        std::string getUndoName() const;
        std::string getRedoName() const;
        void clear();

        void setSavePoint();
        bool isDirty() const;

    private:
        std::vector<std::unique_ptr<Command>> undoStack;
        std::vector<std::unique_ptr<Command>> redoStack;



        long long currentStateId = 0;
        long long nextStateId = 1;
        long long savedStateId = 0;

        std::vector<long long> undoStateIds;
        std::vector<long long> redoStateIds;
    };

}
