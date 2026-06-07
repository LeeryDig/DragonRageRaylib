#ifndef LEVEL_LEVEL_RUNTIME_CONFIG_HPP
#define LEVEL_LEVEL_RUNTIME_CONFIG_HPP

#include <string>
#include <vector>

#include "raylib.h"

#include "interactionSystem.hpp"

enum class FogMode {
    Linear
};

enum class LightType {
    Directional,
    Point,
    Spot
};

struct FogConfig {
    bool enabled;
    Color color;
    float start;
    float end;
    float density;
    FogMode mode;

    FogConfig()
        : enabled(false),
          color{185, 190, 200, 255},
          start(45.0f),
          end(150.0f),
          density(1.0f),
          mode(FogMode::Linear) {}
};

struct LevelLightConfig {
    std::string id;
    LightType type;
    bool enabled;
    Vector3 position;
    Quaternion rotation;
    Color color;
    float intensity;
    float range;
    float innerConeDegrees;
    float outerConeDegrees;
    bool castShadows;

    LevelLightConfig()
        : id("sun"),
          type(LightType::Directional),
          enabled(true),
          position{0.0f, 10.0f, 0.0f},
          rotation{0.0f, 0.0f, 0.0f, 1.0f},
          color{255, 220, 170, 255},
          intensity(1.0f),
          range(8.0f),
          innerConeDegrees(18.0f),
          outerConeDegrees(32.0f),
          castShadows(false) {}
};

struct LightingConfig {
    Color ambient;
    bool shadowsEnabled;
    int shadowResolution;
    float shadowBias;
    float shadowStrength;
    float shadowAreaSize;
    std::vector<LevelLightConfig> lights;

    LightingConfig()
        : ambient{10, 10, 15, 255},
          shadowsEnabled(false),
          shadowResolution(1024),
          shadowBias(0.005f),
          shadowStrength(0.45f),
          shadowAreaSize(40.0f),
          lights() {}
};

struct LevelPropConfig {
    std::string id;
    std::string modelPath;
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;

    LevelPropConfig()
        : id("prop"),
          modelPath(),
          position{0.0f, 0.0f, 0.0f},
          rotation{0.0f, 0.0f, 0.0f, 1.0f},
          scale{1.0f, 1.0f, 1.0f} {}
};

struct LevelRadioConfig {
    std::string id;
    std::string youtubeUrl;
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
    float interactionRadius;
    float audibleRadius;
    bool autoplay;

    LevelRadioConfig()
        : id("radio"),
          youtubeUrl(),
          position{0.0f, 1.0f, 0.0f},
          rotation{0.0f, 0.0f, 0.0f, 1.0f},
          scale{1.0f, 1.0f, 1.0f},
          interactionRadius(2.0f),
          audibleRadius(18.0f),
          autoplay(false) {}
};

struct LevelRuntimeConfig {
    std::string skyboxPath;
    FogConfig fog;
    LightingConfig lighting;
    std::vector<CharacterSpawnConfig> characters;
    std::vector<LevelPropConfig> props;
    std::vector<LevelRadioConfig> radios;
};

LevelRuntimeConfig LoadLevelRuntimeConfig(const std::string& configPath);
bool SaveLevelRuntimeConfig(const std::string& configPath, const LevelRuntimeConfig& config);

#endif
