#include "Pieces.h"

#if defined(_MSC_VER)
    #include <intrin.h>
#endif

namespace WinChess {
    namespace ChessEngine {
        namespace Pieces {

            int LSSBIndex(uint64_t bb) {
                if (bb == 0) return -1;
    #if defined(_MSC_VER)
                unsigned long index;
                _BitScanForward64(&index, bb);
                return static_cast<int>(index);
    #elif defined(__GNUC__) || defined(__clang__)
                return __builtin_ctzll(bb);
    #else
                // Fallback C++20
                return static_cast<int>(std::countr_zero(bb));
    #endif
            }

        }
    }
}
