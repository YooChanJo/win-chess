#include "Magic.h"
#include "Pieces.h"
#include "BitUtils.h"

#if defined(__has_include)
#  if __has_include(<immintrin.h>)
#    include <immintrin.h>
#  endif
#endif

// Define HAVE_PEXT when compiling with BMI2 support or (MSVC x64 provides _pext_u64)
#if defined(__BMI2__) || defined(_MSC_VER)
#  define HAVE_PEXT 1
#endif

namespace WinChess {
    namespace ChessEngine {
        namespace Magic {

            using namespace Pieces;

            static bool g_initialized = false;

            struct MagicInfo {
                uint64_t mask = 0ULL;
                int bits = 0;
                uint64_t magic = 0ULL;
                std::vector<int> indices;         // bit positions included in mask (for fallback)
                std::vector<uint64_t> attacks;    // attack table (size = 1<<bits)
            };

            static MagicInfo RookInfo[64];
            static MagicInfo BishopInfo[64];

            // Forward declarations
            static uint64_t CompressOccupancy(uint64_t occ, const std::vector<int>& indices);
            static uint64_t FindCandidateMagic(const std::vector<uint64_t>& refAttacks, const std::vector<uint64_t>& occList, const std::vector<int>& indices);
            static void BuildMagicForSquare(int square, bool isRook);
            static uint64_t RookAttacksOnTheFly(int square, uint64_t occ);
            static uint64_t BishopAttacksOnTheFly(int square, uint64_t occ);
            static void GenOccupancySubsetsFromIndices(const std::vector<int>& indices, std::vector<uint64_t>& out);

            // -------------------------
            // PEXT wrapper: use hardware PEXT when available; otherwise fallback to loop
            // -------------------------
            static inline uint64_t pext64_hw(uint64_t val, uint64_t mask) {
            #if defined(HAVE_PEXT)
                // On MSVC and most GCC/Clang builds with -mbmi2, _pext_u64 is available
                // include <immintrin.h> above to expose _pext_u64 on supported compilers.
                return _pext_u64(val, mask);
            #else
                // Fallback is intentionally empty here; caller will call fallback function if needed.
                (void)val; (void)mask;
                return 0ULL;
            #endif
            }

            // Portable CompressOccupancy: prefer hardware PEXT if available, otherwise bit-test loop.
            static uint64_t CompressOccupancy(uint64_t occ, const std::vector<int>& indices) {
            #if defined(HAVE_PEXT)
                // Build mask from indices (each set bit in mask corresponds to a relevant board square)
                uint64_t mask = 0ULL;
                for (size_t i = 0; i < indices.size(); ++i) mask |= (1ULL << indices[i]);
                // pext compresses bits of 'occ' according to mask into low-order bits
                return pext64_hw(occ, mask);
            #else
                uint64_t comp = 0ULL;
                for (size_t j = 0; j < indices.size(); ++j) {
                    if (occ & (1ULL << indices[j])) comp |= (1ULL << j);
                }
                return comp;
            #endif
            }

            // ---------- utilities ----------
            static void GenOccupancySubsetsFromIndices(const std::vector<int>& indices, std::vector<uint64_t>& out) {
                // Generate all subsets for the mask indices (size = 1<<indices.size()).
                size_t bits = indices.size();
                size_t total = 1ULL << bits;
                out.clear();
                out.reserve(total);
                for (size_t mask = 0; mask < total; ++mask) {
                    uint64_t occ = 0ULL;
                    for (size_t i = 0; i < bits; ++i) {
                        if (mask & (1ULL << i)) occ |= (1ULL << indices[i]);
                    }
                    out.push_back(occ);
                }
            }

            // On-the-fly rook attacks (used for magic generation)
            static uint64_t RookAttacksOnTheFly(int square, uint64_t occ) {
                uint64_t attacks = 0ULL;
                int r = square / 8;
                int f = square % 8;
                // north
                for (int rr = r + 1; rr <= 7; ++rr) {
                    int idx = rr * 8 + f;
                    attacks |= (1ULL << idx);
                    if (occ & (1ULL << idx)) break;
                }
                // south
                for (int rr = r - 1; rr >= 0; --rr) {
                    int idx = rr * 8 + f;
                    attacks |= (1ULL << idx);
                    if (occ & (1ULL << idx)) break;
                }
                // east
                for (int ff = f + 1; ff <= 7; ++ff) {
                    int idx = r * 8 + ff;
                    attacks |= (1ULL << idx);
                    if (occ & (1ULL << idx)) break;
                }
                // west
                for (int ff = f - 1; ff >= 0; --ff) {
                    int idx = r * 8 + ff;
                    attacks |= (1ULL << idx);
                    if (occ & (1ULL << idx)) break;
                }
                return attacks;
            }

            static uint64_t BishopAttacksOnTheFly(int square, uint64_t occ) {
                uint64_t attacks = 0ULL;
                int r = square / 8;
                int f = square % 8;
                for (int rr = r + 1, ff = f + 1; rr <= 7 && ff <= 7; ++rr, ++ff) {
                    int idx = rr * 8 + ff;
                    attacks |= (1ULL << idx);
                    if (occ & (1ULL << idx)) break;
                }
                for (int rr = r + 1, ff = f - 1; rr <= 7 && ff >= 0; ++rr, --ff) {
                    int idx = rr * 8 + ff;
                    attacks |= (1ULL << idx);
                    if (occ & (1ULL << idx)) break;
                }
                for (int rr = r - 1, ff = f + 1; rr >= 0 && ff <= 7; --rr, ++ff) {
                    int idx = rr * 8 + ff;
                    attacks |= (1ULL << idx);
                    if (occ & (1ULL << idx)) break;
                }
                for (int rr = r - 1, ff = f - 1; rr >= 0 && ff >= 0; --rr, --ff) {
                    int idx = rr * 8 + ff;
                    attacks |= (1ULL << idx);
                    if (occ & (1ULL << idx)) break;
                }
                return attacks;
            }

            // Find a magic number candidate that yields collision-free mapping for this square
            static uint64_t FindCandidateMagic(const std::vector<uint64_t>& refAttacks, const std::vector<uint64_t>& occList, const std::vector<int>& indices) {
                int bits = static_cast<int>(indices.size());
                size_t tableSize = 1ULL << bits;
                std::mt19937_64 rng(0x9e3779b97f4a7c15ULL + (uint64_t)bits * 0xa5a5a5a5ULL);
                std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());

                for (int attempt = 0; attempt < 2000; ++attempt) {
                    uint64_t magic = dist(rng) & dist(rng) & dist(rng);
                    // small heuristic: ensure not too sparse
                    if (magic == 0ULL) continue;

                    std::vector<uint64_t> table(tableSize, 0ULL);
                    bool ok = true;
                    for (size_t i = 0; i < occList.size() && ok; ++i) {
                        uint64_t occ = occList[i];
                        uint64_t comp = CompressOccupancy(occ, indices);
                        uint64_t index = (comp * magic) >> (64 - bits);
                        if (table[index] == 0ULL) table[index] = refAttacks[i] | 1ULL; // store + marker
                        else {
                            if ((table[index] & ~1ULL) != refAttacks[i]) ok = false;
                        }
                    }
                    if (ok) return magic;
                }
                // fallback (very unlikely)
                return 0xFF7FF7FF7FF7FF7FULL ^ (uint64_t(indices.size()) << 1);
            }

            static void BuildMagicForSquare(int square, bool isRook) {
                MagicInfo& mi = (isRook ? RookInfo[square] : BishopInfo[square]);

                mi.mask = 0ULL;
                mi.indices.clear();
                mi.attacks.clear();
                mi.magic = 0ULL;
                mi.bits = 0;

                // Build mask: all sliding rays excluding edges (classical)
                int r = square / 8;
                int f = square % 8;
                if (isRook) {
                    // rook mask (exclude edge squares)
                    for (int rr = r + 1; rr <= 6; ++rr) mi.mask |= (1ULL << (rr * 8 + f));
                    for (int rr = r - 1; rr >= 1; --rr) mi.mask |= (1ULL << (rr * 8 + f));
                    for (int ff = f + 1; ff <= 6; ++ff) mi.mask |= (1ULL << (r * 8 + ff));
                    for (int ff = f - 1; ff >= 1; --ff) mi.mask |= (1ULL << (r * 8 + ff));
                } else {
                    // bishop mask (exclude edges)
                    for (int rr = r + 1, ff = f + 1; rr <= 6 && ff <= 6; ++rr, ++ff) mi.mask |= (1ULL << (rr * 8 + ff));
                    for (int rr = r + 1, ff = f - 1; rr <= 6 && ff >= 1; ++rr, --ff) mi.mask |= (1ULL << (rr * 8 + ff));
                    for (int rr = r - 1, ff = f + 1; rr >= 1 && ff <= 6; --rr, ++ff) mi.mask |= (1ULL << (rr * 8 + ff));
                    for (int rr = r - 1, ff = f - 1; rr >= 1 && ff >= 1; --rr, --ff) mi.mask |= (1ULL << (rr * 8 + ff));
                }

                // Collect mask bit indices (useful for fallback)
                uint64_t mm = mi.mask;
                while (mm) {
                    int b = BitUtils::LSSBIndex(mm);
                    mi.indices.push_back(b);
                    mm &= mm - 1;
                }
                mi.bits = static_cast<int>(mi.indices.size());
                if (mi.bits == 0) {
                    mi.magic = 0ULL;
                    mi.attacks.assign(1, 0ULL);
                    return;
                }

                // generate all occupancy subsets (full 64-bit occupancy bitboards)
                std::vector<uint64_t> occList;
                GenOccupancySubsetsFromIndices(mi.indices, occList);

                // reference attacks computed on-the-fly for each occ
                std::vector<uint64_t> ref;
                ref.reserve(occList.size());
                for (size_t i = 0; i < occList.size(); ++i) {
                    uint64_t occ = occList[i];
                    uint64_t att = isRook ? RookAttacksOnTheFly(square, occ) : BishopAttacksOnTheFly(square, occ);
                    ref.push_back(att);
                }

                // find magic
                mi.magic = FindCandidateMagic(ref, occList, mi.indices);

                // allocate attack table
                size_t tableSize = 1ULL << mi.bits;
                mi.attacks.assign(tableSize, 0ULL);

                // fill attack table
                for (size_t i = 0; i < occList.size(); ++i) {
                    uint64_t occ = occList[i];
                    uint64_t comp = CompressOccupancy(occ, mi.indices);
                    uint64_t index = (comp * mi.magic) >> (64 - mi.bits);
                    mi.attacks[index] = ref[i];
                }
            }

            void InitMagicTables() {
                if (g_initialized) return;
                g_initialized = true;
                for (int sq = 0; sq < 64; ++sq) {
                    BuildMagicForSquare(sq, true);  // rook
                    BuildMagicForSquare(sq, false); // bishop
                }
            }

            uint64_t RookAttacks(int square, uint64_t occupancy) {
                const MagicInfo& mi = RookInfo[square];
                if (mi.bits == 0) return 0ULL;
            #if defined(HAVE_PEXT)
                // if we have PEXT, just build the mask once and call pext
                uint64_t occ = occupancy & mi.mask;
                // use pext with the mask built from indices (construct mask once per table build)
                // mask is mi.mask, pext compresses according to those bit positions
                uint64_t comp = pext64_hw(occ, mi.mask);
                uint64_t index = (comp * mi.magic) >> (64 - mi.bits);
                return mi.attacks[index];
            #else
                uint64_t occ = occupancy & mi.mask;
                uint64_t comp = CompressOccupancy(occ, mi.indices);
                uint64_t index = (comp * mi.magic) >> (64 - mi.bits);
                return mi.attacks[index];
            #endif
            }

            uint64_t BishopAttacks(int square, uint64_t occupancy) {
                const MagicInfo& mi = BishopInfo[square];
                if (mi.bits == 0) return 0ULL;
            #if defined(HAVE_PEXT)
                uint64_t occ = occupancy & mi.mask;
                uint64_t comp = pext64_hw(occ, mi.mask);
                uint64_t index = (comp * mi.magic) >> (64 - mi.bits);
                return mi.attacks[index];
            #else
                uint64_t occ = occupancy & mi.mask;
                uint64_t comp = CompressOccupancy(occ, mi.indices);
                uint64_t index = (comp * mi.magic) >> (64 - mi.bits);
                return mi.attacks[index];
            #endif
            }

            uint64_t QueenAttacks(int square, uint64_t occupancy) {
                return RookAttacks(square, occupancy) | BishopAttacks(square, occupancy);
            }

        } // namespace Magic
    } // namespace ChessEngine
} // namespace WinChess
