#ifndef PARTICLES_PARTICLE_SYSTEM_HPP
#define PARTICLES_PARTICLE_SYSTEM_HPP

#include <string>
#include <vector>

#include <raylib.h>

struct ParticleEmitterConfig;

struct Particle {
    Vector3 position   = {};
    Vector3 velocity   = {};
    float   life       = 0.0f;
    float   maxLife    = 1.0f;
    float   startSize  = 0.1f;
    float   endSize    = 0.4f;
    float   startAlpha = 200.0f;
    float   endAlpha   = 0.0f;
    float   drag       = 0.5f;
    bool    active     = false;
};

struct ParticleEmitter {
    std::vector<Particle> pool;
    Vector3   worldPos   = {};
    float     spawnAccum = 0.0f;
    float     emitTimer  = 0.0f;
    bool      active     = false;
    bool      emitting   = false;
    Texture2D texture    = {};
    bool      texLoaded  = false;

    // spawn params (copied from config at StartEmitting)
    float spawnRate  = 8.0f;
    float lifetime   = 1.5f;
    float startSize  = 0.1f;
    float endSize    = 0.4f;
    float startAlpha = 200.0f;
    float endAlpha   = 0.0f;
    float velMinX    = -0.3f;
    float velMaxX    =  0.3f;
    float velMinY    =  0.5f;
    float velMaxY    =  1.5f;
    float velMinZ    = -0.3f;
    float velMaxZ    =  0.3f;
    float drag       =  0.5f;
};

struct ParticleSystem {
    std::vector<ParticleEmitter> emitters;

    int  CreateEmitter(int maxParticles, const std::string& texturePath);
    void SetEmitterPosition(int handle, Vector3 pos);
    void StartEmitting(int handle, const ParticleEmitterConfig& cfg);
    void UpdateParticles(float dt);
    void DrawParticles(const Camera& camera);
    void Unload();
};

#endif
