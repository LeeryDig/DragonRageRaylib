#ifndef AUDIO_RADIO_SYSTEM_HPP
#define AUDIO_RADIO_SYSTEM_HPP

#include <vector>

#include "level/levelRuntimeConfig.hpp"

struct RuntimeRadio {
    LevelRadioConfig config;
    bool playing;
    float currentVolume;

    RuntimeRadio() : config(), playing(false), currentVolume(0.0f) {}
};

void LoadRuntimeRadios(std::vector<RuntimeRadio>& radios, const std::vector<LevelRadioConfig>& configs);
void UnloadRuntimeRadios(std::vector<RuntimeRadio>& radios);
void UpdateRuntimeRadios(std::vector<RuntimeRadio>& radios, Vector3 playerPosition);
void DrawRuntimeRadios(const std::vector<RuntimeRadio>& radios);
RuntimeRadio* FindInteractableRadio(std::vector<RuntimeRadio>& radios, const Camera& camera, float maxDistance);
void ToggleRuntimeRadio(RuntimeRadio& radio);

#endif
