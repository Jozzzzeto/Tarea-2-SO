#include "table.hpp"

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

int main() {

    // -------------------------------------------------
    // STRESS 1
    // 500 intentos concurrentes de movimiento.
    // -------------------------------------------------

    {
        Table table(
            123,
            1,          // delay minimo
            1,          // delay maximo
            123456      // seed
        );

        std::atomic<int> attempts{0};

        std::vector<std::thread> workers;

        // 12 Perros.
        // Cada uno intenta 38 movimientos:
        //
        // 12 * 38 = 456
        for (int dogId = 0; dogId < 12; ++dogId) {

            workers.emplace_back(
                [&table, &attempts, dogId]() {

                    std::mt19937 generator(
                        123456U +
                        static_cast<unsigned int>(
                            dogId
                        )
                    );

                    std::uniform_int_distribution<int>
                        destination(
                            0,
                            Board::NODE_COUNT - 1
                        );

                    for (
                        int i = 0;
                        i < 38;
                        ++i
                    ) {

                        table.attemptDogMove(
                            dogId,
                            destination(generator)
                        );

                        ++attempts;

                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(1)
                        );
                    }
                }
            );
        }

        // Puma:
        // 44 intentos.
        //
        // 456 + 44 = 500.
        workers.emplace_back(
            [&table, &attempts]() {

                std::mt19937 generator(
                    999999U
                );

                std::uniform_int_distribution<int>
                    destination(
                        0,
                        Board::NODE_COUNT - 1
                    );

                for (
                    int i = 0;
                    i < 44;
                    ++i
                ) {

                    table.attemptPumaMove(
                        destination(generator)
                    );

                    ++attempts;

                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(1)
                    );
                }
            }
        );

        for (auto& worker : workers) {
            worker.join();
        }

        assert(
            attempts.load() == 500
        );

        // Comprobacion basica de integridad:
        // debe seguir existiendo exactamente
        // un Puma y como maximo 12 Perros.
        int pumas = 0;
        int dogs = 0;

        for (
            Board::Node node = 0;
            node < Board::NODE_COUNT;
            ++node
        ) {

            const Piece piece =
                table.pieceAt(node);

            if (piece.type == PieceType::Puma) {
                ++pumas;
            }

            if (piece.type == PieceType::Dog) {
                ++dogs;
            }
        }

        assert(pumas == 1);
        assert(dogs >= 0);
        assert(dogs <= 12);

        std::cout
            << "OK: 500 intentos concurrentes "
            << "a 1 ms sin corromper el tablero\n";
    }

    // -------------------------------------------------
    // STRESS 2
    // Ejecutar la partida real completa:
    //
    // 1 Puma
    // 12 Perros
    // 1 Juez
    //
    // todos con retardo de 1 ms.
    // -------------------------------------------------

    {
        Table table(
            124,
            1,
            1,
            654321
        );

        GameResult finalResult =
            GameResult::Running;

        std::thread gameThread(
            [&table, &finalResult]() {

                finalResult =
                    table.run();
            }
        );

        // La dejamos trabajar bajo carga.
        std::this_thread::sleep_for(
            std::chrono::milliseconds(
                1000
            )
        );

        // Si la partida no termino sola,
        // solicitamos cierre limpio.
        table.requestStop();

        gameThread.join();

        assert(
            finalResult !=
            GameResult::Running
        );

        std::cout
            << "OK: partida completa con "
            << "14 hilos y retardo de 1 ms "
            << "termino limpiamente\n";
    }

    std::cout
        << "OK: STRESS TEST COMPLETO\n";

    return 0;
}
