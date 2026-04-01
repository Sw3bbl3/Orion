#pragma once

#include <JuceHeader.h>

// Small interface implemented by top-level windows that want a
// "maximize to work area" behavior (i.e. respects Windows taskbar).
class OrionWindow
{
public:
    virtual ~OrionWindow() = default;

    // Toggle between restored bounds and the current display's work area.
    virtual void toggleWorkAreaMaximised() = 0;

    // True when currently maximised to the display work area (not OS fullscreen).
    virtual bool isWorkAreaMaximised() const = 0;
};

