#include "board.hpp"
#include "rules.hpp"

#include <cassert>
#include <iostream>

int main() {

    // TEST 1:
    // Colocar el Puma en el tablero.
    {
        Board board;

        const auto puma =
            Board::gridNode(2, 2);

        assert(
            board.place(
                puma,
                Piece{
                    PieceType::Puma,
                    0
                }
            )
        );

        assert(
            board.at(puma).type ==
            PieceType::Puma
        );
    }

    // TEST 2:
    // Movimiento normal de un Perro.
    {
        Board board;

        const auto from =
            Board::gridNode(1, 1);

        const auto to =
            Board::gridNode(1, 2);

        assert(
            board.place(
                from,
                Piece{
                    PieceType::Dog,
                    0
                }
            )
        );

        assert(
            Rules::canDogMove(
                board,
                from,
                to
            )
        );

        const MoveResult result =
            Rules::applyDogMove(
                board,
                from,
                to
            );

        assert(result.success);

        assert(
            board.isEmpty(from)
        );

        assert(
            board.at(to).type ==
            PieceType::Dog
        );
    }

    // TEST 3:
    // Movimiento inválido del Puma:
    // (2,2) -> (4,4)
    {
        Board board;

        const auto puma =
            Board::gridNode(2, 2);

        const auto invalid =
            Board::gridNode(4, 4);

        assert(
            board.place(
                puma,
                Piece{
                    PieceType::Puma,
                    0
                }
            )
        );

        assert(
            !Rules::canPumaMove(
                board,
                puma,
                invalid
            )
        );
    }

    // TEST 4:
    // Captura del Puma:
    //
    // Puma en (2,2)
    // Perro en (2,3)
    // Puma salta a (2,4)
    {
        Board board;

        const auto puma =
            Board::gridNode(2, 2);

        const auto dog =
            Board::gridNode(2, 3);

        const auto destination =
            Board::gridNode(2, 4);

        assert(
            board.place(
                puma,
                Piece{
                    PieceType::Puma,
                    0
                }
            )
        );

        assert(
            board.place(
                dog,
                Piece{
                    PieceType::Dog,
                    7
                }
            )
        );

        const MoveResult result =
            Rules::applyPumaMove(
                board,
                puma,
                destination
            );

        assert(result.success);
        assert(result.capture);

        assert(
            result.capturedDogId == 7
        );

        assert(
            board.isEmpty(dog)
        );

        assert(
            board.isEmpty(puma)
        );

        assert(
            board.at(destination).type ==
            PieceType::Puma
        );
    }

    // TEST 5:
    // Puma gana al capturar 6 perros.
    {
        assert(
            !Rules::pumaWins(5)
        );

        assert(
            Rules::pumaWins(6)
        );

        assert(
            Rules::pumaWins(7)
        );
    }

    // TEST 6:
    // Conexión entre grilla y trono.
    {
        Board board;

        assert(
            board.areAdjacent(
                Board::gridNode(2, 4),
                Board::THRONE_TOP
            )
        );

        assert(
            board.areAdjacent(
                Board::THRONE_TOP,
                Board::THRONE_LEFT
            )
        );
    }

    std::cout
        << "OK: todos los tests de Board/Rules pasaron\n";

    return 0;
}
