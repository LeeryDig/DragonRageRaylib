#ifndef RENDER_FOG_RENDERER_HPP
#define RENDER_FOG_RENDERER_HPP

#include "raylib.h"

#include "level/levelRuntimeConfig.hpp"

struct FogShader {
    static const int MaxDirectionalLights = 2;
    static const int MaxPointLights = 4;
    static const int MaxSpotLights = 2;

    Shader shader;
    int cameraPositionLoc;
    int fogColorLoc;
    int fogStartLoc;
    int fogEndLoc;
    int fogDensityLoc;
    int fogEnabledLoc;
    int ambientColorLoc;
    int directionalLightCountLoc;
    int directionalLightEnabledLoc[MaxDirectionalLights];
    int directionalLightDirectionLoc[MaxDirectionalLights];
    int directionalLightColorLoc[MaxDirectionalLights];
    int directionalLightIntensityLoc[MaxDirectionalLights];
    int pointLightCountLoc;
    int pointLightEnabledLoc[MaxPointLights];
    int pointLightPositionLoc[MaxPointLights];
    int pointLightColorLoc[MaxPointLights];
    int pointLightIntensityLoc[MaxPointLights];
    int pointLightRangeLoc[MaxPointLights];
    int spotLightCountLoc;
    int spotLightEnabledLoc[MaxSpotLights];
    int spotLightPositionLoc[MaxSpotLights];
    int spotLightDirectionLoc[MaxSpotLights];
    int spotLightColorLoc[MaxSpotLights];
    int spotLightIntensityLoc[MaxSpotLights];
    int spotLightRangeLoc[MaxSpotLights];
    int spotLightInnerConeLoc[MaxSpotLights];
    int spotLightOuterConeLoc[MaxSpotLights];

    FogShader();
};

bool LoadFogShader(FogShader& fogShader);
void UnloadFogShader(FogShader& fogShader);
void ApplyFogShaderToModel(Model& model, const FogShader& fogShader);
void UpdateFogShader(const FogShader& fogShader, const FogConfig& config, const Camera& camera);
void UpdateLightingShader(const FogShader& fogShader, const LightingConfig& config);

#endif
