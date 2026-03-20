/*
 * USAGE EXAMPLES
 * 
 * This file demonstrates how to use the refactored chess engine code.
 */

#include "include/chess_engine/board_state.h"
#include "include/chess_engine/move.h"
#include <iostream>

using namespace Chess;

int main() {
    // Create and initialize a board
    BoardState board;
    board.init();

    // Example 1: Make a move
    Move move1(BoardRepresentation::e2, BoardRepresentation::e4);
    board.makeMove(move1);
    std::cout << "Moved: " << move1.toString() << std::endl;

    // Example 2: Create a move with a flag (e.g., pawn two-forward)
    Move move2(BoardRepresentation::e7, BoardRepresentation::e5, Move::Flag::PawnTwoForward);
    board.makeMove(move2);
    std::cout << "Moved: " << move2.toString() << std::endl;

    // Example 3: Query piece information
    int piece = board.getPieceAt(BoardRepresentation::e4);
    int color = board.getColorAt(BoardRepresentation::e4);
    std::cout << "Piece type: " << piece << ", Color: " << color << std::endl;

    // Example 4: Get piece list for iteration
    const PieceList& whitePawns = board.getPieceList(COLOR_WHITE, PIECE_PAWN);
    std::cout << "White pawns: " << whitePawns.count() << std::endl;

    // Example 5: Check if a king is in check
    bool whiteInCheck = board.isInCheck(COLOR_WHITE);
    bool blackInCheck = board.isInCheck(COLOR_BLACK);
    std::cout << "White in check: " << whiteInCheck << std::endl;
    std::cout << "Black in check: " << blackInCheck << std::endl;

    // Example 6: Query game state
    std::cout << "Side to move: " << (board.isWhiteToMove() ? "White" : "Black") << std::endl;
    std::cout << "Half moves: " << board.getHisPly() << std::endl;
    std::cout << "Castling rights: " << board.getCastleRights() << std::endl;

    // Example 7: Create promotion move
    Move promotion(BoardRepresentation::e7, BoardRepresentation::e8, Move::Flag::PromoteToQueen);
    std::cout << "Promotion: " << promotion.toString() << std::endl;
    std::cout << "Is promotion: " << promotion.isPromotion() << std::endl;
    std::cout << "Promotion piece: " << promotion.promotionPieceType() << std::endl;

    // Example 8: Reset board to starting position
    board.reset();
    std::cout << "Board reset to starting position" << std::endl;

    return 0;
}

/*
 * KEY IMPROVEMENTS:
 * 
 * 1. Move Representation:
 *    - Old: Used Player struct with separate fields
 *    - New: Single Move object with bit-packed data (16 bits)
 *    - Benefit: Memory efficient, cleaner API
 * 
 * 2. Type Safety:
 *    - Old: Mixed integer parameters in move() function
 *    - New: Strongly typed Move object
 *    - Benefit: Clearer intent, fewer errors
 * 
 * 3. Documentation:
 *    - Old: Minimal inline documentation
 *    - New: Comprehensive docstrings and comments
 *    - Benefit: Easier to understand and maintain
 * 
 * 4. Modularity:
 *    - Old: Everything in board_state.h
 *    - New: Separated concerns into piece_list.h, hash_keys.h, move.h
 *    - Benefit: Better organization, reusability
 * 
 * 5. New Functionality:
 *    - isInCheck() for checking moves
 *    - getPieceAt() / getColorAt() for quick queries
 *    - isWhiteToMove() / isBlackToMove() convenience functions
 *    - Better move flag support
 * 
 * 6. Performance:
 *    - Bitwise operations where possible
 *    - O(1) piece lookups via mailbox
 *    - Efficient piece list iteration
 *    - Zobrist hashing ready for transposition tables
 */
