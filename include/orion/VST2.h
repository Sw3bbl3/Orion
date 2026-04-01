#pragma once

#include <cstdint>

namespace Orion {
    namespace VST2 {


        using VstInt32 = int32_t;
        using VstIntPtr = intptr_t;

        struct AEffect;


        using audioMasterCallback = VstIntPtr (*)(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
        using AEffectDispatcherProc = VstIntPtr (*)(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
        using AEffectProcessProc = void (*)(AEffect* effect, float** inputs, float** outputs, VstInt32 sampleFrames);
        using AEffectSetParameterProc = void (*)(AEffect* effect, VstInt32 index, float parameter);
        using AEffectGetParameterProc = float (*)(AEffect* effect, VstInt32 index);


        const VstInt32 kEffectMagic = 0x56737450;

        struct ERect {
            int16_t top;
            int16_t left;
            int16_t bottom;
            int16_t right;
        };

        enum VstAEffectFlags {
            effFlagsHasEditor     = 1 << 0,
            effFlagsCanReplacing  = 1 << 4,
            effFlagsProgramChunks = 1 << 5,
            effFlagsIsSynth       = 1 << 8,
            effFlagsNoSoundInStop = 1 << 9,
            effFlagsCanDoubleReplacing = 1 << 12
        };


        struct AEffect {
            VstInt32 magic;
            AEffectDispatcherProc dispatcher;
            AEffectProcessProc process;
            AEffectSetParameterProc setParameter;
            AEffectGetParameterProc getParameter;

            VstInt32 numPrograms;
            VstInt32 numParams;
            VstInt32 numInputs;
            VstInt32 numOutputs;

            VstInt32 flags;

            VstIntPtr resvd1;
            VstIntPtr resvd2;

            VstInt32 initialDelay;

            VstInt32 realQualities;
            VstInt32 offQualities;
            float    ioRatio;

            void* object;
            void* user;

            VstInt32 uniqueID;
            VstInt32 version;

            AEffectProcessProc processReplacing;
            AEffectProcessProc processDoubleReplacing;

            char future[56];
        };


        enum AEffectOpcodes {
            effOpen = 0,
            effClose,
            effSetProgram,
            effGetProgram,
            effSetProgramName,
            effGetProgramName,
            effGetParamLabel,
            effGetParamDisplay,
            effGetParamName,
            effGetVu,
            effSetSampleRate,
            effSetBlockSize,
            effMainsChanged,
            effEditGetRect,
            effEditOpen,
            effEditClose,
            effEditIdle,
            effGetChunk,
            effSetChunk,
            effProcessEvents,
            effCanBeAutomated,
            effString2Parameter,
            effGetNumProgramCategories,
            effGetProgramNameIndexed,
            effCopyProgram,
            effConnectInput,
            effConnectOutput,
            effGetInputProperties,
            effGetOutputProperties,
            effGetPlugCategory,
            effGetCurrentPosition,
            effGetDestinationBuffer,
            effOfflineNotify,
            effFileLoad,
            effFileSave,
            effGetEffectName,

        };


        enum AudioMasterOpcodes {
            audioMasterAutomate = 0,
            audioMasterVersion,
            audioMasterCurrentId,
            audioMasterIdle,
            audioMasterPinConnected,

            audioMasterWantMidi = 6,
            audioMasterGetTime,
            audioMasterProcessEvents,
            audioMasterSetTime,
            audioMasterTempoAt,
            audioMasterGetNumAutomatableParameters,
            audioMasterGetParameterQuantization,
            audioMasterIOChanged,
            audioMasterNeedIdle,
            audioMasterSizeWindow,
            audioMasterGetSampleRate,
            audioMasterGetBlockSize,
            audioMasterGetInputLatency,
            audioMasterGetOutputLatency,
            audioMasterGetPreviousPlug,
            audioMasterGetNextPlug,
            audioMasterWillReplaceOrAccumulate,
            audioMasterGetCurrentProcessLevel,
            audioMasterGetAutomationState,

            audioMasterOfflineStart,
            audioMasterOfflineRead,
            audioMasterOfflineWrite,
            audioMasterOfflineGetCurrentPass,
            audioMasterOfflineGetCurrentMetaPass,

            audioMasterSetOutputSampleRate,
            audioMasterGetOutputSpeakerArrangement,
            audioMasterGetVendorString,
            audioMasterGetProductString,
            audioMasterGetVendorVersion,
            audioMasterVendorSpecific,
            audioMasterSetIcon,
            audioMasterCanDo,
            audioMasterGetLanguage,
            audioMasterOpenWindow,
            audioMasterCloseWindow,
            audioMasterGetDirectory,
            audioMasterUpdateDisplay,
            audioMasterBeginEdit,
            audioMasterEndEdit,
            audioMasterOpenFileSelector,
            audioMasterCloseFileSelector,
            audioMasterEditFile,
            audioMasterGetChunkFile,
            audioMasterGetItem,



































        };


        using VSTPluginMain = AEffect* (*)(audioMasterCallback audioMaster);



        const VstInt32 kVstMidiType = 1;

        struct VstEvent {
            VstInt32 type;
            VstInt32 byteSize;
            VstInt32 deltaFrames;
            VstInt32 flags;
            char data[16];
        };

        struct VstMidiEvent {
            VstInt32 type;
            VstInt32 byteSize;
            VstInt32 deltaFrames;
            VstInt32 flags;
            VstInt32 noteLength;
            VstInt32 noteOffset;
            char midiData[4];
            char detune;
            char noteOffVelocity;
            char reserved1;
            char reserved2;
        };

        struct VstEvents {
            VstInt32 numEvents;
            VstIntPtr reserved;
            VstEvent* events[2];
        };


        struct VstTimeInfo {
            double samplePos;
            double sampleRate;
            double nanoSeconds;
            double ppqPos;
            double tempo;
            double barStartPos;
            double cycleStartPos;
            double cycleEndPos;
            VstInt32 transportChanged;
            VstInt32 systemTimeNanoseconds;
            VstInt32 flags;
            VstInt32 timeSigNumerator;
            VstInt32 timeSigDenominator;
            VstInt32 smpteOffset;
        };

        enum VstTimeInfoFlags {
            kVstTransportChanged = 1,
            kVstTransportPlaying = 1 << 1,
            kVstTransportCycleActive = 1 << 2,
            kVstTransportRecording = 1 << 3,
            kVstAutomationWriting = 1 << 6,
            kVstAutomationReading = 1 << 7,
            kVstNanosValid = 1 << 8,
            kVstPpqPosValid = 1 << 9,
            kVstTempoValid = 1 << 10,
            kVstBarsValid = 1 << 11,
            kVstCyclePosValid = 1 << 12,
            kVstCycleValid = 1 << 12,
            kVstTimeSigValid = 1 << 13,
            kVstSmpteValid = 1 << 14,
            kVstClockValid = 1 << 15
        };

    }
}
