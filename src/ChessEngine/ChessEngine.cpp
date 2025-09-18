#include "ChessEngine.h"

/* Index of Least Significant Set Bit */
static inline int LSSBIndex(uint64_t bb) {
#if defined(_MSC_VER)
    unsigned long index;
    _BitScanForward64(&index, bb);
    return static_cast<int>(index);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_ctzll(bb);
#else
    // Fallback: naive loop
    for (int i = 0; i < 64; ++i)
        if (bb & (1ULL << i))
            return i;
    return -1; // empty board
#endif
}

#define MAKE_PIECE_TEXTURE_UNIQUE(FILE_NAME, DIR_NAME) std::make_unique<::WinChess::Graphics::Texture>(::WinChess::Graphics::ConvertImageFormat(::WinChess::Graphics::ReadImageFile(FILE_NAME, DIR_NAME, ::WinChess::Graphics::ImageFormat::RA8, false), ::WinChess::Graphics::ImageFormat::RGBA8))
namespace WinChess {
    namespace ChessEngine{

        namespace Board {
            
        }

        void ChessEngineAPI::Init(const std::filesystem::path& projectRoot) {
            std::filesystem::path pieceImageDir = projectRoot / "assets/images/pieces";
            s_PieceTextures.clear();

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
                int index = LSSBIndex(bitboard);  // index of least significant set bit
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
            const Board::BoardDisplayProps& props
        ) {
            ImGui::Begin(props.WindowName.c_str(), nullptr, ImGuiWindowFlags_NoResize);
            // Get ImGui style for padding & borders
            const ImGuiStyle& style = ImGui::GetStyle();
            ImGui::SetWindowSize(ImVec2(
                props.CellSize * 8.0f + style.WindowPadding.x * 2.0f,
                props.CellSize * 8.0f + style.WindowPadding.y * 2.0f + ImGui::GetFrameHeight()
            ));
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();

            /* Draw Board */
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    ImU32 col = ((x + y) % 2 == 0) ? props.EvenCellColor : props.OddCellColor;
                    ImVec2 top_left = ImVec2(
                        p.x + x * props.CellSize,
                        p.y + y * props.CellSize
                    );
                    ImVec2 bottom_right = ImVec2(
                        top_left.x + props.CellSize,
                        top_left.y + props.CellSize
                    );
                    drawList->AddRectFilled(top_left, bottom_right, col);
                }
            }
#define DRAW_PIECE_FOR_DISPLAY_WITH_PIECE(PIECE_COLOR, PIECE_TYPE, DRAW_LIST, BOARD_POS, CELL_SIZE) DrawPiecesForDisplay(board.BitBoards.at(PIECE_COLOR).at(PIECE_TYPE), (ImTextureID)(intptr_t)s_PieceTextures[PIECE_COLOR][PIECE_TYPE]->GetGLTexture(), DRAW_LIST, BOARD_POS, CELL_SIZE)
            for (auto color : Pieces::AllPieceColors) { for (auto type : Pieces::AllPieceTypes) {
                DRAW_PIECE_FOR_DISPLAY_WITH_PIECE(color, type, drawList, p, props.CellSize);    
            }}
            ImGui::Dummy(ImVec2(props.CellSize * 8, props.CellSize * 8)); // reserve space
            ImGui::End();
        }

        // Map Mouse to Square
        bool ChessEngineAPI::MouseToBoard(
            const ImVec2& boardPos,
            float cellSize,
            const ImVec2& mousePos,
            int& outX,
            int& outY
        ) {
            float mx = mousePos.x - boardPos.x;
            float my = mousePos.y - boardPos.y;

            // Mouse is not in square
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
            Pieces::PieceType type // Type of the current drawing piece
        ) {
            while (bitboard) {
                int index = LSSBIndex(bitboard);  // index of least significant set bit
                int x = index % 8;
                int y = 7 - (index / 8);

                ImVec2 top_left = ImVec2(boardPos.x + x * cellSize, boardPos.y + y * cellSize);
                ImVec2 bottom_right = ImVec2(top_left.x + cellSize, top_left.y + cellSize);

                // check for drag start
                if (!s_Dragging && ImGui::IsMouseHoveringRect(top_left, bottom_right) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    s_Dragging = true;
                    s_DragColor = color;
                    s_DragType = type;
                    s_SourceIndex = index;
                }

                // if this piece is being dragged, skip drawing it in its original position
                if (!(s_Dragging && s_DragColor == color && s_DragType == type && s_SourceIndex == index)) {
                    drawList->AddImage(texture, top_left, bottom_right);
                }

                bitboard &= bitboard - 1; // clear least significant set bit
            }
        }
        void ChessEngineAPI::DisplayInteractiveGameState(
            Board::BoardState& board,
            const Board::BoardDisplayProps& props
        ) {
            ImGui::Begin(props.WindowName.c_str(), nullptr, ImGuiWindowFlags_NoResize);
            // Get ImGui style for padding & borders
            const ImGuiStyle& style = ImGui::GetStyle();
            ImGui::SetWindowSize(ImVec2(
                props.CellSize * 8.0f + style.WindowPadding.x * 2.0f,
                props.CellSize * 8.0f + style.WindowPadding.y * 2.0f + ImGui::GetFrameHeight()
            ));
            /* A fixed subregion */
            ImGui::BeginChild(
                "BoardArea",
                ImVec2(props.CellSize * 8, props.CellSize * 8),
                false,
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse
            );
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImVec2 mouse = ImGui::GetIO().MousePos;

            /* Draw Board */
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    ImU32 col = ((x + y) % 2 == 0) ? props.EvenCellColor : props.OddCellColor;
                    ImVec2 top_left = ImVec2(
                        p.x + x * props.CellSize,
                        p.y + y * props.CellSize
                    );
                    ImVec2 bottom_right = ImVec2(
                        top_left.x + props.CellSize,
                        top_left.y + props.CellSize
                    );
                    drawList->AddRectFilled(top_left, bottom_right, col);
                }
            }
#define DRAW_PIECE_FOR_INTERACTION_WITH_PIECE(PIECE_COLOR, PIECE_TYPE, DRAW_LIST, BOARD_POS, CELL_SIZE) DrawPiecesForInteraction(board.BitBoards[PIECE_COLOR][PIECE_TYPE], (ImTextureID)(intptr_t)s_PieceTextures[PIECE_COLOR][PIECE_TYPE]->GetGLTexture(), DRAW_LIST, BOARD_POS, CELL_SIZE, PIECE_COLOR, PIECE_TYPE)
            for (auto color : Pieces::AllPieceColors) { for (auto type : Pieces::AllPieceTypes) {
                DRAW_PIECE_FOR_INTERACTION_WITH_PIECE(color, type, drawList, p, props.CellSize);    
            }}
            
            // Draw dragged piece at mouse
            if (s_Dragging) {
                ImTextureID tex = s_PieceTextures[s_DragColor][s_DragType]->GetGLTexture();
                /* Draw the moving piece */
                drawList->AddImage(
                    (ImTextureID)(intptr_t)tex,
                    ImVec2(mouse.x - props.CellSize / 2, mouse.y - props.CellSize / 2),
                    ImVec2(mouse.x + props.CellSize / 2, mouse.y + props.CellSize / 2)
                );

                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    s_Dragging = false;
                    int targetX, targetY;
                    if (MouseToBoard(p, props.CellSize, mouse, targetX, targetY)) {
                        int targetIndex = (7 - targetY) * 8 + targetX; // back to bit index

                        uint64_t& bb = board.BitBoards[s_DragColor][s_DragType];
                        bb &= ~(1ULL << s_SourceIndex); // Remove Piece
                        bb |= 1ULL << targetIndex; // Add it Back
                    }
                }
            }

            ImGui::Dummy(ImVec2(props.CellSize * 8, props.CellSize * 8)); // reserve space
            ImGui::EndChild();
            ImGui::End();
        }
    }
}