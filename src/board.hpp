#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

enum class PieceType {
    Empty,
    Puma,
    Dog
};

struct Piece {
    PieceType type = PieceType::Empty;
    int id = -1;
};

class Board {
public:
    using Node = int;

    static constexpr int GRID_SIZE = 5;
    static constexpr int GRID_NODES = 25;
    static constexpr int NODE_COUNT = 28;

    static constexpr Node THRONE_LEFT = 25;
    static constexpr Node THRONE_TOP = 26;
    static constexpr Node THRONE_RIGHT = 27;

    Board();

    static constexpr Node gridNode(int x, int y) {
        return y * GRID_SIZE + x;
    }

    static constexpr bool validGridCoord(int x, int y) {
        return x >= 0 && x < GRID_SIZE &&
               y >= 0 && y < GRID_SIZE;
    }

    static constexpr bool validNode(Node node) {
        return node >= 0 && node < NODE_COUNT;
    }

    static bool isGridNode(Node node);

    void clear();

    Piece at(Node node) const;

    bool isEmpty(Node node) const;

    bool place(Node node, Piece piece);

    bool move(Node from, Node to);

    bool remove(Node node);

    const std::vector<Node>& neighbors(Node node) const;

    bool areAdjacent(Node a, Node b) const;

    std::optional<Node> jumpedNode(
        Node from,
        Node to
    ) const;

    std::string nodeName(Node node) const;

    std::string toString() const;

private:
    struct Jump {
        Node from;
        Node over;
        Node to;
    };

    std::array<Piece, NODE_COUNT> cells_{};

    std::array<
        std::vector<Node>,
        NODE_COUNT
    > adjacency_{};

    std::vector<Jump> jumps_;

    void buildTopology();

    void addEdge(Node a, Node b);

    void addJump(
        Node from,
        Node over,
        Node to
    );

    static char pieceChar(const Piece& piece);
};

