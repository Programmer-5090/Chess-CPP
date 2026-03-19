#ifndef BOARD_REPRESENTATION
#define BOARD_REPRESENTATION

#include <string>
#include <string_view>

namespace Chess
{
    struct Coord {
        int fileIndex = 0;
        int rankIndex = 0;

        constexpr Coord() = default;
        constexpr Coord(int fileIndex, int rankIndex) : fileIndex(fileIndex), rankIndex(rankIndex) {}

        constexpr bool isLightSq() const {
            return (fileIndex + rankIndex) % 2 != 0;
        }

        constexpr bool isTheSame(const Coord& other) const {
            return (fileIndex == other.fileIndex) && (rankIndex == other.rankIndex);
        }
    };

    class BoardRepresentation
    {
    public:
        static constexpr std::string_view fileNames = "abcdefgh";
        static constexpr std::string_view rankNames = "12345678";

        static constexpr int a1 = 0;
        static constexpr int b1 = 1;
        static constexpr int c1 = 2;
        static constexpr int d1 = 3;
        static constexpr int e1 = 4;
        static constexpr int f1 = 5;
        static constexpr int g1 = 6;
        static constexpr int h1 = 7;

        static constexpr int a8 = 56;
        static constexpr int b8 = 57;
        static constexpr int c8 = 58;
        static constexpr int d8 = 59;
        static constexpr int e8 = 60;
        static constexpr int f8 = 61;
        static constexpr int g8 = 62;
        static constexpr int h8 = 63;

        static constexpr int RankIndex(int squareIndex)
        {
            return squareIndex >> 3;
        }

        static constexpr int FileIndex(int squareIndex)
        {
            return squareIndex & 0b000111;
        }

        static constexpr int IndexFromCoord(int fileIndex, int rankIndex)
        {
            return rankIndex * 8 + fileIndex;
        }

        static constexpr int IndexFromCoord(const Coord& coord)
        {
            return IndexFromCoord(coord.fileIndex, coord.rankIndex);
        }

        static constexpr Coord CoordFromIndex(int squareIndex)
        {
            return Coord(FileIndex(squareIndex), RankIndex(squareIndex));
        }

        static constexpr bool LightSquare(int fileIndex, int rankIndex)
        {
            return ((fileIndex + rankIndex) % 2) != 0;
        }

        static std::string SquareNameFromCoordinate(int fileIndex, int rankIndex)
        {
            return std::string(1, fileNames[static_cast<std::size_t>(fileIndex)]) + std::to_string(rankIndex + 1);
        }

        static std::string SquareNameFromCoordinate(const Coord& coord)
        {
            return SquareNameFromCoordinate(coord.fileIndex, coord.rankIndex);
        }

        static std::string SquareNameFromIndex(int squareIndex)
        {
            return SquareNameFromCoordinate(CoordFromIndex(squareIndex));
        }
    };
}

#endif