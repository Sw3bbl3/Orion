# Orion Lua API Reference

This reference is aligned with the runtime `orion.*` bindings exposed by Orion's extension host.

## Root

- `orion.log(message[, level])`
- `orion.alert(title, message)`
- `orion.getVersion()`
- `orion.getTheme()`

## `orion.transport`

- `play()` / `stop()` / `pause()` / `record()` / `panic()`
- `isPlaying()` / `isPaused()` / `isRecording()`
- `getPosition()` / `setPosition(seconds)`
- `getPositionBeats()` / `setPositionBeats(beats)`
- `isLooping()` / `setLooping(enabled)`
- `getLoopStart()` / `getLoopEnd()`
- `setLoopPoints(startSeconds, endSeconds)`

## `orion.project`

- `getBpm()` / `setBpm(bpm)`
- `getAuthor()` / `setAuthor(author)`
- `getGenre()` / `setGenre(genre)`
- `getKeySignature()` / `setKeySignature(signature)`
- `getTimeSignature()` / `setTimeSignature(num, den)`
- `getSampleRate()`
- `clearAllTracks()`
- `timeToBeats(seconds)`
- `beatsToTime(beats)`

## `orion.track`

- `getCount()` / `getName(trackIndex)` / `getType(trackIndex)`
- `isMuted(trackIndex)` / `isSoloed(trackIndex)`
- `addAudioTrack()` / `addInstrumentTrack()`
- `delete(trackIndex)` / `rename(trackIndex, name)`
- `moveTrack(fromIndex, toIndex)`
- `setColor(trackIndex, r, g, b)`
- `createMidiClip(trackIndex, name, startBeat, lengthBeat)`
- `getMidiClipCount(trackIndex)` / `getAudioClipCount(trackIndex)`

## `orion.clip`

- `addNote(trackIndex, clipIndex, note, velocity, startBeat, durationBeat)`
- `getNoteCount(trackIndex, clipIndex)`
- `getNoteAt(trackIndex, clipIndex, noteIndex)`
- `clearNotes(trackIndex, clipIndex)`
- `setClipName(trackIndex, clipIndex, name)`
- `getClipName(trackIndex, clipIndex)`

## `orion.mixer`

- `setVolume(trackIndex, volume)` / `getVolume(trackIndex)`
- `setPan(trackIndex, pan)` / `getPan(trackIndex)`
- `setMute(trackIndex, muted)`
- `setSolo(trackIndex, soloed)`
- `getMasterVolume()` / `setMasterVolume(volume)`
- `getMasterPan()` / `setMasterPan(pan)`

## `orion.markers`

- `addRegion(positionSeconds, name)`
- `getRegionCount()`
- `getRegion(regionIndex)`
- `removeRegion(regionIndex)`
- `addTempo(positionSeconds, bpm)`
- `clearTempoMarkers()`

## `orion.ui`

- `confirm(title, message, callback)`
- `input(title, message, callback)`
- `getSelectedClips()`
- `getSelectedNotes()`

## Notes

- Indices are 1-based in Lua-facing APIs.
- `confirm` and `input` are asynchronous and invoke callbacks.
- Orion ships a generated API index (`orion_api_index.json`) for IDE hover/completion and dictionary views.
