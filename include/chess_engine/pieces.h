#ifndef PIECE_CONST_H
#define PIECE_CONST_H

#include <cstdint>

namespace chess {

    // Piece type constants (occupies bits 0-2)
    // Values are chosen such that sliding piece types have bit 2 set (BISHOP=5, ROOK=6, QUEEN=7)
    constexpr int PIECE_NONE = 0;
    constexpr int PIECE_KING = 1;
    constexpr int PIECE_PAWN = 2;
    constexpr int PIECE_KNIGHT = 3;
    constexpr int PIECE_BISHOP = 5;    // bit pattern: 0b101
    constexpr int PIECE_ROOK = 6;      // bit pattern: 0b110
    constexpr int PIECE_QUEEN = 7;     // bit pattern: 0b111

    // Color constants (occupies bits 3-4)
    // WHITE = 0b01000 (bit 3), BLACK = 0b10000 (bit 4)
    constexpr int COLOR_WHITE = 8;     // 0b01000
    constexpr int COLOR_BLACK = 16;    // 0b10000

    // Masks to extract piece type (bits 0-2) or color (bits 3-4) from a piece value
    constexpr int TYPE_MASK = 0b00111;  // Extracts piece type
    constexpr int COLOR_MASK = 0b11000; // Extracts color

    // Returns true if the given piece matches the specified color
    inline bool isColor(int piece, int color)
    {
        return (piece & COLOR_MASK) == color;
    }

    // Extracts the color from a piece value
    inline int  colorOf(int piece)
    {
        return piece & COLOR_MASK;
    }

    // Extracts the piece type from a piece value
    inline int  typeOf(int piece)
    {
        return piece & TYPE_MASK;
    }

    // Returns true if the piece type is a rook or queen (sliding pieces on ranks/files)
    // Uses bit pattern: ROOK=0b110, QUEEN=0b111, so (type & 0b110) == 0b110 identifies both
    inline bool isRookOrQueen(int pieceType)
    {
        return (pieceType & 0b110) == 0b110;
    }

    // Returns true if the piece type is a bishop or queen (sliding pieces on diagonals)
    // Uses bit pattern: BISHOP=0b101, QUEEN=0b111, so (type & 0b101) == 0b101 identifies both
    inline bool isBishopOrQueen(int pieceType)
    {
        return (pieceType & 0b101) == 0b101;
    }

    // Returns true if the piece type is a sliding piece (bishop, rook, or queen)
    // All sliding pieces have bit 2 set in their type value
    inline bool isSlidingPiece(int pieceType)
    {
        return (pieceType & 0b100) != 0;
    }

    // Returns true if the bit at position 'sq' is set in the bitboard
    inline bool bbHas(uint64_t bb, int sq) { return (bb & (1ULL << sq)) != 0ULL; }
} // namespace chess
#endif // PIECE_CONST_H