#include "Board.h"
#include "BitUtils.h"
#include "Pieces.h"
#include "Magic.h"

namespace WinChess {
    namespace ChessEngine {
        namespace Board {

            using namespace Pieces;

            // // TT
            // std::vector<TTEntry> TranspositionTable;
            // void TTInit(size_t mb) {
            //     size_t bytes = mb * 1024 * 1024;
            //     size_t entries = bytes / sizeof(TTEntry);
            //     if (entries < 1) entries = 1;
            //     TranspositionTable.assign(entries, TTEntry());
            // }
            // void TTPut(uint64_t key, uint32_t depth, uint32_t value, uint8_t flag) {
            //     if (TranspositionTable.empty()) return;
            //     size_t idx = (key ^ (key >> 32)) % TranspositionTable.size();
            //     TranspositionTable[idx].key = key;
            //     TranspositionTable[idx].depth = depth;
            //     TranspositionTable[idx].value = value;
            //     TranspositionTable[idx].flag = flag;
            // }
            // std::optional<TTEntry> TTProbe(uint64_t key) {
            //     if (TranspositionTable.empty()) return std::nullopt;
            //     size_t idx = (key ^ (key >> 32)) % TranspositionTable.size();
            //     if (TranspositionTable[idx].key == key) return TranspositionTable[idx];
            //     return std::nullopt;
            // }

            // Precomputed non-sliding attack tables
            static const std::array<uint64_t, 64>& KNIGHT_ATTACKS() {
                static std::array<uint64_t, 64> table = []() {
                    std::array<uint64_t, 64> t{};
                    constexpr uint64_t notA     = ~(static_cast<uint64_t>(BitMask::FileA));
                    constexpr uint64_t notAB    = ~(static_cast<uint64_t>(BitMask::FileA) | static_cast<uint64_t>(BitMask::FileB));
                    constexpr uint64_t notH     = ~(static_cast<uint64_t>(BitMask::FileH));
                    constexpr uint64_t notGH    = ~(static_cast<uint64_t>(BitMask::FileG) | static_cast<uint64_t>(BitMask::FileH));
                    for (int idx = 0; idx < 64; ++idx) {
                        uint64_t b = 1ULL << idx;
                        uint64_t m = 0ULL;
                        m |= (b << 17)  & notA;
                        m |= (b << 15)  & notH;
                        m |= (b << 10)  & notAB;
                        m |= (b << 6)   & notGH;
                        m |= (b >> 17)  & notH;
                        m |= (b >> 15)  & notA;
                        m |= (b >> 10)  & notGH;
                        m |= (b >> 6)   & notAB;
                        t[idx] = m;
                    }
                    return t;
                }();
                return table;
            }
            static const std::array<uint64_t, 64>& KING_ATTACKS() {
                static std::array<uint64_t, 64> table = []() {
                    std::array<uint64_t, 64> t{};
                    for (int sq = 0; sq < 64; ++sq) {
                        uint64_t b = 1ULL << sq;
                        const uint64_t notA = ~static_cast<uint64_t>(BitMask::FileA);
                        const uint64_t notH = ~static_cast<uint64_t>(BitMask::FileH);
                        uint64_t m = 0ULL;
                        m |= (b << 8);
                        m |= (b >> 8);
                        m |= (b << 1) & notA;
                        m |= (b >> 1) & notH;
                        m |= (b << 9) & notA;
                        m |= (b << 7) & notH;
                        m |= (b >> 9) & notH;
                        m |= (b >> 7) & notA;
                        t[sq] = m;
                    }
                    return t;
                }();
                return table;
            }
            static const std::array<uint64_t, 64>& PAWN_ATTACKS_WHITE() {
                static std::array<uint64_t, 64> table = []() {
                    std::array<uint64_t, 64> t{};
                    constexpr uint64_t notA = ~static_cast<uint64_t>(BitMask::FileA);
                    constexpr uint64_t notH = ~static_cast<uint64_t>(BitMask::FileH);
                    for (int i = 0; i < 64; ++i) {
                        uint64_t p = 1ULL << i;
                        t[i] = ((p << 7) & notH) | ((p << 9) & notA);
                    }
                    return t;
                }();
                return table;
            }
            static const std::array<uint64_t, 64>& PAWN_ATTACKS_BLACK() {
                static std::array<uint64_t, 64> table = []() {
                    std::array<uint64_t, 64> t{};
                    constexpr uint64_t notA = ~static_cast<uint64_t>(BitMask::FileA);
                    constexpr uint64_t notH = ~static_cast<uint64_t>(BitMask::FileH);
                    for (int i = 0; i < 64; ++i) {
                        uint64_t p = 1ULL << i;
                        t[i] = ((p >> 9) & notH) | ((p >> 7) & notA);
                    }
                    return t;
                }();
                return table;
            }

            // Rebuild caches from bitboards
            void RebuildSquareCaches(BoardState& board) {
                board.SquareColor.fill(-1);
                board.SquareType.fill(-1);
                board.KingSquare.fill(-1);
                for (auto c : PieceColors) {
                    for (auto t : PieceTypes) {
                        uint64_t bb = board.GetBB(c, t);
                        while (bb) {
                            int idx = BitUtils::LSSBIndex(bb);
                            bb &= bb - 1;
                            board.SquareColor[idx]  = static_cast<int8_t>(c);
                            board.SquareType[idx]   = static_cast<int8_t>(t);
                            if (t == PieceType::King) board.KingSquare[static_cast<int>(c)] = idx;
                        }
                    }
                }
            }
            /* Safe function slow */
            bool SafeFindPieceAtSquare(const BoardState& board, int index, Pieces::PieceColor& outColor, Pieces::PieceType& outType) {
                if (index < 0 || index > 63) return false;
                int8_t sc = board.SquareColor[index];
                int8_t st = board.SquareType[index];
                if (sc != -1 && st != -1) {
                    outColor    = static_cast<Pieces::PieceColor>(sc);
                    outType     = static_cast<Pieces::PieceType>(st);
                    return true;
                }
                uint64_t mask = 1ULL << index;
                /* In case of mismatch fallback */
                for (auto c : PieceColors) {
                    for (auto t : PieceTypes) {
                        if (board.GetConstBB(c, t) & mask) {
                            outColor = c; outType = t;
                            return true;
                        }
                    }
                }
                return false;
            }

            void RecomputeScores(BoardState& board) {
                int w = 0, b = 0;
                for (auto pt : PieceTypes) {
                    uint64_t wbb = board.GetBB(PieceColor::White, pt);
                    uint64_t bbb = board.GetBB(PieceColor::Black, pt);
                    int v = PieceTypeScore(pt);
                    w += v * BitUtils::PopCount(wbb);
                    b += v * BitUtils::PopCount(bbb);
                }
                board.WhiteScore = w; board.BlackScore = b;
            }
            void InitializeStartingPosition(BoardState& board) {
                Magic::InitMagicTables();
                board.PieceBB = {};
                board.EnPassantTarget = 0ULL;
                board.CastlingRights = 0x0F;
                board.PromotionStatus = 0x00;
                /* Game Statis */
                board.ActiveColor = PieceColor::White;
                board.GameStatus = GameStatus::Ongoing;
                board.GameOver = false;
                board.Winner = PieceColor::None;
                /* History */
                board.MoveHistory.clear();

                auto place = [&](PieceColor c, PieceType t, int sq) {
                    uint64_t m = 1ULL << sq;
                    board.GetBB(c, t) |= m;
                    board.GetColorOcc(c) |= m;
                };

                for (int f = 0; f < 8; ++f) {
                    place(PieceColor::White, PieceType::Pawn, 8 + f);
                    place(PieceColor::Black, PieceType::Pawn, 48 + f);
                }
                place(PieceColor::White, PieceType::Rook, 0);
                place(PieceColor::White, PieceType::Rook, 7);
                place(PieceColor::Black, PieceType::Rook, 56);
                place(PieceColor::Black, PieceType::Rook, 63);

                place(PieceColor::White, PieceType::Knight, 1);
                place(PieceColor::White, PieceType::Knight, 6);
                place(PieceColor::Black, PieceType::Knight, 57);
                place(PieceColor::Black, PieceType::Knight, 62);

                place(PieceColor::White, PieceType::Bishop, 2);
                place(PieceColor::White, PieceType::Bishop, 5);
                place(PieceColor::Black, PieceType::Bishop, 58);
                place(PieceColor::Black, PieceType::Bishop, 61);

                place(PieceColor::White, PieceType::Queen, 3);
                place(PieceColor::Black, PieceType::Queen, 59);

                place(PieceColor::White, PieceType::King, 4);
                place(PieceColor::Black, PieceType::King, 60);

                RecomputeScores(board);
                RebuildSquareCaches(board);

                // Zobrist: We'll keep a simple scheme (piece keys not implemented here to keep code focused on magic).
                // board.ZobristKey = 0ULL;
                UpdateGameState(board);
            }

            // Non-sliding generators use tables; sliding use magic
            FORCE_INLINE uint64_t GenerateKnightMoves(const BoardState& board, int index) {
                PieceColor color = static_cast<PieceColor>(board.SquareColor[index]);
                uint64_t m = KNIGHT_ATTACKS()[index];
                uint64_t friendly = board.GetConstColorOcc(color);
                return m & ~friendly;
            }
            FORCE_INLINE uint64_t GenerateKingMoves(const BoardState& board, int index) {
                PieceColor color = static_cast<PieceColor>(board.SquareColor[index]);
                uint64_t m = KING_ATTACKS()[index];
                uint64_t friendly = board.GetConstColorOcc(color);
                m &= ~friendly;
                uint64_t occ = board.GetTotalOcc();
                /* Castling */
                if (color == PieceColor::White) {
                    if (board.CastlingRights & static_cast<uint8_t>(CastlingFlags::WhiteKingSide)) {
                        if (!(occ & ((1ULL << 5) | (1ULL << 6)))) m |= (1ULL << 6);
                    }
                    if (board.CastlingRights & static_cast<uint8_t>(CastlingFlags::WhiteQueenSide)) {
                        if (!(occ & ((1ULL << 1) | (1ULL << 2) | (1ULL << 3)))) m |= (1ULL << 2);
                    }
                }
                else {
                    if (board.CastlingRights & static_cast<uint8_t>(CastlingFlags::BlackKingSide)) {
                        if (!(occ & ((1ULL << 61) | (1ULL << 62)))) m |= (1ULL << 62);
                    }
                    if (board.CastlingRights & static_cast<uint8_t>(CastlingFlags::BlackQueenSide)) {
                        if (!(occ & ((1ULL << 57) | (1ULL << 58) | (1ULL << 59)))) m |= (1ULL << 58);
                    }
                }
                return m;
            }

            FORCE_INLINE uint64_t GenerateRookMoves(const BoardState& board, int index) {
                PieceColor color = static_cast<PieceColor>(board.SquareColor[index]);
                uint64_t occ = board.GetTotalOcc();
                uint64_t att = Magic::RookAttacks(index, occ);
                uint64_t friendly = board.GetConstColorOcc(color);
                return att & ~friendly;
            }
            FORCE_INLINE uint64_t GenerateBishopMoves(const BoardState& board, int index) {
                PieceColor color = static_cast<PieceColor>(board.SquareColor[index]);
                uint64_t occ = board.GetTotalOcc();
                uint64_t att = Magic::BishopAttacks(index, occ);
                uint64_t friendly = board.GetConstColorOcc(color);
                return att & ~friendly;
            }
            FORCE_INLINE uint64_t GenerateQueenMoves(const BoardState& board, int index) {
                PieceColor color = static_cast<PieceColor>(board.SquareColor[index]);
                uint64_t occ = board.GetTotalOcc();
                uint64_t att = Magic::QueenAttacks(index, occ);
                uint64_t friendly = board.GetConstColorOcc(color);
                return att & ~friendly;
            }

            FORCE_INLINE uint64_t GeneratePawnMoves(const BoardState& board, int index) {
                PieceColor color = static_cast<PieceColor>(board.SquareColor[index]);
                uint64_t pawn = 1ULL << index;
                uint64_t moves = 0ULL;
                uint64_t occ = board.GetTotalOcc();
                if (color == PieceColor::White) {
                    uint64_t step = pawn << 8;
                    if (!(step & occ)) {
                        moves |= step;
                        if ((pawn & static_cast<uint64_t>(BitMask::Rank2)) && !((pawn << 16) & occ)) moves |= (pawn << 16); // two step at beginning
                    }
                    moves |= PAWN_ATTACKS_WHITE()[index] & board.GetConstColorOcc(PieceColor::Black); // capture
                    if (board.EnPassantTarget) moves |= (PAWN_ATTACKS_WHITE()[index] & board.EnPassantTarget); // en passant
                }
                else {
                    uint64_t step = pawn >> 8;
                    if (!(step & occ)) {
                        moves |= step;
                        if ((pawn & static_cast<uint64_t>(BitMask::Rank7)) && !((pawn >> 16) & occ)) moves |= (pawn >> 16);
                    }
                    moves |= PAWN_ATTACKS_BLACK()[index] & board.GetConstColorOcc(PieceColor::White);
                    if (board.EnPassantTarget) moves |= (PAWN_ATTACKS_BLACK()[index] & board.EnPassantTarget);
                }
                // uint64_t friendly = board.BitBoards[static_cast<int>(color)][static_cast<int>(PieceType::EveryPiece)];
                // return moves & ~friendly;
                return moves;
            }

            FORCE_INLINE uint64_t GeneratePseudoMoves(const BoardState& board, int index) {
                switch (static_cast<PieceType>(board.SquareType[index])) {
                    case PieceType::Pawn:   return GeneratePawnMoves(board, index);
                    case PieceType::Knight: return GenerateKnightMoves(board, index);
                    case PieceType::Bishop: return GenerateBishopMoves(board, index);
                    case PieceType::Rook:   return GenerateRookMoves(board, index);
                    case PieceType::Queen:  return GenerateQueenMoves(board, index);
                    case PieceType::King:   return GenerateKingMoves(board, index);
                    default: return 0ULL;
                }
            }


            // ---------- helpers for ray stepping / lines ----------
            static inline int NextSquareInDir(int sq, int step) {
                // step values used: +8, -8, +1, -1, +9, +7, -7, -9
                switch (step) {
                    case 8:  return (sq <= 55) ? (sq + 8) : -1;
                    case -8: return (sq >= 8)  ? (sq - 8) : -1;
                    case 1:  return ((sq & 7) != 7) ? (sq + 1) : -1;
                    case -1: return ((sq & 7) != 0) ? (sq - 1) : -1;
                    case 9:  return ((sq & 7) != 7 && sq <= 54) ? (sq + 9) : -1;
                    case 7:  return ((sq & 7) != 0 && sq <= 55) ? (sq + 7) : -1;
                    case -7: return ((sq & 7) != 7 && sq >= 7) ? (sq - 7) : -1;
                    case -9: return ((sq & 7) != 0 && sq >= 9) ? (sq - 9) : -1;
                    default: return -1;
                }
            }

            static inline uint64_t LineMaskInclusive(int a, int b) {
                // returns bitboard of all squares on the straight line connecting a..b inclusive
                if (a == b) return 1ULL << a;
                int dr = (b / 8) - (a / 8);
                int df = (b % 8) - (a % 8);
                int step = 0;
                if (dr == 0) step = (df > 0) ? 1 : -1;
                else if (df == 0) step = (dr > 0) ? 8 : -8;
                else if (dr == df) step = (dr > 0) ? 9 : -9;
                else if (dr == -df) step = (dr > 0) ? 7 : -7;
                else return 0ULL; // not on same line

                uint64_t mask = 0ULL;
                int s = a;
                while (s != -1) {
                    mask |= (1ULL << s);
                    if (s == b) break;
                    s = NextSquareInDir(s, step);
                }
                return mask;
            }

            static inline uint64_t BetweenMaskExclusive(int a, int b) {
                // squares strictly between a and b along the straight line (exclusive)
                uint64_t inc = LineMaskInclusive(a, b);
                return inc & ~(1ULL << a) & ~(1ULL << b);
            }

            // KingState analysis: compute check count, checkers bitboard, block mask and pinned-allowed masks
            KingState AnalyzeKingState(const BoardState& board, PieceColor side) {
                using namespace Pieces;
                KingState ks;
                ks.PinnedAllowed.fill(~0ULL);
                ks.CheckCount = 0;
                ks.Checkers = 0ULL;
                ks.BlockMask = 0ULL;
                ks.KingIndex = board.KingSquare[static_cast<int>(side)];
                if (ks.KingIndex < 0) return ks; // invalid or absent king (defensive)

                PieceColor opp = OtherColor(side);
                uint64_t occ = board.GetTotalOcc();
                int ksq = ks.KingIndex;

                // pawn checks (use same logic as SquareAttackedBy)
                if (opp == PieceColor::White) {
                    uint64_t pawns = PAWN_ATTACKS_BLACK()[ksq] & board.GetConstBB(PieceColor::White, PieceType::Pawn);
                    if (pawns) { ks.CheckCount += BitUtils::PopCount(pawns); ks.Checkers |= pawns; ks.BlockMask |= pawns; }
                } else {
                    uint64_t pawns = PAWN_ATTACKS_WHITE()[ksq] & board.GetConstBB(PieceColor::Black, PieceType::Pawn);
                    if (pawns) { ks.CheckCount += BitUtils::PopCount(pawns); ks.Checkers |= pawns; ks.BlockMask |= pawns; }
                }

                // knight checks
                {
                    uint64_t knights = KNIGHT_ATTACKS()[ksq] & board.GetConstBB(opp, PieceType::Knight);
                    if (knights) { ks.CheckCount += BitUtils::PopCount(knights); ks.Checkers |= knights; ks.BlockMask |= knights; }
                }
                // king adjacency (rare)
                {
                    uint64_t kings = KING_ATTACKS()[ksq] & board.GetConstBB(opp, PieceType::King);
                    if (kings) { ks.CheckCount += BitUtils::PopCount(kings); ks.Checkers |= kings; ks.BlockMask |= kings; }
                }

                // sliding rays (8 directions)
                const int steps[8] = { 8, -8, 1, -1, 9, 7, -7, -9 };
                for (int d = 0; d < 8; ++d) {
                    int step = steps[d];
                    bool isDiagonal = (d >= 4);
                    int s = NextSquareInDir(ksq, step);
                    if (s == -1) continue;
                    int first = -1;
                    int second = -1;
                    // scan outwards ---- find up to two occupied squares
                    for (; s != -1; s = NextSquareInDir(s, step)) {
                        uint64_t m = (1ULL << s);
                        if (occ & m) {
                            if (first == -1) { first = s; continue; }
                            else { second = s; break; }
                        }
                    }
                    if (first == -1) continue; // no blocking pieces on this ray

                    // if there is only 'first' and no 'second' -> first may be direct attacker
                    if (second == -1) {
                        int sc = board.SquareColor[first];
                        int st = board.SquareType[first];
                        if (sc == static_cast<int>(opp)) {
                            // is sliding attacker? rook/queen on orthogonal, bishop/queen on diagonal
                            if ((!isDiagonal && (st == static_cast<int>(PieceType::Rook) || st == static_cast<int>(PieceType::Queen))) ||
                                (isDiagonal && (st == static_cast<int>(PieceType::Bishop) || st == static_cast<int>(PieceType::Queen)))) {
                                uint64_t mm = (1ULL << first);
                                ks.CheckCount += 1;
                                ks.Checkers |= mm;
                                ks.BlockMask |= mm;
                            }
                        }
                        continue;
                    }

                    // two pieces found: first (nearest to king), second (beyond first)
                    int fc = board.SquareColor[first];
                    int sc = board.SquareColor[second];
                    int ft = board.SquareType[first];
                    int stt = board.SquareType[second];

                    // pinned piece case: first is friendly and second is opponent sliding attacker
                    if (fc == static_cast<int>(side) && sc == static_cast<int>(opp)) {
                        bool secondIsSliding = (!isDiagonal && (stt == static_cast<int>(PieceType::Rook) || stt == static_cast<int>(PieceType::Queen)))
                                            || (isDiagonal && (stt == static_cast<int>(PieceType::Bishop) || stt == static_cast<int>(PieceType::Queen)));
                        if (secondIsSliding) {
                            // pinned: allowed destinations for the pinned piece are the line between king and attacker (inclusive attacker)
                            uint64_t line = LineMaskInclusive(ksq, second);
                            // destinations must remain on that line, and cannot include the king square as destination
                            ks.PinnedAllowed[first] = line & ~(1ULL << ksq);
                        }
                    }
                }

                // Build BlockMask (squares between king and each checker inclusive)
                // For each checker bit, union the between mask and checker square itself
                uint64_t chk = ks.Checkers;
                while (chk) {
                    int a = BitUtils::LSSBIndex(chk);
                    chk &= chk - 1;
                    ks.BlockMask |= BetweenMaskExclusive(ksq, a) | (1ULL << a);
                }

                return ks;
            }


            // Instead of computing full enemy attack map, we check king safety by checking rays from king square
            static bool SquareAttackedBy(PieceColor sqSide, int square, const BoardState& board) {
                PieceColor attacker = OtherColor(sqSide);
                // pawn attacks
                if (attacker == PieceColor::White) {
                    /* Need to flip the ray direction */
                    if (PAWN_ATTACKS_BLACK()[square] & board.GetConstBB(PieceColor::White, PieceType::Pawn)) return true;
                }
                else {
                    if (PAWN_ATTACKS_WHITE()[square] & board.GetConstBB(PieceColor::Black, PieceType::Pawn)) return true;
                }
                // knights
                if (KNIGHT_ATTACKS()[square] & board.GetConstBB(attacker, PieceType::Knight)) return true;
                // king
                if (KING_ATTACKS()[square] & board.GetConstBB(attacker, PieceType::King)) return true;
                uint64_t occ = board.GetTotalOcc();
                // sliders: use magic attacks but compare with attacker pieces
                uint64_t rookAtt = Magic::RookAttacks(square, occ);
                if (rookAtt & board.GetConstBB(attacker, PieceType::Rook)) return true;
                if (rookAtt & board.GetConstBB(attacker, PieceType::Queen)) return true;
                uint64_t bishopAtt = Magic::BishopAttacks(square, occ);
                if (bishopAtt & board.GetConstBB(attacker, PieceType::Bishop)) return true;
                if (bishopAtt & board.GetConstBB(attacker, PieceType::Queen)) return true;
                return false;
            }
            bool IsKingSafe(PieceColor color, const BoardState& board) {
                uint64_t kingbb = board.GetConstBB(color, PieceType::King);
                if (!kingbb) return false; // --> Usually king always exists
                int kidx = BitUtils::LSSBIndex(kingbb);
                return !SquareAttackedBy(color, kidx, board);
            }
            uint64_t GetLegalMovesForPiece(BoardState& board, int index) {
                uint64_t legal = 0ULL;
                uint64_t pseudo = GeneratePseudoMoves(board, index);
                PieceColor color = static_cast<PieceColor>(board.SquareColor[index]);
                while (pseudo) {
                    int to = BitUtils::LSSBIndex(pseudo);
                    pseudo &= pseudo - 1;
                    MoveOutcome mo = MakeMove(board, index, to);
                    bool ok = false;
                    ok = IsKingSafe(color, board);
                    int kidx = -1;
                    if (MoveFlagsOf(mo.Move) & static_cast<uint32_t>(MoveFlags::Castling)) {
                        // check valid castling
                        uint64_t kingbb = board.GetBB(color, PieceType::King);
                        kidx = BitUtils::LSSBIndex(kingbb);
                    }
                    UnmakeMove(board, mo);
                    switch (kidx) {
                        case 2:     ok = ok && !SquareAttackedBy(color, 3, board)  && IsKingSafe(color, board); break;
                        case 6:     ok = ok && !SquareAttackedBy(color, 5, board)  && IsKingSafe(color, board); break;
                        case 58:    ok = ok && !SquareAttackedBy(color, 59, board) && IsKingSafe(color, board); break;
                        case 62:    ok = ok && !SquareAttackedBy(color, 61, board) && IsKingSafe(color, board); break;
                    }
                    if (ok) legal |= (1ULL << to);
                }
                return legal;
            }

            // No checking --> fast
            MoveOutcome MakeMove(BoardState& board, int fromIndex, int toIndex) {
                PieceColor moverColor = static_cast<PieceColor>(board.SquareColor[fromIndex]);
                PieceType moverType = static_cast<PieceType>(board.SquareType[fromIndex]);

                MoveOutcome o;
                // o.Success = false;
                o.Move = PackMove(fromIndex, toIndex);

                o.PrevEnPassantTarget = board.EnPassantTarget;
                o.PrevCastlingRights = board.CastlingRights;
                o.PrevPromotionStatus = board.PromotionStatus;
                // o.prevZobrist = board.ZobristKey;

                uint64_t fromMask = 1ULL << fromIndex;
                uint64_t toMask = 1ULL << toIndex;
                PieceColor opp = OtherColor(moverColor);

                // remove mover from source
                board.GetBB(moverColor, moverType) &= ~fromMask;
                board.GetColorOcc(moverColor) &= ~fromMask;
                board.SquareColor[fromIndex] = -1;
                board.SquareType[fromIndex] = -1;

                // Direct Capture?
                if (board.GetColorOcc(opp) & toMask) {
                    o.Move |= static_cast<uint32_t>(MoveFlags::Capture) << 12;
                    o.CapturedType = static_cast<PieceType>(board.SquareType[toIndex]);
                    board.GetBB(opp, board.SquareType[toIndex]) &= ~toMask;
                    board.GetColorOcc(opp) &= ~toMask;
                    // board.BitBoards[static_cast<int>(PieceColor::Both)][static_cast<int>(PieceType::EveryPiece)] &= ~toMask;
                    // board.SquareColor[toIndex] = -1;
                    // board.SquareType[toIndex] = -1;
                    if (opp == PieceColor::White) board.WhiteScore -= PieceTypeScore(o.CapturedType); else board.BlackScore -= PieceTypeScore(o.CapturedType);
                } else if (moverType == PieceType::Pawn && (board.EnPassantTarget & toMask)) {
                    o.Move |= (static_cast<uint32_t>(MoveFlags::Capture) | static_cast<uint32_t>(MoveFlags::EnPassant)) << 12;
                    o.CapturedType = PieceType::Pawn;
                    // en-passant capture
                    int capturedIndex = (moverColor == PieceColor::White) ? (toIndex - 8) : (toIndex + 8);
                    uint64_t capMask = 1ULL << capturedIndex;
                    board.GetBB(opp, PieceType::Pawn) &= ~capMask;
                    board.GetColorOcc(opp) &= ~capMask;
                    board.SquareColor[capturedIndex] = -1;
                    board.SquareType[capturedIndex] = -1;
                    if (opp == PieceColor::White) board.WhiteScore -= 1; else board.BlackScore -= 1;
                }
                
                if (moverType == PieceType::King) {
                    if (moverColor == PieceColor::White) {
                        /* Remove Castling Rights */
                        board.CastlingRights &= ~(static_cast<uint8_t>(CastlingFlags::WhiteKingSide) | static_cast<uint8_t>(CastlingFlags::WhiteQueenSide));
                        if (fromIndex == 4 && toIndex == 6) { // O-O
                            o.Move |= static_cast<uint32_t>(MoveFlags::Castling) << 12;
                            uint64_t rfrom = 1ULL << 7, rto = 1ULL << 5;
                            (board.GetBB(PieceColor::White, PieceType::Rook) &= ~rfrom) |= rto;
                            (board.GetColorOcc(PieceColor::White) &= ~rfrom) |= rto;
                            board.SquareColor[7] = -1;
                            board.SquareType[7] = -1;
                            board.SquareColor[5] = static_cast<int8_t>(PieceColor::White);
                            board.SquareType[5] = static_cast<int8_t>(PieceType::Rook);
                        } else if (fromIndex == 4 && toIndex == 2) { // O-O-O
                            o.Move |= static_cast<uint32_t>(MoveFlags::Castling) << 12;
                            uint64_t rfrom = 1ULL << 0, rto = 1ULL << 3;
                            (board.GetBB(PieceColor::White, PieceType::Rook) &= ~rfrom) |= rto;
                            (board.GetColorOcc(PieceColor::White) &= ~rfrom) |= rto;
                            board.SquareColor[0] = -1;
                            board.SquareType[0] = -1;
                            board.SquareColor[3] = static_cast<int8_t>(PieceColor::White);
                            board.SquareType[3] = static_cast<int8_t>(PieceType::Rook);
                        }
                    }
                    else {
                        board.CastlingRights &= ~(static_cast<uint8_t>(CastlingFlags::BlackKingSide) | static_cast<uint8_t>(CastlingFlags::BlackQueenSide));
                        if (fromIndex == 60 && toIndex == 62) { // O-O
                            o.Move |= static_cast<uint32_t>(MoveFlags::Castling) << 12;
                            uint64_t rfrom = 1ULL << 63, rto = 1ULL << 61;
                            (board.GetBB(PieceColor::Black, PieceType::Rook) &= ~rfrom) |= rto;
                            (board.GetColorOcc(PieceColor::Black) &= ~rfrom) |= rto;
                            board.SquareColor[63] = -1;
                            board.SquareType[63] = -1;
                            board.SquareColor[61] = static_cast<int8_t>(PieceColor::Black);
                            board.SquareType[61] = static_cast<int8_t>(PieceType::Rook);
                        } else if (fromIndex == 60 && toIndex == 58) { // O-O-O
                            o.Move |= static_cast<uint32_t>(MoveFlags::Castling) << 12;
                            uint64_t rfrom = 1ULL << 56, rto = 1ULL << 59;
                            (board.GetBB(PieceColor::Black, PieceType::Rook) &= ~rfrom) |= rto;
                            (board.GetColorOcc(PieceColor::Black) &= ~rfrom) |= rto;
                            board.SquareColor[56] = -1;
                            board.SquareType[56] = -1;
                            board.SquareColor[59] = static_cast<int8_t>(PieceColor::Black);
                            board.SquareType[59] = static_cast<int8_t>(PieceType::Rook);
                        }
                    }
                }

                // Rook moved affects castling
                if (moverType == PieceType::Rook) {
                    if (moverColor == PieceColor::White) {
                        if (fromIndex == 0) board.CastlingRights &= ~static_cast<uint8_t>(CastlingFlags::WhiteQueenSide);
                        if (fromIndex == 7) board.CastlingRights &= ~static_cast<uint8_t>(CastlingFlags::WhiteKingSide);
                    }
                    else {
                        if (fromIndex == 56) board.CastlingRights &= ~static_cast<uint8_t>(CastlingFlags::BlackQueenSide);
                        if (fromIndex == 63) board.CastlingRights &= ~static_cast<uint8_t>(CastlingFlags::BlackKingSide);
                    }
                }
                // captured rook affects opponent castling
                if ((o.Move & (static_cast<uint32_t>(MoveFlags::Capture) << 12)) && o.CapturedType == PieceType::Rook) {
                    if (opp == PieceColor::White) {
                        if (toIndex == 0) board.CastlingRights &= ~static_cast<uint8_t>(CastlingFlags::WhiteQueenSide);
                        if (toIndex == 7) board.CastlingRights &= ~static_cast<uint8_t>(CastlingFlags::WhiteKingSide);
                    }
                    else {
                        if (toIndex == 56) board.CastlingRights &= ~static_cast<uint8_t>(CastlingFlags::BlackQueenSide);
                        if (toIndex == 63) board.CastlingRights &= ~static_cast<uint8_t>(CastlingFlags::BlackKingSide);
                    }
                }

                // place mover on destination
                board.GetBB(moverColor, moverType) |= toMask;
                board.GetColorOcc(moverColor) |= toMask;
                board.SquareColor[toIndex] = static_cast<int8_t>(moverColor);
                board.SquareType[toIndex] = static_cast<int8_t>(moverType);

                // en-passant update: clear old EP file, set new if double pawn push
                board.EnPassantTarget = 0ULL;
                if (moverType == PieceType::Pawn) {
                    if (moverColor == PieceColor::White) {
                        // if ((fromIndex / 8) == 1 && (toIndex / 8) == 3) board.EnPassantTarget = 1ULL << (fromIndex + 8);
                        if ((fromIndex >> 3) == 1 && (toIndex >> 3) == 3) board.EnPassantTarget = 1ULL << (fromIndex + 8);
                    }
                    else {
                        // if ((fromIndex / 8) == 6 && (toIndex / 8) == 4) board.EnPassantTarget = 1ULL << (fromIndex - 8);
                        if ((fromIndex >> 3) == 6 && (toIndex >> 3) == 4) board.EnPassantTarget = 1ULL << (fromIndex - 8);
                    }
                }

                // promotion pending
                if (moverType == PieceType::Pawn) {
                    int tr = toIndex >> 3;
                    if ((moverColor == PieceColor::White && tr == 7) || (moverColor == PieceColor::Black && tr == 0)) {
                        o.Move |= static_cast<uint32_t>(MoveFlags::Promotion) << 12;
                        board.PromotionStatus = PackPromotion(true, toIndex, moverColor);
                    }
                }
                // switch side
                board.ActiveColor = OtherColor(board.ActiveColor);
                // o.Success = true;
                return o;
            }
            void UnmakeMove(BoardState& board, const MoveOutcome& outcome) {
                // if (!outcome.success) return;
                // PieceColor moverColor = outcome.moverColor;
                // PieceType moverType = outcome.moverType;
                int fromIndex = MoveFrom(outcome.Move);
                int toIndex = MoveTo(outcome.Move);
                uint64_t fromMask = 1ULL << fromIndex;
                uint64_t toMask = 1ULL << toIndex;
                PieceColor moverColor = static_cast<PieceColor>(board.SquareColor[toIndex]);
                PieceColor opp = OtherColor(moverColor);
                PieceType moverType = static_cast<PieceType>(board.SquareType[toIndex]);
                
                board.ActiveColor = OtherColor(board.ActiveColor);
                // Promotion reverse
                if (outcome.Move & static_cast<uint32_t>(MoveFlags::Promotion) << 12) {
                    if (!PromotionPending(board.PromotionStatus)) { // if promotion is successfully finished
                        PieceColor promoColor = PromotionColor(board.PromotionStatus);
                        PieceType promoType = MovePromoType(outcome.Move);
                        int promoSquareIndex = PromotionIndex(board.PromotionStatus);
                        uint64_t promoMask = 1ULL << promoSquareIndex;
                        board.GetBB(promoColor, promoType) &= ~promoMask;
                        board.GetBB(promoColor, PieceType::Pawn) |= promoMask;
                        board.SquareType[promoSquareIndex] = static_cast<int>(PieceType::Pawn);
                        if (promoColor == PieceColor::White) board.WhiteScore -= (PieceTypeScore(promoType) - PieceTypeScore(PieceType::Pawn));
                        else board.BlackScore -= (PieceTypeScore(promoType) - PieceTypeScore(PieceType::Pawn));
                        moverType = PieceType::Pawn;
                    }
                }
                // Remove from destination
                board.GetBB(moverColor, moverType) &= ~toMask;
                board.GetColorOcc(moverColor) &= ~toMask;
                board.SquareColor[toIndex] = -1;
                board.SquareType[toIndex] = -1;

                if(outcome.Move & static_cast<uint32_t>(MoveFlags::Castling) << 12) { // castling happened
                    if (moverColor == PieceColor::White) {
                        if (fromIndex == 4 && toIndex == 6) { // O-O
                            uint64_t rfrom = 1ULL << 7, rto = 1ULL << 5;
                            (board.GetBB(PieceColor::White, PieceType::Rook) |= rfrom) &= ~rto;
                            (board.GetColorOcc(PieceColor::White) |= rfrom) &= ~rto;
                            board.SquareColor[7] = static_cast<int8_t>(PieceColor::White);
                            board.SquareType[7] = static_cast<int8_t>(PieceType::Rook);
                            board.SquareColor[5] = -1;
                            board.SquareType[5] = -1;
                        } else if (fromIndex == 4 && toIndex == 2) { // O-O-O
                            uint64_t rfrom = 1ULL << 0, rto = 1ULL << 3;
                            (board.GetBB(PieceColor::White, PieceType::Rook) |= rfrom) &= ~rto;
                            (board.GetColorOcc(PieceColor::White) |= rfrom) &= ~rto;
                            board.SquareColor[0] = static_cast<int8_t>(PieceColor::White);
                            board.SquareType[0] = static_cast<int8_t>(PieceType::Rook);
                            board.SquareColor[3] = -1;
                            board.SquareType[3] = -1;
                        }
                    }
                    else {
                        if (fromIndex == 60 && toIndex == 62) { // O-O
                            uint64_t rfrom = 1ULL << 63, rto = 1ULL << 61;
                            (board.GetBB(PieceColor::Black, PieceType::Rook) |= rfrom) &= ~rto;
                            (board.GetColorOcc(PieceColor::Black) |= rfrom) &= ~rto;
                            board.SquareColor[63] = static_cast<int8_t>(PieceColor::Black);
                            board.SquareType[63] = static_cast<int8_t>(PieceType::Rook);
                            board.SquareColor[61] = -1;
                            board.SquareType[61] = -1;
                        }
                        if (fromIndex == 60 && toIndex == 58) { // O-O-O
                            uint64_t rfrom = 1ULL << 56, rto = 1ULL << 59;
                            (board.GetBB(PieceColor::Black, PieceType::Rook) |= rfrom) &= ~rto;
                            (board.GetColorOcc(PieceColor::Black) |= rfrom) &= ~rto;
                            board.SquareColor[56] = static_cast<int8_t>(PieceColor::Black);
                            board.SquareType[56] = static_cast<int8_t>(PieceType::Rook);
                            board.SquareColor[59] = -1;
                            board.SquareType[59] = -1;
                        }
                    }
                } else if (outcome.Move & static_cast<uint32_t>(MoveFlags::EnPassant) << 12) {
                    // en-passant capture
                    int capturedIndex = (moverColor == PieceColor::White) ? (toIndex - 8) : (toIndex + 8);
                    uint64_t capMask = 1ULL << capturedIndex;
                    board.GetBB(opp, PieceType::Pawn) |= capMask;
                    board.GetColorOcc(opp) |= capMask;
                    board.SquareColor[capturedIndex] = static_cast<int>(opp);
                    board.SquareType[capturedIndex] = static_cast<int>(PieceType::Pawn);
                    if (opp == PieceColor::White) board.WhiteScore += 1; else board.BlackScore += 1;
                } else if (outcome.Move & static_cast<uint32_t>(MoveFlags::Capture) << 12) {
                    board.GetBB(opp, outcome.CapturedType) |= toMask;
                    board.GetColorOcc(opp) |= toMask;
                    board.SquareColor[toIndex] = static_cast<int>(opp);
                    board.SquareType[toIndex] = static_cast<int>(outcome.CapturedType);
                    if (opp == PieceColor::White) board.WhiteScore += PieceTypeScore(outcome.CapturedType); else board.BlackScore += PieceTypeScore(outcome.CapturedType);
                }
                board.GetBB(moverColor, moverType) |= fromMask;
                board.GetColorOcc(moverColor) |= fromMask;
                board.SquareColor[fromIndex] = static_cast<int>(moverColor);
                board.SquareType[fromIndex] = static_cast<int>(moverType);

                board.PromotionStatus = outcome.PrevPromotionStatus;
                board.EnPassantTarget = outcome.PrevEnPassantTarget;
                board.CastlingRights = outcome.PrevCastlingRights;
            }

            /* This function is unsafe, assuming that the rawMO is the last move and board's promotion pending is true is to be done */
            bool ApplyPromotion(BoardState& board, MoveOutcome& rawMO, PieceType promoteTo) {
                int idx = PromotionIndex(board.PromotionStatus);
                uint64_t mask = 1ULL << idx;
                PieceColor color = PromotionColor(board.PromotionStatus);
                // Remove pawn
                board.GetBB(color, PieceType::Pawn) &= ~mask;
                // board.BitBoards[static_cast<int>(color)][static_cast<int>(PieceType::EveryPiece)] &= ~mask;
                // board.BitBoards[static_cast<int>(PieceColor::Both)][static_cast<int>(PieceType::EveryPiece)] &= ~mask;
                // Add promoted piece
                board.GetBB(color, promoteTo) |= mask;
                // board.BitBoards[static_cast<int>(color)][static_cast<int>(PieceType::EveryPiece)] |= mask;
                // board.BitBoards[static_cast<int>(PieceColor::Both)][static_cast<int>(PieceType::EveryPiece)] |= mask;

                board.SquareType[idx] = static_cast<int8_t>(promoteTo);
                if (color == PieceColor::White) board.WhiteScore += PieceTypeScore(promoteTo) - PieceTypeScore(PieceType::Pawn);
                else board.BlackScore += PieceTypeScore(promoteTo) - PieceTypeScore(PieceType::Pawn);
                
                board.PromotionStatus = PackPromotion(false, idx, color);
                rawMO.Move = PackMove(MoveFrom(rawMO.Move), MoveTo(rawMO.Move), MoveFlagsOf(rawMO.Move), promoteTo);
                return true;
            }
            /* UnSafe: Can only called directly after promotion */
            void RevertPromotion(BoardState& board) {
                int idx = PromotionIndex(board.PromotionStatus);
                uint64_t mask = 1ULL << idx;
                PieceColor color = PromotionColor(board.PromotionStatus);

                board.PromotionStatus = PackPromotion(true, idx, color);
                if (color == PieceColor::White) board.WhiteScore -= PieceTypeScore(static_cast<PieceType>(board.SquareType[idx])) - PieceTypeScore(PieceType::Pawn);
                else board.BlackScore -= PieceTypeScore(static_cast<PieceType>(board.SquareType[idx])) - PieceTypeScore(PieceType::Pawn);
                
                // Remove promoted piece
                board.GetBB(color, board.SquareType[idx]) &= ~mask;
                // Add pawn
                board.GetBB(color, PieceType::Pawn) |= mask;

                board.SquareType[idx] = static_cast<int8_t>(PieceType::Pawn);
            }

            void UpdateGameState(BoardState& board) {
                if (PromotionPending(board.PromotionStatus)) {
                    board.GameStatus = GameStatus::Ongoing;
                    board.GameOver = false;
                    board.Winner = PieceColor::None;
                    return;
                }
                PieceColor stm = board.ActiveColor;
                uint64_t kingbb = board.GetBB(stm, PieceType::King);
                if (!kingbb) {
                    board.GameStatus = GameStatus::Checkmate; board.GameOver = true;
                    board.Winner = OtherColor(stm);
                    return;
                }
                int kidx = BitUtils::LSSBIndex(kingbb);
                bool inCheck = SquareAttackedBy(stm, kidx, board);
                bool anyLegal = false;
                // for (auto pt : PieceTypes) {
                uint64_t bb = board.GetColorOcc(stm);
                while (bb) {
                    int idx = BitUtils::LSSBIndex(bb);
                    bb &= bb - 1;
                    uint64_t leg = GetLegalMovesForPiece(board, idx);
                    if (leg) { anyLegal = true; break; }
                }
                if (anyLegal) {
                    board.GameStatus = inCheck ? GameStatus::Check : GameStatus::Ongoing;
                    board.GameOver = false;
                    board.Winner = PieceColor::None;
                }
                else {
                    if (inCheck) {
                        board.GameStatus = GameStatus::Checkmate; board.GameOver = true;
                        board.Winner = OtherColor(stm);
                    }
                    else {
                        board.GameStatus = GameStatus::Stalemate; board.GameOver = true;
                        board.Winner = PieceColor::None;
                    }
                }
            }

            MoveOutcome SafeMovePiece(BoardState& board, int fromIndex, int toIndex) {
                MoveOutcome ret;
                PieceColor moverColor; PieceType moverType;
                if (!SafeFindPieceAtSquare(board, fromIndex, moverColor, moverType)) return ret;
                if (board.GameOver) return ret;
                if (moverColor != board.ActiveColor) return ret;
                uint64_t legal = GetLegalMovesForPiece(board, fromIndex);
                uint64_t toMask = 1ULL << toIndex;
                if (!(legal & toMask)) return ret;
                MoveOutcome mo = MakeMove(board, fromIndex, toIndex);
                board.MoveHistory.push_back(IndexToUCI(fromIndex) + IndexToUCI(toIndex));
                UpdateGameState(board);
                return mo;
            }
            bool SafeApplyPromotion(BoardState& board, MoveOutcome& rawMO, PieceType promoteTo) {
                if (!PromotionPending(board.PromotionStatus)) return false;
                bool result = ApplyPromotion(board, rawMO, promoteTo);
                UpdateGameState(board);
                return result;
            }

        } // namespace Board
    } // namespace ChessEngine
} // namespace WinChess
