#ifndef BOARD_CLASS_H
#define BOARD_CLASS_H

#include "pieces.h"

#include <algorithm>
#include <array>
#include <vector>
#include <bit>
#include <cstdint>

namespace Chess {
    struct Player
    {
        int color;
        int pieceType;
        int fromSquare;
        int toSquare;
        bool isPieceSelected;

        void reset()
        {
            color = -1;
            pieceType = -1;
            fromSquare = -1;
            toSquare = -1;
            isPieceSelected = false;
        }
    };

    struct PieceList {
        std::vector<int> squares;
        int count() const { return static_cast<int>(squares.size()); }
        void add(int sq) { squares.push_back(sq); }
        void remove(int sq) {
            auto it = std::find(squares.begin(), squares.end(), sq);
            if (it != squares.end()) squares.erase(it);
        }
        void move(int from, int to) {
            auto it = std::find(squares.begin(), squares.end(), from);
            if (it != squares.end()) *it = to;
        }
        void clear() { squares.clear(); }
    };


    class BoardState
    {
        std::array<uint64_t, 12> pieceBoards{};
        std::array<PieceList, 12> pieceLists{};
        uint64_t whitePieces = 0;
        uint64_t blackPieces = 0;
        uint64_t mainBoard = 0;
        std::array<int, 120> mailbox{};
        Player player{};

        static constexpr int toMailboxIndex(int square64)
        {
            const int file = square64 % 8;
            const int rank = square64 / 8;
            return 21 + file + rank * 10;
        }

        static constexpr int toSquare64(int file, int rank)
        {
            return rank * 8 + file;
        }

        void rebuildOccupancy()
        {
            whitePieces = 0;
            blackPieces = 0;

            for (int type = 0; type < 6; ++type) {
                whitePieces |= pieceBoards[chess::COLOR_WHITE * 6 + type];
                blackPieces |= pieceBoards[chess::COLOR_BLACK * 6 + type];
            }

            mainBoard = whitePieces | blackPieces;
        }

        void rebuildMailbox()
        {
            mailbox.fill(-1);

            for (int color = 0; color < 2; ++color) {
                for (int type = 0; type < 6; ++type) {
                    uint64_t pieceBoard = pieceBoards[color * 6 + type];

                    while (pieceBoard) {
                        const int lowestSetBit = static_cast<int>(std::countr_zero(pieceBoard));
                        const int mailboxIndex = toMailboxIndex(lowestSetBit);
                        mailbox[mailboxIndex] = type;
                        pieceBoard &= (pieceBoard - 1);
                    }
                }
            }
        }

        void rebuildPieceLists()
        {
            for (auto& list : pieceLists) {
                list.clear();
            }

            for (int color = 0; color < 2; ++color) {
                for (int type = 0; type < 6; ++type) {
                    const int index = color * 6 + type;
                    uint64_t pieceBoard = pieceBoards[index];

                    while (pieceBoard) {
                        const int square = static_cast<int>(std::countr_zero(pieceBoard));
                        pieceLists[index].add(square);
                        pieceBoard &= (pieceBoard - 1);
                    }
                }
            }
        }

    public:
        void init()
        {
            pieceBoards.fill(0);

            pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_PAWN] = 0x000000000000FF00ULL;
            pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_ROOK] = 0x0000000000000081ULL;
            pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_KNIGHT] = 0x0000000000000042ULL;
            pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_BISHOP] = 0x0000000000000024ULL;
            pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_QUEEN] = 0x0000000000000008ULL;
            pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_KING] = 0x0000000000000010ULL;

            pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_PAWN] = 0x00FF000000000000ULL;
            pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_ROOK] = 0x8100000000000000ULL;
            pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_KNIGHT] = 0x4200000000000000ULL;
            pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_BISHOP] = 0x2400000000000000ULL;
            pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_QUEEN] = 0x0800000000000000ULL;
            pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_KING] = 0x1000000000000000ULL;

            rebuildOccupancy();
            rebuildMailbox();
            rebuildPieceLists();
            player.reset();
        }

        const PieceList& getPieceList(int color, int pieceType) const
        {
            return pieceLists[color * 6 + pieceType];
        }

        void select(int x, int y)
        {
            if (x < 0 || x > 7 || y < 0 || y > 7) {
                return;
            }

            const int index64 = toSquare64(x, y);
            const int index120 = toMailboxIndex(index64);

            if (mainBoard & (1ULL << index64)) {
                player.color = (whitePieces & (1ULL << index64)) ? chess::COLOR_WHITE : chess::COLOR_BLACK;
                player.pieceType = mailbox[index120];
                player.fromSquare = index64;
                player.isPieceSelected = true;
            }
            else if (player.isPieceSelected) {
                move(x, y);
            }
        }


        void move(int x, int y)
        {
            if (!player.isPieceSelected) {
                return;
            }

            if (x < 0 || x > 7 || y < 0 || y > 7) {
                return;
            }

            player.toSquare = toSquare64(x, y);

            if (player.toSquare == player.fromSquare) {
                player.reset();
                return;
            }

            const uint64_t toMask = (1ULL << player.toSquare);
            const int toIndex120 = toMailboxIndex(player.toSquare);
            if (mainBoard & toMask) {
                const int capturedPieceType = mailbox[toIndex120];
                const int capturedColor = (whitePieces & toMask) ? chess::COLOR_WHITE : chess::COLOR_BLACK;
                pieceBoards[capturedColor * 6 + capturedPieceType] &= ~toMask;
            }

            const uint64_t fromMask = (1ULL << player.fromSquare);
            pieceBoards[player.color * 6 + player.pieceType] &= ~fromMask;
            pieceBoards[player.color * 6 + player.pieceType] |= toMask;

            rebuildOccupancy();
            rebuildMailbox();
            rebuildPieceLists();
            player.reset();
        }
    };
}

#endif


