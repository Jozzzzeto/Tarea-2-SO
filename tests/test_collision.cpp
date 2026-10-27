#include "table.hpp"

#include <array>
#include <atomic>
#include <cassert>
#include <iostream>
#include <pthread.h>

struct CollisionThreadData {
    Table* table;
    int dogId;
    Board::Node destination;

    pthread_barrier_t* barrier;

    std::atomic<int>* successes;
};

void* collisionThread(void* arg) {

    auto* data =
        static_cast<CollisionThreadData*>(arg);

    // Los 4 hilos esperan aqui.
    pthread_barrier_wait(
        data->barrier
    );

    if (
        data->table->attemptDogMove(
            data->dogId,
            data->destination
        )
    ) {
        ++(*data->successes);
    }

    return nullptr;
}

int main() {

    const Board::Node center =
        Board::gridNode(2, 2);

    // 4 perros rodeando el centro.
    const std::array<Board::Node, 12> dogs = {

        Board::gridNode(2, 1),
        Board::gridNode(1, 2),
        Board::gridNode(3, 2),
        Board::gridNode(2, 3),

        Board::gridNode(0, 0),
        Board::gridNode(1, 0),
        Board::gridNode(2, 0),
        Board::gridNode(3, 0),
        Board::gridNode(4, 0),

        Board::gridNode(0, 3),
        Board::gridNode(0, 4),
        Board::gridNode(4, 4)
    };

    Table table(
        50,
        50,
        300,
        555,
        Board::THRONE_TOP,
        dogs
    );

    pthread_barrier_t barrier;

    assert(
        pthread_barrier_init(
            &barrier,
            nullptr,
            4
        ) == 0
    );

    std::atomic<int> successes{0};

    std::array<pthread_t, 4> threads{};

    std::array<
        CollisionThreadData,
        4
    > data{};

    for (
        int dogId = 0;
        dogId < 4;
        ++dogId
    ) {

        data[
            static_cast<std::size_t>(
                dogId
            )
        ] = CollisionThreadData{
            &table,
            dogId,
            center,
            &barrier,
            &successes
        };

        assert(
            pthread_create(
                &threads[
                    static_cast<std::size_t>(
                        dogId
                    )
                ],
                nullptr,
                collisionThread,
                &data[
                    static_cast<std::size_t>(
                        dogId
                    )
                ]
            ) == 0
        );
    }

    for (auto& thread : threads) {

        pthread_join(
            thread,
            nullptr
        );
    }

    pthread_barrier_destroy(
        &barrier
    );

    // Solo uno debe haber conseguido
    // ocupar el centro.
    assert(
        successes.load() == 1
    );

    const Piece winner =
        table.pieceAt(center);

    assert(
        winner.type ==
        PieceType::Dog
    );

    std::cout
        << "OK: 4 Perros intentaron ocupar "
        << "(2,2) simultaneamente y solo "
        << "uno obtuvo la casilla\n";

    return 0;
}
