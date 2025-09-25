#pragma once
#include "pch.h"

namespace WinChess {
    class Timer {
    private:
        std::chrono::high_resolution_clock::time_point m_Start;
    public:
        Timer();

        void Reset();
        double Elapsed(); // seconds
        double ElapsedMillis(); // millies
    };
}