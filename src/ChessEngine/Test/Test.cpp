#include "Test.h"
#include "ChessEngine/BitUtils.h"

namespace WinChess {
    namespace ChessEngine {
        namespace Test {

            using namespace Board;
            using namespace Pieces;

            uint64_t Perft(BoardState& board, int depth) {
                if (depth == 0) return 1ULL;
                uint64_t nodes = 0ULL;
                PieceColor side = board.ActiveColor;
                for (auto pt : Pieces::PieceTypes) {
                    uint64_t bb = board.GetBB(side, pt);
                    while (bb) {
                        int from = BitUtils::LSSBIndex(bb);
                        bb &= bb - 1;
                        uint64_t moves = GetLegalMovesForPiece(board, from);
                        while (moves) {
                            int to = BitUtils::LSSBIndex(moves);
                            moves &= moves - 1;
                            MoveOutcome mo = MakeMove(board, from, to); // perft mode: true
                            if (MoveFlagsOf(mo.Move) & static_cast<uint32_t>(MoveFlags::Promotion)) {
                                ApplyPromotion(board, mo, PieceType::Queen);
                                nodes += Perft(board, depth - 1);
                                RevertPromotion(board);
                                ApplyPromotion(board, mo, PieceType::Rook);
                                nodes += Perft(board, depth - 1);
                                RevertPromotion(board);
                                ApplyPromotion(board, mo, PieceType::Bishop);
                                nodes += Perft(board, depth - 1);
                                RevertPromotion(board);
                                ApplyPromotion(board, mo, PieceType::Knight);
                                nodes += Perft(board, depth - 1);
                                // RevertPromotion(board);
                            }
                            else nodes += Perft(board, depth - 1);
                            UnmakeMove(board, mo);
                        }
                    }
                }
                return nodes;
            }

            bool LoadFEN(BoardState& board, const std::string& fen) {
                /* Current Promotion pending is not loaded and considered */
                // minimal FEN loader (piece placement, active color, castling, en-passant)
                board.PieceBB = {};
                board.EnPassantTarget = 0ULL;
                board.CastlingRights = 0x0F;
                board.MoveHistory.clear();
                board.PromotionStatus = 0x00;
                board.ActiveColor = PieceColor::White;
                board.GameStatus = GameStatus::Ongoing;
                board.GameOver = false;
                board.Winner = PieceColor::None;

                int sq = 56;
                size_t i = 0;
                // piece placement
                while (i < fen.size() && fen[i] != ' ') {
                    char c = fen[i];
                    if (c == '/') { sq -= 16; ++i; continue; }
                    if (isdigit((unsigned char)c)) { sq += c - '0'; ++i; continue; }
                    Pieces::PieceColor color = isupper((unsigned char)c) ? Pieces::PieceColor::White : Pieces::PieceColor::Black;
                    Pieces::PieceType type;
                    switch (tolower(c)) {
                    case 'p': type = Pieces::PieceType::Pawn; break;
                    case 'n': type = Pieces::PieceType::Knight; break;
                    case 'b': type = Pieces::PieceType::Bishop; break;
                    case 'r': type = Pieces::PieceType::Rook; break;
                    case 'q': type = Pieces::PieceType::Queen; break;
                    case 'k': type = Pieces::PieceType::King; break;
                    default: ++i; continue;
                    }
                    uint64_t mask = 1ULL << sq;
                    board.GetBB(color, type) |= mask;
                    board.GetColorOcc(color) |= mask;
                    ++sq; ++i;
                }

                // active color
                while (i < fen.size() && fen[i] == ' ') ++i;
                if (i < fen.size()) {
                    if (fen[i] == 'w') board.ActiveColor = PieceColor::White;
                    else if (fen[i] == 'b') board.ActiveColor = PieceColor::Black;
                    while (i < fen.size() && fen[i] != ' ') ++i;
                }

                // castling
                while (i < fen.size() && fen[i] == ' ') ++i;
                if (i < fen.size()) {
                    if (fen[i] == '-') {
                        board.CastlingRights = 0x00;
                        ++i;
                    }
                    else {
                        board.CastlingRights = 0x00;
                        while (i < fen.size() && fen[i] != ' ') {
                            switch (fen[i]) {
                            case 'K': board.CastlingRights |= static_cast<uint8_t>(CastlingFlags::WhiteKingSide);
                            case 'Q': board.CastlingRights |= static_cast<uint8_t>(CastlingFlags::WhiteQueenSide);
                            case 'k': board.CastlingRights |= static_cast<uint8_t>(CastlingFlags::BlackKingSide);
                            case 'q': board.CastlingRights |= static_cast<uint8_t>(CastlingFlags::BlackQueenSide);
                            default: break;
                            }
                            ++i;
                        }
                    }
                }

                // en-passant
                while (i < fen.size() && fen[i] == ' ') ++i;
                if (i < fen.size()) {
                    if (fen[i] != '-') {
                        if (i + 1 < fen.size()) {
                            char file = fen[i], rank = fen[i + 1];
                            if (file >= 'a' && file <= 'h' && rank >= '1' && rank <= '8') {
                                int fileIdx = 7 - (file - 'a');
                                int rankIdx = rank - '1';
                                int ep = rankIdx * 8 + fileIdx;
                                board.EnPassantTarget = 1ULL << ep;
                            }
                            i += 2;
                        }
                    }
                    else ++i;
                }

                RecomputeScores(board);
                RebuildSquareCaches(board);
                Magic::InitMagicTables(); // ensure magic ready
                UpdateGameState(board);
                return true;
            }

            void RunPerftTest(const std::string& name, const std::string& fen, int maxDepth, const std::vector<uint64_t>& expected) {
                BoardState board;
                if (!fen.empty()) LoadFEN(board, fen);
                else InitializeStartingPosition(board);
                std::cout << "\n=== " << name << " ===\n";
                std::cout << "Loaded FEN: " << fen << std::endl;
                uint64_t totalNodes = 0;
                auto totalStart = std::chrono::high_resolution_clock::now();
                for (int d = 1; d <= maxDepth; ++d) {
                    auto t0 = std::chrono::high_resolution_clock::now();
                    uint64_t nodes = Perft(board, d);
                    auto t1 = std::chrono::high_resolution_clock::now();
                    double elapsed = std::chrono::duration<double>(t1 - t0).count();
                    double nps = elapsed > 0.0 ? (nodes / elapsed) : 0.0;
                    totalNodes += nodes;
                    std::cout << "Depth " << d << ": " << nodes << " nodes in " << elapsed << "s -> NPS=" << static_cast<uint64_t>(nps) << "\n";
                    if (d <= (int)expected.size() && expected[d - 1] != 0 && expected[d - 1] != nodes) {
                        std::cout << "MISMATCH! Expected " << expected[d - 1] << " got " << nodes << "\n";
                    }
                }
                auto totalEnd = std::chrono::high_resolution_clock::now();
                double totalElapsed = std::chrono::duration<double>(totalEnd - totalStart).count();
                std::cout << "=== SUMMARY: " << totalNodes << " nodes in " << totalElapsed << "s -> NPS=" << static_cast<uint64_t>(totalNodes / totalElapsed) << " ===\n";
            }

            void ExampleRunTest() {
                // Perft reference numbers: depth 6 for start pos and kiwipete
                // (source: ChessProgramming Wiki)
                RunPerftTest("Start Position", "", 5, { 20, 400, 8902, 197281, 4865609 });
                RunPerftTest(
                    "Kiwipete",
                    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
                    4,
                    { 48, 2039, 97862, 4085603 }
                );
                RunPerftTest(
                    "Position 3",
                    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
                    5,
                    { 14, 191, 2812, 43238, 674624 }
                );
                RunPerftTest(
                    "Position 4",
                    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
                    5,
                    { 6, 264, 9467, 422333, 15833292 }
                );
                RunPerftTest(
                    "Position 5",
                    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
                    5,
                    { 44, 1486, 62379, 2103487, 89941194 }
                );
                RunPerftTest(
                    "Position 6",
                    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
                    5,
                    { 46, 2079, 89890, 3894594, 164075551 }
                );

                RunPerftTest("Endgame Mate", "7k/7Q/7K/8/8/8/8/8 b - - 0 1", 3, { 0, 0, 0 });
                RunPerftTest("Stalemate Test", "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", 3, { 0, 0, 0});
            }

        } // namespace Test
    } // namespace ChessEngine
} // namespace WinChess


