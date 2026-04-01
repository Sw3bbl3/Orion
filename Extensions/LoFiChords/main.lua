-- Lo-Fi Chord Generator
-- Generates a smooth jazz chord progression on a new track.

orion.log("Starting Lo-Fi Chord Generator...")

-- 1. Create Track
orion.track.addInstrumentTrack()
local trackCount = orion.track.getCount()
orion.track.rename(trackCount, "Lo-Fi Keys")
orion.log("Created Track: Lo-Fi Keys at index " .. trackCount)

-- 2. Create Clip (4 bars at 120bpm is 16 beats)
local clipName = "Smooth Jazz II-V-I"
local startBeat = 0
local lengthBeat = 16.0
orion.track.createMidiClip(trackCount, clipName, startBeat, lengthBeat)

-- Get the clip index (should be the last one, so 1 if track was empty)
local clipCount = orion.track.getMidiClipCount(trackCount)
local clipIndex = clipCount
orion.log("Created Clip: " .. clipName .. " at index " .. clipIndex .. " on track " .. trackCount)

-- 3. Add Chords
-- Helper to add a chord
local function addChord(notes, start, duration, vel)
    for i, note in ipairs(notes) do
        -- Slight velocity randomization for human feel
        local v = vel + (math.random() * 0.1 - 0.05)
        if v > 1.0 then v = 1.0 end
        if v < 0.1 then v = 0.1 end

        -- Slight timing offset (strum)
        local s = start + ((i-1) * 0.02)

        orion.clip.addNote(trackCount, clipIndex, note, v, s, duration)
    end
end

-- Dm9 (ii) - Bar 1 (Start 0, Len 4)
-- D3=50, F3=53, A3=57, C4=60, E4=64
local chord1 = {50, 53, 57, 60, 64}
addChord(chord1, 0, 4.0, 0.6)

-- G13 (V) - Bar 2 (Start 4, Len 4)
-- G2=43, F3=53, B3=59, E4=64
local chord2 = {43, 53, 59, 64}
addChord(chord2, 4, 4.0, 0.65)

-- Cmaj9 (I) - Bar 3 & 4 (Start 8, Len 8)
-- C3=48, E3=52, G3=55, B3=59, D4=62
local chord3 = {48, 52, 55, 59, 62}
addChord(chord3, 8, 8.0, 0.6)

orion.log("Chords Generated Successfully.")
orion.alert("Lo-Fi Chords", "Created a new track with a II-V-I progression.")
