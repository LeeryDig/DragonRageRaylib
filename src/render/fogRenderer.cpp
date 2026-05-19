#include "render/fogRenderer.hpp"

#include "utils.hpp"

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
    return true;
}

void UnloadFogShader(FogShader& fogShader) {
    if (fogShader.shader.id != 0) {
        UnloadShader(fogShader.shader);
    }
    fogShader = FogShader();
}

void ApplyFogShaderToModel(Model& model, const FogShader& fogShader) {
    if (fogShader.shader.id == 0 || model.materialCount <= 0) return;
    for (int i = 0; i < model.materialCount; ++i) {
        model.materials[i].shader = fogShader.shader;
    }
}

void UpdateFogShader(const FogShader& fogShader, const FogConfig& config, const Camera& camera) {
    if (fogShader.shader.id == 0) return;

    int enabled = config.enabled ? 1 : 0;
    float color[3] = {
        static_cast<float>(config.color.r) / 255.0f,
        static_cast<float>(config.color.g) / 255.0f,
        static_cast<float>(config.color.b) / 255.0f
    };
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
