#pragma once
#include "pch.h"

namespace WinChess {
	namespace ChessEngine {
		namespace Magic {

			// Initialize magic tables (call once at program startup)
			void InitMagicTables();

			// Rook/Bishop attack queries (occupancy is full 64-bit occupancy)
			uint64_t RookAttacks(int square, uint64_t occupancy);
			uint64_t BishopAttacks(int square, uint64_t occupancy);
			uint64_t QueenAttacks(int square, uint64_t occupancy); // rook|bishop

		} // namespace Magic
	} // namespace ChessEngine
} // namespace WinChess
