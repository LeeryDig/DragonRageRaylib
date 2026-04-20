#ifndef GAMESTATE_HPP
#define GAMESTATE_HPP

enum class SysState {
    EDITOR,
    MENU,
    OPTIONS,
    PLAYING,
    PAUSED
};

extern SysState sysState;
extern float distanceTraveled;

#endif
