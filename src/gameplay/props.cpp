#include "props.hpp"

#include <algorithm>
#include <cctype>

#include "raymath.h"
#include "utils.hpp"

namespace {

std::string Lower(std::string value) {
    for (char& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

bool IsPropModelPath(const std::string& path) {
    std::string lower = Lower(path);
    return lower.size() >= 4 &&
        (lower.rfind(".glb") == lower.size() - 4 ||
         lower.rfind(".obj") == lower.size() - 4 ||
         (lower.size() >= 5 && lower.rfind(".gltf") == lower.size() - 5));
}

std::string BaseName(const std::string& path) {
    std::size_t slash = path.find_last_of("/\\");
    std::string file = slash == std::string::npos ? path : path.substr(slash + 1);
    std::size_t dot = file.find_last_of('.');
    return dot == std::string::npos ? file : file.substr(0, dot);
}

} // namespace

std::vector<PropLibraryItem> ScanPropLibrary() {
    std::vector<PropLibraryItem> items;
    FilePathList files = LoadDirectoryFiles("resources/models/props");
    for (unsigned int i = 0; i < files.count; ++i) {
        std::string path = files.paths[i];
        if (!IsPropModelPath(path)) continue;
        PropLibraryItem item;
        item.path = path;
        item.name = BaseName(path);
        items.push_back(item);
    }
    UnloadDirectoryFiles(files);
    std::sort(items.begin(), items.end(), [](const PropLibraryItem& a, const PropLibraryItem& b) {
        return a.path < b.path;
    });
    return items;
}

void LoadRuntimeProps(std::vector<RuntimeProp>& props, const std::vector<LevelPropConfig>& configs, const FogShader& fogShader) {
    UnloadRuntimeProps(props);
    props.reserve(configs.size());
    for (const LevelPropConfig& config : configs) {
        RuntimeProp prop;
        prop.config = config;
        std::string resolved = Utils::ResolveProjectPath(config.modelPath);
        if (FileExists(resolved.c_str())) {
            prop.model = LoadModel(resolved.c_str());
            prop.loaded = prop.model.meshCount > 0;
            if (prop.loaded) ApplyFogShaderToModel(prop.model, fogShader);
        } else {
            TraceLog(LOG_WARNING, "Prop model missing: %s", config.modelPath.c_str());
        }
        props.push_back(prop);
    }
}

void UnloadRuntimeProps(std::vector<RuntimeProp>& props) {
    for (RuntimeProp& prop : props) {
        if (prop.loaded) UnloadModel(prop.model);
        prop.loaded = false;
    }
    props.clear();
}

void DrawRuntimeProps(const std::vector<RuntimeProp>& props) {
    for (const RuntimeProp& prop : props) {
        if (!prop.loaded) continue;
        Vector3 axis = {0.0f, 1.0f, 0.0f};
        float angle = 0.0f;
        QuaternionToAxisAngle(prop.config.rotation, &axis, &angle);
        DrawModelEx(prop.model, prop.config.position, axis, angle * RAD2DEG, prop.config.scale, WHITE);
    }
}

void ApplyFogShaderToRuntimeProps(std::vector<RuntimeProp>& props, const FogShader& fogShader) {
    for (RuntimeProp& prop : props) {
        if (prop.loaded) ApplyFogShaderToModel(prop.model, fogShader);
    }
}
