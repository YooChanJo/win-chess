#include "ChessEngineAPI.h"
#include "BitUtils.h"

namespace WinChess {
    namespace ChessEngine {
        using namespace Pieces;
        using namespace Board;

        void ChessEngineAPI::Init(const std::filesystem::path& projectRoot) {
            std::filesystem::path pieceImageDir = projectRoot / "assets/images/pieces";
            s_PieceTextures = {};
#define MAKE_PIECE_TEXTURE_UNIQUE(FILE_NAME, DIR_NAME) std::make_unique<Graphics::Texture>(Graphics::ConvertImageFormat(Graphics::ReadImageFile(FILE_NAME, DIR_NAME, Graphics::ImageFormat::RA8, false), Graphics::ImageFormat::RGBA8))

            ChessEngineAPI::GetPieceTexture(PieceColor::White, PieceType::Pawn  ) = MAKE_PIECE_TEXTURE_UNIQUE("white-pawn.png",     pieceImageDir);
            ChessEngineAPI::GetPieceTexture(PieceColor::White, PieceType::Knight) = MAKE_PIECE_TEXTURE_UNIQUE("white-knight.png",   pieceImageDir);
            ChessEngineAPI::GetPieceTexture(PieceColor::White, PieceType::Bishop) = MAKE_PIECE_TEXTURE_UNIQUE("white-bishop.png",   pieceImageDir);
            ChessEngineAPI::GetPieceTexture(PieceColor::White, PieceType::Rook  ) = MAKE_PIECE_TEXTURE_UNIQUE("white-rook.png",     pieceImageDir);
            ChessEngineAPI::GetPieceTexture(PieceColor::White, PieceType::Queen ) = MAKE_PIECE_TEXTURE_UNIQUE("white-queen.png",    pieceImageDir);
            ChessEngineAPI::GetPieceTexture(PieceColor::White, PieceType::King  ) = MAKE_PIECE_TEXTURE_UNIQUE("white-king.png",     pieceImageDir);
            ChessEngineAPI::GetPieceTexture(PieceColor::Black, PieceType::Pawn  ) = MAKE_PIECE_TEXTURE_UNIQUE("black-pawn.png",     pieceImageDir);
            ChessEngineAPI::GetPieceTexture(PieceColor::Black, PieceType::Knight) = MAKE_PIECE_TEXTURE_UNIQUE("black-knight.png",   pieceImageDir);
            ChessEngineAPI::GetPieceTexture(PieceColor::Black, PieceType::Bishop) = MAKE_PIECE_TEXTURE_UNIQUE("black-bishop.png",   pieceImageDir);
            ChessEngineAPI::GetPieceTexture(PieceColor::Black, PieceType::Rook  ) = MAKE_PIECE_TEXTURE_UNIQUE("black-rook.png",     pieceImageDir);
            ChessEngineAPI::GetPieceTexture(PieceColor::Black, PieceType::Queen ) = MAKE_PIECE_TEXTURE_UNIQUE("black-queen.png",    pieceImageDir);
            ChessEngineAPI::GetPieceTexture(PieceColor::Black, PieceType::King  ) = MAKE_PIECE_TEXTURE_UNIQUE("black-king.png",     pieceImageDir);
        }

        void ChessEngineAPI::DrawPiecesForDisplay(
            uint64_t bitboard,
            ImTextureID texture,
            ImDrawList* drawList,
            const ImVec2& boardPos,
            float cellSize
        ) {
            while (bitboard) {
                int index = BitUtils::LSSBIndex(bitboard);
                int x = index % 8;
                int y = 7 - (index / 8);

                ImVec2 top_left = ImVec2(boardPos.x + x * cellSize, boardPos.y + y * cellSize);
                ImVec2 bottom_right = ImVec2(top_left.x + cellSize, top_left.y + cellSize);
                drawList->AddImage(texture, top_left, bottom_right);

                bitboard &= bitboard - 1;
            }
        }

        inline static std::string GameStatusToString(GameStatus s) {
            switch (s) {
                case GameStatus::Ongoing:    return "Ongoing";
                case GameStatus::Check:      return "Check";
                case GameStatus::Checkmate:  return "Checkmate";
                case GameStatus::Stalemate:  return "Stalemate";
                case GameStatus::Draw:       return "Draw";
                default:                            return "Unknown";
            }
        }

        inline static std::string PlayerColorToString(Pieces::PieceColor c) {
            switch (c) {
                case Pieces::PieceColor::White: return "White";
                case Pieces::PieceColor::Black: return "Black";
                default: return "None";
            }
        }

        void ChessEngineAPI::DisplayGameState(const BoardState& board, const BoardDisplayProps& props) {
            ImGui::Begin(props.WindowName.c_str(), nullptr, ImGuiWindowFlags_NoResize);
            const ImGuiStyle& style = ImGui::GetStyle();

            const float statusBarHeight = ImGui::GetFrameHeight();
            const float sidebarWidth = props.CellSize * 4.0f;
            ImGui::SetWindowSize(ImVec2(
                props.CellSize * 8.0f + sidebarWidth + style.WindowPadding.x * 2.0f + 16.0f,
                props.CellSize * 8.0f + style.WindowPadding.y * 2.0f + statusBarHeight * 2.0f + 8.0f
            ));

            {
                std::string status = "Turn: " + PlayerColorToString(board.ActiveColor) + "    State: " + GameStatusToString(board.GameStatus) + "    Score W:" + std::to_string(board.WhiteScore) + " - B:" + std::to_string(board.BlackScore);
                if (board.GameOver) {
                    if (board.Winner == Pieces::PieceColor::None) status += " (Draw)";
                    else status += " -- Winner: " + PlayerColorToString(board.Winner);
                }
                ImGui::TextUnformatted(status.c_str());
            }

            // Left: Board
            ImGui::BeginChild("BoardArea", ImVec2(props.CellSize * 8, props.CellSize * 8), false);

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();

            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    ImU32 col = ((x + y) % 2 == 0) ? ImVec4ColorToImU32(props.EvenCellColor) : ImVec4ColorToImU32(props.OddCellColor);
                    ImVec2 top_left = ImVec2(p.x + x * props.CellSize, p.y + y * props.CellSize);
                    ImVec2 bottom_right = ImVec2(top_left.x + props.CellSize, top_left.y + props.CellSize);
                    drawList->AddRectFilled(top_left, bottom_right, col);
                }
            }

            for (auto color : Pieces::PieceColors) {
                for (auto type : Pieces::PieceTypes) {
                    uint64_t bb = board.GetConstBB(color, type);
                    if (bb) {
                        ImTextureID tex = (ImTextureID)(intptr_t)ChessEngineAPI::GetPieceTexture(color, type)->GetGLTexture();
                        DrawPiecesForDisplay(bb, tex, drawList, p, props.CellSize);
                    }
                }
            }

            ImGui::Dummy(ImVec2(props.CellSize * 8, props.CellSize * 8));
            ImGui::EndChild();
            // Right Sidebar
            ImGui::SameLine();
            ImGui::BeginChild("RightSidebar", ImVec2(sidebarWidth, props.CellSize * 8), true);

            if (ImGui::CollapsingHeader("Move History", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::BeginChild("HistoryList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
                const auto &hist = board.MoveHistory;
                for (size_t i = 0; i < hist.size(); ++i) {
                    char buf[64];
                    int n = snprintf(buf, sizeof(buf), "%zu. %s", i + 1, hist[i].c_str());
                    ImGui::TextUnformatted(buf, buf + n);
                }
                ImGui::EndChild();
            }

            // placeholder for future tools
            ImGui::Separator();
            ImGui::Text("Future tools go here...");
            ImGui::EndChild();
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
            Pieces::PieceColor color,
            Pieces::PieceType type,
            BoardState& gameBoard
        ) {
            while (bitboard) {
                int index = BitUtils::LSSBIndex(bitboard);
                int x = index % 8;
                int y = 7 - (index / 8);

                ImVec2 top_left = ImVec2(boardPos.x + x * cellSize, boardPos.y + y * cellSize);
                ImVec2 bottom_right = ImVec2(top_left.x + cellSize, top_left.y + cellSize);

                if (!PromotionPending(gameBoard.PromotionStatus) && !s_dragging && color == gameBoard.ActiveColor
                    && ImGui::IsMouseHoveringRect(top_left, bottom_right) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    s_dragging = true;
                    s_dragColor = color;
                    s_dragType = type;

                    s_selected = true;
                    s_selectedIndex = index;
                    s_legalMovesBB = GetLegalMovesForPiece(gameBoard, index);
                }

                if (!(s_dragging && s_dragColor == color && s_dragType == type && s_selectedIndex == index && !PromotionPending(gameBoard.PromotionStatus))) {
                    drawList->AddImage(texture, top_left, bottom_right);
                }

                bitboard &= bitboard - 1;
            }
        }

        void ChessEngineAPI::DisplayInteractiveGameState(
            BoardState& board,
            const BoardDisplayProps& props
        ) {
            ImGui::Begin(props.WindowName.c_str(), nullptr, ImGuiWindowFlags_NoResize);
            const ImGuiStyle& style = ImGui::GetStyle();

            const float statusBarHeight = ImGui::GetFrameHeight();
            const float sidebarWidth = props.CellSize * 4.0f;
            ImGui::SetWindowSize(ImVec2(
                props.CellSize * 8.0f + sidebarWidth + style.WindowPadding.x * 2.0f + 16.0f,
                props.CellSize * 8.0f + style.WindowPadding.y * 2.0f + statusBarHeight * 2.0f + 8.0f
            ));

            {
                std::string status = "Turn: " + PlayerColorToString(board.ActiveColor) + "    State: " + GameStatusToString(board.GameStatus) + "    Score W:" + std::to_string(board.WhiteScore) + " - B:" + std::to_string(board.BlackScore);
                if (board.GameOver) {
                    if (board.Winner == Pieces::PieceColor::None) status += " (Draw)";
                    else status += " -- Winner: " + PlayerColorToString(board.Winner);
                }
                ImGui::TextUnformatted(status.c_str());
            }

            // Left: Board
            ImGui::BeginChild("BoardArea", ImVec2(props.CellSize * 8, props.CellSize * 8), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse);
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImVec2 mouse = ImGui::GetIO().MousePos;

            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    ImU32 col = ((x + y) % 2 == 0) ? ImVec4ColorToImU32(props.EvenCellColor) : ImVec4ColorToImU32(props.OddCellColor);
                    ImVec2 top_left = ImVec2(p.x + x * props.CellSize, p.y + y * props.CellSize);
                    ImVec2 bottom_right = ImVec2(top_left.x + props.CellSize, top_left.y + props.CellSize);
                    drawList->AddRectFilled(top_left, bottom_right, col);
                }
            }

            if (!PromotionPending(board.PromotionStatus) && s_selected) {
                int selX = s_selectedIndex % 8;
                int selY = 7 - (s_selectedIndex / 8);
                ImVec2 top_left = ImVec2(p.x + selX * props.CellSize, p.y + selY * props.CellSize);
                ImVec2 bottom_right = ImVec2(top_left.x + props.CellSize, top_left.y + props.CellSize);
                ImU32 highlightColor = ((selX + selY) % 2 == 0) ? ImVec4ColorToImU32(props.EvenSelectedCellColor) : ImVec4ColorToImU32(props.OddSelectedCellColor);
                drawList->AddRectFilled(top_left, bottom_right, highlightColor);
            }

            for (auto color : Pieces::PieceColors) {
                for (auto type : Pieces::PieceTypes) {
                    uint64_t bb = board.GetConstBB(color, type);
                    if (!bb) continue;
                    ImTextureID tex = (ImTextureID)(intptr_t)ChessEngineAPI::GetPieceTexture(color, type)->GetGLTexture();
                    DrawPiecesForInteraction(bb, tex, drawList, p, props.CellSize, color, type, board);
                }
            }

            uint64_t moves = s_legalMovesBB;
            while (moves) {
                int moveIndex = BitUtils::LSSBIndex(moves);
                int x = moveIndex % 8;
                int y = 7 - (moveIndex / 8);
                ImVec2 center(
                    p.x + x * props.CellSize + props.CellSize / 2,
                    p.y + y * props.CellSize + props.CellSize / 2
                );
                drawList->AddCircleFilled(center, props.CellSize * 0.15f, ImVec4ColorToImU32(props.HighLightDotColor));
                moves &= moves - 1;
            }

            if (s_dragging) {
                ImTextureID tex = (ImTextureID)(intptr_t)ChessEngineAPI::GetPieceTexture(s_dragColor, s_dragType)->GetGLTexture();
                drawList->AddImage(
                    tex,
                    ImVec2(mouse.x - props.CellSize / 2, mouse.y - props.CellSize / 2),
                    ImVec2(mouse.x + props.CellSize / 2, mouse.y + props.CellSize / 2)
                );

                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    s_dragging = false;
                    int targetX, targetY;
                    if (MouseToBoard(p, props.CellSize, mouse, targetX, targetY)) {
                        int targetIndex = (7 - targetY) * 8 + targetX;
                        uint64_t targetMask = 1ULL << targetIndex;
                        if (s_legalMovesBB & targetMask) {
                            MoveOutcome outcome = SafeMovePiece(board, s_selectedIndex, targetIndex);
                            s_selected = false;
                            s_selectedIndex = -1;
                            s_legalMovesBB = 0ULL;
                        }
                    } else {
                        s_selected = false;
                        s_selectedIndex = -1;
                        s_legalMovesBB = 0ULL;
                    }
                }
            }

            ImGui::Dummy(ImVec2(props.CellSize * 8, props.CellSize * 8));
            ImGui::EndChild();

            // Right Sidebar
            ImGui::SameLine();
            ImGui::BeginChild("RightSidebar", ImVec2(sidebarWidth, props.CellSize * 8), true);

            if (ImGui::CollapsingHeader("Move History", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::BeginChild("HistoryList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
                const auto &hist = board.MoveHistory;
                for (size_t i = 0; i < hist.size(); ++i) {
                    char buf[64];
                    int n = snprintf(buf, sizeof(buf), "%zu. %s", i + 1, hist[i].c_str());
                    ImGui::TextUnformatted(buf, buf + n);
                }
                ImGui::EndChild();
            }

            // placeholder for future tools
            ImGui::Separator();
            ImGui::Text("Future tools go here...");

            ImGui::EndChild();

            if (PromotionPending(board.PromotionStatus)) {
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
                        (ImTextureID)(intptr_t)ChessEngineAPI::GetPieceTexture(PromotionColor(board.PromotionStatus), choice)->GetGLTexture(),
                        ImVec2(64, 64)
                    )) {
                        MoveOutcome dummyMO{};
                        if (SafeApplyPromotion(board, dummyMO, choice)) {
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
