#pragma once
#include "Pieces.h"
#include "Board.h"
#include "Graphics/Texture.h"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace WinChess {
    namespace ChessEngine {
        /* Display helpers */
        struct BoardDisplayProps {
            std::string WindowName = "Chess Board";
            float CellSize = 50.f; // Width = Height of cell 
            ImVec4 EvenCellColor         = { 240, 217, 181, 255 };
            ImVec4 OddCellColor          = { 181, 136, 99, 255 };
            ImVec4 EvenSelectedCellColor = { 246, 246, 105, 255 };  // yellow highlight for light squares
            ImVec4 OddSelectedCellColor  = { 186, 202, 68, 255 };   // greenish-yellow highlight for dark squares
            ImVec4 HighLightDotColor     = { 80, 80, 80, 255 };   // medium gray dot
        };
        inline ImU32 ImVec4ColorToImU32(const ImVec4& col) { return IM_COL32(col.x, col.y, col.z, col.w); }
        inline ImVec4 NormalizeImVec4Color(const ImVec4 col) { return ImVec4(col.x / 255.0f, col.y / 255.0f, col.z / 255.0f, col.w / 255.0f); }

        class ChessEngineAPI {
        private:
            inline static PieceMap<std::unique_ptr<Graphics::Texture>> s_PieceTextures{};
            inline static bool s_Dragging = false;
            inline static Pieces::PieceColor s_DragColor;
            inline static Pieces::PieceType s_DragType;

            inline static bool s_Selected = false;       // whether a piece is selected
            inline static int s_SelectedIndex = -1;      // bit index of selected piece
            inline static uint64_t s_LegalMovesBB = 0ULL; // bitboard of legal moves

        public:
            ChessEngineAPI() = default;
            ~ChessEngineAPI() = default;

            // This should only be called after a valid OpenGL context exists
            static void Init(const std::filesystem::path& projectRoot);

            // Pure display of a board (no interaction)
            static void DisplayGameState(
                const Board::BoardState& board,
                const BoardDisplayProps& props = BoardDisplayProps{}
            );

            // Interactive mode that may apply moves to the BoardState (via Board namespace functions)
            static void DisplayInteractiveGameState(
                Board::BoardState& board,
                const BoardDisplayProps& props = BoardDisplayProps{}
            );

        private:
            static void DrawPiecesForDisplay(
                uint64_t bitboard,
                ImTextureID texture,
                ImDrawList* drawList,
                const ImVec2& boardPos,
                float cellSize
            );
            static bool MouseToBoard(
                const ImVec2& boardPos,
                float cellSize,
                const ImVec2& mousePos,
                int& outX,
                int& outY
            );
            static void DrawPiecesForInteraction(
                uint64_t bitboard,
                ImTextureID texture,
                ImDrawList* drawList,
                const ImVec2& boardPos,
                float cellSize,
                Pieces::PieceColor color,
                Pieces::PieceType type,
                const Board::BoardState& gameBoard
            );
        };
    }
}
