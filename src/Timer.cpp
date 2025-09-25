#include "Timer.h"

namespace WinChess {
    Timer::Timer() {
        Reset();
    }

    void Timer::Reset() {
        m_Start = std::chrono::high_resolution_clock::now();
    }

    double Timer::Elapsed() {
        auto current = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = current - m_Start;
        return elapsed.count(); // returns seconds as double
    }

    double Timer::ElapsedMillis() {
        return Elapsed() * 1000.0f;
    }
}