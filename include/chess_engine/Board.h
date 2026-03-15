#ifndef BOARD_CLASS_H
#define BOARD_CLASS_H


#include "pieces.h"


struct Player
{
    int color;
    int pieceType;
    int fromSquare;
    int toSquare;
	bool isPieceSelected;
};


class BoardState
{
 
    uint64_t pieceBoards[12];
	uint64_t whitePieces;
    uint64_t black_pieces;
    uint64_t main_board;
    int[64] mail_box;
    Player player;

    public:
        void init()
        {
            //empty in mailbox is -1
            memset(mailbox, -1, sizeof(mailbox));

            //initializing piece boards for white and black pieces
            pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_PAWN] = 0x000000000000FF00;
            pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_ROOK] = 0x0000000000000081;
            pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_KNIGHT] = 0x0000000000000042;
            pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_BISHOP] = 0x0000000000000024;
            pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_QUEEN] = 0x0000000000000008;
            pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_KING] = 0x0000000000000010;

            pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_PAWN] = 0x00FF000000000000;
            pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_ROOK] = 0x8100000000000000;
            pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_KNIGHT] = 0x4200000000000000;
            pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_BISHOP] = 0x2400000000000000;
            pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_QUEEN] = 0x0800000000000000;
            pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_KING] = 0x1000000000000000;

            //initializing main board by ORing all piece boards together
            uint64_t white_pieces = pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_PAWN] | pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_ROOK] |
                                    pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_KNIGHT] | pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_BISHOP] |
				                    pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_QUEEN] | pieceBoards[chess::COLOR_WHITE * 6 + chess::PIECE_KING];

            uint64_t black_pieces = pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_PAWN] | pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_ROOK] |
				                    pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_KNIGHT] | pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_BISHOP] |
				                    pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_QUEEN] | pieceBoards[chess::COLOR_BLACK * 6 + chess::PIECE_KING];
			
            main_board = white_pieces | black_pieces;

            //initializing mailbox for quick piece type lookup
            for (int color = 0; color < 2; color++) {
                for (int type = 0; type < 6; type++) {
                    uint64_t piece_board = piece_board[color][type];
                    while (piece_board) {
                        int lowest_set_bit = __builtin_ctzll(piece_board);
                        mailbox[lowest_set_bit] = type;
                        piece_board &= piece_board - 1;
                    }
                }
            }


        }
        void selectPiece(int x, int y)
        {
            int index = y * 8 + x;

            if (!(main_board & (1ULL << (index))))
            {

                if (whitePieces & (1ULL << (index)))
                    player.color = chess::COLOR_WHITE;
                
                else
                    player.color = chess::COLOR_BLACK;

                switch (mail_box[index])
                {
                case chess::PIECE_PAWN:
                    player.pieceType = chess::PIECE_PAWN;
                    break;
                case chess::PIECE_KNIGHT:
                    player.pieceType = chess::PIECE_KNIGHT;
                    break;
                case chess::PIECE_BISHOP:
                    player.pieceType = chess::PIECE_BISHOP;
                    break;
                case chess::PIECE_ROOK:
                    player.pieceType = chess::PIECE_ROOK;
                    break;
                case chess::PIECE_QUEEN:
                    player.pieceType = chess::PIECE_QUEEN;
                    break;
                case chess::PIECE_KING:
                    player.pieceType = chess::PIECE_KING;
                    break;
                }

				player.fromSquare = index;

            }
        }

};

#endif 


