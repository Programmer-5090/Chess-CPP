#include "board_renderer.h"

#include "board_state.h"
#include "board_rep.h"

namespace Chess {

    static bool squareToGrid(int square64, bool flipped, int& outRow, int& outCol) {
        if (square64 < 0 || square64 >= 64) return false;
        const int file = BoardRepresentation::FileIndex(square64);
        const int rank = BoardRepresentation::RankIndex(square64);

        if (!flipped) {
            outRow = 7 - rank;
            outCol = file;
        } else {
            outRow = rank;
            outCol = 7 - file;
        }

        return outRow >= 0 && outRow < 8 && outCol >= 0 && outCol < 8;
    }

	BoardRenderer::BoardRenderer(SDL_Renderer* renderer) : renderer(renderer) {}

    BoardRenderer::~BoardRenderer() {
        destroyPieceTextures();
    }

    void BoardRenderer::initialize(float squareSize, bool flipped, const RenderContext& context) {
        this->squareSize = squareSize;
        this->isFlipped = flipped;
        this->context = context;
    }

	void BoardRenderer::setBlendModeAlpha() {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    }
	void BoardRenderer::resetBlendMode() {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

	void BoardRenderer::drawChessBoard(int square, const std::vector<Move>& moves, const BoardState& board) {
        this->board = &board;
        drawBackground();
        drawSquareHighlights(square, moves);
        drawPieces();
    }

    void BoardRenderer::drawBackground() {
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                const bool light = ((row + col) % 2) == 0;
                const SDL_Color c = light ? colors.lightSquare : colors.darkSquare;
                SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
                SDL_RenderFillRect(renderer, &boardGrid[row][col]);
            }
        }
    }

    void BoardRenderer::drawSquareHighlights(int square, const std::vector<Move>& moves) {
        drawSelectedSquareHighlight(square);
        drawPossibleMoveHighlights(moves);
        if (context.highlightLastMove && context.lastMove && context.lastMove->isValid()) {
            drawLastMoveHighlight(context.lastMove->startSquare());
            drawLastMoveHighlight(context.lastMove->targetSquare());
        }
    }

    void BoardRenderer::drawSelectedSquareHighlight(int square) {
        if (square < 0 || square >= 64) return;
        int row = 0;
        int col = 0;
        if (!squareToGrid(square, isFlipped, row, col)) return;

        setBlendModeAlpha();
        SDL_SetRenderDrawColor(renderer, colors.selectedSquare.r, colors.selectedSquare.g, colors.selectedSquare.b, colors.selectedSquare.a);
        SDL_RenderFillRect(renderer, &boardGrid[row][col]);
        resetBlendMode();
    }

    void BoardRenderer::drawPossibleMoveHighlights(const std::vector<Move>& moves) {
        if (moves.empty()) return;

        setBlendModeAlpha();
        SDL_SetRenderDrawColor(renderer, colors.validMove.r, colors.validMove.g, colors.validMove.b, colors.validMove.a);

        for (const auto& m : moves) {
            if (!m.isValid()) continue;
            int row = 0;
            int col = 0;
            if (!squareToGrid(m.targetSquare(), isFlipped, row, col)) continue;
            SDL_RenderFillRect(renderer, &boardGrid[row][col]);
        }

        resetBlendMode();
    }

    void BoardRenderer::drawLastMoveHighlight(int square) {
        if (square < 0 || square >= 64) return;
        int row = 0;
        int col = 0;
        if (!squareToGrid(square, isFlipped, row, col)) return;

        setBlendModeAlpha();
        SDL_SetRenderDrawColor(renderer, colors.lastMove.r, colors.lastMove.g, colors.lastMove.b, colors.lastMove.a);
        SDL_RenderFillRect(renderer, &boardGrid[row][col]);
        resetBlendMode();
    }

    void BoardRenderer::drawPieces() {
        if (!board) return;
        ensurePieceTexturesLoaded();

        for (int sq = 0; sq < 64; ++sq) {
            const int pieceType = board->getPieceTypeAt(sq);
            if (pieceType < 0 || pieceType >= 6) continue;
            const int color = board->getColorAt(sq);
            if (color != COLOR_WHITE && color != COLOR_BLACK) continue;

            int row = 0;
            int col = 0;
            if (!squareToGrid(sq, isFlipped, row, col)) continue;

            const SDL_FRect dst = boardGrid[row][col];
            SDL_Texture* tex = pieceTextures[textureIndex(color, pieceType)];
            if (!tex) continue;

            SDL_RenderTexture(renderer, tex, nullptr, &dst);
        }
    }
    void BoardRenderer::drawBoard() {}
    void BoardRenderer::drawCoordinates() {}

    void BoardRenderer::setFlipped(bool flipped) {
        isFlipped = flipped;
    }

    void BoardRenderer::setColors(const RenderColors& newColors) {
        colors = newColors;
    }

    void BoardRenderer::setGrid(const std::array<std::array<SDL_FRect, 8>, 8>& grid, float squareSize) {
        boardGrid = grid;
        this->squareSize = squareSize;
    }

    SDL_FRect BoardRenderer::getSquareRect(int row, int col) const {
        return boardGrid[row][col];
    }

    int BoardRenderer::textureIndex(int color, int pieceType) {
        return color * 6 + pieceType;
    }

    std::string BoardRenderer::pieceTexturePath(int color, int pieceType) {
        const char* prefix = (color == COLOR_WHITE) ? "W_" : "B_";

        const char* name = "";
        switch (pieceType) {
            case PIECE_PAWN: name = "Pawn"; break;
            case PIECE_KNIGHT: name = "Knight"; break;
            case PIECE_BISHOP: name = "Bishop"; break;
            case PIECE_ROOK: name = "Rook"; break;
            case PIECE_QUEEN: name = "Queen"; break;
            case PIECE_KING: name = "King"; break;
            default: name = ""; break;
        }

        return std::string("resources/") + prefix + name + ".png";
    }

    void BoardRenderer::ensurePieceTexturesLoaded() {
        for (auto& t : pieceTextures) {
            if (t != nullptr) {
                return;
            }
        }

        pieceTextures.fill(nullptr);
        for (int color = 0; color < 2; ++color) {
            for (int type = 0; type < 6; ++type) {
                const int idx = textureIndex(color, type);
                const std::string path = pieceTexturePath(color, type);
                pieceTextures[idx] = IMG_LoadTexture(renderer, path.c_str());
            }
        }
    }

    void BoardRenderer::destroyPieceTextures() {
        for (auto& tex : pieceTextures) {
            if (tex) {
                SDL_DestroyTexture(tex);
                tex = nullptr;
            }
        }
    }
}