#pragma once
#include "pch.h"

namespace WinChess {
    namespace ChessEngine {
        namespace Pieces {
            enum class PieceColor {
                White = 0,
                Black = 1,
                /* Special Color */
                Both
            };
            enum class PieceType {
                Pawn, Knight, Bishop,
                Rook, Queen, King,
                /* Special Pieces */
                EveryPiece,
            };

            /* These are for iterations */
            constexpr std::array<PieceColor, 2> PieceColors = {
                PieceColor::White,
                PieceColor::Black
            };
            constexpr std::array<PieceType, 6> PieceTypes = {
                PieceType::Pawn,
                PieceType::Knight,
                PieceType::Bishop,
                PieceType::Rook,
                PieceType::Queen,
                PieceType::King
            };

            inline PieceColor OtherColor(PieceColor color) { return (PieceColor)(1 - (int)color); }

            // utility: index of least significant set bit
            int LSSBIndex(uint64_t bb);
        }

        // Generic mapping from color -> piece type -> T
        template<typename T>
        using PieceMap = std::unordered_map<Pieces::PieceColor, std::unordered_map<Pieces::PieceType, T>>;
    }
}
