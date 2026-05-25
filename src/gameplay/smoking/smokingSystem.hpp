#ifndef GAMEPLAY_SMOKING_SMOKING_SYSTEM_HPP
#define GAMEPLAY_SMOKING_SMOKING_SYSTEM_HPP

#include <raylib.h>

#include "smokingConfig.hpp"
#include "smokingState.hpp"
#include "input/inputMap.hpp"

struct ParticleSystem;

void UpdateSmoking(SmokingState& state, SmokingConfig& config, const InputMap& input, float dt);
void UpdateSmokingParticles(SmokingState& state, SmokingConfig& config, ParticleSystem& particles, Vector3 playerPos);

#endif
