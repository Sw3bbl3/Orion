# Orion Audio Engine (OAE) v1.2

The Orion Audio Engine (OAE) is the high-performance, modular core powering Orion. Built from the ground up in C++17, it delivers professional-grade audio processing with industry-leading stability and flexibility.

## 🚀 Key Capabilities

### High-Fidelity Audio
*   **64-bit Floating Point Mixing:** All internal processing occurs at 64-bit precision, ensuring zero loss of dynamic range or quality during summing.
*   **Sample Rate Support:** Native support for standard (44.1kHz, 48kHz) and high-resolution (88.2kHz, 96kHz, 176.4kHz, 192kHz) sample rates.
*   **Safety Limiter:** Integrated hard limiter on the master bus prevents digital clipping (> 0dBFS) to protect your ears and equipment.

### Modular Graph Architecture
Unlike traditional linear DAWs, OAE uses a **Pull-Based Directed Acyclic Graph (DAG)**.
*   **Dynamic Routing:** Tracks and effects are nodes in a graph, allowing for unlimited flexibility in signal flow.
*   **Lock-Free Processing:** The real-time audio thread is 100% lock-free, preventing audio dropouts (glitches) even under heavy UI load.
*   **Crash Protection:** Plugin processing is sandboxed where possible; a single bad plugin won't bring down the entire engine.

### Performance
*   **Multithreading:** OAE automatically distributes track processing across available CPU cores using a custom thread pool.
*   **SIMD Optimization:** DSP algorithms (EQ, Compression, Mixing) are optimized with SSE/AVX instructions for maximum efficiency.
*   **Smart Silence Detection:** The engine automatically suspends processing for silent tracks or plugins to save CPU resources.

### Plugin Hosting
*   **Formats:** Native support for **VST3** and **VST2**.
*   **Automatic Delay Compensation (ADC):** The engine automatically calculates and compensates for latency introduced by plugins, keeping all tracks perfectly phase-aligned.
*   **Sidechaining:** flexible sidechain routing for compressors and other dynamic effects.

## 🛠 Technical Specifications

| Feature | Specification |
| :--- | :--- |
| **Version** | 1.2 |
| **Internal Precision** | 64-bit Float (Double) |
| **Max Sample Rate** | 192 kHz |
| **Max Buffer Size** | 8192 Samples |
| **Channels per Track** | Stereo (Interleaved) |
| **Midi Resolution** | Sample-Accurate |
| **Thread Safety** | Lock-Free Real-time Path |
| **Offline Render** | Faster-than-realtime (WAV/MP3) |

## 🎛 Signal Flow

1.  **Input:** Audio clips or Synthesis (Instrument Tracks).
2.  **Insert Rack:** Serial processing of loaded effects.
3.  **Sends:** Parallel routing to Aux/Bus tracks.
4.  **Fader/Pan:** Volume and stereo positioning.
5.  **Summing:** All tracks are summed at the Master Bus.
6.  **Master FX:** Final processing (Limiter, Mastering chain).
7.  **Output:** Delivery to Audio Interface or File.
