# Mixing & Signal Flow

Orion's mixer is built on a 64-bit floating-point engine, ensuring that your audio remains pristine throughout the signal chain.

## The Basic Signal Path

Understanding how audio travels through Orion is key to a great mix:

1. **Source**: Audio Clip or Instrument output.
2. **Gain Staging**: Initial gain adjustment in the Inspector.
3. **Inserts**: Serial processing (EQ, Compression, Distortion).
4. **Fader**: Final volume control for the track.
5. **Panning**: Positioning the sound in the stereo field.
6. **Master Bus**: The final destination for all tracks.

---

## Using Inserts

Inserts are effects applied directly to a track. Use them for "fixing" sounds or adding character.

- To add an insert, click an empty slot in the Mixer or Inspector.
- Drag and drop inserts to reorder them. The order matters!
- **Bypass**: Click the power icon on an insert to temporarily disable it.

---

## Sends & Aux Buses

Sends allow you to route a portion of a track's audio to an Auxiliary (Aux) bus. This is ideal for time-based effects like Reverb and Delay.

- **Parallel Processing**: Keeps the original sound dry while adding effect on top.
- **Efficiency**: Use one reverb plugin for multiple tracks.
- **Pre/Post Fader**: Control whether the send level is affected by the track's main fader.

---

## Monitoring & Levels

Keep an eye on the **Master Meter** to avoid clipping.

- **RMS vs Peak**: Use Peak meters for transients and RMS for perceived loudness.
- **Headroom**: Target around -6dB of headroom on the master bus before exporting for mastering.

> [!NOTE]
> Orion's internal engine will not clip, but your physical outputs will if the signal exceeds 0dB.
