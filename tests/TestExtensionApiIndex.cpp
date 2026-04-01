#include "ExtensionApiIndex.h"
#include <cassert>
#include <iostream>

int main()
{
    using namespace Orion;

    ExtensionApiIndex& index = ExtensionApiIndex::getInstance();
    index.rebuildFromBindings({
        "orion.log",
        "orion.transport.play",
        "orion.project.getBpm",
        "orion.track.getCount",
        "orion.clip.addNote",
        "orion.ui.confirm"
    });

    const auto* exact = index.findEntry("orion.transport.play");
    if (exact == nullptr || !exact->signature.contains("play"))
        return 1;

    const auto* token = index.findBestMatch("addNote");
    if (token == nullptr || token->symbol != "orion.clip.addNote")
        return 1;

    const auto* luaStd = index.findEntry("lua.table.insert");
    if (luaStd == nullptr || !luaStd->fromLuaStdLib)
        return 1;

    auto json = index.toJsonString();
    assert(json.contains("orion.transport.play"));

    std::cout << "TestExtensionApiIndex passed" << std::endl;
    return 0;
}
