#ifndef GAMEPLAY_SMOKING_SMOKING_CONFIG_HPP
#define GAMEPLAY_SMOKING_SMOKING_CONFIG_HPP

#include <string>

struct ParticleEmitterConfig {
    std::string texturePath  = "resources/textures/particles/smoke.png";
    int   maxParticles       = 20;
    float spawnRate          = 8.0f;
    float emitDuration       = 2.0f;
    float lifetime           = 1.5f;
    float startSize          = 0.1f;
    float endSize            = 0.4f;
    float startAlpha         = 200.0f;
    float endAlpha           = 0.0f;
    float velMinX            = -0.3f;
    float velMinY            =  0.5f;
    float velMinZ            = -0.3f;
    float velMaxX            =  0.3f;
    float velMaxY            =  1.5f;
    float velMaxZ            =  0.3f;
    float drag               =  0.5f;
    float emitOffsetX        =  0.0f;
    float emitOffsetY        =  1.5f;
    float emitOffsetZ        =  0.0f;
};

struct SmokingConfig {
    int   defaultPackSize       = 10;
    float drainIdle             = 0.005f;
    float drainPuff             = 0.050f;
    float puffDurationBase      = 1.5f;
    float puffDurationVariance  = 0.5f;
    bool  emitParticles         = true;
    ParticleEmitterConfig postPuffParticles;
};

SmokingConfig DefaultSmokingConfig();
SmokingConfig LoadSmokingConfig(const std::string& filePath, const SmokingConfig& fallback);
bool SaveSmokingConfig(const std::string& filePath, const SmokingConfig& cfg);

#endif
