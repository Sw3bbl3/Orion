#pragma once

#include <juce_core/juce_core.h>
#include <functional>
#include <map>
#include <nlohmann/json.hpp>

namespace Orion {

struct LuaDiagnostic {
    juce::String uri;
    int line = 0;
    int character = 0;
    int severity = 1;
    juce::String message;
    juce::String source;
};

class LuaJsonRpcCodec {
public:
    static std::string encode(const nlohmann::json& payload);
    static bool tryDecode(std::string& buffer, nlohmann::json& payloadOut);
};

class LuaLanguageServerManager : private juce::Thread
{
public:
    using ResponseCallback = std::function<void(const nlohmann::json&)>;

    LuaLanguageServerManager();
    ~LuaLanguageServerManager() override;

    bool start();
    void stop();

    void setWorkspace(const juce::File& workspaceDirectory);

    void didOpen(const juce::File& file, const juce::String& text);
    void didChange(const juce::File& file, const juce::String& text);
    void didSave(const juce::File& file);

    void requestHover(const juce::File& file, int line, int character, ResponseCallback callback);
    void requestCompletion(const juce::File& file, int line, int character, ResponseCallback callback);
    void requestSignatureHelp(const juce::File& file, int line, int character, ResponseCallback callback);
    void requestDefinition(const juce::File& file, int line, int character, ResponseCallback callback);

    std::function<void(const std::vector<LuaDiagnostic>&)> onDiagnostics;
    std::function<void(const juce::String&)> onServerLog;

private:
    void run() override;

    void sendNotification(const juce::String& method, const nlohmann::json& params);
    void sendRequest(const juce::String& method, const nlohmann::json& params, ResponseCallback callback);

    void sendInitialize();
    void handleMessage(const nlohmann::json& message);

    static juce::String fileToUri(const juce::File& file);
    static juce::String uriToFile(const juce::String& uri);
    static juce::String diagnosticsSummary(const std::vector<LuaDiagnostic>& diagnostics);

    juce::String resolveExecutablePath() const;

    juce::ChildProcess process;
    juce::CriticalSection processLock;
    juce::File workspaceDir;

    std::string inboundBuffer;
    int nextRequestId = 1;

    std::map<int, ResponseCallback> pendingCallbacks;
};

} // namespace Orion
