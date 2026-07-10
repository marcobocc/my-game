#include "AssetLoader.hpp"
#include "scripting/LuaScriptSystem.hpp"

void AssetLoader::scanAndRegisterScripts(LuaScriptSystem& luaScriptSystem) {
    for (const auto& vfsPath: vfs_.listFilesRecursive()) {
        if (std::filesystem::path(vfsPath).extension() != ".lua") continue;
        auto* script = get<LuaScript>(vfsPath);
        if (!script) continue;
        luaScriptSystem.registerScript(script->path.stem().string(), script->path);
    }
}
