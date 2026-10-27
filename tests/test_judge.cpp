#include "table.hpp"

#include <array>
#include <cassert>
#include <iostream>

int main() {

    // Puma en el centro.
    const Board::Node puma =
        Board::gridNode(2, 2);

    // Bloqueamos:
    //
    // vecinos inmediatos:
    // (1,2), (3,2), (2,1), (2,3)
    //
    // y tambien los destinos de salto:
    // (0,2), (4,2), (2,0), (2,4)
    //
    // Por lo tanto el Puma no puede:
    // - moverse normalmente
    // - capturar saltando

    const std::array<
        Board::Node,
        12
    > dogs = {

        Board::gridNode(1, 2),
        Board::gridNode(3, 2),
        Board::gridNode(2, 1),
        Board::gridNode(2, 3),

        Board::gridNode(0, 2),
        Board::gridNode(4, 2),
        Board::gridNode(2, 0),
        Board::gridNode(2, 4),

        Board::gridNode(0, 0),
        Board::gridNode(4, 0),
        Board::gridNode(0, 4),
        Board::gridNode(4, 4)
    };

    Table table(
        99,
        100,
        100,
        42,
        puma,
        dogs
    );

    const GameResult result =
        table.run();

    assert(
        result ==
        GameResult::DogsWin
    );

    assert(
        table.capturedDogs() == 0
    );

    std::cout
        << "OK: el Juez detecto automaticamente "
        << "que el Puma estaba acorralado\n";

    return 0;
}
