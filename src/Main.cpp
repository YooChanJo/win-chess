#include "pch.h"
#include "Window.h"
#include "Graphics/Texture.h"
#include "ChessEngine/ChessEngineAPI.h"

using namespace WinChess;
using namespace WinChess::Graphics;
using namespace WinChess::ChessEngine;
int main() {
    try {
        std::filesystem::path projectRoot = "C:/Users/Owner/Desktop/win-chess";
        Window myWindow;
        ChessEngineAPI::Init(projectRoot);
        Board::BoardState board{};
        BoardDisplayProps props{};
        props.CellSize = 80.0f;
        while(!myWindow.GetIsClosed()) {
            myWindow.BeginImGui();
            ChessEngineAPI::DisplayInteractiveGameState(board, props);

            myWindow.EndImGui();
            myWindow.UpdateFrame();
        }
    } catch (std::exception& exception) {
        std::cerr << exception.what() << std::endl;
        return -1;
    }
    return 0;
}