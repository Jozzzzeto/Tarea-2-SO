#include "table.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main() {

    Table table(
        1,      // id mesa
        10,     // delay minimo
        30,     // delay maximo
        42      // seed
    );

    std::thread stopper(
        [&table]() {

            std::this_thread::sleep_for(
                std::chrono::milliseconds(
                    500
                )
            );

            table.requestStop();
        }
    );

    const GameResult result =
        table.run();

    stopper.join();

    assert(
        result ==
        GameResult::Aborted
    );

  std::cout
    << "OK: Puma y 12 Perros ejecutaron "
    << "concurrentemente y la mesa "
    << "se detuvo correctamente\n";

    return 0;
}

