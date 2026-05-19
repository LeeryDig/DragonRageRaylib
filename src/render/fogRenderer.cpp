#include "render/fogRenderer.hpp"

#include <cstdio>

#include "raymath.h"
#include "utils.hpp"

FogShader::FogShader()
    : shader(),
      cameraPositionLoc(-1), fogColorLoc(-1), fogStartLoc(-1), fogEndLoc(-1), fogDensityLoc(-1), fogEnabledLoc(-1), ambientColorLoc(-1),
      directionalLightCountLoc(-1), pointLightCountLoc(-1), spotLightCountLoc(-1) {
    for (int i = 0; i < MaxDirectionalLights; ++i) {
        directionalLightEnabledLoc[i] = directionalLightDirectionLoc[i] = directionalLightColorLoc[i] = directionalLightIntensityLoc[i] = -1;
    }
    for (int i = 0; i < MaxPointLights; ++i) {
        pointLightEnabledLoc[i] = pointLightPositionLoc[i] = pointLightColorLoc[i] = pointLightIntensityLoc[i] = pointLightRangeLoc[i] = -1;
    }
    for (int i = 0; i < MaxSpotLights; ++i) {
        spotLightEnabledLoc[i] = spotLightPositionLoc[i] = spotLightDirectionLoc[i] = spotLightColorLoc[i] = spotLightIntensityLoc[i] = spotLightRangeLoc[i] = spotLightInnerConeLoc[i] = spotLightOuterConeLoc[i] = -1;
    }
}

bool LoadFogShader(FogShader& fogShader) {
    fogShader.shader = LoadShader(
        Utils::ResolveProjectPath("resources/assets/shaders/fog_lit.vs").c_str(),
        Utils::ResolveProjectPath("resources/assets/shaders/fog_lit.fs").c_str());
    if (fogShader.shader.id == 0) {
        TraceLog(LOG_WARNING, "Fog shader: failed to load");
        return false;
    }

    fogShader.shader.locs[SHADER_LOC_VERTEX_POSITION] = GetShaderLocationAttrib(fogShader.shader, "vertexPosition");
    fogShader.shader.locs[SHADER_LOC_VERTEX_TEXCOORD01] = GetShaderLocationAttrib(fogShader.shader, "vertexTexCoord");
    fogShader.shader.locs[SHADER_LOC_VERTEX_COLOR] = GetShaderLocationAttrib(fogShader.shader, "vertexColor");
    fogShader.shader.locs[SHADER_LOC_VERTEX_NORMAL] = GetShaderLocationAttrib(fogShader.shader, "vertexNormal");
    fogShader.shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(fogShader.shader, "mvp");
    fogShader.shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(fogShader.shader, "matModel");
    fogShader.shader.locs[SHADER_LOC_MAP_DIFFUSE] = GetShaderLocation(fogShader.shader, "texture0");
    fogShader.shader.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(fogShader.shader, "colDiffuse");

    fogShader.cameraPositionLoc = GetShaderLocation(fogShader.shader, "cameraPosition");
    fogShader.fogColorLoc = GetShaderLocation(fogShader.shader, "fogColor");
    fogShader.fogStartLoc = GetShaderLocation(fogShader.shader, "fogStart");
    fogShader.fogEndLoc = GetShaderLocation(fogShader.shader, "fogEnd");
    fogShader.fogDensityLoc = GetShaderLocation(fogShader.shader, "fogDensity");
    fogShader.fogEnabledLoc = GetShaderLocation(fogShader.shader, "fogEnabled");
    fogShader.ambientColorLoc = GetShaderLocation(fogShader.shader, "ambientColor");
    fogShader.directionalLightCountLoc = GetShaderLocation(fogShader.shader, "directionalLightCount");
    fogShader.pointLightCountLoc = GetShaderLocation(fogShader.shader, "pointLightCount");
    fogShader.spotLightCountLoc = GetShaderLocation(fogShader.shader, "spotLightCount");

    char name[64] = {};
    for (int i = 0; i < FogShader::MaxDirectionalLights; ++i) {
        std::snprintf(name, sizeof(name), "directionalLightEnabled[%d]", i); fogShader.directionalLightEnabledLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "directionalLightDirection[%d]", i); fogShader.directionalLightDirectionLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "directionalLightColor[%d]", i); fogShader.directionalLightColorLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "directionalLightIntensity[%d]", i); fogShader.directionalLightIntensityLoc[i] = GetShaderLocation(fogShader.shader, name);
    }
    for (int i = 0; i < FogShader::MaxPointLights; ++i) {
        std::snprintf(name, sizeof(name), "pointLightEnabled[%d]", i); fogShader.pointLightEnabledLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "pointLightPosition[%d]", i); fogShader.pointLightPositionLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "pointLightColor[%d]", i); fogShader.pointLightColorLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "pointLightIntensity[%d]", i); fogShader.pointLightIntensityLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "pointLightRange[%d]", i); fogShader.pointLightRangeLoc[i] = GetShaderLocation(fogShader.shader, name);
    }
    for (int i = 0; i < FogShader::MaxSpotLights; ++i) {
        std::snprintf(name, sizeof(name), "spotLightEnabled[%d]", i); fogShader.spotLightEnabledLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "spotLightPosition[%d]", i); fogShader.spotLightPositionLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "spotLightDirection[%d]", i); fogShader.spotLightDirectionLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "spotLightColor[%d]", i); fogShader.spotLightColorLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "spotLightIntensity[%d]", i); fogShader.spotLightIntensityLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "spotLightRange[%d]", i); fogShader.spotLightRangeLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "spotLightInnerCone[%d]", i); fogShader.spotLightInnerConeLoc[i] = GetShaderLocation(fogShader.shader, name);
        std::snprintf(name, sizeof(name), "spotLightOuterCone[%d]", i); fogShader.spotLightOuterConeLoc[i] = GetShaderLocation(fogShader.shader, name);
    }
    return true;
}

void UnloadFogShader(FogShader& fogShader) {
    if (fogShader.shader.id != 0) UnloadShader(fogShader.shader);
    fogShader = FogShader();
}

void ApplyFogShaderToModel(Model& model, const FogShader& fogShader) {
    if (fogShader.shader.id == 0 || model.materialCount <= 0) return;
    for (int i = 0; i < model.materialCount; ++i) model.materials[i].shader = fogShader.shader;
}

void UpdateFogShader(const FogShader& fogShader, const FogConfig& config, const Camera& camera) {
    if (fogShader.shader.id == 0) return;
    int enabled = config.enabled ? 1 : 0;
    float color[3] = {config.color.r / 255.0f, config.color.g / 255.0f, config.color.b / 255.0f};
    float fogStart = config.start;
    float fogEnd = config.end > config.start ? config.end : config.start + 0.001f;
    float density = config.density > 0.001f ? config.density : 0.001f;
    SetShaderValue(fogShader.shader, fogShader.cameraPositionLoc, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(fogShader.shader, fogShader.fogColorLoc, color, SHADER_UNIFORM_VEC3);
    SetShaderValue(fogShader.shader, fogShader.fogStartLoc, &fogStart, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fogShader.shader, fogShader.fogEndLoc, &fogEnd, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fogShader.shader, fogShader.fogDensityLoc, &density, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fogShader.shader, fogShader.fogEnabledLoc, &enabled, SHADER_UNIFORM_INT);
}

void UpdateLightingShader(const FogShader& fogShader, const LightingConfig& config) {
    if (fogShader.shader.id == 0) return;
    float ambient[3] = {config.ambient.r / 255.0f, config.ambient.g / 255.0f, config.ambient.b / 255.0f};
    SetShaderValue(fogShader.shader, fogShader.ambientColorLoc, ambient, SHADER_UNIFORM_VEC3);

    int directionalCount = 0;
    int pointCount = 0;
    int spotCount = 0;
    int enabled = 0;
    Vector3 direction = Vector3{0.0f, -1.0f, 0.0f};
    float color[3] = {1.0f, 1.0f, 1.0f};
    float intensity = 0.0f;
    Vector3 position = Vector3Zero();
    float range = 1.0f;
    float innerCone = 18.0f;
    float outerCone = 32.0f;

    for (std::size_t i = 0; i < config.lights.size(); ++i) {
        const LevelLightConfig& light = config.lights[i];
        enabled = light.enabled ? 1 : 0;
        color[0] = light.color.r / 255.0f; color[1] = light.color.g / 255.0f; color[2] = light.color.b / 255.0f;
        intensity = light.intensity;
        position = light.position;
        direction = Vector3Normalize(Vector3RotateByQuaternion(Vector3{0.0f, 0.0f, -1.0f}, light.rotation));
        range = light.range;
        innerCone = light.innerConeDegrees;
        outerCone = light.outerConeDegrees;

        if (light.type == LightType::Directional && directionalCount < FogShader::MaxDirectionalLights) {
            int idx = directionalCount++;
            SetShaderValue(fogShader.shader, fogShader.directionalLightEnabledLoc[idx], &enabled, SHADER_UNIFORM_INT);
            SetShaderValue(fogShader.shader, fogShader.directionalLightDirectionLoc[idx], &direction, SHADER_UNIFORM_VEC3);
            SetShaderValue(fogShader.shader, fogShader.directionalLightColorLoc[idx], color, SHADER_UNIFORM_VEC3);
            SetShaderValue(fogShader.shader, fogShader.directionalLightIntensityLoc[idx], &intensity, SHADER_UNIFORM_FLOAT);
        } else if (light.type == LightType::Point && pointCount < FogShader::MaxPointLights) {
            int idx = pointCount++;
            SetShaderValue(fogShader.shader, fogShader.pointLightEnabledLoc[idx], &enabled, SHADER_UNIFORM_INT);
            SetShaderValue(fogShader.shader, fogShader.pointLightPositionLoc[idx], &position, SHADER_UNIFORM_VEC3);
            SetShaderValue(fogShader.shader, fogShader.pointLightColorLoc[idx], color, SHADER_UNIFORM_VEC3);
            SetShaderValue(fogShader.shader, fogShader.pointLightIntensityLoc[idx], &intensity, SHADER_UNIFORM_FLOAT);
            SetShaderValue(fogShader.shader, fogShader.pointLightRangeLoc[idx], &range, SHADER_UNIFORM_FLOAT);
        } else if (light.type == LightType::Spot && spotCount < FogShader::MaxSpotLights) {
            int idx = spotCount++;
            SetShaderValue(fogShader.shader, fogShader.spotLightEnabledLoc[idx], &enabled, SHADER_UNIFORM_INT);
            SetShaderValue(fogShader.shader, fogShader.spotLightPositionLoc[idx], &position, SHADER_UNIFORM_VEC3);
            SetShaderValue(fogShader.shader, fogShader.spotLightDirectionLoc[idx], &direction, SHADER_UNIFORM_VEC3);
            SetShaderValue(fogShader.shader, fogShader.spotLightColorLoc[idx], color, SHADER_UNIFORM_VEC3);
            SetShaderValue(fogShader.shader, fogShader.spotLightIntensityLoc[idx], &intensity, SHADER_UNIFORM_FLOAT);
            SetShaderValue(fogShader.shader, fogShader.spotLightRangeLoc[idx], &range, SHADER_UNIFORM_FLOAT);
            SetShaderValue(fogShader.shader, fogShader.spotLightInnerConeLoc[idx], &innerCone, SHADER_UNIFORM_FLOAT);
            SetShaderValue(fogShader.shader, fogShader.spotLightOuterConeLoc[idx], &outerCone, SHADER_UNIFORM_FLOAT);
        }
    }

    SetShaderValue(fogShader.shader, fogShader.directionalLightCountLoc, &directionalCount, SHADER_UNIFORM_INT);
    SetShaderValue(fogShader.shader, fogShader.pointLightCountLoc, &pointCount, SHADER_UNIFORM_INT);
    SetShaderValue(fogShader.shader, fogShader.spotLightCountLoc, &spotCount, SHADER_UNIFORM_INT);
}
