-- Project Cleaner
-- Removes empty tracks and resets mixer settings for remaining tracks.

orion.log("Starting Project Cleaner...")

local trackCount = orion.track.getCount()
local removedCount = 0
local resetCount = 0

-- Iterate backwards to avoid index shifting issues when deleting
for i = trackCount, 1, -1 do
    local midiCount = orion.track.getMidiClipCount(i)
    local audioCount = orion.track.getAudioClipCount(i)
    local totalClips = midiCount + audioCount

    local name = orion.track.getName(i)

    if totalClips == 0 then
        orion.log("Deleting empty track " .. i .. ": " .. name)
        orion.track.delete(i)
        removedCount = removedCount + 1
    else
        -- Reset Mixer
        orion.mixer.setVolume(i, 1.0)
        orion.mixer.setPan(i, 0.0)
        orion.mixer.setMute(i, false)
        orion.mixer.setSolo(i, false)
        resetCount = resetCount + 1
    end
end

local msg = "Removed " .. removedCount .. " empty tracks.\nReset mixer for " .. resetCount .. " tracks."
orion.log(msg)
orion.alert("Project Cleaner", msg)
