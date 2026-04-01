cmake_minimum_required(VERSION 3.22)

project(Orion VERSION 0.1.0 LANGUAGES CXX C)

# --- Configuration ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies directory
set(DEPS_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps)
file(MAKE_DIRECTORY ${DEPS_DIR})

include(FetchContent)

option(ORION_ENABLE_MP3_EXPORT "Enable MP3 export support via LAME." ON)

# 1. JUCE
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
)
FetchContent_MakeAvailable(juce)

# 2. JSON (nlohmann/json)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json
  GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# 3. VST3 SDK (Required for Plugin Hosting in OrionCore)
set(VST3_SDK_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(VST3_SDK_TESTS OFF CACHE BOOL "" FORCE)
set(VST3_SDK_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ADD_VST3_HOSTING_SAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ADD_VST3_PLUGINS_SAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
set(VST3_INSTALL_SDK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  vst3sdk
  GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
  GIT_TAG        v3.7.9_build_61
)
FetchContent_MakeAvailable(vst3sdk)

# 4. Lua (Scripting)
FetchContent_Declare(
  lua
  GIT_REPOSITORY https://github.com/lua/lua.git
  GIT_TAG        v5.4.4
)
FetchContent_GetProperties(lua)
if(NOT lua_POPULATED)
  FetchContent_Populate(lua)

  file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/*.c")
  # Exclude standalone interpreter and compiler
  list(FILTER LUA_SOURCES EXCLUDE REGEX "lua\\.c$")
  list(FILTER LUA_SOURCES EXCLUDE REGEX "luac\\.c$")
  # Exclude single-file build if present
  list(FILTER LUA_SOURCES EXCLUDE REGEX "onelua\\.c$")

  add_library(liblua STATIC ${LUA_SOURCES})
  target_include_directories(liblua PUBLIC "${lua_SOURCE_DIR}")

  if(UNIX)
    target_compile_definitions(liblua PUBLIC LUA_USE_LINUX)
    target_link_libraries(liblua PUBLIC m dl)
  endif()

  # Alias to match previous usage
  add_library(lua::liblua ALIAS liblua)
endif()

if(ORION_ENABLE_MP3_EXPORT)
    find_program(LAME_EXECUTABLE NAMES lame)
    if(NOT LAME_EXECUTABLE)
        message(WARNING "LAME not found in PATH; disabling MP3 export support.")
        set(ORION_ENABLE_MP3_EXPORT OFF)
    else()
        file(TO_CMAKE_PATH "${LAME_EXECUTABLE}" ORION_LAME_EXEC_PATH)
    endif()
endif()

# --- Compiler Warnings ---
if(MSVC)
    add_compile_options(/W4)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- Library: OrionCore ---
# Excludes AudioDeviceManager and miniaudio_impl as we use JUCE for audio IO now
add_library(OrionCore
    src/AudioEngine.cpp
    src/AudioNode.cpp
    src/AudioBlock.cpp
    src/OscillatorNode.cpp
    src/GainNode.cpp
    src/WavWriter.cpp
    src/AudioClip.cpp
    src/AudioFile.cpp
    src/AudioTrack.cpp
    src/Automation.cpp
    src/DelayNode.cpp
    src/InstrumentTrack.cpp
    src/SynthesizerNode.cpp
    src/SamplerNode.cpp
    src/MidiClip.cpp
    src/ProjectSerializer.cpp
    src/PluginManager.cpp
    src/VSTNode.cpp
    src/VST3Node.cpp
    src/MasterNode.cpp
    src/CommandHistory.cpp
    src/EQ3Node.cpp
    src/ReverbNode.cpp
    src/CompressorNode.cpp
    src/LimiterNode.cpp
    src/EnvironmentNode.cpp
    src/Logger.cpp
    src/ThreadPool.cpp
    src/SettingsManager.cpp
    src/TimeStretcher.cpp
    src/EffectRackNode.cpp
    src/GlitchProcessor.cpp
    src/FluxShaperNode.cpp
    src/PrismStackNode.cpp
    src/UxTelemetry.cpp
    src/UiThemeRegistry.cpp
)

target_link_libraries(OrionCore PUBLIC
    nlohmann_json::nlohmann_json
    sdk
    juce::juce_audio_formats
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
)

target_include_directories(OrionCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${vst3sdk_SOURCE_DIR}
)

if(ORION_ENABLE_MP3_EXPORT)
    target_compile_definitions(OrionCore PUBLIC JUCE_USE_MP3AUDIOFORMAT=1 ORION_LAME_EXECUTABLE=\"${ORION_LAME_EXEC_PATH}\")
else()
    target_compile_definitions(OrionCore PUBLIC JUCE_USE_MP3AUDIOFORMAT=1)
endif()

if(UNIX AND NOT APPLE)
    find_package(CURL REQUIRED)
    target_link_libraries(OrionCore PUBLIC pthread dl m CURL::libcurl)
endif()

# --- Application: Orion (JUCE) ---

# Define the JUCE Application
juce_add_gui_app(Orion
    PRODUCT_NAME "Orion"
    VERSION "0.1.0"
    COMPANY_NAME "WayV"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/orion/Orion_logo.png"
)

set_target_properties(Orion PROPERTIES
    OUTPUT_NAME "Orion"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Copy Assets to Build Directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION "${CMAKE_BINARY_DIR}")

# Add Sources (JUCE Style)
target_sources(Orion PRIVATE
    app/Main.cpp
    app/MainComponent.cpp
    app/QwertyMidi.cpp
    app/ProjectManager.cpp
    app/ExtensionManager.cpp
    app/ExtensionApiIndex.cpp
    app/LuaLanguageServerManager.cpp
    app/OrionLookAndFeel.cpp
    app/CommandManager.cpp
    app/ui/OrionDesignSystem.cpp
    app/ui/OrionUIPrimitives.cpp
    app/components/CustomTitleBar.cpp
    app/components/AboutComponent.cpp
    app/components/WindowWrapper.cpp
    app/components/NativeHostingComponent.cpp
    app/components/AdaptiveSpaceWindow.cpp
    app/components/TimelineComponent.cpp
    app/components/AutomationLaneComponent.cpp
    app/components/MarkerListComponent.cpp
    app/components/MixerComponent.cpp
    app/components/HeaderComponent.cpp
    app/components/StatusBarComponent.cpp
    app/components/BrowserComponent.cpp
    app/components/InspectorComponent.cpp
    app/components/EditorComponent.cpp
    app/components/ClipComponent.cpp
    app/components/DockablePanel.cpp
    app/components/ChannelStrip.cpp
    app/components/RulerComponent.cpp
    app/components/PluginListComponent.cpp
    app/components/InsertSlot.cpp
    app/components/SendSlot.cpp
    app/components/ChannelInspectorPopup.cpp
    app/components/MidiClipComponent.cpp
    app/components/PianoRollComponent.cpp
    app/components/SettingsWindow.cpp
    app/components/LoadingWindow.cpp
    app/components/DashboardComponent.cpp
    app/components/MarkdownViewer.cpp
    app/components/SplashScreen.cpp
    app/components/NewProjectDialog.cpp
    app/components/KeyWheelComponent.cpp
    app/components/ExtensionViewComponent.cpp
    app/components/ExtensionEditorComponent.cpp
    app/components/DocumentationViewComponent.cpp
    app/components/GuidedStartWizard.cpp
    app/components/CommandPaletteComponent.cpp
    app/components/InternalPluginEditor.cpp
    app/components/SamplerEditor.cpp
    app/components/SynthEditor.cpp
    app/components/FluxShaperEditor.cpp
    app/components/PrismStackEditor.cpp
    app/PluginWindowManager.cpp
)

# Link Libraries
target_compile_definitions(Orion PRIVATE JUCE_WEB_BROWSER=1)

target_link_libraries(Orion PRIVATE
    OrionCore
    lua::liblua
    juce::juce_audio_utils
    juce::juce_gui_extra
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_core
    juce::juce_events
    juce::juce_dsp
    juce::juce_recommended_config_flags
)

if (WIN32)
    target_link_libraries(Orion PRIVATE dwmapi)
endif ()

juce_generate_juce_header(Orion)

# Keep assets in sync for iterative development (especially IDE web assets).
add_custom_command(TARGET Orion POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${CMAKE_BINARY_DIR}/assets"
)

# Ensure include directories
target_include_directories(Orion PRIVATE
    app
    include
)

# Enable VST/VST3 Hosting in JUCE if we decide to switch OrionCore's backend to JUCE later.
# For now, OrionCore does its own VST hosting logic via VSTNode.cpp/VST3Node.cpp.
# But JUCE app needs to coexist with it.

# Tests
add_executable(TestBpmSync tests/TestBpmSync.cpp)
target_link_libraries(TestBpmSync PRIVATE OrionCore)

add_executable(TestTranspose tests/TestTranspose.cpp)
target_link_libraries(TestTranspose PRIVATE OrionCore)

add_executable(OscBenchmark tests/osc_benchmark.cpp)
target_link_libraries(OscBenchmark PRIVATE OrionCore)

add_executable(OscTest tests/osc_test.cpp)
target_link_libraries(OscTest PRIVATE OrionCore)

add_executable(SettingsMigrationTest tests/settings_migration_test.cpp)
target_link_libraries(SettingsMigrationTest PRIVATE OrionCore)

add_executable(TestUxState tests/TestUxState.cpp)
target_link_libraries(TestUxState PRIVATE OrionCore)

add_executable(TestGuidedStartCommand
    tests/TestGuidedStartCommand.cpp
    app/CommandManager.cpp
)
target_link_libraries(TestGuidedStartCommand PRIVATE
    OrionCore
    juce::juce_gui_basics
    juce::juce_core
)
target_include_directories(TestGuidedStartCommand PRIVATE
    app
    include
    ${CMAKE_CURRENT_BINARY_DIR}/Orion_artefacts/JuceLibraryCode
)

add_executable(TestExtensionApiIndex
    tests/TestExtensionApiIndex.cpp
    app/ExtensionApiIndex.cpp
)
target_link_libraries(TestExtensionApiIndex PRIVATE
    OrionCore
    juce::juce_core
)
target_include_directories(TestExtensionApiIndex PRIVATE
    app
    include
)

add_executable(TestLuaJsonRpc
    tests/TestLuaJsonRpc.cpp
    app/LuaLanguageServerManager.cpp
)
target_link_libraries(TestLuaJsonRpc PRIVATE
    OrionCore
    juce::juce_core
)
target_include_directories(TestLuaJsonRpc PRIVATE
    app
    include
)

if(BUILD_TESTING)
    add_executable(ExtensionTest
        tests/ExtensionTest.cpp
        app/ExtensionManager.cpp
        app/ExtensionApiIndex.cpp
        app/ProjectManager.cpp
    )
    target_link_libraries(ExtensionTest PRIVATE
        OrionCore
        lua::liblua
        juce::juce_events
        juce::juce_core
        juce::juce_data_structures
        juce::juce_gui_basics
        juce::juce_graphics
    )
    target_compile_definitions(ExtensionTest PRIVATE ORION_TEST_HEADLESS=1)
    target_include_directories(ExtensionTest PRIVATE
        app
        include
    )
endif()

# --- Install / Packaging ---
include(GNUInstallDirs)

if(WIN32)
    # Bundle the MSVC runtime so Orion runs on clean Windows systems.
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
endif()
include(InstallRequiredSystemLibraries)

install(TARGETS Orion
    COMPONENT OrionRuntime
    RUNTIME DESTINATION "."
    BUNDLE DESTINATION "."
)

if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
    install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
        COMPONENT OrionRuntime
        DESTINATION "."
    )
endif()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    COMPONENT OrionRuntime
    DESTINATION "."
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        COMPONENT OrionRuntime
        DESTINATION "."
    )
endif()
