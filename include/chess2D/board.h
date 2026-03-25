#ifndef BOARD_H
#define BOARD_H

#include <SDL3/SDL.h>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include "input.h"
#include "move.h"
#include "pieces.h"

namespace Chess {
    // Forward declarations
    class  BoardRenderer;
    class  UIPromotionDialog;
    struct RenderContext;
    class  BoardState;
    class  MoveGenerator;


    class Board {
    private:
        int screenHeight;
        int screenWidth;
        float startXPos;
        float startYPos;
        float endXPos;
        float endYPos;
        bool isFlipped = false;

        std::unique_ptr<BoardRenderer> boardRenderer;
        std::unique_ptr<UIPromotionDialog> promotionDialog;
        std::array<std::array<SDL_FRect, 8>, 8> boardGrid;

        std::unique_ptr<BoardState> boardState;
        std::unique_ptr<MoveGenerator> gen;

        std::string startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        int currentPlayer = COLOR_WHITE;
        int halfMoveClock = 0;
        int fullMoveNumber = 1;

    public:

        Board(int width, int height, float offSet);
        ~Board();
        Board(const Board& other);
        void loadFEN(const std::string& fen, SDL_Renderer* gameRenderer);
        void syncUI(SDL_Renderer* gameRenderer);
        void initializeBoard(SDL_Renderer* gameRenderer);
        void resetBoard(SDL_Renderer* gameRenderer);
        void setStartFEN(const std::string& fen) { startFEN = fen; }

        void setFlipped(bool flipped);

        void draw(SDL_Renderer* renderer, int selectedSquare, const std::vector<Move>& possibleMoves);

        bool screenToBoardCoords(int screenX, int screenY, int& boardR, int& boardC) const;
        SDL_FRect getSquareRect(int r, int c) const;

        bool isCheckMate(int color);
        bool isStaleMate(int color);
        void handlePawnPromotion(int row, int col);
        void updatePromotionDialog(Input& input);
        void renderPromotionDialog(SDL_Renderer* renderer);
        bool isPromotionDialogActive() const;

        // Promotion helpers
        void promotePawnTo(int row, int col, int color, int pieceType, SDL_Renderer* renderer);
        void showPromotionDialog(bool show);


        std::string getStartFEN() const { return startFEN; }
        std::string getCurrentFEN() const;
        bool getIsFlipped() const { return isFlipped; }
        int getCurrentPlayer() const { return currentPlayer; }
        int getHalfMoveClock() const { return halfMoveClock; }
        int getFullMoveNumber() const { return fullMoveNumber; }

        void setCurrentPlayer(int player) { currentPlayer = player; }
    };
}

#endif // BOARD_H