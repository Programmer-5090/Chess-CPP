/*
 * Piece List
 * 
 * Maintains a list of squares occupied by pieces of a particular type and color.
 * This allows for efficient iteration over all pieces of a given type without
 * needing to iterate through the entire bitboard.
 */

#ifndef PIECE_LIST_H
#define PIECE_LIST_H

#include <vector>
#include <algorithm>

namespace Chess {

    class PieceList {
    private:
        std::vector<int> squares;

    public:
        PieceList() = default;

        // Get the number of pieces in this list
        int count() const {
            return static_cast<int>(squares.size());
        }

        // Add a piece at the given square
        void add(int square) {
            squares.push_back(square);
        }

        // Remove a piece from the given square
        void remove(int square) {
            auto it = std::find(squares.begin(), squares.end(), square);
            if (it != squares.end()) {
                squares.erase(it);
            }
        }

        // Move a piece from one square to another
        void move(int fromSquare, int toSquare) {
            auto it = std::find(squares.begin(), squares.end(), fromSquare);
            if (it != squares.end()) {
                *it = toSquare;
            }
        }

        // Clear all pieces from this list
        void clear() {
            squares.clear();
        }

        // Get the underlying vector (for iteration)
        const std::vector<int>& getSquares() const {
            return squares;
        }

        // Direct access to squares by index
        int operator[](int index) const {
            return squares[index];
        }
    };

}  // namespace Chess

#endif  // PIECE_LIST_H
