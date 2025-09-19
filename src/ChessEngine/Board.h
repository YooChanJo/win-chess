#pragma once
#include "pch.h"
#include "Pieces.h"

namespace WinChess {
    namespace ChessEngine {
        namespace Board {

            enum class BitMask : uint64_t {
                Empty = 0x0000000000000000ULL,
                Full  = 0xFFFFFFFFFFFFFFFFULL,
                // Ranks
                Rank1 = 0x00000000000000FFULL,
                Rank2 = 0x000000000000FF00ULL,
                Rank3 = 0x0000000000FF0000ULL,
                Rank4 = 0x00000000FF000000ULL,
                Rank5 = 0x000000FF00000000ULL,
                Rank6 = 0x0000FF0000000000ULL,
                Rank7 = 0x00FF000000000000ULL,
                Rank8 = 0xFF00000000000000ULL,
                // Files
                FileA = 0x0101010101010101ULL,
                FileB = 0x0202020202020202ULL,
                FileC = 0x0404040404040404ULL,
                FileD = 0x0808080808080808ULL,
                FileE = 0x1010101010101010ULL,
                FileF = 0x2020202020202020ULL,
                FileG = 0x4040404040404040ULL,
                FileH = 0x8080808080808080ULL,

                // Squares (File & Rank masks)
                A1 = FileA & Rank1, B1 = FileB & Rank1, C1 = FileC & Rank1, D1 = FileD & Rank1,
                E1 = FileE & Rank1, F1 = FileF & Rank1, G1 = FileG & Rank1, H1 = FileH & Rank1,

                A2 = FileA & Rank2, B2 = FileB & Rank2, C2 = FileC & Rank2, D2 = FileD & Rank2,
                E2 = FileE & Rank2, F2 = FileF & Rank2, G2 = FileG & Rank2, H2 = FileH & Rank2,

                A3 = FileA & Rank3, B3 = FileB & Rank3, C3 = FileC & Rank3, D3 = FileD & Rank3,
                E3 = FileE & Rank3, F3 = FileF & Rank3, G3 = FileG & Rank3, H3 = FileH & Rank3,

                A4 = FileA & Rank4, B4 = FileB & Rank4, C4 = FileC & Rank4, D4 = FileD & Rank4,
                E4 = FileE & Rank4, F4 = FileF & Rank4, G4 = FileG & Rank4, H4 = FileH & Rank4,

                A5 = FileA & Rank5, B5 = FileB & Rank5, C5 = FileC & Rank5, D5 = FileD & Rank5,
                E5 = FileE & Rank5, F5 = FileF & Rank5, G5 = FileG & Rank5, H5 = FileH & Rank5,

                A6 = FileA & Rank6, B6 = FileB & Rank6, C6 = FileC & Rank6, D6 = FileD & Rank6,
                E6 = FileE & Rank6, F6 = FileF & Rank6, G6 = FileG & Rank6, H6 = FileH & Rank6,

                A7 = FileA & Rank7, B7 = FileB & Rank7, C7 = FileC & Rank7, D7 = FileD & Rank7,
                E7 = FileE & Rank7, F7 = FileF & Rank7, G7 = FileG & Rank7, H7 = FileH & Rank7,

                A8 = FileA & Rank8, B8 = FileB & Rank8, C8 = FileC & Rank8, D8 = FileD & Rank8,
                E8 = FileE & Rank8, F8 = FileF & Rank8, G8 = FileG & Rank8, H8 = FileH & Rank8,
            };

            struct BoardState {
                // bitboards: color -> piece type -> bitboard
                PieceMap<uint64_t> BitBoards = {
                    { Pieces::PieceColor::White, {
                        { Pieces::PieceType::Pawn,   (uint64_t)BitMask::Rank2 },
                        { Pieces::PieceType::Knight, (uint64_t)BitMask::B1 | (uint64_t)BitMask::G1 },
                        { Pieces::PieceType::Bishop, (uint64_t)BitMask::C1 | (uint64_t)BitMask::F1 },
                        { Pieces::PieceType::Rook,   (uint64_t)BitMask::A1 | (uint64_t)BitMask::H1 },
                        { Pieces::PieceType::Queen,  (uint64_t)BitMask::D1 },
                        { Pieces::PieceType::King,   (uint64_t)BitMask::E1 },
                        { Pieces::PieceType::EveryPiece, (uint64_t)BitMask::Rank1 | (uint64_t)BitMask::Rank2 },
                    }},
                    { Pieces::PieceColor::Black, {
                        { Pieces::PieceType::Pawn,   (uint64_t)BitMask::Rank7 },
                        { Pieces::PieceType::Knight, (uint64_t)BitMask::B8 | (uint64_t)BitMask::G8 },
                        { Pieces::PieceType::Bishop, (uint64_t)BitMask::C8 | (uint64_t)BitMask::F8 },
                        { Pieces::PieceType::Rook,   (uint64_t)BitMask::A8 | (uint64_t)BitMask::H8 },
                        { Pieces::PieceType::Queen,  (uint64_t)BitMask::D8 },
                        { Pieces::PieceType::King,   (uint64_t)BitMask::E8 },
                        { Pieces::PieceType::EveryPiece, (uint64_t)BitMask::Rank7 | (uint64_t)BitMask::Rank8 },
                    }},
                    { Pieces::PieceColor::Both, {
                        { Pieces::PieceType::Pawn,   (uint64_t)BitMask::Rank2 | (uint64_t)BitMask::Rank7 },
                        { Pieces::PieceType::Knight, (uint64_t)BitMask::B1 | (uint64_t)BitMask::G1 | (uint64_t)BitMask::B8 | (uint64_t)BitMask::G8 },
                        { Pieces::PieceType::Bishop, (uint64_t)BitMask::C1 | (uint64_t)BitMask::F1 | (uint64_t)BitMask::C8 | (uint64_t)BitMask::F8 },
                        { Pieces::PieceType::Rook,   (uint64_t)BitMask::A1 | (uint64_t)BitMask::H1 | (uint64_t)BitMask::A8 | (uint64_t)BitMask::H8 },
                        { Pieces::PieceType::Queen,  (uint64_t)BitMask::D1 | (uint64_t)BitMask::D8 },
                        { Pieces::PieceType::King,   (uint64_t)BitMask::E1 | (uint64_t)BitMask::E8 },
                        { Pieces::PieceType::EveryPiece, (uint64_t)BitMask::Rank1 | (uint64_t)BitMask::Rank2 | (uint64_t)BitMask::Rank7 | (uint64_t)BitMask::Rank8 },
                    }}
                };

                /* Special State Trackers */
                uint64_t EnPassantTarget = 0ULL;
                struct CastlingRightsStruct {
                    bool WhiteKingSide  = true;
                    bool WhiteQueenSide = true;
                    bool BlackKingSide  = true;
                    bool BlackQueenSide = true;
                } CastlingRights;

                // Promotion pending is handled inside BoardState now.
                bool PromotionPending = false;
                int PromotionSquare = -1;
                Pieces::PieceColor PromotionColor = Pieces::PieceColor::White;

                /* Checks between caches and states and sees corruptancy */
                inline bool IsValid() const {
                    uint64_t whiteEvery =
                        (uint64_t)BitBoards.at(Pieces::PieceColor::White).at(Pieces::PieceType::Pawn) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::White).at(Pieces::PieceType::Knight) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::White).at(Pieces::PieceType::Bishop) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::White).at(Pieces::PieceType::Rook) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::White).at(Pieces::PieceType::Queen) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::White).at(Pieces::PieceType::King);

                    uint64_t blackEvery =
                        (uint64_t)BitBoards.at(Pieces::PieceColor::Black).at(Pieces::PieceType::Pawn) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::Black).at(Pieces::PieceType::Knight) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::Black).at(Pieces::PieceType::Bishop) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::Black).at(Pieces::PieceType::Rook) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::Black).at(Pieces::PieceType::Queen) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::Black).at(Pieces::PieceType::King);

                    uint64_t bothEvery =
                        (uint64_t)BitBoards.at(Pieces::PieceColor::Both).at(Pieces::PieceType::Pawn) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::Both).at(Pieces::PieceType::Knight) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::Both).at(Pieces::PieceType::Bishop) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::Both).at(Pieces::PieceType::Rook) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::Both).at(Pieces::PieceType::Queen) |
                        (uint64_t)BitBoards.at(Pieces::PieceColor::Both).at(Pieces::PieceType::King);

                    bool ok = (whiteEvery == (uint64_t)BitBoards.at(Pieces::PieceColor::White).at(Pieces::PieceType::EveryPiece))
                        && (blackEvery == (uint64_t)BitBoards.at(Pieces::PieceColor::Black).at(Pieces::PieceType::EveryPiece))
                        && (bothEvery == (uint64_t)BitBoards.at(Pieces::PieceColor::Both).at(Pieces::PieceType::EveryPiece));

                    // Check both aggregates
                    for (auto pt : Pieces::PieceTypes) {
                        uint64_t both = (uint64_t)BitBoards.at(Pieces::PieceColor::White).at(pt) | (uint64_t)BitBoards.at(Pieces::PieceColor::Black).at(pt);
                        if (both != (uint64_t)BitBoards.at(Pieces::PieceColor::Both).at(pt)) {
                            ok = false; break;
                        }
                    }
                    return ok;
                }
            };

            /* Generate Possible Moves */
            uint64_t GeneratePseudoMoves(
                Pieces::PieceColor color,
                Pieces::PieceType type,
                int index,
                const BoardState& board
            );
            uint64_t GenerateKnightMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board
            );
            uint64_t GenerateKingMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board
            );
            uint64_t GenerateSlidingMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board,
                const std::array<int, 4>& directions
            );
            uint64_t GenerateRookMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board
            );
            uint64_t GenerateBishopMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board
            );
            uint64_t GenerateQueenMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board
            );
            uint64_t GeneratePawnMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board
            );

            /* Attack / Safety */
            uint64_t GetAllEnemyAttacks(
                Pieces::PieceColor color,
                const BoardState& board
            );
            bool IsKingSafeAfterMove(
                Pieces::PieceColor color,
                const BoardState& board
            );

            /* Final Validation */
            uint64_t GetLegalMovesForPiece(
                Pieces::PieceColor color,
                Pieces::PieceType type,
                int index,
                const BoardState& board
            );

            /*
                Move abstraction: performs the move and updates caches.
                If pawn reaches promotion rank, it will set board.PromotionPending = true
                and leave the pawn on the promotion square until ApplyPromotion is called.
            */
            struct MoveOutcome {
                bool success = false;
                bool didCapture = false;
                Pieces::PieceType capturedType = Pieces::PieceType::Pawn;
                int capturedIndex = -1;
                bool promotionRequired = false;
            };

            MoveOutcome MovePiece(BoardState& board, int fromIndex, int toIndex);
            bool ApplyPromotion(BoardState& board, Pieces::PieceType promoteTo); // returns true if applied
        }
    }
}
