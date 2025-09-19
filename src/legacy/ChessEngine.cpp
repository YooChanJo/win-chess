// #include "ChessEngine.h"

// /* Index of Least Significant Set Bit */
// static inline int LSSBIndex(uint64_t bb) {
// #if defined(_MSC_VER)
//     unsigned long index;
//     _BitScanForward64(&index, bb);
//     return static_cast<int>(index);
// #elif defined(__GNUC__) || defined(__clang__)
//     return __builtin_ctzll(bb);
// #else
//     return std::countr_zero(bb);
// #endif
// }

// namespace WinChess {
//     namespace ChessEngine{

//         namespace Board {
//             uint64_t GeneratePseudoMoves(
//                 Pieces::PieceColor color,
//                 Pieces::PieceType type,
//                 int index,
//                 const BoardState& board
//             ) {
//                 switch (type) {
//                     case Pieces::PieceType::Pawn:   return GeneratePawnMoves(color, index, board);
//                     case Pieces::PieceType::Knight: return GenerateKnightMoves(color, index, board);
//                     case Pieces::PieceType::Bishop: return GenerateBishopMoves(color, index, board);
//                     case Pieces::PieceType::Rook:   return GenerateRookMoves(color, index, board);
//                     case Pieces::PieceType::Queen:  return GenerateQueenMoves(color, index, board);
//                     case Pieces::PieceType::King:   return GenerateKingMoves(color, index, board);
//                 }
//             }
//             uint64_t GenerateKnightMoves(
//                 Pieces::PieceColor color,
//                 int index,
//                 const BoardState& board
//             ) {
//                 uint64_t knight = 1ULL << index;
//                 uint64_t moves = 0ULL;

//                 const uint64_t notA  = ~((uint64_t)BitMask::FileA);
//                 const uint64_t notAB = ~((uint64_t)BitMask::FileA | (uint64_t)Board::BitMask::FileB);
//                 const uint64_t notH  = ~((uint64_t)BitMask::FileH);
//                 const uint64_t notGH = ~((uint64_t)BitMask::FileG | (uint64_t)Board::BitMask::FileH);

//                 moves |= ( knight << 17 ) & notA; // block wrap around
//                 moves |= ( knight << 15 ) & notH; // block wrap around
//                 moves |= ( knight << 10 ) & notAB; // block wrap around
//                 moves |= ( knight << 6 )  & notGH; // block wrap around
//                 moves |= ( knight >> 17 ) & notH; // block wrap around
//                 moves |= ( knight >> 15 ) & notA; // block wrap around
//                 moves |= ( knight >> 10 ) & notGH; // block wrap around
//                 moves |= ( knight >> 6 )  & notAB; // block wrap around

//                 // Remove friendly pieces
//                 uint64_t friendly = board.BitBoards.at(color).at(Pieces::PieceType::EveryPiece);
//                 moves &= ~friendly;
//                 return moves;
//             }
//             uint64_t GenerateKingMoves(
//                 Pieces::PieceColor color,
//                 int index,
//                 const BoardState& board
//             ) {
//                 uint64_t king = 1ULL << index;
//                 uint64_t moves = 0ULL;

//                 const uint64_t notA = ~(uint64_t)BitMask::FileA;
//                 const uint64_t notH = ~(uint64_t)BitMask::FileH;

//                 moves |= (king << 8);
//                 moves |= (king >> 8);
//                 moves |= (king << 1) & notA; // block wrap around
//                 moves |= (king >> 1) & notH; // block wrap around
//                 moves |= (king << 9) & notA; // block wrap around
//                 moves |= (king << 7) & notH; // block wrap around
//                 moves |= (king >> 9) & notH; // block wrap around
//                 moves |= (king >> 7) & notA; // block wrap around

//                 // Remove friendly pieces
//                 uint64_t friendly = board.BitBoards.at(color).at(Pieces::PieceType::EveryPiece);
//                 moves &= ~friendly;

//                 // ---- Castling ----
//                 // We assume BoardState has flags: board.CastlingRights[White/Black][KingSide/QueenSide]

//                 uint64_t occupied = board.BitBoards.at(Pieces::PieceColor::Both).at(Pieces::PieceType::EveryPiece);
                
//                 if (color == Pieces::PieceColor::White) {
//                     // White kingside castling (E1 → G1, rook H1 → F1)
//                     if (
//                         board.CastlingRights.WhiteKingSide &&
//                         !(occupied & (
//                             (uint64_t)Board::BitMask::F1 |
//                             (uint64_t)Board::BitMask::G1
//                         ))
//                     ) moves |= (uint64_t)Board::BitMask::G1; // King move to G1

//                     // White queenside castling (E1 → C1, rook A1 → D1)
//                     if (board.CastlingRights.WhiteQueenSide &&
//                         !(occupied & (
//                             (uint64_t)Board::BitMask::B1 |
//                             (uint64_t)Board::BitMask::C1 |
//                             (uint64_t)Board::BitMask::D1
//                         ))
//                     ) moves |= (uint64_t)Board::BitMask::C1; // King move to C1
//                 } else {
//                     // Black kingside castling (E8 → G8, rook H8 → F8)
//                     if (board.CastlingRights.BlackKingSide &&
//                         !(occupied & (
//                             (uint64_t)Board::BitMask::F8 |
//                             (uint64_t)Board::BitMask::G8
//                         ))
//                     ) moves |= (uint64_t)Board::BitMask::G8; // King move to G8

//                     // Black queenside castling (E8 → C8, rook A8 → D8)
//                     if (board.CastlingRights.BlackQueenSide &&
//                         !(occupied & (
//                             (uint64_t)Board::BitMask::B8 |
//                             (uint64_t)Board::BitMask::C8 |
//                             (uint64_t)Board::BitMask::D8
//                         ))
//                     ) moves |= (uint64_t)Board::BitMask::C8; // King move to C8
//                 }
//                 return moves;
//             }
//             uint64_t GenerateSlidingMoves(
//                 Pieces::PieceColor color,
//                 int index,
//                 const BoardState& board,
//                 const std::array<int, 4>& directions
//             ) {
//                 uint64_t moves = 0ULL;

//                 int startFile = index % 8;  // file = 0..7
//                 int startRank = index / 8;  // rank = 0..7

//                 for (int dir : directions) {
//                     int target = index;
//                     while (true) {
//                         int prev = target;
//                         target += dir;

//                         // board boundaries
//                         if (target < 0 || target > 63) break;

//                         int prevFile = prev % 8;
//                         int currFile = target % 8;
//                         int prevRank = prev / 8;
//                         int currRank = target / 8;

//                         // detect horizontal wrap-around
//                         if (std::abs(currFile - prevFile) > 1) break;

//                         uint64_t targetMask = 1ULL << target;
//                         moves |= targetMask;

//                         // stop at *any* piece (can capture, but not go past)
//                         if (board.BitBoards.at(Pieces::PieceColor::Both).at(Pieces::PieceType::EveryPiece) & targetMask) break;
//                     }
//                 }

//                 // remove friendly occupied squares
//                 uint64_t friendly = board.BitBoards.at(color).at(Pieces::PieceType::EveryPiece);
//                 moves &= ~friendly;
//                 return moves;
//             }

//             uint64_t GenerateRookMoves(
//                 Pieces::PieceColor color,
//                 int index,
//                 const BoardState& board
//             ) { return GenerateSlidingMoves(color, index, board, { 8,-8, 1, -1 }); }
//             uint64_t GenerateBishopMoves(
//                 Pieces::PieceColor color,
//                 int index,
//                 const BoardState& board
//             ) { return GenerateSlidingMoves(color, index, board, { 9, 7, -9, -7 }); }
//             uint64_t GenerateQueenMoves(
//                 Pieces::PieceColor color,
//                 int index,
//                 const BoardState& board
//             ) { 
//                 return (
//                     GenerateSlidingMoves(color, index, board, { 8,-8, 1, -1 }) |
//                     GenerateSlidingMoves(color, index, board, { 9, 7, -9, -7 })
//                 );
//             }
//             uint64_t GeneratePawnMoves(
//                 Pieces::PieceColor color,
//                 int index,
//                 const BoardState& board
//             ) {
//                 uint64_t pawn = 1ULL << index;
//                 uint64_t moves = 0;
//                 const uint64_t notA = ~(uint64_t)(Board::BitMask::FileA);
//                 const uint64_t notH = ~(uint64_t)(Board::BitMask::FileH);

//                 uint64_t stepFoward, toACapture, toHCapture, doubleForward, startRankMask;
//                 switch (color) {
//                     case Pieces::PieceColor::White:
//                         stepFoward = pawn << 8;
//                         toACapture = pawn << 7;
//                         toHCapture = pawn << 9;
//                         doubleForward = pawn << 16;
//                         startRankMask = (uint64_t)Board::BitMask::Rank2;
//                         break;
//                     case Pieces::PieceColor::Black:
//                         stepFoward = pawn >> 8;
//                         toACapture = pawn >> 9;
//                         toHCapture = pawn >> 7;
//                         doubleForward = pawn >> 16;
//                         startRankMask = (uint64_t)Board::BitMask::Rank7;
//                         break;
//                     default: throw std::runtime_error("Invalid Pawn Color");
//                 }

//                 uint64_t occupied = board.BitBoards.at(Pieces::PieceColor::Both).at(Pieces::PieceType::EveryPiece);
//                 /* Forward */
//                 if (!(stepFoward & occupied)) {
//                     moves |= stepFoward;
//                     if ((pawn & startRankMask) && !(doubleForward & occupied)) {
//                         moves |= doubleForward;
//                     }
//                 }
//                 /* Capture */
//                 Pieces::PieceColor opp = Pieces::OtherColor(color);
//                 moves |= (toACapture) & notH & board.BitBoards.at(opp).at(Pieces::PieceType::EveryPiece);
//                 moves |= (toHCapture) & notA & board.BitBoards.at(opp).at(Pieces::PieceType::EveryPiece);

//                 /* En Passant Captures */ // Remember this doesn't distinguish piece color
//                 if (board.EnPassantTarget) {
//                     moves |= (toACapture & notH & board.EnPassantTarget);
//                     moves |= (toHCapture & notA & board.EnPassantTarget);
//                 }

//                 // Remove friendly pieces
//                 uint64_t friendly = board.BitBoards.at(color).at(Pieces::PieceType::EveryPiece);
//                 moves &= ~friendly;
//                 return moves;
//             }

//             uint64_t GetAllEnemyAttacks(
//                 Pieces::PieceColor color,
//                 const BoardState& board
//             ) {
//                 Pieces::PieceColor opp = Pieces::OtherColor(color);
//                 uint64_t attacks = 0ULL;

//                 // Iterate over each piece type
//                 for (auto type : Pieces::PieceTypes) {
//                     if (type == Pieces::PieceType::EveryPiece) continue; // skip aggregate

//                     uint64_t bb = board.BitBoards.at(opp).at(type);
//                     while (bb) {
//                         int index = LSSBIndex(bb); // lowest set bit index
//                         bb &= bb - 1; // pop LSB

//                         switch (type) {
//                             case Pieces::PieceType::Pawn:
//                                 // Pawns are special: they only attack diagonally
//                                 {
//                                     uint64_t pawn = 1ULL << index;
//                                     if (opp == Pieces::PieceColor::White) {
//                                         attacks |= (pawn << 7) & ~(uint64_t)Board::BitMask::FileH;
//                                         attacks |= (pawn << 9) & ~(uint64_t)Board::BitMask::FileA;
//                                     } else {
//                                         attacks |= (pawn >> 9) & ~(uint64_t)Board::BitMask::FileH;
//                                         attacks |= (pawn >> 7) & ~(uint64_t)Board::BitMask::FileA;
//                                     }
//                                 }
//                                 break;
//                             case Pieces::PieceType::Knight:
//                                 attacks |= GenerateKnightMoves(opp, index, board);
//                                 break;
//                             case Pieces::PieceType::Bishop:
//                                 attacks |= GenerateBishopMoves(opp, index, board);
//                                 break;
//                             case Pieces::PieceType::Rook:
//                                 attacks |= GenerateRookMoves(opp, index, board);
//                                 break;
//                             case Pieces::PieceType::Queen:
//                                 attacks |= GenerateQueenMoves(opp, index, board);
//                                 break;
//                             case Pieces::PieceType::King:
//                                 // King moves without castling (attacks only adjacent squares)
//                                 {
//                                     uint64_t king = 1ULL << index;
//                                     const uint64_t notA = ~(uint64_t)Board::BitMask::FileA;
//                                     const uint64_t notH = ~(uint64_t)Board::BitMask::FileH;

//                                     attacks |= (king << 8);
//                                     attacks |= (king >> 8);
//                                     attacks |= (king << 1) & notA;
//                                     attacks |= (king >> 1) & notH;
//                                     attacks |= (king << 9) & notA;
//                                     attacks |= (king << 7) & notH;
//                                     attacks |= (king >> 9) & notH;
//                                     attacks |= (king >> 7) & notA;
//                                 }
//                                 break;
//                             default: break;
//                         }
//                     }
//                 }
//                 return attacks;
//             }
//             bool IsKingSafeAfterMove(
//                 Pieces::PieceColor color,
//                 const BoardState& board
//             ) {
//                 // Find king position
//                 uint64_t kingBB = board.BitBoards.at(color).at(Pieces::PieceType::King);
//                 if (!kingBB) return false; // no king found = invalid

//                 int kingIndex = LSSBIndex(kingBB);
//                 uint64_t kingMask = 1ULL << kingIndex;

//                 // Get all opponent attacks
//                 uint64_t oppAttacks = GetAllEnemyAttacks(color, board);

//                 // If the king's square is attacked, unsafe
//                 return (oppAttacks & kingMask) == 0ULL;
//             }

//             uint64_t GetLegalMovesForPiece(
//                 Pieces::PieceColor color,
//                 Pieces::PieceType type,
//                 int index,
//                 const BoardState& board
//             ) {
//                 uint64_t legalMoves = 0ULL;

//                 // Generate pseudo-legal moves for this piece
//                 uint64_t pseudoMoves = GeneratePseudoMoves(color, type, index, board);

//                 // Iterate through each pseudo move
//                 while (pseudoMoves) {
//                     int targetIndex = LSSBIndex(pseudoMoves);
//                     uint64_t targetMask = 1ULL << targetIndex;

//                     // Copy board to test move
//                     BoardState boardCopy = board;

//                     // --- 1) Remove piece from source ---
//                     boardCopy.BitBoards[color][type] &= ~(1ULL << index);
//                     boardCopy.BitBoards[Pieces::PieceColor::Both][type] &= ~(1ULL << index);
//                     boardCopy.BitBoards[color][Pieces::PieceType::EveryPiece] &= ~(1ULL << index);
//                     boardCopy.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~(1ULL << index);

//                     // --- 2) Capture enemy piece if present ---
//                     Pieces::PieceColor opponent = Pieces::OtherColor(color);
//                     for (auto pt : Pieces::PieceTypes) {
//                         if (pt == Pieces::PieceType::EveryPiece) continue;
//                         if (boardCopy.BitBoards[opponent][pt] & targetMask) {
//                             boardCopy.BitBoards[opponent][pt] &= ~targetMask;
//                             boardCopy.BitBoards[Pieces::PieceColor::Both][pt] &= ~targetMask;
//                             boardCopy.BitBoards[opponent][Pieces::PieceType::EveryPiece] &= ~targetMask;
//                             boardCopy.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~targetMask;
//                             break; // only one piece per square
//                         }
//                     }

//                     // --- 3) Move piece to target ---
//                     boardCopy.BitBoards[color][type] |= targetMask;
//                     boardCopy.BitBoards[Pieces::PieceColor::Both][type] |= targetMask;
//                     boardCopy.BitBoards[color][Pieces::PieceType::EveryPiece] |= targetMask;
//                     boardCopy.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] |= targetMask;

//                     // --- 4) Check king safety ---
//                     if (IsKingSafeAfterMove(color, boardCopy)) {
//                         legalMoves |= targetMask;
//                     }

//                     // Clear least significant bit and continue
//                     pseudoMoves &= pseudoMoves - 1;
//                 }

//                 return legalMoves;
//             }

//         }

//         void ChessEngineAPI::Init(const std::filesystem::path& projectRoot) {
//             std::filesystem::path pieceImageDir = projectRoot / "assets/images/pieces";
//             s_PieceTextures.clear();
// #define MAKE_PIECE_TEXTURE_UNIQUE(FILE_NAME, DIR_NAME) std::make_unique<Graphics::Texture>(Graphics::ConvertImageFormat(Graphics::ReadImageFile(FILE_NAME, DIR_NAME, Graphics::ImageFormat::RA8, false), Graphics::ImageFormat::RGBA8))
//             std::unordered_map<Pieces::PieceType, std::unique_ptr<Graphics::Texture>> whitePieces;
//             whitePieces.emplace(Pieces::PieceType::Pawn,   MAKE_PIECE_TEXTURE_UNIQUE("white-pawn.png", pieceImageDir));
//             whitePieces.emplace(Pieces::PieceType::Knight, MAKE_PIECE_TEXTURE_UNIQUE("white-knight.png", pieceImageDir));
//             whitePieces.emplace(Pieces::PieceType::Bishop, MAKE_PIECE_TEXTURE_UNIQUE("white-bishop.png", pieceImageDir));
//             whitePieces.emplace(Pieces::PieceType::Rook,   MAKE_PIECE_TEXTURE_UNIQUE("white-rook.png", pieceImageDir));
//             whitePieces.emplace(Pieces::PieceType::Queen,  MAKE_PIECE_TEXTURE_UNIQUE("white-queen.png", pieceImageDir));
//             whitePieces.emplace(Pieces::PieceType::King,   MAKE_PIECE_TEXTURE_UNIQUE("white-king.png", pieceImageDir));

//             std::unordered_map<Pieces::PieceType, std::unique_ptr<Graphics::Texture>> blackPieces;
//             blackPieces.emplace(Pieces::PieceType::Pawn,   MAKE_PIECE_TEXTURE_UNIQUE("black-pawn.png", pieceImageDir));
//             blackPieces.emplace(Pieces::PieceType::Knight, MAKE_PIECE_TEXTURE_UNIQUE("black-knight.png", pieceImageDir));
//             blackPieces.emplace(Pieces::PieceType::Bishop, MAKE_PIECE_TEXTURE_UNIQUE("black-bishop.png", pieceImageDir));
//             blackPieces.emplace(Pieces::PieceType::Rook,   MAKE_PIECE_TEXTURE_UNIQUE("black-rook.png", pieceImageDir));
//             blackPieces.emplace(Pieces::PieceType::Queen,  MAKE_PIECE_TEXTURE_UNIQUE("black-queen.png", pieceImageDir));
//             blackPieces.emplace(Pieces::PieceType::King,   MAKE_PIECE_TEXTURE_UNIQUE("black-king.png", pieceImageDir));

//             s_PieceTextures.emplace(Pieces::PieceColor::White, std::move(whitePieces));
//             s_PieceTextures.emplace(Pieces::PieceColor::Black, std::move(blackPieces));
//         }

//         void ChessEngineAPI::DrawPiecesForDisplay(
//             uint64_t bitboard,
//             ImTextureID texture,
//             ImDrawList* drawList,
//             const ImVec2& boardPos,
//             float cellSize
//         ) {
//             while (bitboard) {
//                 int index = LSSBIndex(bitboard);  // index of least significant set bit
//                 int x = index % 8;
//                 int y = 7 - (index / 8);

//                 ImVec2 top_left = ImVec2(boardPos.x + x * cellSize, boardPos.y + y * cellSize);
//                 ImVec2 bottom_right = ImVec2(top_left.x + cellSize, top_left.y + cellSize);
//                 drawList->AddImage(texture, top_left, bottom_right);

//                 bitboard &= bitboard - 1; // clear least significant set bit
//             }
//         }
//         void ChessEngineAPI::DisplayGameState(
//             const Board::BoardState& board,
//             const Board::BoardDisplayProps& props
//         ) {
//             ImGui::Begin(props.WindowName.c_str(), nullptr, ImGuiWindowFlags_NoResize);
//             // Get ImGui style for padding & borders
//             const ImGuiStyle& style = ImGui::GetStyle();
//             ImGui::SetWindowSize(ImVec2(
//                 props.CellSize * 8.0f + style.WindowPadding.x * 2.0f,
//                 props.CellSize * 8.0f + style.WindowPadding.y * 2.0f + ImGui::GetFrameHeight()
//             ));
//             ImDrawList* drawList = ImGui::GetWindowDrawList();
//             ImVec2 p = ImGui::GetCursorScreenPos();

//             /* Draw Board */
//             for (int y = 0; y < 8; y++) {
//                 for (int x = 0; x < 8; x++) {
//                     ImU32 col = ((x + y) % 2 == 0) ? Board::ImVec4ColorToImU32(props.EvenCellColor) : Board::ImVec4ColorToImU32(props.OddCellColor);
//                     ImVec2 top_left = ImVec2(
//                         p.x + x * props.CellSize,
//                         p.y + y * props.CellSize
//                     );
//                     ImVec2 bottom_right = ImVec2(
//                         top_left.x + props.CellSize,
//                         top_left.y + props.CellSize
//                     );
//                     drawList->AddRectFilled(top_left, bottom_right, col);
//                 }
//             }
// #define DRAW_PIECE_FOR_DISPLAY_WITH_PIECE(PIECE_COLOR, PIECE_TYPE, DRAW_LIST, BOARD_POS, CELL_SIZE) DrawPiecesForDisplay(board.BitBoards.at(PIECE_COLOR).at(PIECE_TYPE), (ImTextureID)(intptr_t)s_PieceTextures[PIECE_COLOR][PIECE_TYPE]->GetGLTexture(), DRAW_LIST, BOARD_POS, CELL_SIZE)
//             for (auto color : Pieces::PieceColors) { for (auto type : Pieces::PieceTypes) {
//                 DRAW_PIECE_FOR_DISPLAY_WITH_PIECE(color, type, drawList, p, props.CellSize);    
//             }}
//             ImGui::Dummy(ImVec2(props.CellSize * 8, props.CellSize * 8)); // reserve space
//             ImGui::End();
//         }

//         // Map Mouse to Square
//         bool ChessEngineAPI::MouseToBoard(
//             const ImVec2& boardPos,
//             float cellSize,
//             const ImVec2& mousePos,
//             int& outX,
//             int& outY
//         ) {
//             float mx = mousePos.x - boardPos.x;
//             float my = mousePos.y - boardPos.y;

//             // Mouse is not in square
//             if (mx < 0 || my < 0 || mx >= cellSize * 8 || my >= cellSize * 8) return false;

//             outX = static_cast<int>(mx / cellSize);
//             outY = static_cast<int>(my / cellSize);
//             return true;
//         }
//         void ChessEngineAPI::DrawPiecesForInteraction(
//             uint64_t bitboard,
//             ImTextureID texture,
//             ImDrawList* drawList,
//             const ImVec2& boardPos,
//             float cellSize,
//             Pieces::PieceColor color, // Color of the current drawing piece
//             Pieces::PieceType type, // Type of the current drawing piece
//             const Board::BoardState& gameBoard
//         ) {
//             while (bitboard) {
//                 int index = LSSBIndex(bitboard);  // index of least significant set bit
//                 int x = index % 8;
//                 int y = 7 - (index / 8);

//                 ImVec2 top_left = ImVec2(boardPos.x + x * cellSize, boardPos.y + y * cellSize);
//                 ImVec2 bottom_right = ImVec2(top_left.x + cellSize, top_left.y + cellSize);

//                 // check for drag start
//                 if (!s_PromotionPending && !s_Dragging && ImGui::IsMouseHoveringRect(top_left, bottom_right) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
//                     s_Dragging = true;
//                     s_DragColor = color;
//                     s_DragType = type;

//                     s_Selected = true;
//                     s_SelectedIndex = index;
//                     s_LegalMovesBB = Board::GetLegalMovesForPiece(color, type, index, gameBoard);
//                 }

//                 // if this piece is being dragged, skip drawing it in its original position
//                 if (s_PromotionPending || !(s_Dragging && s_DragColor == color && s_DragType == type && s_SelectedIndex == index)) {
//                     drawList->AddImage(texture, top_left, bottom_right);
//                 }

//                 bitboard &= bitboard - 1; // clear least significant set bit
//             }
//         }
//         void ChessEngineAPI::DisplayInteractiveGameState(
//             Board::BoardState& board,
//             const Board::BoardDisplayProps& props
//         ) {
//             ImGui::Begin(props.WindowName.c_str(), nullptr, ImGuiWindowFlags_NoResize);
//             // Get ImGui style for padding & borders
//             const ImGuiStyle& style = ImGui::GetStyle();
//             ImGui::SetWindowSize(ImVec2(
//                 props.CellSize * 8.0f + style.WindowPadding.x * 2.0f,
//                 props.CellSize * 8.0f + style.WindowPadding.y * 2.0f + ImGui::GetFrameHeight()
//             ));
//             /* A fixed subregion */
//             ImGui::BeginChild(
//                 "BoardArea",
//                 ImVec2(props.CellSize * 8, props.CellSize * 8),
//                 false,
//                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse
//             );
//             ImDrawList* drawList = ImGui::GetWindowDrawList();
//             ImVec2 p = ImGui::GetCursorScreenPos();
//             ImVec2 mouse = ImGui::GetIO().MousePos;

//             /* Draw Board */
//             for (int y = 0; y < 8; y++) {
//                 for (int x = 0; x < 8; x++) {
//                     ImU32 col = ((x + y) % 2 == 0) ? Board::ImVec4ColorToImU32(props.EvenCellColor) : Board::ImVec4ColorToImU32(props.OddCellColor);
//                     ImVec2 top_left = ImVec2(
//                         p.x + x * props.CellSize,
//                         p.y + y * props.CellSize
//                     );
//                     ImVec2 bottom_right = ImVec2(
//                         top_left.x + props.CellSize,
//                         top_left.y + props.CellSize
//                     );
//                     drawList->AddRectFilled(top_left, bottom_right, col);
//                 }
//             }
//             // Highlight selected cell
//             if (!s_PromotionPending && s_Selected) {
//                 int selX = s_SelectedIndex % 8;
//                 int selY = 7 - (s_SelectedIndex / 8);
//                 ImVec2 top_left = ImVec2(p.x + selX * props.CellSize, p.y + selY * props.CellSize);
//                 ImVec2 bottom_right = ImVec2(top_left.x + props.CellSize, top_left.y + props.CellSize);

//                 ImU32 highlightColor = ((selX + selY) % 2 == 0) ? Board::ImVec4ColorToImU32(props.EvenSelectedCellColor) : Board::ImVec4ColorToImU32(props.OddSelectedCellColor);
//                 drawList->AddRectFilled(top_left, bottom_right, highlightColor);
//             }

//             // Draw all pieces except dragging
// #define DRAW_PIECE_FOR_INTERACTION_WITH_PIECE(PIECE_COLOR, PIECE_TYPE, DRAW_LIST, BOARD_POS, CELL_SIZE, GAME_BOARD) DrawPiecesForInteraction(board.BitBoards[PIECE_COLOR][PIECE_TYPE], (ImTextureID)(intptr_t)s_PieceTextures[PIECE_COLOR][PIECE_TYPE]->GetGLTexture(), DRAW_LIST, BOARD_POS, CELL_SIZE, PIECE_COLOR, PIECE_TYPE, GAME_BOARD)
//             for (auto color : Pieces::PieceColors) { for (auto type : Pieces::PieceTypes) {
//                 DRAW_PIECE_FOR_INTERACTION_WITH_PIECE(color, type, drawList, p, props.CellSize, board);    
//             }}
            
//             // Draw legal move dots
//             uint64_t moves = s_LegalMovesBB;
//             while (moves) {
//                 int moveIndex = LSSBIndex(moves);
//                 int x = moveIndex % 8;
//                 int y = 7 - (moveIndex / 8);
//                 ImVec2 center(
//                     p.x + x * props.CellSize + props.CellSize / 2,
//                     p.y + y * props.CellSize + props.CellSize / 2
//                 );
//                 drawList->AddCircleFilled(center, props.CellSize * 0.15f, Board::ImVec4ColorToImU32(props.HighLightDotColor));
//                 moves &= moves - 1; // clear LSB
//             }

//             // Draw dragged piece at mouse
//             if (s_Dragging) {
//                 ImTextureID tex = s_PieceTextures[s_DragColor][s_DragType]->GetGLTexture();
//                 /* Draw the moving piece */
//                 drawList->AddImage(
//                     (ImTextureID)(intptr_t)tex,
//                     ImVec2(mouse.x - props.CellSize / 2, mouse.y - props.CellSize / 2),
//                     ImVec2(mouse.x + props.CellSize / 2, mouse.y + props.CellSize / 2)
//                 );
//                 if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
//                     s_Dragging = false;
//                     int targetX, targetY;
//                     if (MouseToBoard(p, props.CellSize, mouse, targetX, targetY)) {
//                         int targetIndex = (7 - targetY) * 8 + targetX; // back to bit index
//                         uint64_t targetMask = 1ULL << targetIndex;

//                         // Only apply move if it's legal
//                         if (s_LegalMovesBB & targetMask) {
//                             Pieces::PieceColor opponent = Pieces::OtherColor(s_DragColor);

//                             // Track whether we performed a capture and which square/type was removed
//                             bool didCapture = false;
//                             int capturedIndex = -1;
//                             Pieces::PieceType capturedType = Pieces::PieceType::Pawn; // default init

//                             // --- 1) Normal capture on target square ---
//                             if (board.BitBoards[opponent][Pieces::PieceType::EveryPiece] & targetMask) {
//                                 // Find which piece type occupies the target and remove it
//                                 for (auto type : Pieces::PieceTypes) {
//                                     uint64_t& oppBB = board.BitBoards[opponent][type];
//                                     if (oppBB & targetMask) {
//                                         oppBB &= ~targetMask;
//                                         capturedType = type;
//                                         capturedIndex = targetIndex;
//                                         didCapture = true;
//                                         break; // only one piece per square
//                                     }
//                                 }

//                                 // Update opponent cached bitboards
//                                 board.BitBoards[opponent][Pieces::PieceType::EveryPiece] &= ~targetMask;
//                                 board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~targetMask;
//                                 board.BitBoards[Pieces::PieceColor::Both][capturedType] &= ~targetMask;
//                             }
//                             // --- 2) En passant capture ---
//                             else if (s_DragType == Pieces::PieceType::Pawn /* && board.EnPassantTarget */ && (targetMask & board.EnPassantTarget)) {
//                                 // En passant capture removes pawn behind the target square
//                                 if (s_DragColor == Pieces::PieceColor::White) {
//                                     capturedIndex = targetIndex - 8;
//                                 } else {
//                                     capturedIndex = targetIndex + 8;
//                                 }
//                                 uint64_t capturedMask = 1ULL << capturedIndex;
//                                 // Remove captured pawn from opponent pawn bitboard
//                                 board.BitBoards[opponent][Pieces::PieceType::Pawn] &= ~capturedMask;

//                                 // Update cached opponent bitboards
//                                 board.BitBoards[opponent][Pieces::PieceType::EveryPiece] &= ~capturedMask;
//                                 board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~capturedMask;
//                                 board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Pawn] &= ~capturedMask;

//                                 capturedType = Pieces::PieceType::Pawn;
//                                 didCapture = true;
//                             }
//                             // --- 3) Castling ---
//                             if (s_DragType == Pieces::PieceType::King) {
//                                 // White castling
//                                 if (s_DragColor == Pieces::PieceColor::White) {
//                                     // Kingside castle (E1 -> G1)
//                                     if (s_SelectedIndex == 4 && targetIndex == 6) {
//                                         // Move rook from H1 -> F1
//                                         uint64_t rookMaskFrom = 1ULL << 7;
//                                         uint64_t rookMaskTo   = 1ULL << 5;
//                                         board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::Rook] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::Rook] |= rookMaskTo;
//                                         board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::EveryPiece] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::EveryPiece] |= rookMaskTo;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] |= rookMaskTo;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] |= rookMaskTo;
//                                     }
//                                     // Queenside castle (E1 -> C1)
//                                     else if (s_SelectedIndex == 4 && targetIndex == 2) {
//                                         // Move rook from A1 -> D1
//                                         uint64_t rookMaskFrom = 1ULL << 0;
//                                         uint64_t rookMaskTo   = 1ULL << 3;
//                                         board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::Rook] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::Rook] |= rookMaskTo;
//                                         board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::EveryPiece] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::White][Pieces::PieceType::EveryPiece] |= rookMaskTo;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] |= rookMaskTo;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] |= rookMaskTo;
//                                     }
//                                     board.CastlingRights.WhiteKingSide  = false;
//                                     board.CastlingRights.WhiteQueenSide = false;

//                                 }
//                                 // Black castling
//                                 else if (s_DragColor == Pieces::PieceColor::Black) {
//                                     // Kingside castle (E8 -> G8)
//                                     if (s_SelectedIndex == 60 && targetIndex == 62) {
//                                         // Move rook from H8 -> F8
//                                         uint64_t rookMaskFrom = 1ULL << 63;
//                                         uint64_t rookMaskTo   = 1ULL << 61;
//                                         board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::Rook] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::Rook] |= rookMaskTo;
//                                         board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::EveryPiece] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::EveryPiece] |= rookMaskTo;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] |= rookMaskTo;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] |= rookMaskTo;
//                                     }
//                                     // Queenside castle (E8 -> C8)
//                                     else if (s_SelectedIndex == 60 && targetIndex == 58) {
//                                         // Move rook from A8 -> D8
//                                         uint64_t rookMaskFrom = 1ULL << 56;
//                                         uint64_t rookMaskTo   = 1ULL << 59;
//                                         board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::Rook] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::Rook] |= rookMaskTo;
//                                         board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::EveryPiece] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::Black][Pieces::PieceType::EveryPiece] |= rookMaskTo;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook] |= rookMaskTo;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] &= ~rookMaskFrom;
//                                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece] |= rookMaskTo;
//                                     }
//                                     board.CastlingRights.BlackKingSide  = false;
//                                     board.CastlingRights.BlackQueenSide = false;
//                                 }
//                             } else if (s_DragType == Pieces::PieceType::Rook) {
//                                 switch (s_SelectedIndex) {
//                                     // White rooks
//                                     case 0: board.CastlingRights.WhiteQueenSide = false; break; // a1
//                                     case 7: board.CastlingRights.WhiteKingSide  = false; break; // h1
//                                     // Black rooks
//                                     case 56: board.CastlingRights.BlackQueenSide = false; break; // a8
//                                     case 63: board.CastlingRights.BlackKingSide  = false; break; // h8
//                                 }
//                             }

//                             // --- 3) Move the dragged piece from source to target ---
//                             uint64_t& bb = board.BitBoards[s_DragColor][s_DragType];
//                             bb &= ~(1ULL << s_SelectedIndex);    // Remove piece from source
//                             bb |= 1ULL << targetIndex;           // Place piece at target

//                             // Update cached "EveryPiece" for this color
//                             uint64_t& everyBB = board.BitBoards[s_DragColor][Pieces::PieceType::EveryPiece];
//                             everyBB &= ~(1ULL << s_SelectedIndex); // remove from old position
//                             everyBB |= 1ULL << targetIndex;        // add to new position

//                             // Update cached "Both" for this piece type
//                             uint64_t& bothBB_for_type = board.BitBoards[Pieces::PieceColor::Both][s_DragType];
//                             bothBB_for_type &= ~(1ULL << s_SelectedIndex); // remove from old position
//                             bothBB_for_type |= 1ULL << targetIndex;        // add to new position

//                             // Update cached "EveryPiece" for Both colors
//                             uint64_t& everyBothBB = board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece];
//                             everyBothBB &= ~(1ULL << s_SelectedIndex);
//                             everyBothBB |= 1ULL << targetIndex;

//                             // --- 4) Handle en passant target setting / clearing ---
//                             // Default: clear EnPassantTarget
//                             board.EnPassantTarget = 0ULL;
//                             // If the moved piece is a pawn and moved two squares, set EnPassantTarget to the square behind it
//                             if (s_DragType == Pieces::PieceType::Pawn && (std::abs(targetIndex / 8 - s_SelectedIndex / 8) == 2)) {
//                                 // set en passant square to the intermediate square
//                                 int epSquare = (s_SelectedIndex + targetIndex) / 2; // midpoint index
//                                 board.EnPassantTarget = 1ULL << epSquare;
//                             }

//                             // Remove Castling right for captured rooks
//                             if (didCapture && capturedType == Pieces::PieceType::Rook) {
//                                 switch (capturedIndex) {
//                                     // White rooks
//                                     case 0: board.CastlingRights.WhiteQueenSide = false; break;
//                                     case 7: board.CastlingRights.WhiteKingSide  = false; break;
//                                     // Black rooks
//                                     case 56: board.CastlingRights.BlackQueenSide = false; break;
//                                     case 63: board.CastlingRights.BlackKingSide  = false; break;
//                                 }
//                             }

//                             // Pawn reached promotion rank
//                             if (s_DragType == Pieces::PieceType::Pawn) {
//                                 int targetRank = targetIndex / 8;
//                                 if ((s_DragColor == Pieces::PieceColor::White && targetRank == 7) ||
//                                     (s_DragColor == Pieces::PieceColor::Black && targetRank == 0)) {
//                                     // Block game state
//                                     s_PromotionPending = true;
//                                     s_PromotionSquare = targetIndex;
//                                     s_PromotionColor = s_DragColor;
//                                 }
//                             }
//                             // Optionally clear selection after a successful move
//                             s_Selected = false;
//                             s_SelectedIndex = -1;
//                             s_LegalMovesBB = 0ULL;
//                         }
//                     }
//                 }

//             }
//             ImGui::Dummy(ImVec2(props.CellSize * 8, props.CellSize * 8)); // reserve space
//             ImGui::EndChild();

//             if (s_PromotionPending) {
//                 ImGui::OpenPopup("PromotionPopup");
//             }

//             if (ImGui::BeginPopup("PromotionPopup")) {
//                 ImGui::Text("Promote to:");
//                 const Pieces::PieceType options[] = {
//                     Pieces::PieceType::Queen,
//                     Pieces::PieceType::Rook,
//                     Pieces::PieceType::Bishop,
//                     Pieces::PieceType::Knight
//                 };
        
//                 for (auto choice : options) {
//                     std::string label;
//                     switch (choice) {
//                         case Pieces::PieceType::Queen:  label = "Queen"; break;
//                         case Pieces::PieceType::Rook:   label = "Rook"; break;
//                         case Pieces::PieceType::Bishop: label = "Bishop"; break;
//                         case Pieces::PieceType::Knight: label = "Knight"; break;
//                         default: continue;
//                     }
        
//                     if (ImGui::ImageButton(
//                         label.c_str(),
//                         (ImTextureID)(intptr_t)s_PieceTextures[s_PromotionColor][choice]->GetGLTexture(),
//                         ImVec2(64, 64),
//                         ImVec2(0, 0), ImVec2(1, 1),
//                         ((s_PromotionSquare / 8 + s_PromotionSquare % 8) % 2 == 1) ? Board::NormalizeImVec4Color(props.EvenCellColor) : Board::NormalizeImVec4Color(props.OddCellColor)
//                     )) {
//                         uint64_t mask = 1ULL << s_PromotionSquare;
//                         // Remove pawn from target
//                         board.BitBoards[s_PromotionColor][Pieces::PieceType::Pawn] &= ~mask;
//                         board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Pawn] &= ~mask;

//                         board.BitBoards[s_PromotionColor][choice] |= mask;
//                         board.BitBoards[Pieces::PieceColor::Both][choice] |= mask;
        
//                         // unblock state
//                         s_PromotionPending = false;
//                         s_PromotionSquare = -1;
//                         ImGui::CloseCurrentPopup();
//                     }
//                 }
//                 ImGui::EndPopup();
//             }
//             ImGui::End();
//         }
//         // // helper to move rook inside board state
//         // void Board::MoveRook(
//         //     Pieces::PieceColor color,
//         //     Board::BoardState& board,
//         //     int from, int to
//         // ) {
//         //     uint64_t fromMask = 1ULL << from;
//         //     uint64_t toMask   = 1ULL << to;

//         //     uint64_t& rookBB = board.BitBoards[color][Pieces::PieceType::Rook];
//         //     rookBB &= ~fromMask;
//         //     rookBB |= toMask;

//         //     uint64_t& everyBB = board.BitBoards[color][Pieces::PieceType::EveryPiece];
//         //     everyBB &= ~fromMask;
//         //     everyBB |= toMask;

//         //     uint64_t& bothBB = board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::Rook];
//         //     bothBB &= ~fromMask;
//         //     bothBB |= toMask;

//         //     uint64_t& everyBothBB = board.BitBoards[Pieces::PieceColor::Both][Pieces::PieceType::EveryPiece];
//         //     everyBothBB &= ~fromMask;
//         //     everyBothBB |= toMask;
//         // }
//     }
// }

