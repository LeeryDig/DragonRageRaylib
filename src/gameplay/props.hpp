#ifndef GAMEPLAY_PROPS_HPP
#define GAMEPLAY_PROPS_HPP

#include <string>
#include <vector>

#include "raylib.h"
#include "level/levelRuntimeConfig.hpp"
#include "render/fogRenderer.hpp"

struct RuntimeProp {
    LevelPropConfig config;
    Model model;
    bool loaded;

    RuntimeProp() : config(), model(), loaded(false) {}
};

struct PropLibraryItem {
    std::string path;
    std::string name;
};

std::vector<PropLibraryItem> ScanPropLibrary();
void LoadRuntimeProps(std::vector<RuntimeProp>& props, const std::vector<LevelPropConfig>& configs, const FogShader& fogShader);
void UnloadRuntimeProps(std::vector<RuntimeProp>& props);
void DrawRuntimeProps(const std::vector<RuntimeProp>& props);
void ApplyFogShaderToRuntimeProps(std::vector<RuntimeProp>& props, const FogShader& fogShader);

#endif
