#include "radioSystem.hpp"

#include <raylib.h>
#include <raymath.h>

#include "interaction/interactionRay.hpp"
#include "audio/youtubeAudioClient.hpp"

namespace {
YouTubeAudioClient gYoutubeRadioClient;
RuntimeRadio* gActiveRadio = nullptr;
}

void LoadRuntimeRadios(std::vector<RuntimeRadio>& radios, const std::vector<LevelRadioConfig>& configs) {
    radios.clear();
    radios.reserve(configs.size());
    for (std::size_t i = 0; i < configs.size(); ++i) {
        RuntimeRadio radio;
        radio.config = configs[i];
        radio.playing = false;
        radio.currentVolume = 0.0f;
        radios.push_back(radio);
        if (radio.config.autoplay && !radio.config.youtubeUrl.empty()) {
            ToggleRuntimeRadio(radios.back());
        }
    }
}

void UnloadRuntimeRadios(std::vector<RuntimeRadio>& radios) {
    ShutdownYouTubeAudioClient(gYoutubeRadioClient);
    gActiveRadio = nullptr;
    radios.clear();
}

void UpdateRuntimeRadios(std::vector<RuntimeRadio>& radios, Vector3 playerPosition) {
    for (std::size_t i = 0; i < radios.size(); ++i) {
        RuntimeRadio& radio = radios[i];
        float radius = radio.config.audibleRadius > 0.0f ? radio.config.audibleRadius : 1.0f;
        float distance = Vector3Distance(playerPosition, radio.config.position);
        radio.currentVolume = radio.playing ? 1.0f - Clamp(distance / radius, 0.0f, 1.0f) : 0.0f;
    }
    if (gActiveRadio != nullptr && gActiveRadio->playing) {
        SetYouTubeAudioClientVolume(gYoutubeRadioClient, gActiveRadio->currentVolume);
        UpdateYouTubeAudioClient(gYoutubeRadioClient);
    }
}

void DrawRuntimeRadios(const std::vector<RuntimeRadio>& radios) {
    for (std::size_t i = 0; i < radios.size(); ++i) {
        const RuntimeRadio& radio = radios[i];
        Vector3 size = Vector3{0.9f * radio.config.scale.x, 0.55f * radio.config.scale.y, 0.45f * radio.config.scale.z};
        Color body = radio.playing ? Color{60, 120, 70, 255} : Color{45, 45, 52, 255};
        DrawCubeV(radio.config.position, size, body);
        DrawCubeWiresV(radio.config.position, size, radio.playing ? LIME : YELLOW);
    }
}

RuntimeRadio* FindInteractableRadio(std::vector<RuntimeRadio>& radios, const Camera& camera, float maxDistance) {
    RuntimeRadio* best = nullptr;
    float bestDistance = maxDistance;
    Ray ray = BuildInteractionRay(camera);

    for (std::size_t i = 0; i < radios.size(); ++i) {
        const RuntimeRadio& radio = radios[i];
        Vector3 half = Vector3{0.45f * radio.config.scale.x, 0.275f * radio.config.scale.y, 0.225f * radio.config.scale.z};
        BoundingBox box = BoundingBox{
            Vector3Subtract(radio.config.position, half),
            Vector3Add(radio.config.position, half)};
        float hitDistance = 0.0f;
        if (RayHitsInteractionBox(ray, box, bestDistance, hitDistance)) {
            best = &radios[i];
            bestDistance = hitDistance;
        }
    }
    return best;
}

void ToggleRuntimeRadio(RuntimeRadio& radio) {
    if (radio.playing) {
        StopYouTubeStream(gYoutubeRadioClient);
        radio.playing = false;
        radio.currentVolume = 0.0f;
        if (gActiveRadio == &radio) gActiveRadio = nullptr;
        TraceLog(LOG_INFO, "Radio %s OFF", radio.config.id.c_str());
        return;
    }

    if (radio.config.youtubeUrl.empty()) {
        TraceLog(LOG_WARNING, "Radio %s has empty YouTube URL", radio.config.id.c_str());
        return;
    }

    if (gActiveRadio != nullptr) {
        gActiveRadio->playing = false;
        gActiveRadio->currentVolume = 0.0f;
    }
    if (StartYouTubeStream(gYoutubeRadioClient, radio.config.youtubeUrl)) {
        radio.playing = true;
        gActiveRadio = &radio;
        TraceLog(LOG_INFO, "Radio %s ON", radio.config.id.c_str());
    } else {
        TraceLog(LOG_WARNING, "Radio %s failed to start YouTube stream. Is tools/youtube-audio-server running?", radio.config.id.c_str());
    }
}
