#pragma once

#include <SDL3/SDL.h>

class Timer {
public:
    Timer() : m_last(SDL_GetTicks()), m_delta(0.0f) {}

    // Call once per frame; updates deltaTime
    void tick()
    {
        Uint64 now = SDL_GetTicks();
        m_delta = (now - m_last) / 1000.0f;
        m_last  = now;
    }

    float deltaTime() const { return m_delta; }

private:
    Uint64 m_last;
    float  m_delta;
};
