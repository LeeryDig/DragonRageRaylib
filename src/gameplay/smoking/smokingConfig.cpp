#include "smokingConfig.hpp"

#include <fstream>
#include <iomanip>
#include <string>

#include <raylib.h>

#include "assets/json.hpp"
#include "utils.hpp"

namespace {

using assets::BoolMember;
using assets::FloatMember;
using assets::GetMember;
using assets::IntMember;
using assets::JsonParser;
using assets::JsonValue;

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

    JsonParser parser(json);
    JsonValue root = parser.Parse();
    if (parser.HadError()) {
        TraceLog(LOG_WARNING, "SmokingConfig: JSON parse warning in %s: %s", filePath.c_str(), parser.Error().c_str());
    }
    if (root.type != JsonValue::Object) {
        TraceLog(LOG_WARNING, "SmokingConfig: root is not object in %s", filePath.c_str());
        return cfg;
    }

    cfg.defaultPackSize      = IntMember(root,   "default_pack_size", fallback.defaultPackSize);
    cfg.drainIdle            = FloatMember(root, "drain_idle",            fallback.drainIdle);
    cfg.drainPuff            = FloatMember(root, "drain_puff",            fallback.drainPuff);
    cfg.puffDurationBase     = FloatMember(root, "puff_duration_base",    fallback.puffDurationBase);
    cfg.puffDurationVariance = FloatMember(root, "puff_duration_variance",fallback.puffDurationVariance);
    cfg.emitParticles        = BoolMember(root,  "emit_particles",        fallback.emitParticles);

    const JsonValue* particles = GetMember(root, "particles");
    if (particles && particles->type == JsonValue::Object) {
        ParticleEmitterConfig& p = cfg.postPuffParticles;
        p.maxParticles  = IntMember(*particles,   "max_particles", fallback.postPuffParticles.maxParticles);
        p.spawnRate     = FloatMember(*particles, "spawn_rate",     fallback.postPuffParticles.spawnRate);
        p.emitDuration  = FloatMember(*particles, "emit_duration",  fallback.postPuffParticles.emitDuration);
        p.lifetime      = FloatMember(*particles, "lifetime",       fallback.postPuffParticles.lifetime);
        p.startSize     = FloatMember(*particles, "start_size",     fallback.postPuffParticles.startSize);
        p.endSize       = FloatMember(*particles, "end_size",       fallback.postPuffParticles.endSize);
        p.startAlpha    = FloatMember(*particles, "start_alpha",    fallback.postPuffParticles.startAlpha);
        p.endAlpha      = FloatMember(*particles, "end_alpha",      fallback.postPuffParticles.endAlpha);
        p.velMinX       = FloatMember(*particles, "vel_min_x",      fallback.postPuffParticles.velMinX);
        p.velMinY       = FloatMember(*particles, "vel_min_y",      fallback.postPuffParticles.velMinY);
        p.velMinZ       = FloatMember(*particles, "vel_min_z",      fallback.postPuffParticles.velMinZ);
        p.velMaxX       = FloatMember(*particles, "vel_max_x",      fallback.postPuffParticles.velMaxX);
        p.velMaxY       = FloatMember(*particles, "vel_max_y",      fallback.postPuffParticles.velMaxY);
        p.velMaxZ       = FloatMember(*particles, "vel_max_z",      fallback.postPuffParticles.velMaxZ);
        p.drag          = FloatMember(*particles, "drag",           fallback.postPuffParticles.drag);
        p.emitOffsetX   = FloatMember(*particles, "emit_offset_x",  fallback.postPuffParticles.emitOffsetX);
        p.emitOffsetY   = FloatMember(*particles, "emit_offset_y",  fallback.postPuffParticles.emitOffsetY);
        p.emitOffsetZ   = FloatMember(*particles, "emit_offset_z",  fallback.postPuffParticles.emitOffsetZ);
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
