#include "smokingConfig.hpp"

#include <fstream>
#include <iomanip>
#include <string>

#include <raylib.h>

#include "utils.hpp"

namespace {

float ExtractFloat(const std::string& json, const std::string& key, float fallback) {
    std::string searchKey = "\"" + key + "\"";
    std::size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return fallback;
    std::size_t colon = json.find(':', keyPos);
    if (colon == std::string::npos) return fallback;
    char* end = nullptr;
    float val = strtof(json.c_str() + colon + 1, &end);
    return (end == json.c_str() + colon + 1) ? fallback : val;
}

bool ExtractBool(const std::string& json, const std::string& key, bool fallback) {
    std::string searchKey = "\"" + key + "\"";
    std::size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return fallback;
    std::size_t colon = json.find(':', keyPos);
    if (colon == std::string::npos) return fallback;
    std::size_t val = json.find_first_not_of(" \t\n\r", colon + 1);
    if (val == std::string::npos) return fallback;
    if (json[val] == 't') return true;
    if (json[val] == 'f') return false;
    return fallback;
}

std::string ExtractObjectBlock(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    std::size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";
    std::size_t blockStart = json.find('{', keyPos);
    if (blockStart == std::string::npos) return "";
    int depth = 0;
    for (std::size_t i = blockStart; i < json.size(); ++i) {
        if (json[i] == '{') depth++;
        else if (json[i] == '}') {
            depth--;
            if (depth == 0) return json.substr(blockStart, i - blockStart + 1);
        }
    }
    return "";
}

}  // namespace

SmokingConfig DefaultSmokingConfig() {
    return SmokingConfig{};
}

SmokingConfig LoadSmokingConfig(const std::string& filePath, const SmokingConfig& fallback) {
    SmokingConfig cfg = fallback;

    char* raw = LoadFileText(filePath.c_str());
    if (raw == nullptr) return cfg;
    std::string json = raw;
    UnloadFileText(raw);

    cfg.defaultPackSize      = static_cast<int>(ExtractFloat(json, "default_pack_size", static_cast<float>(fallback.defaultPackSize)));
    cfg.drainIdle            = ExtractFloat(json, "drain_idle",            fallback.drainIdle);
    cfg.drainPuff            = ExtractFloat(json, "drain_puff",            fallback.drainPuff);
    cfg.puffDurationBase     = ExtractFloat(json, "puff_duration_base",    fallback.puffDurationBase);
    cfg.puffDurationVariance = ExtractFloat(json, "puff_duration_variance",fallback.puffDurationVariance);
    cfg.emitParticles        = ExtractBool(json,  "emit_particles",        fallback.emitParticles);

    std::string pBlock = ExtractObjectBlock(json, "particles");
    if (!pBlock.empty()) {
        ParticleEmitterConfig& p = cfg.postPuffParticles;
        p.maxParticles  = static_cast<int>(ExtractFloat(pBlock, "max_particles",  static_cast<float>(fallback.postPuffParticles.maxParticles)));
        p.spawnRate     = ExtractFloat(pBlock, "spawn_rate",     fallback.postPuffParticles.spawnRate);
        p.emitDuration  = ExtractFloat(pBlock, "emit_duration",  fallback.postPuffParticles.emitDuration);
        p.lifetime      = ExtractFloat(pBlock, "lifetime",       fallback.postPuffParticles.lifetime);
        p.startSize     = ExtractFloat(pBlock, "start_size",     fallback.postPuffParticles.startSize);
        p.endSize       = ExtractFloat(pBlock, "end_size",       fallback.postPuffParticles.endSize);
        p.startAlpha    = ExtractFloat(pBlock, "start_alpha",    fallback.postPuffParticles.startAlpha);
        p.endAlpha      = ExtractFloat(pBlock, "end_alpha",      fallback.postPuffParticles.endAlpha);
        p.velMinX       = ExtractFloat(pBlock, "vel_min_x",      fallback.postPuffParticles.velMinX);
        p.velMinY       = ExtractFloat(pBlock, "vel_min_y",      fallback.postPuffParticles.velMinY);
        p.velMinZ       = ExtractFloat(pBlock, "vel_min_z",      fallback.postPuffParticles.velMinZ);
        p.velMaxX       = ExtractFloat(pBlock, "vel_max_x",      fallback.postPuffParticles.velMaxX);
        p.velMaxY       = ExtractFloat(pBlock, "vel_max_y",      fallback.postPuffParticles.velMaxY);
        p.velMaxZ       = ExtractFloat(pBlock, "vel_max_z",      fallback.postPuffParticles.velMaxZ);
        p.drag          = ExtractFloat(pBlock, "drag",           fallback.postPuffParticles.drag);
        p.emitOffsetX   = ExtractFloat(pBlock, "emit_offset_x",  fallback.postPuffParticles.emitOffsetX);
        p.emitOffsetY   = ExtractFloat(pBlock, "emit_offset_y",  fallback.postPuffParticles.emitOffsetY);
        p.emitOffsetZ   = ExtractFloat(pBlock, "emit_offset_z",  fallback.postPuffParticles.emitOffsetZ);
    }

    return cfg;
}

bool SaveSmokingConfig(const std::string& filePath, const SmokingConfig& cfg) {
    std::ofstream f(Utils::ResolveWritableProjectPath(filePath).c_str(), std::ios::out | std::ios::trunc);
    if (!f.is_open()) {
        TraceLog(LOG_WARNING, "SmokingConfig: failed to save %s", filePath.c_str());
        return false;
    }

    f << std::fixed << std::setprecision(6);
    f << "{\n";
    f << "  \"default_pack_size\": "       << cfg.defaultPackSize       << ",\n";
    f << "  \"drain_idle\": "              << cfg.drainIdle             << ",\n";
    f << "  \"drain_puff\": "              << cfg.drainPuff             << ",\n";
    f << "  \"puff_duration_base\": "      << cfg.puffDurationBase      << ",\n";
    f << "  \"puff_duration_variance\": "  << cfg.puffDurationVariance  << ",\n";
    f << "  \"emit_particles\": "          << (cfg.emitParticles ? "true" : "false") << ",\n";
    f << "  \"particles\": {\n";
    const ParticleEmitterConfig& p = cfg.postPuffParticles;
    f << "    \"max_particles\": "   << p.maxParticles   << ",\n";
    f << "    \"spawn_rate\": "      << p.spawnRate      << ",\n";
    f << "    \"emit_duration\": "   << p.emitDuration   << ",\n";
    f << "    \"lifetime\": "        << p.lifetime       << ",\n";
    f << "    \"start_size\": "      << p.startSize      << ",\n";
    f << "    \"end_size\": "        << p.endSize        << ",\n";
    f << "    \"start_alpha\": "     << p.startAlpha     << ",\n";
    f << "    \"end_alpha\": "       << p.endAlpha       << ",\n";
    f << "    \"vel_min_x\": "       << p.velMinX        << ",\n";
    f << "    \"vel_min_y\": "       << p.velMinY        << ",\n";
    f << "    \"vel_min_z\": "       << p.velMinZ        << ",\n";
    f << "    \"vel_max_x\": "       << p.velMaxX        << ",\n";
    f << "    \"vel_max_y\": "       << p.velMaxY        << ",\n";
    f << "    \"vel_max_z\": "       << p.velMaxZ        << ",\n";
    f << "    \"drag\": "            << p.drag           << ",\n";
    f << "    \"emit_offset_x\": "  << p.emitOffsetX    << ",\n";
    f << "    \"emit_offset_y\": "  << p.emitOffsetY    << ",\n";
    f << "    \"emit_offset_z\": "  << p.emitOffsetZ    << "\n";
    f << "  }\n";
    f << "}\n";

    return true;
}
