#include "Board.h"

namespace WinChess {
    namespace ChessEngine {

        using namespace Pieces;
        namespace Board {

            /* Helper: find the piece occupying a square; returns true if found with color & type */
            static bool FindPieceAtSquare(
                const BoardState& board,
                int index,
                Pieces::PieceColor& outColor,
                Pieces::PieceType& outType
            ) {
                uint64_t mask = 1ULL << index;
                for (auto color : Pieces::PieceColors) {
                    for (auto type : Pieces::PieceTypes) {
                        if (board.BitBoards.at(color).at(type) & mask) {
                            outColor = color;
                            outType = type;
                            return true;
                        }
                    }
                }
                return false;
            }

            uint64_t GeneratePseudoMoves(
                Pieces::PieceColor color,
                Pieces::PieceType type,
                int index,
                const BoardState& board
            ) {
                switch (type) {
                    case Pieces::PieceType::Pawn:   return GeneratePawnMoves(color, index, board);
                    case Pieces::PieceType::Knight: return GenerateKnightMoves(color, index, board);
                    case Pieces::PieceType::Bishop: return GenerateBishopMoves(color, index, board);
                    case Pieces::PieceType::Rook:   return GenerateRookMoves(color, index, board);
                    case Pieces::PieceType::Queen:  return GenerateQueenMoves(color, index, board);
                    case Pieces::PieceType::King:   return GenerateKingMoves(color, index, board);
                    default: return 0ULL;
                }
            }

            uint64_t GenerateKnightMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board
            ) {
                uint64_t knight = 1ULL << index;
                uint64_t moves = 0ULL;

                const uint64_t notA  = ~((uint64_t)BitMask::FileA);
                const uint64_t notAB = ~((uint64_t)BitMask::FileA | (uint64_t)BitMask::FileB);
                const uint64_t notH  = ~((uint64_t)BitMask::FileH);
                const uint64_t notGH = ~((uint64_t)BitMask::FileG | (uint64_t)BitMask::FileH);

                moves |= ( knight << 17 ) & notA;
                moves |= ( knight << 15 ) & notH;
                moves |= ( knight << 10 ) & notAB;
                moves |= ( knight << 6 )  & notGH;
                moves |= ( knight >> 17 ) & notH;
                moves |= ( knight >> 15 ) & notA;
                moves |= ( knight >> 10 ) & notGH;
                moves |= ( knight >> 6 )  & notAB;

                // Remove friendly pieces
                uint64_t friendly = board.BitBoards.at(color).at(Pieces::PieceType::EveryPiece);
                moves &= ~friendly;
                return moves;
            }

            uint64_t GenerateKingMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board
            ) {
                uint64_t king = 1ULL << index;
                uint64_t moves = 0ULL;

                const uint64_t notA = ~(uint64_t)BitMask::FileA;
                const uint64_t notH = ~(uint64_t)BitMask::FileH;

                moves |= (king << 8);
                moves |= (king >> 8);
                moves |= (king << 1) & notA;
                moves |= (king >> 1) & notH;
                moves |= (king << 9) & notA;
                moves |= (king << 7) & notH;
                moves |= (king >> 9) & notH;
                moves |= (king >> 7) & notA;

                // Remove friendly pieces
                uint64_t friendly = board.BitBoards.at(color).at(Pieces::PieceType::EveryPiece);
                moves &= ~friendly;

                // Castling generation (ensure squares not occupied)
                uint64_t occupied = board.BitBoards.at(Pieces::PieceColor::Both).at(Pieces::PieceType::EveryPiece);

                if (color == Pieces::PieceColor::White) {
                    if (board.CastlingRights.WhiteKingSide) {
                        if (!(occupied & ((uint64_t)BitMask::F1 | (uint64_t)BitMask::G1)))
                            moves |= (uint64_t)BitMask::G1;
                    }
                    if (board.CastlingRights.WhiteQueenSide) {
                        if (!(occupied & ((uint64_t)BitMask::B1 | (uint64_t)BitMask::C1 | (uint64_t)BitMask::D1)))
                            moves |= (uint64_t)BitMask::C1;
                    }
                } else {
                    if (board.CastlingRights.BlackKingSide) {
                        if (!(occupied & ((uint64_t)BitMask::F8 | (uint64_t)BitMask::G8)))
                            moves |= (uint64_t)BitMask::G8;
                    }
                    if (board.CastlingRights.BlackQueenSide) {
                        if (!(occupied & ((uint64_t)BitMask::B8 | (uint64_t)BitMask::C8 | (uint64_t)BitMask::D8)))
                            moves |= (uint64_t)BitMask::C8;
                    }
                }

                return moves;
            }

            uint64_t GenerateSlidingMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board,
                const std::array<int, 4>& directions
            ) {
                uint64_t moves = 0ULL;

                for (int dir : directions) {
                    int target = index;
                    while (true) {
                        int prev = target;
                        target += dir;

                        // board boundaries
                        if (target < 0 || target > 63) break;

                        int prevFile = prev % 8;
                        int currFile = target % 8;

                        // detect horizontal wrap-around
                        if (std::abs(currFile - prevFile) > 1) break;

                        uint64_t targetMask = 1ULL << target;
                        moves |= targetMask;

                        // stop at *any* piece (can capture, but not go past)
                        if (board.BitBoards.at(Pieces::PieceColor::Both).at(Pieces::PieceType::EveryPiece) & targetMask) break;
                    }
                }

                // remove friendly occupied squares
                uint64_t friendly = board.BitBoards.at(color).at(Pieces::PieceType::EveryPiece);
                moves &= ~friendly;
                return moves;
            }

            uint64_t GenerateRookMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board
            ) { return GenerateSlidingMoves(color, index, board, { 8,-8, 1, -1 }); }

            uint64_t GenerateBishopMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board
            ) { return GenerateSlidingMoves(color, index, board, { 9, 7, -9, -7 }); }

            uint64_t GenerateQueenMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board
            ) { 
                return (
                    GenerateSlidingMoves(color, index, board, { 8,-8, 1, -1 }) |
                    GenerateSlidingMoves(color, index, board, { 9, 7, -9, -7 })
                );
            }

            uint64_t GeneratePawnMoves(
                Pieces::PieceColor color,
                int index,
                const BoardState& board
            ) {
                uint64_t pawn = 1ULL << index;
                uint64_t moves = 0;
                const uint64_t notA = ~(uint64_t)(Board::BitMask::FileA);
                const uint64_t notH = ~(uint64_t)(Board::BitMask::FileH);

                uint64_t stepFoward, toACapture, toHCapture, doubleForward, startRankMask;
                switch (color) {
                    case Pieces::PieceColor::White:
                        stepFoward = pawn << 8;
                        toACapture = pawn << 7;
                        toHCapture = pawn << 9;
                        doubleForward = pawn << 16;
                        startRankMask = (uint64_t)Board::BitMask::Rank2;
                        break;
                    case Pieces::PieceColor::Black:
                        stepFoward = pawn >> 8;
                        toACapture = pawn >> 9;
                        toHCapture = pawn >> 7;
                        doubleForward = pawn >> 16;
                        startRankMask = (uint64_t)Board::BitMask::Rank7;
                        break;
                    default: throw std::runtime_error("Invalid Pawn Color");
                }

                uint64_t occupied = board.BitBoards.at(Pieces::PieceColor::Both).at(Pieces::PieceType::EveryPiece);
                /* Forward */
                if (!(stepFoward & occupied)) {
                    moves |= stepFoward;
                    if ((pawn & startRankMask) && !(doubleForward & occupied)) {
                        moves |= doubleForward;
                    }
                }
                /* Capture */
                Pieces::PieceColor opp = Pieces::OtherColor(color);
                moves |= (toACapture) & notH & board.BitBoards.at(opp).at(Pieces::PieceType::EveryPiece);
                moves |= (toHCapture) & notA & board.BitBoards.at(opp).at(Pieces::PieceType::EveryPiece);

                /* En Passant Captures */
                if (board.EnPassantTarget) {
                    moves |= (toACapture & notH & board.EnPassantTarget);
                    moves |= (toHCapture & notA & board.EnPassantTarget);
                }

                // Remove friendly pieces
                uint64_t friendly = board.BitBoards.at(color).at(Pieces::PieceType::EveryPiece);
                moves &= ~friendly;
                return moves;
            }

            uint64_t GetAllEnemyAttacks(
                Pieces::PieceColor color,
                const BoardState& board
            ) {
                Pieces::PieceColor opp = Pieces::OtherColor(color);
                uint64_t attacks = 0ULL;

                // Iterate over each piece type
                for (auto type : Pieces::PieceTypes) {
                    uint64_t bb = board.BitBoards.at(opp).at(type);
                    while (bb) {
                        int index = Pieces::LSSBIndex(bb);
                        bb &= bb - 1; // pop LSB

                        switch (type) {
                            case Pieces::PieceType::Pawn:
                                {
                                    uint64_t pawn = 1ULL << index;
                                    if (opp == Pieces::PieceColor::White) {
                                        attacks |= (pawn << 7) & ~(uint64_t)Board::BitMask::FileH;
                                        attacks |= (pawn << 9) & ~(uint64_t)Board::BitMask::FileA;
                                    } else {
                                        attacks |= (pawn >> 9) & ~(uint64_t)Board::BitMask::FileH;
                                        attacks |= (pawn >> 7) & ~(uint64_t)Board::BitMask::FileA;
                                    }
                                }
                                break;
                            case Pieces::PieceType::Knight:
                                attacks |= GenerateKnightMoves(opp, index, board);
                                break;
                            case Pieces::PieceType::Bishop:
                                attacks |= GenerateBishopMoves(opp, index, board);
                                break;
                            case Pieces::PieceType::Rook:
                                attacks |= GenerateRookMoves(opp, index, board);
                                break;
                            case Pieces::PieceType::Queen:
                                attacks |= GenerateQueenMoves(opp, index, board);
                                break;
                            case Pieces::PieceType::King:
                                {
                                    uint64_t king = 1ULL << index;
                                    const uint64_t notA = ~(uint64_t)Board::BitMask::FileA;
                                    const uint64_t notH = ~(uint64_t)Board::BitMask::FileH;

                                    attacks |= (king << 8);
                                    attacks |= (king >> 8);
                                    attacks |= (king << 1) & notA;
                                    attacks |= (king >> 1) & notH;
                                    attacks |= (king << 9) & notA;
                                    attacks |= (king << 7) & notH;
                                    attacks |= (king >> 9) & notH;
                                    attacks |= (king >> 7) & notA;
                                }
                                break;
                            default: break;
                        }
                    }
                }
                return attacks;
            }

            bool IsKingSafeAfterMove(
                Pieces::PieceColor color,
                const BoardState& board
            ) {
                uint64_t kingBB = board.BitBoards.at(color).at(Pieces::PieceType::King);
                if (!kingBB) return false; // no king found = invalid

                int kingIndex = Pieces::LSSBIndex(kingBB);
                uint64_t kingMask = 1ULL << kingIndex;

                uint64_t oppAttacks = GetAllEnemyAttacks(color, board);
                return (oppAttacks & kingMask) == 0ULL;
            }

            uint64_t GetLegalMovesForPiece(
                Pieces::PieceColor color,
                Pieces::PieceType type,
                int index,
                const BoardState& board
            ) {
                uint64_t legalMoves = 0ULL;
                uint64_t pseudoMoves = GeneratePseudoMoves(color, type, index, board);

                while (pseudoMoves) {
                    int targetIndex = Pieces::LSSBIndex(pseudoMoves);
                    uint64_t targetMask = 1ULL << targetIndex;

                    BoardState boardCopy = board;

                    // Remove piece from source in copy
                    boardCopy.BitBoards[color][type] &= ~(1ULL << index);
                    boardCopy.BitBoards[Pieces::PieceColor::Both][type] &= ~(1ULL << index);
                    boardCopy.BitBoards[color][Pieces::PieceType::EveryPiece] &= ~(1ULL << index);
                    boardCopy.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~(1ULL << index);

                    // Capture in copy if exists
                    Pieces::PieceColor opponent = Pieces::OtherColor(color);
                    for (auto pt : Pieces::PieceTypes) {
                        if (boardCopy.BitBoards[opponent][pt] & targetMask) {
                            boardCopy.BitBoards[opponent][pt] &= ~targetMask;
                            boardCopy.BitBoards[Pieces::PieceColor::Both][pt] &= ~targetMask;
                            boardCopy.BitBoards[opponent][Pieces::PieceType::EveryPiece] &= ~targetMask;
                            boardCopy.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~targetMask;
                            break;
                        }
                    }
                    // En passant: if target is en-passant square and moving pawn remove captured pawn in copy
                    if (type == Pieces::PieceType::Pawn && boardCopy.EnPassantTarget && (boardCopy.EnPassantTarget & targetMask)) {
                        int capturedIndex;
                        if (color == Pieces::PieceColor::White) capturedIndex = targetIndex - 8;
                        else capturedIndex = targetIndex + 8;
                        uint64_t capturedMask = 1ULL << capturedIndex;
                        boardCopy.BitBoards[opponent][Pieces::PieceType::Pawn] &= ~capturedMask;
                        boardCopy.BitBoards[opponent][Pieces::PieceType::EveryPiece] &= ~capturedMask;
                        boardCopy.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Pawn] &= ~capturedMask;
                        boardCopy.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~capturedMask;
                    }

                    // Move piece to target in copy
                    boardCopy.BitBoards[color][type] |= targetMask;
                    boardCopy.BitBoards[Pieces::PieceColor::Both][type] |= targetMask;
                    boardCopy.BitBoards[color][Pieces::PieceType::EveryPiece] |= targetMask;
                    boardCopy.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] |= targetMask;

                    // If king safe then legal
                    if (IsKingSafeAfterMove(color, boardCopy)) {
                        legalMoves |= targetMask;
                    }

                    pseudoMoves &= pseudoMoves - 1;
                }

                return legalMoves;
            }

            MoveOutcome MovePiece(BoardState& board, int fromIndex, int toIndex) {
                MoveOutcome ret;
                uint64_t fromMask = 1ULL << fromIndex;
                uint64_t toMask   = 1ULL << toIndex;

                // Find moving piece
                Pieces::PieceColor moverColor;
                Pieces::PieceType moverType;
                if (!FindPieceAtSquare(board, fromIndex, moverColor, moverType)) {
                    return ret; // no piece to move
                }

                // Check legality
                uint64_t legal = GetLegalMovesForPiece(moverColor, moverType, fromIndex, board);
                if (!(legal & toMask)) {
                    return ret; // illegal
                }

                Pieces::PieceColor opponent = Pieces::OtherColor(moverColor);

                // Remove piece from source (all caches)
                board.BitBoards[moverColor][moverType] &= ~fromMask;
                board.BitBoards[Pieces::PieceColor::Both][moverType] &= ~fromMask;
                board.BitBoards[moverColor][Pieces::PieceType::EveryPiece] &= ~fromMask;
                board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~fromMask;

                // Capture on target square?
                if (board.BitBoards[opponent][Pieces::PieceType::EveryPiece] & toMask) {
                    ret.didCapture = true;
                    ret.capturedIndex = toIndex;
                    // find captured type
                    for (auto pt : Pieces::PieceTypes) {
                        if (board.BitBoards[opponent][pt] & toMask) {
                            board.BitBoards[opponent][pt] &= ~toMask;
                            board.BitBoards[Pieces::PieceColor::Both][pt] &= ~toMask;
                            board.BitBoards[opponent][Pieces::PieceType::EveryPiece] &= ~toMask;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~toMask;
                            ret.capturedType = pt;
                            break;
                        }
                    }
                }
                // En-passant capture?
                else if (moverType == Pieces::PieceType::Pawn && board.EnPassantTarget && (toMask & board.EnPassantTarget)) {
                    ret.didCapture = true;
                    ret.capturedType = Pieces::PieceType::Pawn;
                    if (moverColor == Pieces::PieceColor::White) {
                        ret.capturedIndex = toIndex - 8;
                    } else {
                        ret.capturedIndex = toIndex + 8;
                    }
                    uint64_t capturedMask = 1ULL << ret.capturedIndex;
                    board.BitBoards[opponent][Pieces::PieceType::Pawn] &= ~capturedMask;
                    board.BitBoards[opponent][Pieces::PieceType::EveryPiece] &= ~capturedMask;
                    board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Pawn] &= ~capturedMask;
                    board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~capturedMask;
                }

                // Castling (if king moved two squares)
                if (moverType == Pieces::PieceType::King) {
                    // remove castling rights for mover color
                    if (moverColor == Pieces::PieceColor::White) {
                        board.CastlingRights.WhiteKingSide = false;
                        board.CastlingRights.WhiteQueenSide = false;
                        // kingside: E1 -> G1
                        if (fromIndex == 4 && toIndex == 6) {
                            uint64_t rookFrom = 1ULL << 7;
                            uint64_t rookTo   = 1ULL << 5;
                            board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::Rook] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::Rook] |= rookTo;
                            board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::EveryPiece] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::EveryPiece] |= rookTo;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] |= rookTo;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] |= rookTo;
                        }
                        // queenside: E1 -> C1
                        else if (fromIndex == 4 && toIndex == 2) {
                            uint64_t rookFrom = 1ULL << 0;
                            uint64_t rookTo   = 1ULL << 3;
                            board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::Rook] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::Rook] |= rookTo;
                            board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::EveryPiece] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::EveryPiece] |= rookTo;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] |= rookTo;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] |= rookTo;
                        }
                    } else {
                        board.CastlingRights.BlackKingSide = false;
                        board.CastlingRights.BlackQueenSide = false;
                        // kingside: E8 -> G8
                        if (fromIndex == 60 && toIndex == 62) {
                            uint64_t rookFrom = 1ULL << 63;
                            uint64_t rookTo   = 1ULL << 61;
                            board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::Rook] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::Rook] |= rookTo;
                            board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::EveryPiece] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::EveryPiece] |= rookTo;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] |= rookTo;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] |= rookTo;
                        }
                        // queenside: E8 -> C8
                        else if (fromIndex == 60 && toIndex == 58) {
                            uint64_t rookFrom = 1ULL << 56;
                            uint64_t rookTo   = 1ULL << 59;
                            board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::Rook] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::Rook] |= rookTo;
                            board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::EveryPiece] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::EveryPiece] |= rookTo;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] |= rookTo;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~rookFrom;
                            board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] |= rookTo;
                        }
                    }
                } // end castling handling

                // If mover is rook and moves from original place -> remove castling rights
                if (moverType == Pieces::PieceType::Rook) {
                    switch (fromIndex) {
                        case 0:  board.CastlingRights.WhiteQueenSide = false; break;
                        case 7:  board.CastlingRights.WhiteKingSide  = false; break;
                        case 56: board.CastlingRights.BlackQueenSide = false; break;
                        case 63: board.CastlingRights.BlackKingSide  = false; break;
                        default: break;
                    }
                }

                // Place moved piece to destination
                board.BitBoards[moverColor][moverType] |= toMask;
                board.BitBoards[Pieces::PieceColor::Both][moverType] |= toMask;
                board.BitBoards[moverColor][Pieces::PieceType::EveryPiece] |= toMask;
                board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] |= toMask;

                // Clear EnPassantTarget by default
                board.EnPassantTarget = 0ULL;

                // If pawn moved two squares, set en-passant target (midpoint)
                if (moverType == Pieces::PieceType::Pawn) {
                    int fromRank = fromIndex / 8;
                    int toRank   = toIndex / 8;
                    if (std::abs(toRank - fromRank) == 2) {
                        int epSquare = (fromIndex + toIndex) / 2;
                        board.EnPassantTarget = 1ULL << epSquare;
                    }
                }

                // If captured rook -> update castling rights accordingly
                if (ret.didCapture && ret.capturedType == Pieces::PieceType::Rook) {
                    switch (ret.capturedIndex) {
                        case 0:  board.CastlingRights.WhiteQueenSide = false; break;
                        case 7:  board.CastlingRights.WhiteKingSide  = false; break;
                        case 56: board.CastlingRights.BlackQueenSide = false; break;
                        case 63: board.CastlingRights.BlackKingSide  = false; break;
                        default: break;
                    }
                }

                // Promotion detection (if pawn reaches last rank)
                if (moverType == Pieces::PieceType::Pawn) {
                    int targetRank = toIndex / 8;
                    if ((moverColor == Pieces::PieceColor::White && targetRank == 7) ||
                        (moverColor == Pieces::PieceColor::Black && targetRank == 0)) {
                        // set promotion pending; we leave the pawn in place until ApplyPromotion
                        board.PromotionPending = true;
                        board.PromotionSquare  = toIndex;
                        board.PromotionColor   = moverColor;
                        ret.promotionRequired = true;
                    }
                }

                ret.success = true;
                return ret;
            }

            bool ApplyPromotion(BoardState& board, Pieces::PieceType promoteTo) {
                if (!board.PromotionPending) return false;
                if (!(promoteTo == Pieces::PieceType::Queen || promoteTo == Pieces::PieceType::Rook ||
                      promoteTo == Pieces::PieceType::Bishop || promoteTo == Pieces::PieceType::Knight)) {
                    return false;
                }

                int sq = board.PromotionSquare;
                uint64_t mask = 1ULL << sq;
                Pieces::PieceColor col = board.PromotionColor;

                // Remove pawn (should be in pawn bitboard already)
                board.BitBoards[col][Pieces::PieceType::Pawn] &= ~mask;
                board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Pawn] &= ~mask;

                // Add new piece
                board.BitBoards[col][promoteTo] |= mask;
                board.BitBoards[Pieces::PieceColor::Both][promoteTo] |= mask;

                // EveryPiece caches already contain the square (pawn replaced by piece). Make sure they reflect correctly:
                // Remove pawn from EveryPiece, then add promoted piece to EveryPiece (EveryPiece is per-color and Both)
                board.BitBoards[col][Pieces::PieceType::EveryPiece] &= ~mask;
                board.BitBoards[col][Pieces::PieceType::EveryPiece] |= mask;
                board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~mask;
                board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] |= mask;

                // Clear promotion tracking
                board.PromotionPending = false;
                board.PromotionSquare = -1;
                return true;
            }

        }
    }
}
