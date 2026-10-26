#include "rules.hpp"

bool Rules::canDogMove(
    const Board& board,
    Board::Node from,
    Board::Node to
) {
    if (!Board::validNode(from) ||
        !Board::validNode(to)) {
        return false;
    }

    const Piece dog = board.at(from);

    return dog.type == PieceType::Dog &&
           board.isEmpty(to) &&
           board.areAdjacent(from, to);
}

std::optional<Board::Node>
Rules::captureMiddle(
    const Board& board,
    Board::Node from,
    Board::Node to
) {
    if (!Board::validNode(from) ||
        !Board::validNode(to)) {
        return std::nullopt;
    }

    if (board.at(from).type != PieceType::Puma ||
        !board.isEmpty(to)) {
        return std::nullopt;
    }

    const auto middle =
        board.jumpedNode(from, to);

    if (!middle.has_value()) {
        return std::nullopt;
    }

    if (board.at(*middle).type != PieceType::Dog) {
        return std::nullopt;
    }

    return middle;
}

bool Rules::canPumaMove(
    const Board& board,
    Board::Node from,
    Board::Node to
) {
    if (!Board::validNode(from) ||
        !Board::validNode(to)) {
        return false;
    }

    if (board.at(from).type != PieceType::Puma ||
        !board.isEmpty(to)) {
        return false;
    }

    return board.areAdjacent(from, to) ||
           captureMiddle(
               board,
               from,
               to
           ).has_value();
}

MoveResult Rules::applyDogMove(
    Board& board,
    Board::Node from,
    Board::Node to
) {
    if (!canDogMove(
            board,
            from,
            to
        )) {
        return MoveResult{};
    }

    if (!board.move(from, to)) {
        return MoveResult{};
    }

    MoveResult result;
    result.success = true;

    return result;
}

MoveResult Rules::applyPumaMove(
    Board& board,
    Board::Node from,
    Board::Node to
) {
    if (!canPumaMove(
            board,
            from,
            to
        )) {
        return MoveResult{};
    }

    MoveResult result;

    const auto middle =
        captureMiddle(
            board,
            from,
            to
        );

    if (middle.has_value()) {
        const Piece captured =
            board.at(*middle);

        board.remove(*middle);

        result.capture = true;
        result.capturedDogId = captured.id;
        result.capturedNode = *middle;
    }

    if (!board.move(from, to)) {
        return MoveResult{};
    }

    result.success = true;

    return result;
}

std::vector<Board::Node>
Rules::validDogMoves(
    const Board& board,
    Board::Node from
) {
    std::vector<Board::Node> result;

    if (!Board::validNode(from) ||
        board.at(from).type != PieceType::Dog) {
        return result;
    }

    for (Board::Node to : board.neighbors(from)) {
        if (canDogMove(
                board,
                from,
                to
            )) {
            result.push_back(to);
        }
    }

    return result;
}

std::vector<Board::Node>
Rules::validPumaMoves(
    const Board& board,
    Board::Node from
) {
    std::vector<Board::Node> result;

    if (!Board::validNode(from) ||
        board.at(from).type != PieceType::Puma) {
        return result;
    }

    // Movimientos normales.
    for (Board::Node to : board.neighbors(from)) {
        if (canPumaMove(
                board,
                from,
                to
            )) {
            result.push_back(to);
        }
    }

    // Posibles saltos/capturas.
    for (
        Board::Node to = 0;
        to < Board::NODE_COUNT;
        ++to
    ) {
        if (board.jumpedNode(
                from,
                to
            ).has_value() &&
            canPumaMove(
                board,
                from,
                to
            )) {

            bool repeated = false;

            for (Board::Node existing : result) {
                if (existing == to) {
                    repeated = true;
                    break;
                }
            }

            if (!repeated) {
                result.push_back(to);
            }
        }
    }

    return result;
}

bool Rules::pumaWins(
    int capturedDogs
) {
    return capturedDogs >= 6;
}

bool Rules::dogsWin(
    const Board& board,
    Board::Node pumaNode
) {
    return validPumaMoves(
        board,
        pumaNode
    ).empty();
}

