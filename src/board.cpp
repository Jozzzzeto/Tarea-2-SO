#include "board.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

Board::Board() {
    clear();
    buildTopology();
}

bool Board::isGridNode(Node node) {
    return node >= 0 && node < GRID_NODES;
}

void Board::clear() {
    for (auto& cell : cells_) {
        cell = Piece{};
    }
}

Piece Board::at(Node node) const {
    if (!validNode(node)) {
        throw std::out_of_range("Nodo invalido");
    }

    return cells_[static_cast<std::size_t>(node)];
}

bool Board::isEmpty(Node node) const {
    return validNode(node) &&
           at(node).type == PieceType::Empty;
}

bool Board::place(Node node, Piece piece) {
    if (!validNode(node) ||
        !isEmpty(node) ||
        piece.type == PieceType::Empty) {
        return false;
    }

    cells_[static_cast<std::size_t>(node)] = piece;
    return true;
}

bool Board::move(Node from, Node to) {
    if (!validNode(from) || !validNode(to)) {
        return false;
    }

    if (cells_[static_cast<std::size_t>(from)].type ==
            PieceType::Empty ||
        cells_[static_cast<std::size_t>(to)].type !=
            PieceType::Empty) {
        return false;
    }

    cells_[static_cast<std::size_t>(to)] =
        cells_[static_cast<std::size_t>(from)];

    cells_[static_cast<std::size_t>(from)] = Piece{};

    return true;
}

bool Board::remove(Node node) {
    if (!validNode(node) || isEmpty(node)) {
        return false;
    }

    cells_[static_cast<std::size_t>(node)] = Piece{};
    return true;
}

const std::vector<Board::Node>&
Board::neighbors(Node node) const {
    if (!validNode(node)) {
        throw std::out_of_range("Nodo invalido");
    }

    return adjacency_[static_cast<std::size_t>(node)];
}

bool Board::areAdjacent(Node a, Node b) const {
    if (!validNode(a) || !validNode(b)) {
        return false;
    }

    const auto& list =
        adjacency_[static_cast<std::size_t>(a)];

    return std::find(
        list.begin(),
        list.end(),
        b
    ) != list.end();
}

std::optional<Board::Node>
Board::jumpedNode(Node from, Node to) const {
    if (!validNode(from) || !validNode(to)) {
        return std::nullopt;
    }

    for (const Jump& jump : jumps_) {
        if (jump.from == from &&
            jump.to == to) {
            return jump.over;
        }
    }

    return std::nullopt;
}

std::string Board::nodeName(Node node) const {
    if (!validNode(node)) {
        return "INVALID";
    }

    if (node == THRONE_LEFT) {
        return "TRONO_LEFT";
    }

    if (node == THRONE_TOP) {
        return "TRONO_TOP";
    }

    if (node == THRONE_RIGHT) {
        return "TRONO_RIGHT";
    }

    const int x = node % GRID_SIZE;
    const int y = node / GRID_SIZE;

    return "(" +
           std::to_string(x) +
           "," +
           std::to_string(y) +
           ")";
}

char Board::pieceChar(const Piece& piece) {
    switch (piece.type) {
        case PieceType::Puma:
            return 'P';

        case PieceType::Dog:
            return 'D';

        case PieceType::Empty:
        default:
            return '.';
    }
}

std::string Board::toString() const {
    std::ostringstream out;

    out << "          "
        << pieceChar(at(THRONE_TOP))
        << "\n";

    out << "      "
        << pieceChar(at(THRONE_LEFT))
        << "       "
        << pieceChar(at(THRONE_RIGHT))
        << "\n";

    for (int y = GRID_SIZE - 1; y >= 0; --y) {
        out << "y=" << y << "   ";

        for (int x = 0; x < GRID_SIZE; ++x) {
            out << pieceChar(at(gridNode(x, y)));

            if (x + 1 < GRID_SIZE) {
                out << ' ';
            }
        }

        out << '\n';
    }

    return out.str();
}

void Board::addEdge(Node a, Node b) {
    adjacency_[static_cast<std::size_t>(a)].push_back(b);
    adjacency_[static_cast<std::size_t>(b)].push_back(a);
}

void Board::addJump(
    Node from,
    Node over,
    Node to
) {
    jumps_.push_back({
        from,
        over,
        to
    });
}

void Board::buildTopology() {
    // Grilla 5x5:
    // conexiones horizontales y verticales.
    for (int y = 0; y < GRID_SIZE; ++y) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            const Node here =
                gridNode(x, y);

            if (x + 1 < GRID_SIZE) {
                addEdge(
                    here,
                    gridNode(x + 1, y)
                );
            }

            if (y + 1 < GRID_SIZE) {
                addEdge(
                    here,
                    gridNode(x, y + 1)
                );
            }
        }
    }

    // Trono.
    addEdge(THRONE_LEFT, THRONE_TOP);
    addEdge(THRONE_TOP, THRONE_RIGHT);
    addEdge(THRONE_LEFT, THRONE_RIGHT);

    addEdge(
        THRONE_LEFT,
        gridNode(0, 4)
    );

    addEdge(
        THRONE_LEFT,
        gridNode(1, 4)
    );

    addEdge(
        THRONE_TOP,
        gridNode(2, 4)
    );

    addEdge(
        THRONE_RIGHT,
        gridNode(3, 4)
    );

    addEdge(
        THRONE_RIGHT,
        gridNode(4, 4)
    );

    // Saltos horizontales y verticales
    // de la grilla.
    const int directions[4][2] = {
        {1, 0},
        {-1, 0},
        {0, 1},
        {0, -1}
    };

    for (int y = 0; y < GRID_SIZE; ++y) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            for (const auto& dir : directions) {
                const int middleX =
                    x + dir[0];

                const int middleY =
                    y + dir[1];

                const int targetX =
                    x + 2 * dir[0];

                const int targetY =
                    y + 2 * dir[1];

                if (validGridCoord(middleX, middleY) &&
                    validGridCoord(targetX, targetY)) {

                    addJump(
                        gridNode(x, y),
                        gridNode(middleX, middleY),
                        gridNode(targetX, targetY)
                    );
                }
            }
        }
    }

    // Saltos relacionados con el trono.

    addJump(
        gridNode(1, 3),
        gridNode(1, 4),
        THRONE_LEFT
    );

    addJump(
        THRONE_LEFT,
        gridNode(1, 4),
        gridNode(1, 3)
    );

    addJump(
        gridNode(2, 3),
        gridNode(2, 4),
        THRONE_TOP
    );

    addJump(
        THRONE_TOP,
        gridNode(2, 4),
        gridNode(2, 3)
    );

    addJump(
        gridNode(3, 3),
        gridNode(3, 4),
        THRONE_RIGHT
    );

    addJump(
        THRONE_RIGHT,
        gridNode(3, 4),
        gridNode(3, 3)
    );

    addJump(
        gridNode(0, 4),
        THRONE_LEFT,
        THRONE_TOP
    );

    addJump(
        THRONE_TOP,
        THRONE_LEFT,
        gridNode(0, 4)
    );

    addJump(
        gridNode(4, 4),
        THRONE_RIGHT,
        THRONE_TOP
    );

    addJump(
        THRONE_TOP,
        THRONE_RIGHT,
        gridNode(4, 4)
    );
}
