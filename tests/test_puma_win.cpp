#include "table.hpp"

#include <array>
#include <cassert>
#include <iostream>

int main() {

    /*
        Secuencia preparada:

        Puma empieza en (0,0).

        Captura 1:
        (0,0) -> (2,0)
        sobre perro en (1,0)

        Captura 2:
        (2,0) -> (4,0)
        sobre perro en (3,0)

        Captura 3:
        (4,0) -> (4,2)
        sobre perro en (4,1)

        Captura 4:
        (4,2) -> (2,2)
        sobre perro en (3,2)

        Captura 5:
        (2,2) -> (0,2)
        sobre perro en (1,2)

        Captura 6:
        (0,2) -> (0,4)
        sobre perro en (0,3)
    */

    const Board::Node puma =
        Board::gridNode(0, 0);

    const std::array<
        Board::Node,
        12
    > dogs = {

        // Los 6 que seran capturados.
        Board::gridNode(1, 0),
        Board::gridNode(3, 0),
        Board::gridNode(4, 1),
        Board::gridNode(3, 2),
        Board::gridNode(1, 2),
        Board::gridNode(0, 3),

        // Otros 6 perros.
        Board::gridNode(1, 1),
        Board::gridNode(2, 1),
        Board::gridNode(3, 1),
        Board::gridNode(1, 4),
        Board::gridNode(2, 4),
        Board::gridNode(3, 4)
    };

    Table table(
        77,
        1,
        1,
        42,
        puma,
        dogs
    );

    // Captura 1
    assert(
        table.attemptPumaMove(
            Board::gridNode(2, 0)
        )
    );

    // Captura 2
    assert(
        table.attemptPumaMove(
            Board::gridNode(4, 0)
        )
    );

    // Captura 3
    assert(
        table.attemptPumaMove(
            Board::gridNode(4, 2)
        )
    );

    // Captura 4
    assert(
        table.attemptPumaMove(
            Board::gridNode(2, 2)
        )
    );

    // Captura 5
    assert(
        table.attemptPumaMove(
            Board::gridNode(0, 2)
        )
    );

    // Captura 6
    assert(
        table.attemptPumaMove(
            Board::gridNode(0, 4)
        )
    );

    assert(
        table.capturedDogs() == 6
    );

    /*
        Ahora iniciamos la Mesa.

        El Juez debe observar que ya hay
        6 perros capturados y declarar
        victoria del Puma automaticamente.
    */

    const GameResult result =
        table.run();

    assert(
        result ==
        GameResult::PumaWin
    );

    std::cout
        << "OK: el Puma capturo 6 Perros "
        << "y el Juez declaro su victoria "
        << "automaticamente\n";

    return 0;
}
