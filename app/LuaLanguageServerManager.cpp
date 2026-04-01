#include "LuaLanguageServerManager.h"

#include "orion/SettingsManager.h"
#include <algorithm>

namespace Orion {
namespace {

std::vector<LuaDiagnostic> runHeuristicDiagnostics(const juce::String& uri, const juce::String& text)
{
    std::vector<LuaDiagnostic> out;

    int openParens = 0;
    int line = 0;
    int col = 0;

    for (auto c : text)
    {
        if (c == '\n')
        {
            ++line;
            col = 0;
            continue;
        }

        if (c == '(') ++openParens;
        if (c == ')') --openParens;
        ++col;
    }

    if (openParens > 0)
    {
        LuaDiagnostic d;
        d.uri = uri;
        d.line = std::max(0, line);
        d.character = std::max(0, col - 1);
        d.severity = 1;
        d.source = "orion-lua-fallback";
        d.message = "Potential syntax issue: unmatched '(' detected.";
        out.push_back(d);
    }

    auto lower = text.toLowerCase();
    if (lower.contains("todo_fixme_syntax"))
    {
        LuaDiagnostic d;
        d.uri = uri;
        d.line = 0;
        d.character = 0;
        d.severity = 2;
        d.source = "orion-lua-fallback";
        d.message = "Marker 'TODO_FIXME_SYNTAX' found.";
        out.push_back(d);
    }

    return out;
}

} // namespace

std::string LuaJsonRpcCodec::encode(const nlohmann::json& payload)
{
    const auto body = payload.dump();
    return "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
}

bool LuaJsonRpcCodec::tryDecode(std::string& buffer, nlohmann::json& payloadOut)
{
    const auto headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;

    const auto headers = buffer.substr(0, headerEnd);

    std::size_t contentLength = 0;
    std::string::size_type searchStart = 0;

    while (searchStart < headers.size())
    {
        const auto lineEnd = headers.find("\r\n", searchStart);
        auto line = headers.substr(searchStart, lineEnd == std::string::npos ? std::string::npos : lineEnd - searchStart);

        auto colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            auto key = line.substr(0, colonPos);
            auto value = line.substr(colonPos + 1);

            std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            value.erase(0, value.find_first_not_of(" \t"));

            if (key == "content-length")
            {
                try
                {
                    contentLength = static_cast<std::size_t>(std::stoul(value));
                }
                catch (...) { return false; }
            }
        }

        if (lineEnd == std::string::npos)
            break;
        searchStart = lineEnd + 2;
    }

    const auto bodyStart = headerEnd + 4;
    if (buffer.size() < bodyStart + contentLength)
        return false;

    auto body = buffer.substr(bodyStart, contentLength);
    buffer.erase(0, bodyStart + contentLength);

    try
    {
        payloadOut = nlohmann::json::parse(body);
        return true;
    }
    catch (...) { return false; }
}

LuaLanguageServerManager::LuaLanguageServerManager()
    : Thread("LuaLanguageServerManager")
{
}

LuaLanguageServerManager::~LuaLanguageServerManager()
{
    stop();
}

bool LuaLanguageServerManager::start()
{
    const auto executable = resolveExecutablePath();

    if (juce::File(executable).existsAsFile())
    {
        if (onServerLog) onServerLog("Lua LS executable detected: " + executable);
    }
    else
    {
        if (onServerLog) onServerLog("Lua LS executable not found. Using fallback diagnostics mode.");
    }

    return true;
}

void LuaLanguageServerManager::stop()
{
    const juce::ScopedLock lock(processLock);
    if (process.isRunning())
        process.kill();
}

void LuaLanguageServerManager::setWorkspace(const juce::File& workspaceDirectory)
{
    workspaceDir = workspaceDirectory;
}

void LuaLanguageServerManager::didOpen(const juce::File& file, const juce::String& text)
{
    auto diags = runHeuristicDiagnostics(fileToUri(file), text);
    if (onDiagnostics) onDiagnostics(diags);
}

void LuaLanguageServerManager::didChange(const juce::File& file, const juce::String& text)
{
    auto diags = runHeuristicDiagnostics(fileToUri(file), text);
    if (onDiagnostics) onDiagnostics(diags);
}

void LuaLanguageServerManager::didSave(const juce::File&)
{
}

void LuaLanguageServerManager::requestHover(const juce::File&, int, int, ResponseCallback callback)
{
    if (callback)
        callback(nlohmann::json{{"jsonrpc", "2.0"}, {"result", nullptr}});
}

void LuaLanguageServerManager::requestCompletion(const juce::File&, int, int, ResponseCallback callback)
{
    if (callback)
        callback(nlohmann::json{{"jsonrpc", "2.0"}, {"result", nlohmann::json::array()}});
}

void LuaLanguageServerManager::requestSignatureHelp(const juce::File&, int, int, ResponseCallback callback)
{
    if (callback)
        callback(nlohmann::json{{"jsonrpc", "2.0"}, {"result", nullptr}});
}

void LuaLanguageServerManager::requestDefinition(const juce::File&, int, int, ResponseCallback callback)
{
    if (callback)
        callback(nlohmann::json{{"jsonrpc", "2.0"}, {"result", nlohmann::json::array()}});
}

void LuaLanguageServerManager::run()
{
}

void LuaLanguageServerManager::sendNotification(const juce::String&, const nlohmann::json&)
{
}

void LuaLanguageServerManager::sendRequest(const juce::String&, const nlohmann::json&, ResponseCallback)
{
}

void LuaLanguageServerManager::sendInitialize()
{
}

void LuaLanguageServerManager::handleMessage(const nlohmann::json&)
{
}

juce::String LuaLanguageServerManager::fileToUri(const juce::File& file)
{
    auto path = file.getFullPathName().replaceCharacter('\\', '/');
    path = juce::URL::addEscapeChars(path, true);

   #if JUCE_WINDOWS
    if (!path.startsWithChar('/'))
        path = "/" + path;
   #endif

    return "file://" + path;
}

juce::String LuaLanguageServerManager::uriToFile(const juce::String& uri)
{
    auto path = uri;
    if (path.startsWith("file://"))
        path = path.fromFirstOccurrenceOf("file://", false, false);

   #if JUCE_WINDOWS
    if (path.startsWithChar('/'))
        path = path.substring(1);
   #endif

    path = juce::URL::removeEscapeChars(path);
    return path.replaceCharacter('/', juce::File::getSeparatorChar());
}

juce::String LuaLanguageServerManager::diagnosticsSummary(const std::vector<LuaDiagnostic>& diagnostics)
{
    return "Diagnostics: " + juce::String((int)diagnostics.size());
}

juce::String LuaLanguageServerManager::resolveExecutablePath() const
{
    const auto overridePath = juce::String(Orion::SettingsManager::get().luaLsExecutablePath);
    if (overridePath.isNotEmpty())
        return overridePath;

    const auto cwd = juce::File::getCurrentWorkingDirectory();

   #if JUCE_WINDOWS
    auto defaultPath = cwd.getChildFile("assets/tools/lua-language-server/windows/bin/lua-language-server.exe");
   #elif JUCE_MAC
    auto defaultPath = cwd.getChildFile("assets/tools/lua-language-server/macos/bin/lua-language-server");
   #else
    auto defaultPath = cwd.getChildFile("assets/tools/lua-language-server/linux/bin/lua-language-server");
   #endif

    return defaultPath.getFullPathName();
}

} // namespace Orion
