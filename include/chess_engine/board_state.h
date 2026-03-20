#ifndef BOARD_STATE_H
#define BOARD_STATE_H

#include "pieces.h"
#include "bitboard.h"
#include "board_rep.h"
#include "move.h"
#include "piece_list.h"
#include "hash_keys.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <vector>
#include <string>

namespace Chess {

    /*
     * Bit indices for the mailbox representation
     * Used for quick piece lookup without iterating through bitboards
     */
    static constexpr int BRD_SQ_NUM = 120;

    // Forward declaration for FEN loading function
    // Defined in fen_util.h
    class BoardState;
    void loadFEN(BoardState& board, const std::string& fen);

    class BoardState {
    private:
        // Zobrist hash keys for position hashing (centralized in ZobristKeys)
        static inline ZobristKeys zobristKeys;

        // Bitboard representation - one bitboard per piece type and color
        // Index: color * 6 + pieceType
        // Example: COLOR_WHITE * 6 + PIECE_PAWN gives the index for white pawns
        std::array<uint64_t, 12> pieceBoards{};

        // Piece lists for efficient piece iteration
        std::array<PieceList, 12> pieceLists{};

        // Occupancy bitboards for quick piece presence checks
        uint64_t whitePieces = 0;     // All white pieces
        uint64_t blackPieces = 0;     // All black pieces
        uint64_t mainBoard = 0;       // All pieces (white | black)

        // Mailbox representation for O(1) piece lookup by square
        // Maps mailbox index (0-119) to piece type, or -1 if empty
        std::array<int, BRD_SQ_NUM> mailbox{};

        // Game state
        int side = COLOR_WHITE;              // Current side to move
        int enPas = -1;                      // En passant target square (-1 if none)
        int fiftyMove = 0;                   // Half-move clock for 50-move rule
        int hisPly = 0;                      // Full move counter
        int castleRights = 0;                // Bitmask of castling rights

        // Last move made
        Move lastMove;

        // Position hash for transposition tables
        uint64_t posKey = 0;

        /*
         * Convert 64-bit square index to 120-bit mailbox index
         * The mailbox is a 10x12 grid with padding to simplify boundary checks
         * Standard 0x88 representation mapping
         */
        static constexpr int toMailboxIndex(int square64) {
            const int file = BoardRepresentation::FileIndex(square64);
            const int rank = BoardRepresentation::RankIndex(square64);
            return 21 + file + rank * 10;
        }

        /*
         * Update castling rights based on piece movement
         * Removes castling rights when the king or rook moves from their starting square
         */
        void updateCastlingRights(int pieceType, int fromFile, int fromRank, 
                                  int capturedType, int captureFile, int captureRank) {
            // Lose castling rights when king moves
            if (pieceType == PIECE_KING) {
                if (side == COLOR_WHITE) {
                    castleRights &= ~0x03;  // Clear white castling rights
                } else {
                    castleRights &= ~0x0C;  // Clear black castling rights
                }
            }

            // Lose castling rights when rook moves from starting square
            if (pieceType == PIECE_ROOK) {
                if (fromFile == 0 && fromRank == 0) castleRights &= ~0x02;  // White queenside
                if (fromFile == 7 && fromRank == 0) castleRights &= ~0x01;  // White kingside
                if (fromFile == 0 && fromRank == 7) castleRights &= ~0x08;  // Black queenside
                if (fromFile == 7 && fromRank == 7) castleRights &= ~0x04;  // Black kingside
            }

            // Lose opponent's castling rights if their rook is captured
            if (capturedType == PIECE_ROOK) {
                if (captureFile == 0 && captureRank == 0) castleRights &= ~0x02;
                if (captureFile == 7 && captureRank == 0) castleRights &= ~0x01;
                if (captureFile == 0 && captureRank == 7) castleRights &= ~0x08;
                if (captureFile == 7 && captureRank == 7) castleRights &= ~0x04;
            }
        }

        /*
         * Update en passant square based on pawn movement
         * Sets the en passant square only if a pawn moves two squares forward
         */
        void updateEnPassantSquare(int fromSquare, int toSquare, int pieceType) {
            enPas = -1;
            if (pieceType != PIECE_PAWN) return;

            const int fromRank = BoardRepresentation::RankIndex(fromSquare);
            const int toRank = BoardRepresentation::RankIndex(toSquare);
            const int file = BoardRepresentation::FileIndex(fromSquare);

            // White pawn moves from rank 1 to rank 3
            if (side == COLOR_WHITE && fromRank == 1 && toRank == 3) {
                enPas = BoardRepresentation::IndexFromCoord(file, 2);
            }
            // Black pawn moves from rank 6 to rank 4
            else if (side == COLOR_BLACK && fromRank == 6 && toRank == 4) {
                enPas = BoardRepresentation::IndexFromCoord(file, 5);
            }
        }

    public:
        /*
         * Initialize the board with Zobrist keys (called once in constructor)
         */
        BoardState() {
            // Initialize Zobrist keys once when the first board is created
            if (!zobristKeys.isInitialized()) {
                zobristKeys.init();
            }
            // Initialize to starting position
            reset();
        }

        /*
         * Initialize the board to the standard chess starting position
         * Or to a specific position given a FEN string
         */
        void init(const std::string& fen = "") {
            if (fen.empty()) {
                reset();
            } else {
                // Use FEN to initialize - defined in fen_util.h
                loadFEN(*this, fen);
            }
        }

        /*
         * Reset the board to the standard chess starting position
         */
        void reset() {
            side = COLOR_WHITE;
            enPas = -1;
            fiftyMove = 0;
            hisPly = 0;
            castleRights = 0x0F;
            lastMove = Move::invalid();

            pieceBoards.fill(0);

            // White pieces on ranks 0 and 1
            pieceBoards[COLOR_WHITE * 6 + PIECE_PAWN] = 0x000000000000FF00ULL;
            pieceBoards[COLOR_WHITE * 6 + PIECE_ROOK] = 0x0000000000000081ULL;
            pieceBoards[COLOR_WHITE * 6 + PIECE_KNIGHT] = 0x0000000000000042ULL;
            pieceBoards[COLOR_WHITE * 6 + PIECE_BISHOP] = 0x0000000000000024ULL;
            pieceBoards[COLOR_WHITE * 6 + PIECE_QUEEN] = 0x0000000000000008ULL;
            pieceBoards[COLOR_WHITE * 6 + PIECE_KING] = 0x0000000000000010ULL;

            // Black pieces on ranks 6 and 7
            pieceBoards[COLOR_BLACK * 6 + PIECE_PAWN] = 0x00FF000000000000ULL;
            pieceBoards[COLOR_BLACK * 6 + PIECE_ROOK] = 0x8100000000000000ULL;
            pieceBoards[COLOR_BLACK * 6 + PIECE_KNIGHT] = 0x4200000000000000ULL;
            pieceBoards[COLOR_BLACK * 6 + PIECE_BISHOP] = 0x2400000000000000ULL;
            pieceBoards[COLOR_BLACK * 6 + PIECE_QUEEN] = 0x0800000000000000ULL;
            pieceBoards[COLOR_BLACK * 6 + PIECE_KING] = 0x1000000000000000ULL;

            rebuildOccupancy();
            rebuildMailbox();
            rebuildPieceLists();
            posKey = generatePosKey();
        }

        /*
         * Update occupancy bitboards incrementally for a single square change
         * Called when a piece moves or is captured
         */
        void updateOccupancy(int square64, int color, bool add) {
            const uint64_t mask = (1ULL << square64);
            if (add) {
                if (color == COLOR_WHITE) {
                    whitePieces |= mask;
                } else {
                    blackPieces |= mask;
                }
            } else {
                if (color == COLOR_WHITE) {
                    whitePieces &= ~mask;
                } else {
                    blackPieces &= ~mask;
                }
            }
            mainBoard = whitePieces | blackPieces;
        }

        /*
         * Update mailbox incrementally for a single square
         * Called when a piece moves or is captured
         */
        void updateMailbox(int square64, int pieceType) {
            const int index120 = toMailboxIndex(square64);
            mailbox[index120] = pieceType;
        }

        /*
         * Clear mailbox entry for a square
         * Called when a piece is removed
         */
        void clearMailbox(int square64) {
            const int index120 = toMailboxIndex(square64);
            mailbox[index120] = -1;
        }

        /*
         * Rebuild occupancy bitboards from piece bitboards
         * Combines all pieces into aggregate occupancy boards
         * Only called during reset/initialization
         */
        void rebuildOccupancy() {
            whitePieces = 0;
            blackPieces = 0;

            // Combine all white pieces
            for (int type = 0; type < 6; ++type) {
                whitePieces |= pieceBoards[COLOR_WHITE * 6 + type];
            }

            // Combine all black pieces
            for (int type = 0; type < 6; ++type) {
                blackPieces |= pieceBoards[COLOR_BLACK * 6 + type];
            }

            // Combine all pieces
            mainBoard = whitePieces | blackPieces;
        }

        /*
         * Rebuild mailbox array from piece bitboards
         * Maps each square to its piece type for O(1) lookup
         * Only called during reset/initialization
         */
        void rebuildMailbox() {
            mailbox.fill(-1);

            for (int color = 0; color < 2; ++color) {
                for (int type = 0; type < 6; ++type) {
                    uint64_t pieceBoard = pieceBoards[color * 6 + type];

                    while (pieceBoard) {
                        const int lowestSetBit = static_cast<int>(getLSB(pieceBoard));
                        const int mailboxIndex = toMailboxIndex(lowestSetBit);
                        mailbox[mailboxIndex] = type;
                        pieceBoard &= (pieceBoard - 1);
                    }
                }
            }
        }

        /*
         * Rebuild piece lists from piece bitboards
         * Updates cached lists of pieces for each type and color
         */
        void rebuildPieceLists() {
            for (auto& list : pieceLists) {
                list.clear();
            }

            for (int color = 0; color < 2; ++color) {
                for (int type = 0; type < 6; ++type) {
                    const int index = color * 6 + type;
                    uint64_t pieceBoard = pieceBoards[index];

                    while (pieceBoard) {
                        const int square = static_cast<int>(getLSB(pieceBoard));
                        pieceLists[index].add(square);
                        pieceBoard &= (pieceBoard - 1);
                    }
                }
            }
        }

        /*
         * Update a piece's position in its piece list
         */
        void updatePieceList(int color, int pieceType, int fromSquare, int toSquare) {
            const int index = color * 6 + pieceType;
            pieceLists[index].move(fromSquare, toSquare);
        }

        /*
         * Remove a piece from its piece list
         */
        void removePieceFromList(int color, int pieceType, int square) {
            const int index = color * 6 + pieceType;
            pieceLists[index].remove(square);
        }

        /*
         * Add a piece to its piece list
         */
        void addPieceToList(int color, int pieceType, int square) {
            const int index = color * 6 + pieceType;
            pieceLists[index].add(square);
        }

        /*
         * Get the piece list for a given piece type and color
         */
        const PieceList& getPieceList(int color, int pieceType) const {
            return pieceLists[color * 6 + pieceType];
        }

        /*
         * Get the piece type at a given square
         * Returns -1 if the square is empty
         */
        int getPieceAt(int square64) const {
            const int index120 = toMailboxIndex(square64);
            if (index120 < 0 || index120 >= BRD_SQ_NUM) {
                return -1;
            }
            return mailbox[index120];
        }

        /*
         * Get the color of the piece at a given square
         * Returns -1 if the square is empty
         */
        int getColorAt(int square64) const {
            const uint64_t mask = (1ULL << square64);
            if (whitePieces & mask) return COLOR_WHITE;
            if (blackPieces & mask) return COLOR_BLACK;
            return -1;
        }

        /*
         * Make a move on the board
         * Updates all representations (bitboards, mailbox, piece lists)
         * Parameters: move - Move object with from and to squares
         */
        void makeMove(Move move) {
            lastMove = move;
            const int fromSquare = move.startSquare();
            const int toSquare = move.targetSquare();

            const uint64_t fromMask = (1ULL << fromSquare);
            const uint64_t toMask = (1ULL << toSquare);
            const int toIndex120 = toMailboxIndex(toSquare);

            // Get piece information
            const int pieceType = getPieceAt(fromSquare);
            if (pieceType < 0) return;  // Invalid move - no piece at source

            // Detect capture
            int capturedType = -1;
            int capturedColor = -1;
            if (mainBoard & toMask) {
                capturedType = mailbox[toIndex120];
                capturedColor = getColorAt(toSquare);
            }

            // Get coordinates for castling and rights updates
            const int fromFile = BoardRepresentation::FileIndex(fromSquare);
            const int toFile = BoardRepresentation::FileIndex(toSquare);
            const int fromRank = BoardRepresentation::RankIndex(fromSquare);
            const int toRank = BoardRepresentation::RankIndex(toSquare);

            // Remove captured piece from all representations
            if (capturedType != -1) {
                pieceBoards[capturedColor * 6 + capturedType] &= ~toMask;
                updateOccupancy(toSquare, capturedColor, false);
                clearMailbox(toSquare);
                removePieceFromList(capturedColor, capturedType, toSquare);
            }

            // Move the piece on bitboards
            pieceBoards[side * 6 + pieceType] &= ~fromMask;
            pieceBoards[side * 6 + pieceType] |= toMask;

            // Update occupancy for the piece move
            updateOccupancy(fromSquare, side, false);
            updateOccupancy(toSquare, side, true);

            // Update mailbox for the piece move
            clearMailbox(fromSquare);
            updateMailbox(toSquare, pieceType);

            // Update game state
            updateCastlingRights(pieceType, fromFile, fromRank, capturedType, toFile, toRank);
            updateEnPassantSquare(fromSquare, toSquare, pieceType);

            // Toggle side to move
            side ^= 1;
            hisPly++;

            // Update piece lists
            updatePieceList(side ^ 1, pieceType, fromSquare, toSquare);

            // Handle castling - move rook in bitboards, occupancy, mailbox, and piece lists
            if (pieceType == PIECE_KING) {
                if (toFile - fromFile == 2) {
                    // Kingside castling: rook from h-file to f-file
                    const int rookFromSq = BoardRepresentation::IndexFromCoord(7, fromRank);
                    const int rookToSq = BoardRepresentation::IndexFromCoord(5, fromRank);
                    const uint64_t rookFromMask = (1ULL << rookFromSq);
                    const uint64_t rookToMask = (1ULL << rookToSq);
                    const int rookColor = side ^ 1;
                    pieceBoards[rookColor * 6 + PIECE_ROOK] &= ~rookFromMask;
                    pieceBoards[rookColor * 6 + PIECE_ROOK] |= rookToMask;
                    updateOccupancy(rookFromSq, rookColor, false);
                    updateOccupancy(rookToSq, rookColor, true);
                    clearMailbox(rookFromSq);
                    updateMailbox(rookToSq, PIECE_ROOK);
                    updatePieceList(rookColor, PIECE_ROOK, rookFromSq, rookToSq);
                } else if (fromFile - toFile == 2) {
                    // Queenside castling: rook from a-file to d-file
                    const int rookFromSq = BoardRepresentation::IndexFromCoord(0, fromRank);
                    const int rookToSq = BoardRepresentation::IndexFromCoord(3, fromRank);
                    const uint64_t rookFromMask = (1ULL << rookFromSq);
                    const uint64_t rookToMask = (1ULL << rookToSq);
                    const int rookColor = side ^ 1;
                    pieceBoards[rookColor * 6 + PIECE_ROOK] &= ~rookFromMask;
                    pieceBoards[rookColor * 6 + PIECE_ROOK] |= rookToMask;
                    updateOccupancy(rookFromSq, rookColor, false);
                    updateOccupancy(rookToSq, rookColor, true);
                    clearMailbox(rookFromSq);
                    updateMailbox(rookToSq, PIECE_ROOK);
                    updatePieceList(rookColor, PIECE_ROOK, rookFromSq, rookToSq);
                }
            }

            // Update position hash
            posKey = generatePosKey();
        }


        /*
         * Generate position hash key using Zobrist hashing
         * XORs together piece keys, side key, castling key, and en passant key
         * Uses the centralized ZobristKeys class for key management
         */
        uint64_t generatePosKey() const {
            uint64_t finalKey = 0ULL;

            // XOR in piece keys for each piece on the board
            for (int color = 0; color < 2; ++color) {
                for (int type = 0; type < 6; ++type) {
                    uint64_t board = pieceBoards[color * 6 + type];
                    const int pieceIndex = color * 6 + type + 1;

                    while (board) {
                        const int sq64 = static_cast<int>(getLSB(board));
                        const int sq120 = toMailboxIndex(sq64);
                        finalKey ^= zobristKeys.getPieceKey(pieceIndex, sq120);
                        board &= (board - 1);
                    }
                }
            }

            // XOR in side-to-move key
            if (side == COLOR_WHITE) {
                finalKey ^= zobristKeys.getSideKey();
            }

            // XOR in castling rights key
            finalKey ^= zobristKeys.getCastleKey(castleRights);

            // XOR in en passant key if applicable
            if (enPas >= 0 && enPas < BRD_SQ_NUM) {
                finalKey ^= zobristKeys.getPieceKey(0, enPas);
            }

            return finalKey;
        }

        bool checkBoard() const {

            return true;
        }


        // Getters and Setters

        const std::array<uint64_t, 12>& getPieceBoards() const { return pieceBoards; }
        std::array<uint64_t, 12>& getPieceBoards() { return pieceBoards; }

        int getSide() const { return side; }
        void setSide(int s) { side = s; }

        int getEnPas() const { return enPas; }
        void setEnPas(int ep) { enPas = ep; }

        int getFiftyMove() const { return fiftyMove; }
        void setFiftyMove(int fm) { fiftyMove = fm; }

        int getHisPly() const { return hisPly; }
        void setHisPly(int hp) { hisPly = hp; }

        int getCastleRights() const { return castleRights; }
        void setCastleRights(int cr) { castleRights = cr; }

        uint64_t getPosKey() const { return posKey; }
        void setPosKey(uint64_t pk) { posKey = pk; }

        Move getLastMove() const { return lastMove; }

        bool isWhiteToMove() const { return side == COLOR_WHITE; }
        bool isBlackToMove() const { return side == COLOR_BLACK; }
    };

}  // namespace Chess

#endif  // BOARD_STATE_H