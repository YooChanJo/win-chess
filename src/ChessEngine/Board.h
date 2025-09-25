#pragma once
#include "Pieces.h"
#include "Magic.h"

/* 
    TODO: Split code for each color to remove color branches using templates
*/

namespace WinChess {
    namespace ChessEngine {
        namespace Board {

            // FORCE_INLINE macro
#if defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline __attribute__((always_inline))
#endif

#if defined(_MSC_VER)
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#else
#define LIKELY(x) __builtin_expect(!!(x),1)
#define UNLIKELY(x) __builtin_expect(!!(x),0)
#endif

            using namespace Pieces;

            enum class BitMask : uint64_t {
                FileA = 0x0101010101010101ULL,
                FileB = 0x0202020202020202ULL,
                FileC = 0x0404040404040404ULL,
                FileD = 0x0808080808080808ULL,
                FileE = 0x1010101010101010ULL,
                FileF = 0x2020202020202020ULL,
                FileG = 0x4040404040404040ULL,
                FileH = 0x8080808080808080ULL,

                Rank1 = 0x00000000000000FFULL,
                Rank2 = 0x000000000000FF00ULL,
                Rank3 = 0x0000000000FF0000ULL,
                Rank4 = 0x00000000FF000000ULL,
                Rank5 = 0x000000FF00000000ULL,
                Rank6 = 0x0000FF0000000000ULL,
                Rank7 = 0x00FF000000000000ULL,
                Rank8 = 0xFF00000000000000ULL,
            };

            enum class GameStatus {
                Ongoing,
                Check,
                Checkmate,
                Stalemate,
                Draw
            };

            enum class CastlingFlags: uint8_t {
                WhiteKingSide   = 1 << 0,   // 0001
                WhiteQueenSide  = 1 << 1,   // 0010
                BlackKingSide   = 1 << 2,   // 0100
                BlackQueenSide  = 1 << 3,   // 1000
            };
            FORCE_INLINE uint8_t PackPromotion(bool promotionPending, int index = 0, PieceColor promotionColor = PieceColor::White) {
                return static_cast<uint8_t>((promotionPending & 0x1) | ((index & 0x3F) << 1) | ((static_cast<uint8_t>(promotionColor) & 0x1) << 7));
            }
            FORCE_INLINE bool PromotionPending(uint8_t status) { return status & 0x1; }
            FORCE_INLINE int PromotionIndex(uint8_t status) { return (status >> 1) & 0x3F; }
            FORCE_INLINE PieceColor PromotionColor(uint8_t status) { return static_cast<PieceColor>((status >> 7) & 0x1); }
            struct BoardState {
                std::array<uint64_t, 14> PieceBB{}; // zero-initialized
                /* 
                    index 0 ~ 5: White Pawn Knight Bishop Rook Queen King
                    index 6 ~ 11: Black same
                    index 12: White Occupation
                    index 13: Black Occupation
                */
                uint64_t EnPassantTarget = 0ULL;
                uint8_t CastlingRights = 0x0F;
                uint8_t PromotionStatus = 0x00;
                /* 
                    PromotionColor          [1 bit] // PieceColor
                    PromotionSquareIndex    [6 bits]
                    (bool)PromotionPending  [1 bit]
                */
                /* Game Status */
                PieceColor ActiveColor = PieceColor::White;
                bool GameOver = false;
                GameStatus GameStatus = GameStatus::Ongoing;
                PieceColor Winner = PieceColor::None;

                int WhiteScore = 39;
                int BlackScore = 39;

                // Possible change this to a vector of a struct containing all information for full reversability
                std::vector<std::string> MoveHistory;

                /* Cache for fast reading postions */
                std::array<int8_t, 64> SquareColor{}; // -1 refers to as empty
                std::array<int8_t, 64> SquareType{};  // -1 refers to as empty

                /* Fast king index for each color (White=0, Black=1). -1 if not found. */
                std::array<int, 2> KingSquare = { -1, -1 };

                /* For Transposition Tables */
                // uint64_t ZobristKey = 0ULL;
                
                inline uint64_t& GetBB(PieceColor c, PieceType t) { return PieceBB[static_cast<int>(c) * 6 + static_cast<int>(t)]; }
                inline uint64_t& GetBB(int c, int t) { return PieceBB[c * 6 + t]; }
                inline uint64_t& GetBB(PieceColor c, int t) { return PieceBB[static_cast<int>(c) * 6 + t]; }
                inline uint64_t& GetBB(int c, PieceColor t) { return PieceBB[c * 6 + static_cast<int>(t)]; }

                inline uint64_t GetConstBB(PieceColor c, PieceType t) const { return PieceBB[static_cast<int>(c) * 6 + static_cast<int>(t)]; }
                inline uint64_t GetConstBB(int c, int t) const { return PieceBB[c * 6 + t]; }
                inline uint64_t GetConstBB(PieceColor c, int t) { return PieceBB[static_cast<int>(c) * 6 + t]; }
                inline uint64_t GetConstBB(int c, PieceColor t) { return PieceBB[c * 6 + static_cast<int>(t)]; }

                inline uint64_t& GetColorOcc(PieceColor c) { return PieceBB[12 + static_cast<int>(c)]; }
                inline uint64_t& GetColorOcc(int c) { return PieceBB[12 + c]; }

                inline uint64_t GetConstColorOcc(PieceColor c) const { return PieceBB[12 + static_cast<int>(c)]; }
                inline uint64_t GetConstColorOcc(int c) const { return PieceBB[12 + c]; }

                inline uint64_t GetTotalOcc() const { return PieceBB[12] | PieceBB[13]; }
            };
            // King safety + pin info (small struct returned once per node)
            struct KingState {
                int KingIndex = -1;               // king square index
                int CheckCount = 0;               // number of distinct checking pieces
                uint64_t Checkers = 0ULL;         // bitboard of checking pieces
                uint64_t BlockMask = 0ULL;        // union of squares that can block the check (between king and checker, includes checker)
                std::array<uint64_t, 64> PinnedAllowed; // per-from-square mask of allowed destinations if pinned (else ~0ULL)
            };

            // Analyze the current board for pins/checks (call once per node for ActiveColor)
            KingState AnalyzeKingState(const BoardState& board, Pieces::PieceColor side);

            // New fast legal-move generator that **uses** precomputed KingState (avoids Make/Unmake per-pseudo-move)
            uint64_t GetLegalMovesForPiece(BoardState& board, int index, const KingState& ks);

            // Packed move utility
            enum class MoveFlags : uint32_t {
                None        = 0,        // 0000
                Capture     = 1 << 0,   // 0001
                EnPassant   = 1 << 1,   // 0010
                Castling    = 1 << 2,   // 0100
                Promotion   = 1 << 3    // 1000
            };
            FORCE_INLINE uint32_t PackMove(int from, int to, uint32_t flags = 0, PieceType promoType = PieceType::Knight) {
                return static_cast<uint32_t>((from & 0x3F) | ((to & 0x3F) << 6) | ((flags & 0xF) << 12) | ((static_cast<uint32_t>(promoType) - 1) & 0x3) << 16);
                /*
                    UNUSED      [14 bits]
                    Promotion   [2 bits] PieceType - 1 stored
                    MoveFlag    [4 bits] MovFlags
                    To          [6 bits] 0 ~ 63, square index
                    From        [6 bits] 0 ~ 63, square index                
                */
            }
            FORCE_INLINE int MoveFrom(uint32_t move) { return move & 0x3F; }
            FORCE_INLINE int MoveTo(uint32_t move) { return (move >> 6) & 0x3F; }
            FORCE_INLINE uint32_t MoveFlagsOf(uint32_t move) { return (move >> 12) & 0xF; }
            FORCE_INLINE PieceType MovePromoType(uint32_t move) { return static_cast<PieceType>(((move >> 16) & 0x3) + 1); }

            struct MoveOutcome {
                uint32_t Move; // promoType is not to be read since it would not be set
                // bool Success = false;
                Pieces::PieceType CapturedType = Pieces::PieceType::Pawn; // For restoring

                /* Previous storage */
                uint64_t PrevEnPassantTarget = 0ULL;
                uint8_t PrevCastlingRights = 0x0F;
                uint8_t PrevPromotionStatus = 0x00; 
                // uint64_t PrevZobristKey = 0ULL;
                
                // bool DidCapture = false;
                // int CapturedIndex = -1;
                // bool promotionRequired = false;
                // int PrevWhiteScore = 0;
                // int PrevBlackScore = 0;
                // Pieces::PieceColor moverColor = Pieces::PieceColor::White;
                // Pieces::PieceType moverType = Pieces::PieceType::Pawn;
            };

            /* API util functions */
            void InitializeStartingPosition(BoardState& board);
            void RecomputeScores(BoardState& board);
            void RebuildSquareCaches(BoardState& board);

            /* Safe util function slow */
            bool SafeFindPieceAtSquare(const BoardState& board, int index, Pieces::PieceColor& outColor, Pieces::PieceType& outType);

            uint64_t GeneratePseudoMoves(const BoardState& board, int index);
            uint64_t GenerateKnightMoves(const BoardState& board, int index);
            uint64_t GenerateKingMoves(const BoardState& board, int index);
            uint64_t GenerateRookMoves(const BoardState& board, int index);
            uint64_t GenerateBishopMoves(const BoardState& board, int index);
            uint64_t GenerateQueenMoves(const BoardState& board, int index);
            uint64_t GeneratePawnMoves(const BoardState& board, int index);

            bool IsKingSafe(Pieces::PieceColor color, const BoardState& board);

            /* Does not handle promotion --> up to pending, for node search one needs to implement promotion by itself */
            /* Later On Perhaps discard double checks since it is more efficient */
            uint64_t GetLegalMovesForPiece(BoardState& board, int index);

            MoveOutcome MakeMove(BoardState& board, int fromIndex, int toIndex); // this does not update history
            void UnmakeMove(BoardState& board, const MoveOutcome& outcome);
            bool ApplyPromotion(BoardState& board, MoveOutcome& rawMO, Pieces::PieceType promoteTo);
            void RevertPromotion(BoardState& board);
            void UpdateGameState(BoardState& board);
            
            /* Moving pieces with check etc */
            MoveOutcome SafeMovePiece(BoardState& board, int fromIndex, int toIndex); // Safer API, yet slower
            bool SafeApplyPromotion(BoardState& board, MoveOutcome& rawMO, Pieces::PieceType promoteTo);
            

            // // Small TT helper
            // struct TTEntry {
            //     uint64_t key    = 0; // Zobrist Key
            //     uint32_t depth  = 0; // Evaluated Depth
            //     uint32_t value  = 0; // Evaluated Score
            //     uint8_t flag    = 0; // Flags for lowerbound upperbound etc
            // };
            // extern std::vector<TTEntry> TranspositionTable;
            // void TTInit(size_t mb);
            // void TTPut(uint64_t key, uint32_t depth, uint32_t value, uint8_t flag);
            // std::optional<TTEntry> TTProbe(uint64_t key);
            inline std::string IndexToUCI(int idx) { return std::string{ char('a' + (idx & 7)), char('1' + (idx >> 3)) }; }
            inline std::string BitBoardString(uint64_t bb) {
                uint64_t occ = bb;
                std::string result = "";
                int cnt = 0;
                std::string tmp = "";
                for (int i = 0; i < 8 * 8; i++) {
                    int digit = occ & 1;
                    tmp += (char)(digit + '0');
                    if(++cnt % 8 == 0) {
                        result = tmp + "\n" + result;
                        tmp = "";
                    }
                    occ >>= 1;
                }
                return result;
            }
        } // namespace Board
    } // namespace ChessEngine
} // namespace WinChess

