#include "smokingSystem.hpp"

#include <raylib.h>
#include <raymath.h>

#include "particles/particleSystem.hpp"

void UpdateSmoking(SmokingState& state, SmokingConfig& config, const InputMap& input, float dt) {
    if (IsActionPressed(input, GameAction::LightCigarette)) {
        if (state.phase == SmokingPhase::NONE && state.cigarettesInPack > 0) {
            state.cigarettesInPack--;
            state.cigaretteLife = 1.0f;
            state.phase         = SmokingPhase::IDLE_LIT;
        }
    }

    if (IsActionPressed(input, GameAction::TakePuff)) {
        if (state.phase == SmokingPhase::IDLE_LIT) {
            float variance      = static_cast<float>(GetRandomValue(-100, 100)) / 100.0f * config.puffDurationVariance;
            state.puffDuration  = config.puffDurationBase + variance;
            state.puffTimer     = state.puffDuration;
            state.phase         = SmokingPhase::PUFFING;
        }
    }

    switch (state.phase) {
        case SmokingPhase::PUFFING:
            state.puffTimer     -= dt;
            state.cigaretteLife -= config.drainPuff * dt;
            if (state.puffTimer <= 0.0f) {
                state.puffTimer = 0.0f;
                state.phase     = SmokingPhase::IDLE_LIT;
                if (config.emitParticles) {
                    state.postPuffEmitTimer = config.postPuffParticles.emitDuration;
                }
            }
            break;
        case SmokingPhase::IDLE_LIT:
            state.cigaretteLife -= config.drainIdle * dt;
            break;
        case SmokingPhase::NONE:
            break;
    }

    if (state.postPuffEmitTimer > 0.0f) {
        state.postPuffEmitTimer -= dt;
        if (state.postPuffEmitTimer < 0.0f) state.postPuffEmitTimer = 0.0f;
    }

    if (state.phase != SmokingPhase::NONE && state.cigaretteLife <= 0.0f) {
        state.cigaretteLife     = 0.0f;
        state.postPuffEmitTimer = 0.0f;
        state.phase             = SmokingPhase::NONE;
    }
}

void UpdateSmokingParticles(SmokingState& state, SmokingConfig& config, ParticleSystem& particles, Vector3 playerPos) {
    if (!config.emitParticles) return;

    const ParticleEmitterConfig& pcfg = config.postPuffParticles;

    if (state.emitterHandle < 0) {
        state.emitterHandle = particles.CreateEmitter(pcfg.maxParticles, pcfg.texturePath);
    }

    Vector3 emitPos = Vector3{
        playerPos.x + pcfg.emitOffsetX,
        playerPos.y + pcfg.emitOffsetY,
        playerPos.z + pcfg.emitOffsetZ
    };
    particles.SetEmitterPosition(state.emitterHandle, emitPos);

    if (state.postPuffEmitTimer > 0.0f
        && state.emitterHandle < static_cast<int>(particles.emitters.size())
        && !particles.emitters[state.emitterHandle].emitting) {
        particles.StartEmitting(state.emitterHandle, pcfg);
    }
}
