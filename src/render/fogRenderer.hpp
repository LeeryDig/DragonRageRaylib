#ifndef RENDER_FOG_RENDERER_HPP
#define RENDER_FOG_RENDERER_HPP

#include "raylib.h"

#include "level/levelRuntimeConfig.hpp"

struct FogShader {
    Shader shader;
    int cameraPositionLoc;
    int fogColorLoc;
    int fogStartLoc;
    int fogEndLoc;
    int fogDensityLoc;
    int fogEnabledLoc;

    FogShader()
        : shader(),
          cameraPositionLoc(-1),
          fogColorLoc(-1),
          fogStartLoc(-1),
          fogEndLoc(-1),
          fogDensityLoc(-1),
          fogEnabledLoc(-1) {}
};

bool LoadFogShader(FogShader& fogShader);
void UnloadFogShader(FogShader& fogShader);
void ApplyFogShaderToModel(Model& model, const FogShader& fogShader);
void UpdateFogShader(const FogShader& fogShader, const FogConfig& config, const Camera& camera);

#endif
