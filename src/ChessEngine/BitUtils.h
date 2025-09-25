#pragma once
#include "pch.h"
#if defined(_MSC_VER)
    #include <intrin.h>
#endif
namespace WinChess {
    namespace ChessEngine {
        namespace BitUtils {
            // Platform intrinsics wrappers
            inline int LSSBIndex(uint64_t bb) {
        #if defined(_MSC_VER)
                unsigned long idx;
                _BitScanForward64(&idx, bb);
                return static_cast<int>(idx);
        #elif defined(__GNUC__) || defined(__clang__)
                return static_cast<int>(__builtin_ctzll(bb));
        #else
                return static_cast<int>(std::countr_zero(bb));
        #endif
            }
            inline int PopCount(uint64_t bb) {
        #if defined(_MSC_VER)
                return static_cast<int>(__popcnt64(static_cast<unsigned long long>(bb)));
        #elif defined(__GNUC__) || defined(__clang__)
                return __builtin_popcountll(bb);
        #else
            bb = bb - ((bb >> 1) & 0x5555555555555555ULL);
            bb = (bb & 0x3333333333333333ULL) + ((bb >> 2) & 0x3333333333333333ULL);
            bb = (bb + (bb >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
            return static_cast<int>((bb * 0x0101010101010101ULL) >> 56);
        #endif
            }
        }
    }
}