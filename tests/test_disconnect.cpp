#include "table.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main() {

    // --------------------------------------------
    // CASO 1:
    // Se desconecta el Puma.
    // Deben ganar los Perros.
    // --------------------------------------------

    {
        Table table(
            90,
            10,
            30,
            1234
        );

        GameResult finalResult =
            GameResult::Running;

        std::thread gameThread(
            [&]() {
                finalResult =
                    table.run();
            }
        );

        std::this_thread::sleep_for(
            std::chrono::milliseconds(
                100
            )
        );

        table.disconnect(
            Side::Puma
        );

        gameThread.join();

        assert(
            finalResult ==
            GameResult::DogsWin
        );

        std::cout
            << "OK: desconexion del Puma -> victoria de los Perros"
            << std::endl;
    }

    // --------------------------------------------
    // CASO 2:
    // Se desconecta el jugador de Perros.
    // Debe ganar el Puma.
    // --------------------------------------------

    {
        Table table(
            91,
            10,
            30,
            5678
        );

        GameResult finalResult =
            GameResult::Running;

        std::thread gameThread(
            [&]() {
                finalResult =
                    table.run();
            }
        );

        std::this_thread::sleep_for(
            std::chrono::milliseconds(
                100
            )
        );

        table.disconnect(
            Side::Dogs
        );

        gameThread.join();

        assert(
            finalResult ==
            GameResult::PumaWin
        );

        std::cout
            << "OK: desconexion de Perros -> victoria del Puma"
            << std::endl;
    }

    std::cout
        << "OK: TEST DE DESCONEXION COMPLETO"
        << std::endl;

    return 0;
}
