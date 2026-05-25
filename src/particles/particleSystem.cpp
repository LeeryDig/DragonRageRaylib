#include "particleSystem.hpp"

#include <raylib.h>
#include <raymath.h>

#include "gameplay/smoking/smokingConfig.hpp"
#include "utils.hpp"

int ParticleSystem::CreateEmitter(int maxParticles, const std::string& texturePath) {
    ParticleEmitter e;
    e.pool.resize(static_cast<std::size_t>(maxParticles));
    e.active = true;
    if (!texturePath.empty()) {
        e.texture = LoadTexture(Utils::ResolveProjectPath(texturePath).c_str());
        e.texLoaded = e.texture.id != 0;
    }
    emitters.push_back(std::move(e));
    return static_cast<int>(emitters.size()) - 1;
}

void ParticleSystem::SetEmitterPosition(int handle, Vector3 pos) {
    if (handle < 0 || handle >= static_cast<int>(emitters.size())) return;
    emitters[handle].worldPos = pos;
}

void ParticleSystem::StartEmitting(int handle, const ParticleEmitterConfig& cfg) {
    if (handle < 0 || handle >= static_cast<int>(emitters.size())) return;
    ParticleEmitter& e = emitters[handle];
    e.emitTimer  = cfg.emitDuration;
    e.emitting   = true;
    e.spawnRate  = cfg.spawnRate;
    e.lifetime   = cfg.lifetime;
    e.startSize  = cfg.startSize;
    e.endSize    = cfg.endSize;
    e.startAlpha = cfg.startAlpha;
    e.endAlpha   = cfg.endAlpha;
    e.velMinX    = cfg.velMinX;
    e.velMaxX    = cfg.velMaxX;
    e.velMinY    = cfg.velMinY;
    e.velMaxY    = cfg.velMaxY;
    e.velMinZ    = cfg.velMinZ;
    e.velMaxZ    = cfg.velMaxZ;
    e.drag       = cfg.drag;
}

void ParticleSystem::UpdateParticles(float dt) {
    for (auto& e : emitters) {
        if (!e.active) continue;

        if (e.emitting) {
            e.emitTimer -= dt;
            if (e.emitTimer <= 0.0f) {
                e.emitTimer = 0.0f;
                e.emitting  = false;
            } else {
                e.spawnAccum += e.spawnRate * dt;
                while (e.spawnAccum >= 1.0f) {
                    e.spawnAccum -= 1.0f;
                    for (auto& p : e.pool) {
                        if (!p.active) {
                            p.active     = true;
                            p.life       = e.lifetime;
                            p.maxLife    = e.lifetime;
                            p.position   = e.worldPos;
                            float rx = static_cast<float>(GetRandomValue(0, 1000)) / 1000.0f;
                            float ry = static_cast<float>(GetRandomValue(0, 1000)) / 1000.0f;
                            float rz = static_cast<float>(GetRandomValue(0, 1000)) / 1000.0f;
                            p.velocity   = Vector3{
                                e.velMinX + (e.velMaxX - e.velMinX) * rx,
                                e.velMinY + (e.velMaxY - e.velMinY) * ry,
                                e.velMinZ + (e.velMaxZ - e.velMinZ) * rz
                            };
                            p.startSize  = e.startSize;
                            p.endSize    = e.endSize;
                            p.startAlpha = e.startAlpha;
                            p.endAlpha   = e.endAlpha;
                            p.drag       = e.drag;
                            break;
                        }
                    }
                }
            }
        }

        for (auto& p : e.pool) {
            if (!p.active) continue;
            float dragFactor = 1.0f - Clamp(p.drag * dt, 0.0f, 1.0f);
            p.velocity = Vector3Scale(p.velocity, dragFactor);
            p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
            p.life -= dt;
            if (p.life <= 0.0f) p.active = false;
        }
    }
}

void ParticleSystem::DrawParticles(const Camera& camera) {
    for (const auto& e : emitters) {
        if (!e.active) continue;
        bool anyActive = false;
        for (const auto& p : e.pool) {
            if (p.active) { anyActive = true; break; }
        }
        if (!anyActive) continue;

        BeginBlendMode(BLEND_ADDITIVE);
        for (const auto& p : e.pool) {
            if (!p.active) continue;
            float t = (p.maxLife > 0.0f)
                ? Clamp(1.0f - p.life / p.maxLife, 0.0f, 1.0f) : 1.0f;
            float size  = p.startSize  + (p.endSize  - p.startSize)  * t;
            float alpha = p.startAlpha + (p.endAlpha - p.startAlpha) * t;
            Color color = Color{255, 255, 255,
                static_cast<unsigned char>(Clamp(alpha, 0.0f, 255.0f))};
            if (e.texLoaded) {
                DrawBillboard(camera, e.texture, p.position, size, color);
            } else {
                DrawSphere(p.position, size * 0.5f, color);
            }
        }
        EndBlendMode();
    }
}

void ParticleSystem::Unload() {
    for (auto& e : emitters) {
        if (e.texLoaded) {
            UnloadTexture(e.texture);
            e.texLoaded = false;
        }
    }
    emitters.clear();
}
