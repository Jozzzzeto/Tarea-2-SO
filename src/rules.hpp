#pragma once

#include "board.hpp"

#include <optional>
#include <vector>

struct MoveResult {
    bool success = false;
    bool capture = false;

    int capturedDogId = -1;

    Board::Node capturedNode = -1;
};

class Rules {
public:
    static bool canDogMove(
        const Board& board,
        Board::Node from,
        Board::Node to
    );

    static bool canPumaMove(
        const Board& board,
        Board::Node from,
        Board::Node to
    );

    static std::optional<Board::Node>
    captureMiddle(
        const Board& board,
        Board::Node from,
        Board::Node to
    );

    static MoveResult applyDogMove(
        Board& board,
        Board::Node from,
        Board::Node to
    );

    static MoveResult applyPumaMove(
        Board& board,
        Board::Node from,
        Board::Node to
    );

    static std::vector<Board::Node>
    validDogMoves(
        const Board& board,
        Board::Node from
    );

    static std::vector<Board::Node>
    validPumaMoves(
        const Board& board,
        Board::Node from
    );

    static bool pumaWins(
        int capturedDogs
    );

    static bool dogsWin(
        const Board& board,
        Board::Node pumaNode
    );
};
