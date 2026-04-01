#include "orion/VST3Node.h"
#include "orion/AudioBlock.h"
#include "orion/HostContext.h"
#include "orion/Logger.h"
#include "orion/RingBuffer.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <cstring>
#include <mutex>
#include <filesystem>
#include <cmath>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstunits.h"

using namespace Steinberg;
using namespace Steinberg::Vst;
namespace fs = std::filesystem;

namespace Orion {


    static std::string getLastErrorString() {
#ifdef _WIN32
        DWORD error = GetLastError();
        if (error == 0) return "No error";
        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
        std::string message(messageBuffer, size);
        LocalFree(messageBuffer);

        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
            message.pop_back();
        }
        return message + " (Code: " + std::to_string(error) + ")";
#else
        const char* err = dlerror();
        return err ? std::string(err) : "Unknown error";
#endif
    }


    template <class T>
    inline tresult PLUGIN_API queryInterfaceImpl(T* obj, const TUID iid, void** outObj) {
        if (FUnknownPrivate::iidEqual(iid, T::iid) || FUnknownPrivate::iidEqual(iid, FUnknown::iid)) {
            obj->addRef();
            *outObj = obj;
            return kResultOk;
        }
        return kResultFalse;
    }


    class MemoryStream : public IBStream {
    public:
        std::vector<char> data;
        int64_t pos = 0;

        MemoryStream() {}
        MemoryStream(const std::vector<char>& d) : data(d) {}

        tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) override {
            if (FUnknownPrivate::iidEqual(_iid, IBStream::iid) || FUnknownPrivate::iidEqual(_iid, FUnknown::iid)) {
                addRef();
                *obj = this;
                return kResultOk;
            }
            *obj = nullptr;
            return kResultFalse;
        }
        uint32 PLUGIN_API addRef() override { return 1; }
        uint32 PLUGIN_API release() override { return 1; }

        tresult PLUGIN_API read(void* buffer, int32 numBytes, int32* numBytesRead) override {
            if (numBytes <= 0) {
                if (numBytesRead) *numBytesRead = 0;
                return kResultOk;
            }

            int64_t avail = (int64_t)data.size() - pos;
            if (avail < 0) avail = 0;
            int32 toRead = (numBytes > avail) ? (int32)avail : numBytes;
            if (toRead > 0) {
                std::memcpy(buffer, data.data() + pos, toRead);
                pos += toRead;
            }
            if (numBytesRead) *numBytesRead = toRead;

            // Some plugins read in chunks and expect kResultOk with the actual bytes read.
            return toRead > 0 ? kResultOk : kResultFalse;
        }

        tresult PLUGIN_API write(void* buffer, int32 numBytes, int32* numBytesWritten) override {
            if (pos + numBytes > (int64_t)data.size()) {
                data.resize(pos + numBytes);
            }
            std::memcpy(data.data() + pos, buffer, numBytes);
            pos += numBytes;
            if (numBytesWritten) *numBytesWritten = numBytes;
            return kResultOk;
        }

        tresult PLUGIN_API seek(int64 offset, int32 mode, int64* result) override {
            if (mode == IBStream::kIBSeekSet) pos = offset;
            else if (mode == IBStream::kIBSeekCur) pos += offset;
            else if (mode == IBStream::kIBSeekEnd) pos = (int64_t)data.size() + offset;

            if (pos < 0) pos = 0;

            if (result) *result = pos;
            return kResultOk;
        }

        tresult PLUGIN_API tell(int64* result) override {
            if (result) *result = pos;
            return kResultOk;
        }
    };


    class HostApplication : public IHostApplication {
    public:
        tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) override {
            if (FUnknownPrivate::iidEqual(_iid, IHostApplication::iid) || FUnknownPrivate::iidEqual(_iid, FUnknown::iid)) {
                addRef();
                *obj = this;
                return kResultOk;
            }
            *obj = nullptr;
            return kResultFalse;
        }
        uint32 PLUGIN_API addRef() override { return 1; }
        uint32 PLUGIN_API release() override { return 1; }

        tresult PLUGIN_API getName(String128 name) override {
            UString(name, 128).fromAscii("Orion", 128);
            return kResultOk;
        }
        tresult PLUGIN_API createInstance(TUID , TUID , void** ) override { return kResultFalse; }
    };


    class ComponentHandler : public IComponentHandler {
    public:
        tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) override {
            if (FUnknownPrivate::iidEqual(_iid, IComponentHandler::iid) || FUnknownPrivate::iidEqual(_iid, FUnknown::iid)) {
                addRef();
                *obj = this;
                return kResultOk;
            }
            *obj = nullptr;
            return kResultFalse;
        }
        uint32 PLUGIN_API addRef() override { return 1; }
        uint32 PLUGIN_API release() override { return 1; }

        tresult PLUGIN_API beginEdit(ParamID ) override { return kResultOk; }
        tresult PLUGIN_API performEdit(ParamID , ParamValue ) override { return kResultOk; }
        tresult PLUGIN_API endEdit(ParamID ) override { return kResultOk; }
        tresult PLUGIN_API restartComponent(int32 ) override { return kResultOk; }
    };


    class PlugFrame : public IPlugFrame {
    public:
#ifdef _WIN32
        HWND parentWindow = nullptr;
#else
        void* parentWindow = nullptr;
#endif
        VST3Node* owner = nullptr;

        tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) override {
            if (FUnknownPrivate::iidEqual(_iid, IPlugFrame::iid) || FUnknownPrivate::iidEqual(_iid, FUnknown::iid)) {
                addRef();
                *obj = this;
                return kResultOk;
            }
            *obj = nullptr;
            return kResultFalse;
        }
        uint32 PLUGIN_API addRef() override { return 1; }
        uint32 PLUGIN_API release() override { return 1; }

        tresult PLUGIN_API resizeView(IPlugView* view, ViewRect* newSize) override {
            if (view && newSize) {
                if (owner && owner->onSizeChanged) {
                    owner->onSizeChanged(newSize->right - newSize->left, newSize->bottom - newSize->top);
                }
                view->onSize(newSize);
            }
            return kResultOk;
        }
    };


    class EventList : public IEventList {
    public:
        std::vector<Event> events;

        EventList() {
            events.reserve(256);
        }

        tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) override {
            if (FUnknownPrivate::iidEqual(_iid, IEventList::iid) || FUnknownPrivate::iidEqual(_iid, FUnknown::iid)) {
                addRef();
                *obj = this;
                return kResultOk;
            }
            *obj = nullptr;
            return kResultFalse;
        }
        uint32 PLUGIN_API addRef() override { return 1; }
        uint32 PLUGIN_API release() override { return 1; }

        int32 PLUGIN_API getEventCount() override { return (int32)events.size(); }
        tresult PLUGIN_API getEvent(int32 index, Event& e) override {
            if (index < 0 || index >= (int32)events.size()) return kResultFalse;
            e = events[index];
            return kResultOk;
        }
        tresult PLUGIN_API addEvent(Event& e) override {
            if (events.size() < events.capacity()) {
                events.push_back(e);
                return kResultOk;
            }
            return kResultFalse;
        }
        void clear() { events.clear(); }
    };


    class ParamValueQueue : public IParamValueQueue {
    public:
        ParamID paramId;
        struct Point { int32 offset; double value; };
        std::vector<Point> points;

        ParamValueQueue(ParamID id = 0) : paramId(id) {
            points.reserve(32);
        }
        virtual ~ParamValueQueue() {}

        tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) override {
            if (FUnknownPrivate::iidEqual(_iid, IParamValueQueue::iid) || FUnknownPrivate::iidEqual(_iid, FUnknown::iid)) {
                addRef();
                *obj = this;
                return kResultOk;
            }
            *obj = nullptr;
            return kResultFalse;
        }
        uint32 PLUGIN_API addRef() override { return 1; }
        uint32 PLUGIN_API release() override { return 1; }

        ParamID PLUGIN_API getParameterId() override { return paramId; }
        int32 PLUGIN_API getPointCount() override { return (int32)points.size(); }
        tresult PLUGIN_API getPoint(int32 index, int32& sampleOffset, ParamValue& value) override {
            if (index < 0 || index >= (int32)points.size()) return kResultFalse;
            sampleOffset = points[index].offset;
            value = points[index].value;
            return kResultOk;
        }
        tresult PLUGIN_API addPoint(int32 sampleOffset, ParamValue value, int32& index) override {
            if (points.size() < points.capacity()) {
                points.push_back({sampleOffset, value});
                index = (int32)points.size() - 1;
                return kResultOk;
            }
            return kResultFalse;
        }
        void clear() { points.clear(); }
    };

    class ParameterChanges : public IParameterChanges {
    public:
        std::vector<std::unique_ptr<ParamValueQueue>> allQueues;
        std::vector<ParamValueQueue*> activeQueues;
        std::map<ParamID, ParamValueQueue*> idToQueue;

        ParameterChanges() {
            activeQueues.reserve(128);
        }

        tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) override {
            if (FUnknownPrivate::iidEqual(_iid, IParameterChanges::iid) || FUnknownPrivate::iidEqual(_iid, FUnknown::iid)) {
                addRef();
                *obj = this;
                return kResultOk;
            }
            *obj = nullptr;
            return kResultFalse;
        }
        uint32 PLUGIN_API addRef() override { return 1; }
        uint32 PLUGIN_API release() override { return 1; }

        int32 PLUGIN_API getParameterCount() override { return (int32)activeQueues.size(); }
        IParamValueQueue* PLUGIN_API getParameterData(int32 index) override {
            if (index < 0 || index >= (int32)activeQueues.size()) return nullptr;
            return activeQueues[index];
        }
        IParamValueQueue* PLUGIN_API addParameterData(const ParamID& id, int32& index) override {
            auto it = idToQueue.find(id);
            if (it != idToQueue.end()) {
                ParamValueQueue* q = it->second;
                for (size_t i = 0; i < activeQueues.size(); ++i) {
                    if (activeQueues[i] == q) {
                        index = (int32)i;
                        return q;
                    }
                }
                if (activeQueues.size() < activeQueues.capacity()) {
                    activeQueues.push_back(q);
                    index = (int32)activeQueues.size() - 1;
                    return q;
                }
            }
            return nullptr;
        }

        void preAllocate(const std::vector<ParamID>& ids) {
            allQueues.clear();
            idToQueue.clear();
            activeQueues.clear();
            for (auto id : ids) {
                auto q = std::make_unique<ParamValueQueue>(id);
                idToQueue[id] = q.get();
                allQueues.push_back(std::move(q));
            }
            activeQueues.reserve(allQueues.size());
        }

        void clear() {
            for (auto& q : allQueues) q->clear();
            activeQueues.clear();
        }
    };


    typedef IPluginFactory* (PLUGIN_API *GetFactoryProc) ();

    struct VST3Node::Impl {
        VST3Node* owner = nullptr;
        std::string path;
        std::string uidString;

#ifdef _WIN32
        HMODULE libHandle = nullptr;
#else
        void* libHandle = nullptr;
#endif


        IPtr<IComponent> component;
        IPtr<IEditController> controller;
        IPtr<IMidiMapping> midiMapping;
        IPtr<IAudioProcessor> processor;
        IPtr<IPlugView> view;
        IPtr<IConnectionPoint> componentConnection;
        IPtr<IConnectionPoint> controllerConnection;


        HostApplication hostApp;
        ComponentHandler componentHandler;
        PlugFrame plugFrame;


        EventList inputEvents;
        ParameterChanges inputParamChanges;
        ParameterChanges outputParamChanges;
        Orion::RingBuffer<Event> eventQueue{8192};

        struct MidiParamChange {
            ParamID id;
            double value;
            int32 sampleOffset;
        };
        Orion::RingBuffer<MidiParamChange> paramQueue{8192};
        Orion::RingBuffer<MidiParamChange> uiParamQueue{8192};

        std::mutex pushMutex;
        ProcessContext processContext;

        std::vector<float*> inputChannelPtrs;
        std::vector<float*> outputChannelPtrs;
        std::vector<float> silenceBuffer;
        std::vector<float> trashBuffer;


        struct ParamInfo {
            ParamID id;
            std::string name;
            float value;
            ParameterRange range;
        };
        std::vector<ParamInfo> parameters;

        bool isInitialized = false;
        bool isProcessing = false;
        bool loggedProcessDetails = false;
        bool isInstrument = false;

        Impl() {}
        ~Impl() {
            unload();
        }

        void unload() {
            if (view) {

                view->removed();
                view = nullptr;
            }

            if (componentConnection && controllerConnection) {
                componentConnection->disconnect(controllerConnection);
                controllerConnection->disconnect(componentConnection);
            }
            componentConnection = nullptr;
            controllerConnection = nullptr;

            if (processor && isProcessing) {
                processor->setProcessing(false);
                isProcessing = false;
            }

            if (component) {
                component->setActive(false);
            }

            if (controller) {
                controller->terminate();
                controller = nullptr;
            }

            if (component) {
                component->terminate();
                component = nullptr;
            }

            processor = nullptr;

#ifdef _WIN32
            if (libHandle) { FreeLibrary(libHandle); libHandle = nullptr; }
#else
            if (libHandle) { dlclose(libHandle); libHandle = nullptr; }
#endif
            isInitialized = false;
        }

        bool load(const std::string& dllPath, const std::string& cidStr) {
            path = dllPath;
            uidString = cidStr;
            Logger::info("VST3 Loading: " + path);


#ifdef _WIN32
            libHandle = LoadLibraryA(path.c_str());
            if (!libHandle) {
                Logger::error("VST3 LoadLibrary failed: " + getLastErrorString());
                return false;
            }
            auto getFactory = (GetFactoryProc)GetProcAddress(libHandle, "GetPluginFactory");
#else
            libHandle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
            if (!libHandle) {
                Logger::error("VST3 dlopen failed: " + getLastErrorString());
                return false;
            }
            auto getFactory = (GetFactoryProc)dlsym(libHandle, "GetPluginFactory");
#endif
            if (!getFactory) {
                Logger::error("VST3: Failed to get factory for " + path);
                return false;
            }

            IPluginFactory* factory = getFactory();
            if (!factory) {
                Logger::error("VST3: Factory function returned null.");
                return false;
            }


            TUID uid;
            FUID fuid;
            fuid.fromString(cidStr.c_str());
            fuid.toTUID(uid);


            int numClasses = factory->countClasses();
            for (int i = 0; i < numClasses; ++i) {
                PClassInfo info;
                if (factory->getClassInfo(i, &info) == kResultOk) {
                     if (std::memcmp(info.cid, uid, sizeof(TUID)) == 0) {



                         PClassInfo2 info2;
                         IPluginFactory2* factory2 = nullptr;
                         if (factory->queryInterface(IPluginFactory2::iid, (void**)&factory2) == kResultOk) {
                             if (factory2->getClassInfo2(i, &info2) == kResultOk) {
                                 std::string subCats = info2.subCategories;
                                 if (subCats.find("Instrument") != std::string::npos) {
                                     isInstrument = true;
                                 }
                             }
                             factory2->release();
                         }
                         break;
                     }
                }
            }

            if (factory->createInstance(uid, IComponent::iid, (void**)&component) != kResultOk) {
                Logger::error("VST3: Failed to create component instance for CID: " + cidStr);
                return false;
            }


            if (component->initialize(&hostApp) != kResultOk) {
                Logger::error("VST3: Failed to initialize component.");
                return false;
            }


            bool createdController = false;
            if (component->queryInterface(IEditController::iid, (void**)&controller) != kResultOk) {
                TUID controllerCID;
                if (component->getControllerClassId(controllerCID) == kResultOk) {
                    if (factory->createInstance(controllerCID, IEditController::iid, (void**)&controller) == kResultOk) {
                        createdController = true;
                        Logger::info("VST3: Created separate Controller instance.");
                    }
                }

                if (!controller) {
                    Logger::info("VST3: Separate Controller interface not found (might be Single Component).");
                }
            }


            if (controller) {
                if (controller->initialize(&hostApp) != kResultOk) {
                     Logger::error("VST3: Controller init failed.");
                } else {
                     Logger::info("VST3: Controller initialized.");
                }

                if (createdController) {
                    IConnectionPoint* componentCP = nullptr;
                    if (component->queryInterface(IConnectionPoint::iid, (void**)&componentCP) == kResultOk) {
                        componentConnection = componentCP;
                        componentCP->release();
                    }

                    IConnectionPoint* controllerCP = nullptr;
                    if (controller->queryInterface(IConnectionPoint::iid, (void**)&controllerCP) == kResultOk) {
                        controllerConnection = controllerCP;
                        controllerCP->release();
                    }

                    if (componentConnection && controllerConnection) {
                        componentConnection->connect(controllerConnection);
                        controllerConnection->connect(componentConnection);
                    }
                }

                controller->setComponentHandler(&componentHandler);


                controller->queryInterface(IMidiMapping::iid, (void**)&midiMapping);
            }


            if (component->queryInterface(IAudioProcessor::iid, (void**)&processor) != kResultOk) {
                Logger::error("VST3: No AudioProcessor interface found.");
                return false;
            }

            if (processor->canProcessSampleSize(kSample32) != kResultOk) {
                Logger::error("VST3: Plugin does not support 32-bit float processing.");
                return false;
            }

            isInitialized = true;
            Logger::info("VST3: Successfully loaded and initialized.");
            scanParameters();


            setupProcessing(2, 512, 44100.0);

            return true;
        }

        void scanParameters() {
            parameters.clear();
            if (!controller) return;

            std::vector<ParamID> ids;
            int count = controller->getParameterCount();
            for (int i = 0; i < count; ++i) {
                ParameterInfo info = {};
                if (controller->getParameterInfo(i, info) == kResultOk) {
                    if (info.flags & ParameterInfo::kIsHidden) continue;

                    ParamInfo pi;
                    pi.id = info.id;
                    ids.push_back(info.id);

                    char buf[256];
                    UString(info.title, 128).toAscii(buf, 256);
                    pi.name = buf;

                    pi.value = controller->getParamNormalized(info.id);
                    pi.range = {0.0f, 1.0f, 0.01f};

                    parameters.push_back(pi);
                }
            }
            inputParamChanges.preAllocate(ids);
            outputParamChanges.preAllocate(ids);
        }

        void setupProcessing(int numChannels, int blockSz, double rate) {
            if (!component || !processor) return;
            std::lock_guard<std::mutex> lock(pushMutex);

            (void)numChannels;
            loggedProcessDetails = false;


            if (isProcessing) {
                processor->setProcessing(false);
                component->setActive(false);
            }

            ProcessSetup setup = {};
            setup.processMode = kRealtime;
            setup.symbolicSampleSize = kSample32;
            setup.maxSamplesPerBlock = blockSz;
            setup.sampleRate = rate;

            processor->setupProcessing(setup);



            int32_t pluginInCh = 0;
            int32_t pluginOutCh = 0;

            if (component->getBusCount(kAudio, kInput) > 0) {
                component->activateBus(kAudio, kInput, 0, true);
                BusInfo info;
                if (component->getBusInfo(kAudio, kInput, 0, info) == kResultOk) pluginInCh = info.channelCount;
            }
            if (component->getBusCount(kAudio, kOutput) > 0) {
                component->activateBus(kAudio, kOutput, 0, true);
                BusInfo info;
                if (component->getBusInfo(kAudio, kOutput, 0, info) == kResultOk) pluginOutCh = info.channelCount;
            }

            int32 eventBusCount = component->getBusCount(kEvent, kInput);
            for (int32 i = 0; i < eventBusCount; ++i) {
                component->activateBus(kEvent, kInput, i, true);
            }

            Logger::info("VST3 SetupProcessing: Rate=" + std::to_string(rate) + ", Block=" + std::to_string(blockSz));
            Logger::info("VST3 Active Buses: InputCh=" + std::to_string(pluginInCh) + ", OutputCh=" + std::to_string(pluginOutCh));


            if (silenceBuffer.size() < (size_t)blockSz) silenceBuffer.assign(blockSz, 0.0f);
            if (trashBuffer.size() < (size_t)blockSz) trashBuffer.resize(blockSz);
            if (inputChannelPtrs.size() < (size_t)pluginInCh) inputChannelPtrs.resize(pluginInCh);
            if (outputChannelPtrs.size() < (size_t)pluginOutCh) outputChannelPtrs.resize(pluginOutCh);

            component->setActive(true);
            processor->setProcessing(true);
            isProcessing = true;
        }
    };

    VST3Node::VST3Node(const std::string& path, const std::string& cidString) {
        impl = new Impl();
        impl->owner = this;
        impl->plugFrame.owner = this;
        if (!impl->load(path, cidString)) {

        }
    }

    VST3Node::~VST3Node() {
        delete impl;
    }

    bool VST3Node::isValid() const {
        return impl && impl->isInitialized;
    }

    std::string VST3Node::getName() const {
        if (!impl) return "Invalid VST3";
        return fs::path(impl->path).stem().string();
    }

    bool VST3Node::isExternal() const {
        return true;
    }

    void VST3Node::setFormat(size_t newChannels, size_t newBlockSize) {
        if (newChannels == this->channels && newBlockSize == this->blockSize) return;
        AudioNode::setFormat(newChannels, newBlockSize);
        if (impl && impl->isInitialized) {
             impl->setupProcessing((int)newChannels, (int)newBlockSize, getSampleRate());
        }
    }

    int VST3Node::getLatency() const {
        if (impl && impl->processor) {
            return (int)impl->processor->getLatencySamples();
        }
        return 0;
    }

    bool VST3Node::isInstrument() const {
        return impl && impl->isInstrument;
    }

    bool VST3Node::getEditorSize(int& width, int& height) const {
        if (!impl || !impl->controller) return false;

        if (!impl->view) {
            impl->view = impl->controller->createView(ViewType::kEditor);
        }

        if (impl->view) {
            ViewRect size;
            if (impl->view->getSize(&size) == kResultOk) {
                width = size.right - size.left;
                height = size.bottom - size.top;
                return true;
            }
        }
        return false;
    }

    void VST3Node::openEditor(void* parentWindow) {
        if (!impl || !impl->controller) return;

        if (!impl->view) {
            impl->view = impl->controller->createView(ViewType::kEditor);
        }

        if (!impl->view) return;

#ifdef _WIN32
        impl->plugFrame.parentWindow = (HWND)parentWindow;
        impl->view->setFrame(&impl->plugFrame);
        if (impl->view->attached(parentWindow, kPlatformTypeHWND) != kResultOk) {
            Logger::error("VST3: Failed to attach view.");
            impl->view = nullptr;
            return;
        }
#else
        impl->plugFrame.parentWindow = parentWindow;
        impl->view->setFrame(&impl->plugFrame);
        if (impl->view->attached(parentWindow, kPlatformTypeX11EmbedWindowID) != kResultOk) {
             Logger::error("VST3: Failed to attach view.");
             impl->view = nullptr;
             return;
        }
#endif


        ViewRect size;
        if (impl->view->getSize(&size) == kResultOk) {
            impl->plugFrame.resizeView(impl->view, &size);
        }
    }

    void VST3Node::closeEditor() {
        if (impl && impl->view) {
            impl->view->removed();
            impl->view = nullptr;
        }
    }

    void VST3Node::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
        (void)cacheKey;
        (void)frameStart;
        if (!impl || !impl->processor || !impl->isProcessing) return;
        int numChannels = (int)block.getChannelCount();
        int32_t numSamples = (int32_t)block.getSampleCount();

        if (impl->silenceBuffer.size() >= (size_t)numSamples) {
             std::memset(impl->silenceBuffer.data(), 0, numSamples * sizeof(float));
        }



        BusInfo inInfo = {};
        BusInfo outInfo = {};
        int32_t pluginInCh = 0;
        int32_t pluginOutCh = 0;

        if (impl->component->getBusInfo(kAudio, kInput, 0, inInfo) == kResultOk) pluginInCh = inInfo.channelCount;
        if (impl->component->getBusInfo(kAudio, kOutput, 0, outInfo) == kResultOk) pluginOutCh = outInfo.channelCount;


        if (impl->inputChannelPtrs.size() < (size_t)pluginInCh) return;
        if (impl->outputChannelPtrs.size() < (size_t)pluginOutCh) return;


        for (int i=0; i<pluginInCh; ++i) {
            if (i < numChannels) impl->inputChannelPtrs[i] = block.getChannelPointer(i);
            else impl->inputChannelPtrs[i] = impl->silenceBuffer.data();
        }

        for (int i=0; i<pluginOutCh; ++i) {
             if (i < numChannels) impl->outputChannelPtrs[i] = block.getChannelPointer(i);
             else impl->outputChannelPtrs[i] = impl->trashBuffer.data();
        }

        if (!impl->loggedProcessDetails) {
            Logger::info("VST3 First Process Block:");
            Logger::info("  Host: Channels=" + std::to_string(numChannels) + ", Samples=" + std::to_string(numSamples));
            Logger::info("  Plugin: InCh=" + std::to_string(pluginInCh) + ", OutCh=" + std::to_string(pluginOutCh));
            impl->loggedProcessDetails = true;
        }


        ProcessData data = {};
        data.processMode = kRealtime;
        data.symbolicSampleSize = kSample32;
        data.numSamples = numSamples;

        AudioBusBuffers inBus = {};
        inBus.numChannels = pluginInCh;
        inBus.channelBuffers32 = impl->inputChannelPtrs.data();

        AudioBusBuffers outBus = {};
        outBus.numChannels = pluginOutCh;
        outBus.channelBuffers32 = impl->outputChannelPtrs.data();

        if (pluginInCh > 0) {
            data.numInputs = 1;
            data.inputs = &inBus;
        }
        if (pluginOutCh > 0) {
            data.numOutputs = 1;
            data.outputs = &outBus;
        }


        impl->processContext.sampleRate = (double)sampleRate.load();
        impl->processContext.projectTimeSamples = (int64)gHostContext.projectTimeSamples;
        impl->processContext.systemTime = (int64)(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        impl->processContext.tempo = gHostContext.bpm;
        impl->processContext.state = ProcessContext::kSystemTimeValid;

        if (gHostContext.playing) {
             impl->processContext.state |= ProcessContext::kPlaying;
        }


        double samplesPerBeat = 0.0;
        if (gHostContext.bpm > 0.0) {
            samplesPerBeat = (60.0 / gHostContext.bpm) * impl->processContext.sampleRate;
            impl->processContext.projectTimeMusic = (double)gHostContext.projectTimeSamples / samplesPerBeat;
            impl->processContext.state |= ProcessContext::kProjectTimeMusicValid | ProcessContext::kTempoValid;
        } else {
            impl->processContext.projectTimeMusic = 0.0;
        }


        impl->processContext.timeSigNumerator = (int32)gHostContext.timeSigNumerator;
        impl->processContext.timeSigDenominator = (int32)gHostContext.timeSigDenominator;
        impl->processContext.state |= ProcessContext::kTimeSigValid;


        if (gHostContext.bpm > 0.0 && gHostContext.timeSigDenominator > 0.0) {
            double beatValue = 4.0 / gHostContext.timeSigDenominator;
            double ppqPerBar = gHostContext.timeSigNumerator * beatValue;
            if (ppqPerBar > 0.0) {
                impl->processContext.barPositionMusic = std::floor(impl->processContext.projectTimeMusic / ppqPerBar) * ppqPerBar;
                impl->processContext.state |= ProcessContext::kBarPositionValid;
            }
        }


        if (gHostContext.loopActive) {
            impl->processContext.state |= ProcessContext::kCycleActive;
            if (samplesPerBeat > 0.0) {
                impl->processContext.cycleStartMusic = (double)gHostContext.loopStart / samplesPerBeat;
                impl->processContext.cycleEndMusic = (double)gHostContext.loopEnd / samplesPerBeat;
            }
        } else {
             impl->processContext.cycleStartMusic = 0.0;
             impl->processContext.cycleEndMusic = 0.0;
        }

        data.processContext = &impl->processContext;


        Event e;
        while (impl->eventQueue.pop(e)) {
            impl->inputEvents.addEvent(e);
        }

        if (impl->inputEvents.getEventCount() > 0) {
            data.inputEvents = &impl->inputEvents;
        }


        Impl::MidiParamChange pc;
        while(impl->paramQueue.pop(pc)) {
            int32 index = 0;
            auto* queue = impl->inputParamChanges.addParameterData(pc.id, index);
            if (queue) {
                int32 idx;
                queue->addPoint(pc.sampleOffset, pc.value, idx);
            }
        }


        while(impl->uiParamQueue.pop(pc)) {
            int32 index = 0;
            auto* queue = impl->inputParamChanges.addParameterData(pc.id, index);
            if (queue) {
                int32 idx;
                queue->addPoint(pc.sampleOffset, pc.value, idx);
            }
        }

        if (impl->inputParamChanges.getParameterCount() > 0) {
             data.inputParameterChanges = &impl->inputParamChanges;
        }

        data.outputParameterChanges = &impl->outputParamChanges;


        impl->processor->process(data);


        for (int32 i = 0; i < impl->outputParamChanges.getParameterCount(); ++i) {
            auto* queue = impl->outputParamChanges.activeQueues[i];
            if (queue && queue->getPointCount() > 0) {
                int32 sampleOffset;
                ParamValue value;
                if (queue->getPoint(queue->getPointCount() - 1, sampleOffset, value) == kResultOk) {
                    for (auto& pi : impl->parameters) {
                        if (pi.id == queue->paramId) {
                            pi.value = (float)value;
                            break;
                        }
                    }
                }
            }
        }

        impl->inputEvents.clear();
        impl->inputParamChanges.clear();
        impl->outputParamChanges.clear();
    }

    std::string VST3Node::getPath() const { return impl ? impl->path : ""; }
    std::string VST3Node::getCID() const { return impl ? impl->uidString : ""; }

    bool VST3Node::getChunk(std::vector<char>& data) {
        if (!impl || !impl->component) return false;

        try {
            MemoryStream compStream;
            if (impl->component->getState(&compStream) != kResultOk) {
                 return false;
            }

            MemoryStream ctrlStream;
            bool hasCtrl = false;
            if (impl->controller) {
                if (impl->controller->getState(&ctrlStream) == kResultOk) {
                    hasCtrl = true;
                }
            }



            const char header[] = "OrionVST3";
            int32_t version = 1;
            int32_t compSize = (int32_t)compStream.data.size();
            int32_t ctrlSize = hasCtrl ? (int32_t)ctrlStream.data.size() : 0;

            size_t totalSize = 10 + 4 + 4 + compSize + 4 + ctrlSize;
            data.resize(totalSize);

            char* ptr = data.data();
            std::memcpy(ptr, header, 10); ptr += 10;
            std::memcpy(ptr, &version, 4); ptr += 4;
            std::memcpy(ptr, &compSize, 4); ptr += 4;
            if (compSize > 0) {
                std::memcpy(ptr, compStream.data.data(), compSize); ptr += compSize;
            }
            std::memcpy(ptr, &ctrlSize, 4); ptr += 4;
            if (ctrlSize > 0) {
                std::memcpy(ptr, ctrlStream.data.data(), ctrlSize); ptr += ctrlSize;
            }

            return true;
        } catch (...) {
            Logger::error("VST3 getChunk: Exception during state retrieval.");
            return false;
        }
    }

    bool VST3Node::setChunk(const std::vector<char>& data) {
        if (!impl || !impl->component) {
            Logger::error("VST3 setChunk: Component not initialized.");
            return false;
        }
        if (data.empty()) {
            Logger::error("VST3 setChunk: Data is empty.");
            return false;
        }

        try {

            bool isNewFormat = false;
            if (data.size() >= 10) {
                if (std::memcmp(data.data(), "OrionVST3", 10) == 0) {
                    isNewFormat = true;
                }
            }

            std::vector<char> compData;
            std::vector<char> ctrlData;

            if (isNewFormat) {
                if (data.size() < 18) {
                    Logger::error("VST3 setChunk: Invalid size for OrionVST3 header (expected >= 18, got " + std::to_string(data.size()) + ")");
                    return false;
                }

                const char* ptr = data.data() + 10;
                int32_t version = 0;
                std::memcpy(&version, ptr, 4); ptr += 4;

                int32_t compSize = 0;
                std::memcpy(&compSize, ptr, 4); ptr += 4;

                if (compSize < 0) {
                    Logger::error("VST3 setChunk: Invalid component size (" + std::to_string(compSize) + ")");
                    return false;
                }
                size_t compSizeSz = (size_t)compSize;

                if (data.size() - 18 < compSizeSz) {
                    Logger::error("VST3 setChunk: Component data truncated (expected " + std::to_string(compSizeSz) + ", got " + std::to_string(data.size() - 18) + ")");
                    return false;
                }

                if (compSizeSz > 0) {
                    compData.assign(ptr, ptr + compSizeSz);
                    ptr += compSizeSz;
                }


                size_t bytesConsumed = 18 + compSizeSz;
                if (data.size() >= bytesConsumed + 4) {
                    int32_t ctrlSize = 0;
                    std::memcpy(&ctrlSize, ptr, 4); ptr += 4;

                    if (ctrlSize < 0) {
                        Logger::error("VST3 setChunk: Invalid controller size (" + std::to_string(ctrlSize) + ")");
                        return false;
                    }
                    size_t ctrlSizeSz = (size_t)ctrlSize;

                    if (data.size() - bytesConsumed - 4 < ctrlSizeSz) {
                        Logger::error("VST3 setChunk: Controller data truncated (expected " + std::to_string(ctrlSizeSz) + ", got " + std::to_string(data.size() - bytesConsumed - 4) + ")");
                        return false;
                    }

                    if (ctrlSizeSz > 0) {
                        ctrlData.assign(ptr, ptr + ctrlSizeSz);
                    }
                }
            } else {

                compData = data;
            }


            // Applying a chunk should be authoritative; drop pending UI param updates.
            Impl::MidiParamChange queuedChange;
            while (impl->uiParamQueue.pop(queuedChange)) {}

            bool wasActive = impl->isProcessing;
            if (wasActive) {
                impl->processor->setProcessing(false);
                impl->component->setActive(false);
            }

            bool result = false;
            if (!compData.empty()) {
                MemoryStream compStream(compData);
                if (impl->component->setState(&compStream) == kResultOk) {
                    result = true;

                    if (impl->controller) {
                        compStream.seek(0, IBStream::kIBSeekSet, nullptr);
                        if (impl->controller->setComponentState(&compStream) != kResultOk) {
                            Logger::error("VST3 setChunk: Failed to set controller component state.");

                        }
                    }
                } else {
                    Logger::error("VST3 setChunk: component->setState failed.");
                }
            } else {
                 Logger::error("VST3 setChunk: Component data empty.");
            }

            if (impl->controller && !ctrlData.empty()) {
                MemoryStream ctrlStream(ctrlData);
                if (impl->controller->setState(&ctrlStream) != kResultOk) {
                    Logger::error("VST3 setChunk: controller->setState failed.");
                }
            }

            if (wasActive) {
                impl->component->setActive(true);
                impl->processor->setProcessing(true);
            }

            return result;
        } catch (...) {
             Logger::error("VST3 setChunk: Exception during state restoration.");
             return false;
        }
    }

    void VST3Node::processMidi(int status, int data1, int data2, int sampleOffset) {
        if (!impl || !impl->isInitialized) return;

        int command = status & 0xF0;
        int channel = status & 0x0F;

        if (command == 0x90 || command == 0x80) {
            Event e = {};
            e.busIndex = 0;
            e.sampleOffset = sampleOffset;
            e.ppqPosition = impl->processContext.projectTimeMusic;
            e.flags = 0;

            bool isNoteOn = (command == 0x90 && data2 > 0);
            if (isNoteOn) {
                e.type = Event::kNoteOnEvent;
                e.noteOn.channel = (int16)channel;
                e.noteOn.pitch = (int16)data1;
                e.noteOn.tuning = 0.0f;
                e.noteOn.velocity = (float)data2 / 127.0f;
                e.noteOn.length = 0;
                e.noteOn.noteId = -1;
            } else {
                e.type = Event::kNoteOffEvent;
                e.noteOff.channel = (int16)channel;
                e.noteOff.pitch = (int16)data1;
                e.noteOff.velocity = (float)data2 / 127.0f;
                e.noteOff.noteId = -1;
                e.noteOff.tuning = 0.0f;
            }
            impl->eventQueue.push(e);
        }
        else if (command == 0xB0) {
            if (impl->midiMapping) {
                ParamID paramId;
                if (impl->midiMapping->getMidiControllerAssignment(0, channel, data1, paramId) == kResultOk) {
                    Impl::MidiParamChange pc;
                    pc.id = paramId;
                    pc.value = (double)data2 / 127.0;
                    pc.sampleOffset = sampleOffset;
                    impl->paramQueue.push(pc);
                }
            }
        }
    }

    int VST3Node::getParameterCount() const {
        return impl ? (int)impl->parameters.size() : 0;
    }

    float VST3Node::getParameter(int index) const {
        if (!impl || index < 0 || index >= (int)impl->parameters.size()) return 0.0f;
        return impl->parameters[index].value;
    }

    void VST3Node::setParameter(int index, float value) {
        if (!impl || index < 0 || index >= (int)impl->parameters.size()) return;


        impl->parameters[index].value = value;

        ParamID id = impl->parameters[index].id;


        if (impl->controller) {
            impl->controller->setParamNormalized(id, value);
        }


        Impl::MidiParamChange pc;
        pc.id = id;
        pc.value = value;
        pc.sampleOffset = 0;
        impl->uiParamQueue.push(pc);
    }

    std::string VST3Node::getParameterName(int index) const {
        if (!impl || index < 0 || index >= (int)impl->parameters.size()) return "";
        return impl->parameters[index].name;
    }

}
