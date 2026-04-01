-- Techno Rumble Builder
-- Creates a driving kick and bass rumble pattern.

orion.log("Starting Techno Rumble Builder...")

-- 1. Create Kick Track
orion.track.addInstrumentTrack()
local kickTrack = orion.track.getCount()
orion.track.rename(kickTrack, "Techno Kick")
orion.log("Created Track: Techno Kick at index " .. kickTrack)

-- 2. Create Kick Clip
local clipLen = 16.0 -- 4 bars
orion.track.createMidiClip(kickTrack, "Kick 4/4", 0, clipLen)
local kickClipIndex = orion.track.getMidiClipCount(kickTrack)

-- 3. Add Kick Notes
-- C1 = 24, C2 = 36. Kick usually around C1.
local kickNote = 36
for i = 0, 15 do -- 16 beats (4 bars)
    orion.clip.addNote(kickTrack, kickClipIndex, kickNote, 1.0, i, 0.25)
end

-- 4. Create Rumble Track
orion.track.addInstrumentTrack()
local rumbleTrack = orion.track.getCount()
orion.track.rename(rumbleTrack, "Rumble Bass")
orion.log("Created Track: Rumble Bass at index " .. rumbleTrack)

-- 5. Create Rumble Clip
orion.track.createMidiClip(rumbleTrack, "Rumble Pattern", 0, clipLen)
local rumbleClipIndex = orion.track.getMidiClipCount(rumbleTrack)

-- 6. Add Rumble Notes
-- Often a rolling 16th note pattern on the off-beats.
-- Beats: 0.0 (Kick), 0.25, 0.5, 0.75
-- Rumble usually on 0.25, 0.5, 0.75 or just 0.5, 0.75
local rumbleNote = 36 -- Same pitch, maybe an octave lower/higher depending on patch
-- Let's put rumble on the 16th notes: .25, .50, .75
for beat = 0, 15 do
    -- 16th note at +0.25
    orion.clip.addNote(rumbleTrack, rumbleClipIndex, rumbleNote, 0.6, beat + 0.25, 0.20)
    -- 16th note at +0.50
    orion.clip.addNote(rumbleTrack, rumbleClipIndex, rumbleNote, 0.5, beat + 0.50, 0.20)
    -- 16th note at +0.75
    orion.clip.addNote(rumbleTrack, rumbleClipIndex, rumbleNote, 0.4, beat + 0.75, 0.20)
end

-- Set basic mix
orion.mixer.setVolume(kickTrack, 0.9)
orion.mixer.setVolume(rumbleTrack, 0.7)

-- Pan slight separation? Usually mono for techno kick/bass.
orion.mixer.setPan(kickTrack, 0.0)
orion.mixer.setPan(rumbleTrack, 0.0)

orion.log("Techno Rumble Generated.")
orion.alert("Techno Rumble", "Created Kick and Rumble tracks.")
