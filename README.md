# <img src="assets/orion/Orion_logo_transpant_bg.png" width="48" height="48" valign="middle"> Orion DAW

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![CMake](https://img.shields.io/badge/CMake-3.22%2B-green.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-GPL/Commercial-orange.svg)](https://juce.com/license)
[![Maintenance](https://img.shields.io/badge/Maintained%3F-yes-green.svg)](https://github.com/WayV/Orion/graphs/commit-activity)

Orion is a modern, open-source Digital Audio Workstation (DAW) designed for performance, flexibility, and extensibility. Built in C++ with a modular architecture, Orion combines real-time audio processing, plugin hosting, and scripting into a unified creative environment.

This project is fully open-source and can be modified and extended by anyone.

---

## ✨ Features

- 🚀 **Real-time Audio Engine:** High-performance, node-based architecture for low-latency processing.
- 🎹 **MIDI & Instruments:** Full MIDI support with clips, instrument tracks, and a built-in piano roll.
- 🔌 **VST3 Hosting:** Robust support for third-party VST3 plugins via the Steinberg SDK.
- 🛠 **Built-in Effects:**
  - EQ, Reverb, Compressor, Limiter, Delay.
  - Creative processors: GlitchProcessor, FluxShaper, PrismStack.
- 🎹 **Synthesizer & Sampler:** Native support for sound synthesis and sample playback.
- 📜 **Lua Scripting:** Extensible automation and tool creation using embedded Lua 5.4.
- 📈 **Automation System:** Precise control over parameters via dedicated automation lanes.
- 🧵 **Multi-threaded:** Optimized processing using a custom thread pool for high-performance DSP.
- ⏳ **Time-stretching:** Advanced capabilities for audio manipulation.
- 💾 **Project Management:** Full serialization system for saving and loading complex projects.
- 📤 **Export Options:** High-quality WAV export with optional MP3 support (requires LAME).

---

## 🏗 Architecture Overview

Orion is built around a modular, pull-based audio graph system designed for sample-accurate processing.

- **AudioNode:** The fundamental processing unit.
- **AudioTrack & InstrumentTrack:** High-level abstractions for managing audio and MIDI data.
- **EffectRackNode:** Specialized node for managing serial effect chains.
- **MasterNode:** The final summation stage of the audio graph.
- **PluginManager:** Central hub for scanning, loading, and managing external plugins.

Audio is processed in blocks, allowing for efficient real-time DSP operations while maintaining thread safety.

---

## 💻 Technology Stack

- **Core Engine:** C++17
- **Build System:** CMake 3.22+
- **Audio Framework:** [JUCE 8](https://juce.com/) (Audio engine, DSP, and UI)
- **Serialization:** [nlohmann/json](https://github.com/nlohmann/json)
- **Plugin API:** [VST3 SDK](https://github.com/steinbergmedia/vst3sdk)
- **Scripting:** [Lua 5.4](https://www.lua.org/)

---

## 🚀 Installation & Building

### 🛠 Prerequisites

- **CMake 3.22** or higher
- **C++17 compatible compiler**:
  - Windows: MSVC 2022
  - macOS/Linux: Clang 12+ or GCC 10+

### 📦 Quick Build

1. **Clone the repository:**
   ```bash
   git clone https://github.com/WayV/Orion.git
   cd Orion
   ```

2. **Build via CMake:**
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . --config Release
   ```

### 🪟 Windows Setup (Automated)

For Windows users, we provide a batch script for a streamlined experience:
- Double-click `build_windows.bat` to configure, build, and package Orion.
- It generates a portable ZIP and a branded installer (if Inno Setup is installed).

---

## 🎤 Optional MP3 Export

Orion supports MP3 export via **LAME**. To enable this:
1. Install LAME on your system.
2. Ensure `lame` is available in your system `PATH`.
3. Verify with `lame --version`.

If LAME is not detected during the CMake configuration, MP3 export will be disabled automatically.

---

## 📁 Project Structure

```text
Orion/
├── app/                # UI Layer (JUCE Components, Main Entry)
├── assets/             # Graphics, Icons, and Fonts
├── include/            # Core Engine Headers
├── src/                # Core Engine Implementation
│   ├── AudioEngine.cpp
│   ├── PluginManager.cpp
│   ├── VST3Node.cpp
│   ├── ...             # DSP Nodes (EQ, Reverb, etc.)
├── tests/              # Unit tests and Benchmarks
└── CMakeLists.txt      # Build configuration
```

---

## 🤝 Contributing

Contributions are what make the open-source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## Community
Join the discussion: https://github.com/WayV-Inc/Orion/discussions

---

## 🔮 Vision & Roadmap

Orion aims to become a next-generation open ecosystem for audio tools, providing a programmable environment for modern music production.

- [ ] Graphical user interface polish and skinning support
- [ ] Real-time collaboration features
- [ ] AI-assisted production tools
- [ ] Cloud synchronization for projects
- [ ] Plugin sandboxing for improved stability
- [ ] Low-latency engine optimizations

---

## ⚖️ License

Orion is released under the **GPLv3 License** (inherited from JUCE's open-source tier). Ensure compliance with third-party licenses for dependencies:
- **JUCE**: GPLv3 / Commercial
- **VST3 SDK**: Steinberg VST3 License
- **nlohmann/json**: MIT
- **Lua**: MIT

---

**Made with ❤️ by the Wavium Team at WayV**
