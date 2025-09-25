#if !NDEBUG
#include "pch.h"
#include "Timer.h"
#include "Window.h"
#include "Graphics/Texture.h"
#include "ChessEngine/Test/Test.h"

#include "ChessEngine/Pieces.h"
#include "ChessEngine/Board.h"
#include "ChessEngine/ChessEngineAPI.h"

using namespace WinChess;
using namespace WinChess::Graphics;
using namespace WinChess::ChessEngine;
using namespace WinChess::ChessEngine::Pieces;
using namespace WinChess::ChessEngine::Board;

int main() {
    try {
        std::filesystem::path projectRoot = "C:/Users/Owner/Desktop/win-chess";
        Window myWindow;
        ChessEngineAPI::Init(projectRoot);
        BoardState board{};
        InitializeStartingPosition(board);
        // Test::LoadFEN(board, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

        BoardDisplayProps interactiveProps{};
        interactiveProps.CellSize = 80.0f;
        interactiveProps.WindowName = "Interactive Chess Board";

        Timer t; int cnt = 0;
        while(!myWindow.GetIsClosed()) {
            myWindow.BeginImGui();

            ChessEngineAPI::DisplayInteractiveGameState(board, interactiveProps);

            myWindow.EndImGui();
            myWindow.UpdateFrame();

            /* FPS */
            ++cnt;
            if (t.Elapsed() >= 1.0) {
                printf("\rFPS: %d", cnt);
                t.Reset();
                cnt = 0;
            }
        }
    } catch (std::exception& exception) {
        std::cerr << exception.what() << std::endl;
        return -1;
    }
    return 0;
}
#else
#include "ChessEngine/Test/Test.h"

int main() {
    using namespace WinChess::ChessEngine;
    Test::ExampleRunTest();
    return 0;
}
#endif