#include "LuaLanguageServerManager.h"
#include <cassert>
#include <iostream>

int main()
{
    nlohmann::json payload = {
        {"jsonrpc", "2.0"},
        {"id", 7},
        {"method", "textDocument/hover"},
        {"params", {{"x", 1}}}
    };

    auto framed = Orion::LuaJsonRpcCodec::encode(payload);
    assert(framed.find("Content-Length:") == 0);

    std::string buffer = framed;
    nlohmann::json decoded;
    bool ok = Orion::LuaJsonRpcCodec::tryDecode(buffer, decoded);
    if (!ok || !buffer.empty()) return 1;
    if (decoded["id"].get<int>() != 7) return 1;
    if (decoded["method"].get<std::string>() != "textDocument/hover") return 1;

    std::cout << "TestLuaJsonRpc passed" << std::endl;
    return 0;
}
