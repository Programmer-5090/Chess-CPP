/*
 * Zobrist Hash Keys
 * 
 * Manages pseudo-random hash keys for Zobrist hashing of chess positions.
 * This allows for fast position lookups in transposition tables and other hash-based structures.
 * 
 * The hash is composed of:
 * - XOR of piece keys for each piece on the board
 * - Side-to-move key
 * - Castling rights key
 * - En passant square key
 */

#ifndef HASH_KEYS_H
#define HASH_KEYS_H

#include <cstdint>
#include <cstdlib>
#include <random>
#include <array>

namespace Chess {

    // Thread-safe Zobrist hash key generator
    class ZobristKeys {
    private:
        // Hard-coded to 120 for 0x88 board representation
        // (BRD_SQ_NUM is defined in board_state.h)
        static constexpr int BOARD_SIZE = 120;

        std::array<std::array<uint64_t, BOARD_SIZE>, 13> pieceKeys{};
        uint64_t sideKey = 0;
        std::array<uint64_t, 16> castleKeys{};
        bool initialized = false;

        // Generate a pseudo-random 64-bit number
        static uint64_t random64() {
            return (static_cast<uint64_t>(std::rand()) << 48) ^
                   (static_cast<uint64_t>(std::rand()) << 32) ^
                   (static_cast<uint64_t>(std::rand()) << 16) ^
                   (static_cast<uint64_t>(std::rand()));
        }

    public:
        ZobristKeys() = default;

        // Initialize all hash keys with random values
        void init() {
            static bool seeded = false;
            if (!seeded) {
                std::srand(static_cast<unsigned>(std::random_device{}()));
                seeded = true;
            }

            // Generate random keys for each piece on each square
            for (int i = 0; i < 13; ++i) {
                for (int j = 0; j < BOARD_SIZE; ++j) {
                    pieceKeys[i][j] = random64();
                }
            }

            // Generate key for side-to-move
            sideKey = random64();

            // Generate keys for each castling rights combination (16 possible)
            for (int i = 0; i < 16; ++i) {
                castleKeys[i] = random64();
            }

            initialized = true;
        }

        // Check if keys have been initialized
        bool isInitialized() const {
            return initialized;
        }

        // Get piece key for a specific piece and square
        uint64_t getPieceKey(int pieceIndex, int square) const {
            return pieceKeys[pieceIndex][square];
        }

        // Get side-to-move key
        uint64_t getSideKey() const {
            return sideKey;
        }

        // Get castling rights key
        uint64_t getCastleKey(int rights) const {
            return castleKeys[rights];
        }

        // Get piece key array for external access
        const std::array<std::array<uint64_t, BOARD_SIZE>, 13>& getPieceKeys() const {
            return pieceKeys;
        }
    };

    // Global Zobrist keys instance (can be made thread-local if needed)
    extern ZobristKeys g_zobristKeys;

}  // namespace Chess

#endif  // HASH_KEYS_H
