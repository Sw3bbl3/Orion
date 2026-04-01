-- MarkerHelper
-- Automatically adds region markers at every N bars based on current BPM and time signature.
-- Part of the Orion Factory Extensions.

local bpm = orion.project.getBpm()
local num, den = orion.project.getTimeSignature()
local key = orion.project.getKeySignature()
local sr = orion.project.getSampleRate()

orion.log("MarkerHelper: BPM=" .. bpm .. "  Time=" .. num .. "/" .. den .. "  Key=" .. key)

-- Add region markers every 8 bars
local beatsPerBar = num
local barsInterval = 8
local beatsInterval = beatsPerBar * barsInterval
local totalBeats = 64  -- mark up to 64 bars worth

local count = 0
local b = 0
while b < totalBeats do
    local secs = orion.project.beatsToTime(b)
    local barNum = math.floor(b / beatsPerBar) + 1
    orion.markers.addRegion(secs, "Bar " .. barNum)
    count = count + 1
    b = b + beatsInterval
end

orion.log("MarkerHelper: Added " .. count .. " region markers every " .. barsInterval .. " bars.", 3)
