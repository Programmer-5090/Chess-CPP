#ifndef BOARD_RENDERER_H
#define BOARD_RENDERER_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <array>
#include <string>
#include <vector>

#include "move.h"
#include "pieces.h"

namespace Chess{
    // Forward declarations
    struct Move;
    class BoardState;


    struct RenderColors {
        SDL_Color selectedSquare =  { 0  , 255, 0  , 150 };      // Semi-transparent green
        SDL_Color validMove =       { 0  , 255, 0  , 150 };      // Semi-transparent green  
        SDL_Color invalidMove =     { 255, 0  , 0  , 150 };      // Semi-transparent red
        SDL_Color lastMove =        { 0  , 0  , 255, 150 };      // Semi-transparent blue
        SDL_Color lightSquare =     { 240, 217, 181, 255 };      // Light chess square
        SDL_Color darkSquare =      { 181, 136, 99 , 255 };      // Dark chess square
    };

    struct RenderContext {
        int selectedSquare = -1;
        const std::vector<Move>* possibleMoves = nullptr;
        bool showCoordinates = false;
        bool highlightLastMove = false;
        const Move* lastMove = nullptr;
    };


    class BoardRenderer {
    private:
        SDL_Renderer* renderer;
        RenderColors colors;
        RenderContext context;

        const BoardState* board = nullptr;
        std::array<std::array<SDL_FRect, 8>, 8> boardGrid;

        std::array<SDL_Texture*, 12> pieceTextures{};

        bool isFlipped = false;
        float squareSize = 0;

        static int textureIndex(int color, int pieceType);
        static std::string pieceTexturePath(int color, int pieceType);
        void ensurePieceTexturesLoaded();
        void destroyPieceTextures();

        void setBlendModeAlpha();
        void resetBlendMode();

    public:
        BoardRenderer(SDL_Renderer* renderer);
        ~BoardRenderer();

        void initialize(float squareSize, bool flipped, const RenderContext& context);

        void drawChessBoard(int square, const std::vector<Move>& moves, const BoardState& board);

        void drawBackground();
        void drawSquareHighlights(int square, const std::vector<Move>& moves);
        void drawSelectedSquareHighlight(int square);
        void drawPossibleMoveHighlights(const std::vector<Move>& moves);
        void drawLastMoveHighlight(int square);
        void drawPieces();
        void drawBoard();
        void drawCoordinates();

        void setFlipped(bool flipped);
        void setColors(const RenderColors& newColors);

        void setGrid(const std::array<std::array<SDL_FRect, 8>, 8>& grid, float squareSize);

        SDL_FRect getSquareRect(int row, int col) const { return boardGrid[row][col]; }
    };
} // namespace Chess
#endif // BOARD_RENDERER_H