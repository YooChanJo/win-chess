#include "ChessEngineAPI.h"

namespace WinChess {
    namespace ChessEngine {

        void ChessEngineAPI::Init(const std::filesystem::path& projectRoot) {
            std::filesystem::path pieceImageDir = projectRoot / "assets/images/pieces";
            s_PieceTextures.clear();
#define MAKE_PIECE_TEXTURE_UNIQUE(FILE_NAME, DIR_NAME) std::make_unique<Graphics::Texture>(Graphics::ConvertImageFormat(Graphics::ReadImageFile(FILE_NAME, DIR_NAME, Graphics::ImageFormat::RA8, false), Graphics::ImageFormat::RGBA8))

            std::unordered_map<Pieces::PieceType, std::unique_ptr<Graphics::Texture>> whitePieces;
            whitePieces.emplace(Pieces::PieceType::Pawn,   MAKE_PIECE_TEXTURE_UNIQUE("white-pawn.png", pieceImageDir));
            whitePieces.emplace(Pieces::PieceType::Knight, MAKE_PIECE_TEXTURE_UNIQUE("white-knight.png", pieceImageDir));
            whitePieces.emplace(Pieces::PieceType::Bishop, MAKE_PIECE_TEXTURE_UNIQUE("white-bishop.png", pieceImageDir));
            whitePieces.emplace(Pieces::PieceType::Rook,   MAKE_PIECE_TEXTURE_UNIQUE("white-rook.png", pieceImageDir));
            whitePieces.emplace(Pieces::PieceType::Queen,  MAKE_PIECE_TEXTURE_UNIQUE("white-queen.png", pieceImageDir));
            whitePieces.emplace(Pieces::PieceType::King,   MAKE_PIECE_TEXTURE_UNIQUE("white-king.png", pieceImageDir));

            std::unordered_map<Pieces::PieceType, std::unique_ptr<Graphics::Texture>> blackPieces;
            blackPieces.emplace(Pieces::PieceType::Pawn,   MAKE_PIECE_TEXTURE_UNIQUE("black-pawn.png", pieceImageDir));
            blackPieces.emplace(Pieces::PieceType::Knight, MAKE_PIECE_TEXTURE_UNIQUE("black-knight.png", pieceImageDir));
            blackPieces.emplace(Pieces::PieceType::Bishop, MAKE_PIECE_TEXTURE_UNIQUE("black-bishop.png", pieceImageDir));
            blackPieces.emplace(Pieces::PieceType::Rook,   MAKE_PIECE_TEXTURE_UNIQUE("black-rook.png", pieceImageDir));
            blackPieces.emplace(Pieces::PieceType::Queen,  MAKE_PIECE_TEXTURE_UNIQUE("black-queen.png", pieceImageDir));
            blackPieces.emplace(Pieces::PieceType::King,   MAKE_PIECE_TEXTURE_UNIQUE("black-king.png", pieceImageDir));

            s_PieceTextures.emplace(Pieces::PieceColor::White, std::move(whitePieces));
            s_PieceTextures.emplace(Pieces::PieceColor::Black, std::move(blackPieces));
        }

        void ChessEngineAPI::DrawPiecesForDisplay(
            uint64_t bitboard,
            ImTextureID texture,
            ImDrawList* drawList,
            const ImVec2& boardPos,
            float cellSize
        ) {
            while (bitboard) {
                int index = Pieces::LSSBIndex(bitboard);  // index of least significant set bit
                int x = index % 8;
                int y = 7 - (index / 8);

                ImVec2 top_left = ImVec2(boardPos.x + x * cellSize, boardPos.y + y * cellSize);
                ImVec2 bottom_right = ImVec2(top_left.x + cellSize, top_left.y + cellSize);
                drawList->AddImage(texture, top_left, bottom_right);

                bitboard &= bitboard - 1; // clear least significant set bit
            }
        }

        void ChessEngineAPI::DisplayGameState(
            const Board::BoardState& board,
            const BoardDisplayProps& props
        ) {
            ImGui::Begin(props.WindowName.c_str(), nullptr, ImGuiWindowFlags_NoResize);
            const ImGuiStyle& style = ImGui::GetStyle();
            ImGui::SetWindowSize(ImVec2(
                props.CellSize * 8.0f + style.WindowPadding.x * 2.0f,
                props.CellSize * 8.0f + style.WindowPadding.y * 2.0f + ImGui::GetFrameHeight()
            ));
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();

            // Draw board colors
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    ImU32 col = ((x + y) % 2 == 0) ? ImVec4ColorToImU32(props.EvenCellColor) : ImVec4ColorToImU32(props.OddCellColor);
                    ImVec2 top_left = ImVec2(p.x + x * props.CellSize, p.y + y * props.CellSize);
                    ImVec2 bottom_right = ImVec2(top_left.x + props.CellSize, top_left.y + props.CellSize);
                    drawList->AddRectFilled(top_left, bottom_right, col);
                }
            }

            // Draw pieces for both colors and normal piece types
            for (auto color : Pieces::PieceColors) {
                for (auto type : Pieces::PieceTypes) {
                    uint64_t bb = board.BitBoards.at(color).at(type);
                    if (bb) {
                        ImTextureID tex = (ImTextureID)(intptr_t)s_PieceTextures[color][type]->GetGLTexture();
                        DrawPiecesForDisplay(bb, tex, drawList, p, props.CellSize);
                    }
                }
            }

            ImGui::Dummy(ImVec2(props.CellSize * 8, props.CellSize * 8)); // reserve space
            ImGui::End();
        }

        bool ChessEngineAPI::MouseToBoard(
            const ImVec2& boardPos,
            float cellSize,
            const ImVec2& mousePos,
            int& outX,
            int& outY
        ) {
            float mx = mousePos.x - boardPos.x;
            float my = mousePos.y - boardPos.y;

            if (mx < 0 || my < 0 || mx >= cellSize * 8 || my >= cellSize * 8) return false;

            outX = static_cast<int>(mx / cellSize);
            outY = static_cast<int>(my / cellSize);
            return true;
        }

        void ChessEngineAPI::DrawPiecesForInteraction(
            uint64_t bitboard,
            ImTextureID texture,
            ImDrawList* drawList,
            const ImVec2& boardPos,
            float cellSize,
            Pieces::PieceColor color, // Color of the current drawing piece
            Pieces::PieceType type, // Type of the current drawing piece
            const Board::BoardState& gameBoard
        ) {
            while (bitboard) {
                int index = Pieces::LSSBIndex(bitboard);  // index of least significant set bit
                int x = index % 8;
                int y = 7 - (index / 8);

                ImVec2 top_left = ImVec2(boardPos.x + x * cellSize, boardPos.y + y * cellSize);
                ImVec2 bottom_right = ImVec2(top_left.x + cellSize, top_left.y + cellSize);

                // check for drag start
                if (!gameBoard.PromotionPending && !s_Dragging && ImGui::IsMouseHoveringRect(top_left, bottom_right) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    s_Dragging = true;
                    s_DragColor = color;
                    s_DragType = type;

                    s_Selected = true;
                    s_SelectedIndex = index;
                    s_LegalMovesBB = Board::GetLegalMovesForPiece(color, type, index, gameBoard);
                }

                // if this piece is being dragged, skip drawing it in its original position
                if (!(s_Dragging && s_DragColor == color && s_DragType == type && s_SelectedIndex == index && !gameBoard.PromotionPending)) {
                    drawList->AddImage(texture, top_left, bottom_right);
                }

                bitboard &= bitboard - 1; // clear least significant set bit
            }
        }

        void ChessEngineAPI::DisplayInteractiveGameState(
            Board::BoardState& board,
            const BoardDisplayProps& props
        ) {
            ImGui::Begin(props.WindowName.c_str(), nullptr, ImGuiWindowFlags_NoResize);
            const ImGuiStyle& style = ImGui::GetStyle();
            ImGui::SetWindowSize(ImVec2(
                props.CellSize * 8.0f + style.WindowPadding.x * 2.0f,
                props.CellSize * 8.0f + style.WindowPadding.y * 2.0f + ImGui::GetFrameHeight()
            ));

            ImGui::BeginChild(
                "BoardArea",
                ImVec2(props.CellSize * 8, props.CellSize * 8),
                false,
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse
            );
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImVec2 mouse = ImGui::GetIO().MousePos;

            // Draw board
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    ImU32 col = ((x + y) % 2 == 0) ? ImVec4ColorToImU32(props.EvenCellColor) : ImVec4ColorToImU32(props.OddCellColor);
                    ImVec2 top_left = ImVec2(p.x + x * props.CellSize, p.y + y * props.CellSize);
                    ImVec2 bottom_right = ImVec2(top_left.x + props.CellSize, top_left.y + props.CellSize);
                    drawList->AddRectFilled(top_left, bottom_right, col);
                }
            }

            // Highlight selected cell
            if (!board.PromotionPending && s_Selected) {
                int selX = s_SelectedIndex % 8;
                int selY = 7 - (s_SelectedIndex / 8);
                ImVec2 top_left = ImVec2(p.x + selX * props.CellSize, p.y + selY * props.CellSize);
                ImVec2 bottom_right = ImVec2(top_left.x + props.CellSize, top_left.y + props.CellSize);

                ImU32 highlightColor = ((selX + selY) % 2 == 0) ? ImVec4ColorToImU32(props.EvenSelectedCellColor) : ImVec4ColorToImU32(props.OddSelectedCellColor);
                drawList->AddRectFilled(top_left, bottom_right, highlightColor);
            }

            // Draw all pieces except dragging ones
            for (auto color : Pieces::PieceColors) {
                for (auto type : Pieces::PieceTypes) {
                    uint64_t bb = board.BitBoards[color][type];
                    if (!bb) continue;
                    ImTextureID tex = (ImTextureID)(intptr_t)s_PieceTextures[color][type]->GetGLTexture();
                    DrawPiecesForInteraction(bb, tex, drawList, p, props.CellSize, color, type, board);
                }
            }

            // Draw legal move dots
            uint64_t moves = s_LegalMovesBB;
            while (moves) {
                int moveIndex = Pieces::LSSBIndex(moves);
                int x = moveIndex % 8;
                int y = 7 - (moveIndex / 8);
                ImVec2 center(
                    p.x + x * props.CellSize + props.CellSize / 2,
                    p.y + y * props.CellSize + props.CellSize / 2
                );
                drawList->AddCircleFilled(center, props.CellSize * 0.15f, ImVec4ColorToImU32(props.HighLightDotColor));
                moves &= moves - 1; // clear LSB
            }

            // Draw dragged piece at mouse
            if (s_Dragging) {
                ImTextureID tex = (ImTextureID)(intptr_t)s_PieceTextures[s_DragColor][s_DragType]->GetGLTexture();
                drawList->AddImage(
                    tex,
                    ImVec2(mouse.x - props.CellSize / 2, mouse.y - props.CellSize / 2),
                    ImVec2(mouse.x + props.CellSize / 2, mouse.y + props.CellSize / 2)
                );

                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    s_Dragging = false;
                    int targetX, targetY;
                    if (MouseToBoard(p, props.CellSize, mouse, targetX, targetY)) {
                        int targetIndex = (7 - targetY) * 8 + targetX;
                        uint64_t targetMask = 1ULL << targetIndex;
                        // Only apply move if it's legal
                        if (s_LegalMovesBB & targetMask) {
                            // Call Board::MovePiece which updates board state and sets promotion pending if needed
                            Board::MoveOutcome outcome = Board::MovePiece(board, s_SelectedIndex, targetIndex);
                            // If promotion required, Board::MovePiece sets board.PromotionPending; UI will show popup below
                            // Clear selection if move succeeded (if promotion pending we still clear selection to avoid re-entrancy)
                            if (outcome.success) {
                                s_Selected = false;
                                s_SelectedIndex = -1;
                                s_LegalMovesBB = 0ULL;
                            }
                        }
                    } else {
                        // Released outside board - clear selection
                        s_Selected = false;
                        s_SelectedIndex = -1;
                        s_LegalMovesBB = 0ULL;
                    }
                }
            }

            ImGui::Dummy(ImVec2(props.CellSize * 8, props.CellSize * 8)); // reserve space
            ImGui::EndChild();

            // Promotion popup handled via BoardState.PromotionPending
            if (board.PromotionPending) {
                ImGui::OpenPopup("PromotionPopup");
            }

            if (ImGui::BeginPopup("PromotionPopup")) {
                ImGui::Text("Promote to:");
                const Pieces::PieceType options[] = {
                    Pieces::PieceType::Queen,
                    Pieces::PieceType::Rook,
                    Pieces::PieceType::Bishop,
                    Pieces::PieceType::Knight
                };

                for (auto choice : options) {
                    std::string label;
                    switch (choice) {
                        case Pieces::PieceType::Queen:  label = "Queen"; break;
                        case Pieces::PieceType::Rook:   label = "Rook"; break;
                        case Pieces::PieceType::Bishop: label = "Bishop"; break;
                        case Pieces::PieceType::Knight: label = "Knight"; break;
                        default: continue;
                    }

                    if (ImGui::ImageButton(
                        label.c_str(),
                        (ImTextureID)(intptr_t)s_PieceTextures[board.PromotionColor][choice]->GetGLTexture(),
                        ImVec2(64, 64),
                        ImVec2(0, 0), ImVec2(1, 1),
                        ((board.PromotionSquare / 8 + board.PromotionSquare % 8) % 2 == 1) ? NormalizeImVec4Color(props.EvenCellColor) : NormalizeImVec4Color(props.OddCellColor)
                    )) {
                        // Apply promotion via Board function
                        if (Board::ApplyPromotion(board, choice)) {
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                ImGui::EndPopup();
            }

            ImGui::End();
        }
    }
}
