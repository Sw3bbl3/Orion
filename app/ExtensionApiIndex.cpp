#include "ExtensionApiIndex.h"

#include "orion/SettingsManager.h"
#include <algorithm>
#include <map>
#include <nlohmann/json.hpp>

namespace Orion {
namespace {

using CuratedMap = std::map<std::string, ExtensionApiEntry>;

ExtensionApiEntry makeEntry(const juce::String& symbol,
                            const juce::String& signature,
                            const juce::String& summary,
                            const juce::String& returns,
                            const juce::String& example,
                            std::initializer_list<ExtensionApiParam> params = {})
{
    ExtensionApiEntry e;
    e.symbol = symbol;
    e.signature = signature;
    e.summary = summary;
    e.returns = returns;
    e.example = example;
    e.params = params;
    return e;
}

CuratedMap& curated()
{
    static CuratedMap map;
    return map;
}

void addCurated(const ExtensionApiEntry& entry)
{
    curated()[entry.symbol.toStdString()] = entry;
}

juce::String prettyNameFromSymbol(const juce::String& symbol)
{
    auto parts = juce::StringArray::fromTokens(symbol, ".", "");
    if (parts.isEmpty())
        return symbol;

    juce::String fn = parts[parts.size() - 1];
    return fn + "(...)";
}

juce::String lowerKey(const juce::String& s)
{
    return s.trim().toLowerCase();
}

nlohmann::json toJson(const ExtensionApiEntry& entry)
{
    nlohmann::json j;
    j["symbol"] = entry.symbol.toStdString();
    j["signature"] = entry.signature.toStdString();
    j["summary"] = entry.summary.toStdString();
    j["returns"] = entry.returns.toStdString();
    j["example"] = entry.example.toStdString();
    j["fromLuaStdLib"] = entry.fromLuaStdLib;

    nlohmann::json params = nlohmann::json::array();
    for (const auto& p : entry.params)
    {
        params.push_back({
            {"name", p.name.toStdString()},
            {"type", p.type.toStdString()},
            {"description", p.description.toStdString()}
        });
    }
    j["params"] = params;
    return j;
}

} // namespace

ExtensionApiIndex& ExtensionApiIndex::getInstance()
{
    static ExtensionApiIndex instance;
    return instance;
}

void ExtensionApiIndex::ensureCuratedDefaults()
{
    if (curatedLoaded)
        return;

    curatedLoaded = true;

    addCurated(makeEntry(
        "orion.log",
        "orion.log(message[, level])",
        "Write a message to the extension output console.",
        "nil",
        "orion.log('Build complete', 3)",
        {
            {"message", "string", "Text to write."},
            {"level", "integer", "Optional level: 0 info, 1 warning, 2 error, 3 success."}
        }));

    addCurated(makeEntry(
        "orion.alert",
        "orion.alert(title, message)",
        "Show a native message dialog.",
        "nil",
        "orion.alert('Done', 'Render finished')",
        {
            {"title", "string", "Dialog title."},
            {"message", "string", "Dialog body."}
        }));

    addCurated(makeEntry(
        "orion.getVersion",
        "orion.getVersion()",
        "Return Orion's version string.",
        "string",
        "local v = orion.getVersion()"));

    addCurated(makeEntry(
        "orion.getTheme",
        "orion.getTheme()",
        "Return the current app theme name.",
        "string",
        "orion.log(orion.getTheme())"));

    addCurated(makeEntry(
        "orion.transport.play",
        "orion.transport.play()",
        "Start transport playback.",
        "nil",
        "orion.transport.play()"));

    addCurated(makeEntry(
        "orion.transport.stop",
        "orion.transport.stop()",
        "Stop transport playback/recording.",
        "nil",
        "orion.transport.stop()"));

    addCurated(makeEntry(
        "orion.transport.setPosition",
        "orion.transport.setPosition(seconds)",
        "Set transport position in seconds.",
        "nil",
        "orion.transport.setPosition(12.0)",
        {
            {"seconds", "number", "Absolute timeline time in seconds."}
        }));

    addCurated(makeEntry(
        "orion.transport.setPositionBeats",
        "orion.transport.setPositionBeats(beats)",
        "Set transport position in beats.",
        "nil",
        "orion.transport.setPositionBeats(16)",
        {
            {"beats", "number", "Absolute timeline beat position."}
        }));

    addCurated(makeEntry(
        "orion.project.getBpm",
        "orion.project.getBpm()",
        "Get project BPM.",
        "number",
        "local bpm = orion.project.getBpm()"));

    addCurated(makeEntry(
        "orion.project.setBpm",
        "orion.project.setBpm(bpm)",
        "Set project BPM.",
        "nil",
        "orion.project.setBpm(128)",
        {
            {"bpm", "number", "Tempo in beats per minute."}
        }));

    addCurated(makeEntry(
        "orion.track.getCount",
        "orion.track.getCount()",
        "Get total project track count.",
        "integer",
        "for i=1,orion.track.getCount() do end"));

    addCurated(makeEntry(
        "orion.track.createMidiClip",
        "orion.track.createMidiClip(trackIndex, name, startBeat, lengthBeat)",
        "Create a MIDI clip on an instrument track.",
        "nil",
        "orion.track.createMidiClip(1, 'Verse', 0, 16)",
        {
            {"trackIndex", "integer", "1-based track index."},
            {"name", "string", "Clip name."},
            {"startBeat", "number", "Clip start beat."},
            {"lengthBeat", "number", "Clip duration in beats."}
        }));

    addCurated(makeEntry(
        "orion.clip.addNote",
        "orion.clip.addNote(trackIndex, clipIndex, note, velocity, startBeat, durationBeat)",
        "Add one MIDI note to a clip.",
        "nil",
        "orion.clip.addNote(1, 1, 60, 0.9, 0, 1)",
        {
            {"trackIndex", "integer", "1-based track index."},
            {"clipIndex", "integer", "1-based clip index."},
            {"note", "integer", "MIDI note number 0-127."},
            {"velocity", "number", "Velocity 0.0-1.0."},
            {"startBeat", "number", "Beat offset in clip."},
            {"durationBeat", "number", "Duration in beats."}
        }));

    addCurated(makeEntry(
        "orion.mixer.setVolume",
        "orion.mixer.setVolume(trackIndex, volume)",
        "Set track output volume.",
        "nil",
        "orion.mixer.setVolume(2, 0.8)",
        {
            {"trackIndex", "integer", "1-based track index."},
            {"volume", "number", "Linear volume amount."}
        }));

    addCurated(makeEntry(
        "orion.ui.confirm",
        "orion.ui.confirm(title, message, callback)",
        "Show async Yes/No dialog and call callback(confirmed).",
        "nil",
        "orion.ui.confirm('Delete?', 'Delete clip?', function(ok) if ok then ... end end)",
        {
            {"title", "string", "Dialog title."},
            {"message", "string", "Dialog body."},
            {"callback", "function", "Receives boolean result."}
        }));

    addCurated(makeEntry(
        "orion.ui.input",
        "orion.ui.input(title, message, callback)",
        "Show async text input dialog and call callback(valueOrNil).",
        "nil",
        "orion.ui.input('Rename', 'New clip name:', function(v) if v then ... end end)",
        {
            {"title", "string", "Dialog title."},
            {"message", "string", "Dialog body."},
            {"callback", "function", "Receives string or nil."}
        }));

    auto addLuaStd = [](const juce::String& symbol,
                        const juce::String& signature,
                        const juce::String& summary,
                        const juce::String& returns,
                        const juce::String& example)
    {
        auto e = makeEntry(symbol, signature, summary, returns, example);
        e.fromLuaStdLib = true;
        addCurated(e);
    };

    addLuaStd("lua.print", "print(...)", "Write values to standard output.", "nil", "print('hello')");
    addLuaStd("lua.pairs", "pairs(table)", "Iterate key/value pairs of a table.", "iterator", "for k,v in pairs(t) do end");
    addLuaStd("lua.ipairs", "ipairs(table)", "Iterate integer-indexed entries in order.", "iterator", "for i,v in ipairs(arr) do end");
    addLuaStd("lua.type", "type(value)", "Return Lua type name.", "string", "if type(x) == 'table' then end");
    addLuaStd("lua.math.random", "math.random([m[, n]])", "Generate random numbers.", "number", "local r = math.random(1, 16)");
    addLuaStd("lua.string.find", "string.find(s, pattern[, init[, plain]])", "Find pattern match bounds.", "integer, integer", "local i, j = string.find(name, 'kick')");
    addLuaStd("lua.table.insert", "table.insert(list[, pos], value)", "Insert value into a table.", "nil", "table.insert(notes, 60)");
}

void ExtensionApiIndex::rebuildFromBindings(const std::vector<juce::String>& symbols)
{
    ensureCuratedDefaults();

    std::vector<juce::String> dedup = symbols;
    std::sort(dedup.begin(), dedup.end(), [](const juce::String& a, const juce::String& b) { return a.compareNatural(b) < 0; });
    dedup.erase(std::unique(dedup.begin(), dedup.end(), [](const juce::String& a, const juce::String& b) {
        return a.equalsIgnoreCase(b);
    }), dedup.end());

    entries.clear();
    entries.reserve(dedup.size() + 16);

    for (const auto& symbol : dedup)
    {
        auto it = curated().find(symbol.toStdString());
        if (it != curated().end())
        {
            entries.push_back(it->second);
        }
        else
        {
            ExtensionApiEntry entry;
            entry.symbol = symbol;
            entry.signature = prettyNameFromSymbol(symbol);
            entry.summary = "Auto-indexed Orion API symbol.";
            entry.returns = "varies";
            entry.example = "-- TODO: add example";
            entries.push_back(entry);
        }
    }

    for (const auto& [_, entry] : curated())
    {
        if (entry.fromLuaStdLib)
            entries.push_back(entry);
    }

    std::sort(entries.begin(), entries.end(), [](const ExtensionApiEntry& a, const ExtensionApiEntry& b) {
        return a.symbol.compareNatural(b.symbol) < 0;
    });

    writeArtifactToDisk();
}

const ExtensionApiEntry* ExtensionApiIndex::findEntry(const juce::String& symbol) const
{
    auto key = lowerKey(symbol);
    for (const auto& e : entries)
    {
        if (lowerKey(e.symbol) == key)
            return &e;
    }
    return nullptr;
}

const ExtensionApiEntry* ExtensionApiIndex::findBestMatch(const juce::String& tokenOrSymbol) const
{
    auto token = lowerKey(tokenOrSymbol);
    if (token.isEmpty())
        return nullptr;

    if (auto* exact = findEntry(token))
        return exact;

    for (const auto& e : entries)
    {
        auto candidate = lowerKey(e.symbol);
        if (candidate.endsWith("." + token) || candidate.endsWith(token))
            return &e;
    }

    return nullptr;
}

juce::String ExtensionApiIndex::toJsonString() const
{
    nlohmann::json out = nlohmann::json::array();
    for (const auto& e : entries)
        out.push_back(toJson(e));

    return out.dump(2);
}

juce::var ExtensionApiIndex::toVarArray() const
{
    juce::Array<juce::var> arr;

    for (const auto& e : entries)
    {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("symbol", e.symbol);
        obj->setProperty("signature", e.signature);
        obj->setProperty("summary", e.summary);
        obj->setProperty("returns", e.returns);
        obj->setProperty("example", e.example);
        obj->setProperty("fromLuaStdLib", e.fromLuaStdLib);

        juce::Array<juce::var> params;
        for (const auto& p : e.params)
        {
            juce::DynamicObject::Ptr pObj = new juce::DynamicObject();
            pObj->setProperty("name", p.name);
            pObj->setProperty("type", p.type);
            pObj->setProperty("description", p.description);
            params.add(juce::var(pObj.get()));
        }

        obj->setProperty("params", juce::var(params));
        arr.add(juce::var(obj.get()));
    }

    return juce::var(arr);
}

void ExtensionApiIndex::writeArtifactToDisk() const
{
    auto baseDir = juce::File(Orion::SettingsManager::getSettingsDirectory())
        .getChildFile("cache");

    if (!baseDir.exists())
        baseDir.createDirectory();

    const auto artifact = baseDir.getChildFile("orion_api_index.json");
    artifact.replaceWithText(toJsonString());
}

} // namespace Orion
