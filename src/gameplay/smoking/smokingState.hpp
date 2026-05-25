#ifndef GAMEPLAY_SMOKING_SMOKING_STATE_HPP
#define GAMEPLAY_SMOKING_SMOKING_STATE_HPP

enum class SmokingPhase { NONE, IDLE_LIT, PUFFING };

struct SmokingState {
    int cigarettesInPack    = 10;
    SmokingPhase phase      = SmokingPhase::NONE;
    float cigaretteLife     = 0.0f;
    float puffTimer         = 0.0f;
    float puffDuration      = 0.0f;
    float postPuffEmitTimer = 0.0f;
    int   emitterHandle     = -1;
};


#endif
