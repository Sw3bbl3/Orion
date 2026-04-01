-- MixResetAll
-- Resets volume, pan, mute and solo for every track to default values.
-- Useful for cleaning up a session that has accumulated random mixer state.
-- Part of the Orion Factory Extensions.

local count = orion.track.getCount()

if count == 0 then
    orion.log("MixResetAll: No tracks found. Nothing to reset.", 1)
else
    for i = 1, count do
        local name = orion.track.getName(i)
        orion.mixer.setVolume(i, 1.0)
        orion.mixer.setPan(i, 0.0)
        orion.mixer.setMute(i, false)
        orion.mixer.setSolo(i, false)
        orion.log("  Reset: " .. name)
    end

    -- Also reset master
    orion.mixer.setMasterVolume(1.0)
    orion.mixer.setMasterPan(0.0)

    orion.log("MixResetAll: Reset " .. count .. " tracks + master to defaults.", 3)
end
