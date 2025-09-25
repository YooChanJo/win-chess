#pragma once
#include "ChessEngine/Pieces.h"
#include "ChessEngine/Board.h"

namespace WinChess {
    namespace ChessEngine {
        namespace Test {

            bool LoadFEN(Board::BoardState& board, const std::string& fen);
            uint64_t Perft(Board::BoardState& board, int depth);
            void RunPerftTest(const std::string& name, const std::string& fen, int maxDepth, const std::vector<uint64_t>& expected = {});
            void ExampleRunTest();

        } // namespace Test
    } // namespace ChessEngine
} // namespace WinChess
