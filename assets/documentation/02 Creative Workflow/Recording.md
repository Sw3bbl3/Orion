# Recording Audio & MIDI

Recording in Orion is designed to be low-latency and high-fidelity. This guide covers how to capture your performances effectively.

## Setting Up Your Interface

Before recording, ensure your audio device is correctly configured in **Settings > Audio**.

1. Select your **Audio Device Type** (ASIO on Windows, CoreAudio on macOS).
2. Set your **Input** and **Output** channels.
3. Choose a **Buffer Size**. For recording, lower buffer sizes (e.g., 128 or 256 samples) are recommended to reduce latency.

---

## Recording Audio

1. **Create an Audio Track**: Right-click in the track header area and select "Add Audio Track".
2. **Arm for Recording**: Click the small **R** icon in the track header.
3. **Monitor**: Click the **Input Monitor** icon (speaker symbol) to hear yourself during recording.
4. **Record**: Press the Global Record button in the transport (or `R` on your keyboard).

---

## Recording MIDI

1. **Create an Instrument Track**: Right-click and select "Add Instrument Track".
2. **Select an Instrument**: Load a built-in instrument or a third-party extension.
3. **Connect MIDI**: Ensure your MIDI controller is enabled in **Settings > MIDI**.
4. **Record**: Just like audio, arm the track and hit record.

---

## Recording Modes

- **Standard**: Appends new data to the timeline.
- **Overdub**: Merges new MIDI notes into an existing clip.
- **Punch In/Out**: Automatically starts and stops recording at predefined points on the timeline.

> [!TIP]
> Use the **Count-in** feature (found in the Transport settings) to give yourself a few bars before recording starts.
