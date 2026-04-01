# Orion Lua API Reference

Orion extension scripts use the global `orion` namespace.

## Core

- `orion.log(message[, level])`
- `orion.alert(title, message)`
- `orion.getVersion()`
- `orion.getTheme()`

## Transport

- `orion.transport.play()`
- `orion.transport.stop()`
- `orion.transport.pause()`
- `orion.transport.record()`
- `orion.transport.panic()`
- `orion.transport.isPlaying()`
- `orion.transport.isPaused()`
- `orion.transport.isRecording()`
- `orion.transport.getPosition()` / `setPosition(seconds)`
- `orion.transport.getPositionBeats()` / `setPositionBeats(beats)`
- `orion.transport.isLooping()` / `setLooping(enabled)`
- `orion.transport.getLoopStart()` / `getLoopEnd()`
- `orion.transport.setLoopPoints(startSeconds, endSeconds)`

## Project

- `orion.project.getBpm()` / `setBpm(bpm)`
- `orion.project.getAuthor()` / `setAuthor(name)`
- `orion.project.getGenre()` / `setGenre(genre)`
- `orion.project.getKeySignature()` / `setKeySignature(signature)`
- `orion.project.getTimeSignature()` / `setTimeSignature(num, den)`
- `orion.project.getSampleRate()`
- `orion.project.clearAllTracks()`
- `orion.project.timeToBeats(seconds)`
- `orion.project.beatsToTime(beats)`

## Track/Clip/Mixer/Markers/UI

See:
- `assets/documentation/04 Extension System/APIReference.md`

## Notes

- Indices are 1-based.
- `orion.ui.confirm` and `orion.ui.input` are callback-based asynchronous APIs.
- IDE hover and dictionary content are generated from the bound C++ API index.
