#pragma once
#include "pch.h"

namespace WinChess {
    namespace ChessEngine {
        namespace Pieces {

            enum class PieceColor : int {
                White       = 0,
                Black       = 1,
                /* For a special purpose regarding game winner */
                None        = 2
            };

            enum class PieceType : int {
                Pawn        = 0,
                Knight      = 1,
                Bishop      = 2,
                Rook        = 3,
                Queen       = 4,
                King        = 5,
                // EveryPiece  = 6
            };

            static constexpr std::array<PieceColor, 2> PieceColors = { PieceColor::White, PieceColor::Black };
            static constexpr std::array<PieceType, 6> PieceTypes = {
                PieceType::Pawn, PieceType::Knight, PieceType::Bishop,
                PieceType::Rook, PieceType::Queen, PieceType::King
            };

            inline PieceColor OtherColor(PieceColor c) { return static_cast<PieceColor>(1 - static_cast<int>(c)); }
            inline int PieceTypeScore(PieceType pt) {
                switch (pt) {
                    case PieceType::Pawn:   return 1;
                    case PieceType::Knight: return 3;
                    case PieceType::Bishop: return 3;
                    case PieceType::Rook:   return 5;
                    case PieceType::Queen:  return 9;
                    default:                return 0;
                }
            }
            // template <typename T>
            // using PieceMap = std::array<std::array<T, 7>, 3>; // color index 0..2, type 0..6
        } // namespace Pieces
    } // namespace ChessEngine
} // namespace WinChess
