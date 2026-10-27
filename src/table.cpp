#include "table.hpp"
#include "rules.hpp"

#include <chrono>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace {

pthread_mutex_t consoleMutex =
    PTHREAD_MUTEX_INITIALIZER;

}

// --------------------------------------------------
// CONSTRUCTORES
// --------------------------------------------------

Table::Table(
    int tableId,
    int delayMinMs,
    int delayMaxMs,
    unsigned int seed
)
    : Table(
          tableId,
          delayMinMs,
          delayMaxMs,
          seed,
          Board::THRONE_TOP,
          std::array<Board::Node, 12>{
              Board::gridNode(0, 0),
              Board::gridNode(1, 0),
              Board::gridNode(2, 0),
              Board::gridNode(3, 0),
              Board::gridNode(4, 0),

              Board::gridNode(0, 1),
              Board::gridNode(1, 1),
              Board::gridNode(2, 1),
              Board::gridNode(3, 1),
              Board::gridNode(4, 1),

              Board::gridNode(0, 2),
              Board::gridNode(4, 2)
          }
      ) {
}

Table::Table(
    int tableId,
    int delayMinMs,
    int delayMaxMs,
    unsigned int seed,
    Board::Node pumaStart,
    const std::array<Board::Node, 12>& dogStarts
)
    : tableId_(tableId),
      delayMinMs_(delayMinMs),
      delayMaxMs_(delayMaxMs),
      seed_(seed),
      initialPumaNode_(pumaStart),
      initialDogNodes_(dogStarts),
      pumaNode_(pumaStart),
      capturedDogs_(0),
      finished_(false),
      result_(GameResult::Running) {

    if (
        delayMinMs_ < 0 ||
        delayMaxMs_ < 0 ||
        delayMinMs_ > delayMaxMs_
    ) {
        throw std::invalid_argument(
            "Rango de retardos invalido"
        );
    }

    if (!Board::validNode(initialPumaNode_)) {
        throw std::invalid_argument(
            "Posicion inicial del Puma invalida"
        );
    }

    if (
        pthread_mutex_init(
            &boardMutex_,
            nullptr
        ) != 0
    ) {
        throw std::runtime_error(
            "No se pudo inicializar el mutex"
        );
    }

    if (
        pthread_cond_init(
            &stateChanged_,
            nullptr
        ) != 0
    ) {
        pthread_mutex_destroy(
            &boardMutex_
        );

        throw std::runtime_error(
            "No se pudo inicializar pthread_cond"
        );
    }

    initializeBoard();
}

// --------------------------------------------------
// DESTRUCTOR
// --------------------------------------------------

Table::~Table() {

    pthread_cond_destroy(
        &stateChanged_
    );

    pthread_mutex_destroy(
        &boardMutex_
    );
}

// --------------------------------------------------
// INICIALIZACION DEL TABLERO
// --------------------------------------------------

void Table::initializeBoard() {

    board_.clear();

    pumaNode_ =
        initialPumaNode_;

    if (
        !board_.place(
            pumaNode_,
            Piece{
                PieceType::Puma,
                0
            }
        )
    ) {
        throw std::runtime_error(
            "No se pudo colocar el Puma"
        );
    }

    for (
        int dogId = 0;
        dogId < 12;
        ++dogId
    ) {

        const Board::Node node =
            initialDogNodes_[
                static_cast<std::size_t>(
                    dogId
                )
            ];

        if (!Board::validNode(node)) {
            throw std::runtime_error(
                "Posicion inicial de Perro invalida"
            );
        }

        dogNodes_[
            static_cast<std::size_t>(
                dogId
            )
        ] = node;

        if (
            !board_.place(
                node,
                Piece{
                    PieceType::Dog,
                    dogId
                }
            )
        ) {
            throw std::runtime_error(
                "No se pudo colocar un Perro"
            );
        }
    }
}

// --------------------------------------------------
// CONSULTAS DE ESTADO
// --------------------------------------------------

GameResult Table::result() const {

    pthread_mutex_lock(
        &boardMutex_
    );

    const GameResult current =
        result_;

    pthread_mutex_unlock(
        &boardMutex_
    );

    return current;
}

int Table::capturedDogs() const {

    pthread_mutex_lock(
        &boardMutex_
    );

    const int value =
        capturedDogs_;

    pthread_mutex_unlock(
        &boardMutex_
    );

    return value;
}

bool Table::isFinished() {

    pthread_mutex_lock(
        &boardMutex_
    );

    const bool value =
        finished_;

    pthread_mutex_unlock(
        &boardMutex_
    );

    return value;
}

// --------------------------------------------------
// TERMINO DE PARTIDA
// --------------------------------------------------

void Table::finishGame(
    GameResult result
) {

    // Esta funcion se llama con boardMutex_ ya bloqueado.

    finished_ = true;
    result_ = result;

    pthread_cond_broadcast(
        &stateChanged_
    );
}

void Table::requestStop() {

    pthread_mutex_lock(
        &boardMutex_
    );

    if (!finished_) {

        finishGame(
            GameResult::Aborted
        );
    }

    pthread_mutex_unlock(
        &boardMutex_
    );
}

void Table::disconnect(
    Side side
) {

    pthread_mutex_lock(
        &boardMutex_
    );

    if (!finished_) {

        if (side == Side::Puma) {

            finishGame(
                GameResult::DogsWin
            );

        } else {

            finishGame(
                GameResult::PumaWin
            );
        }
    }

    pthread_mutex_unlock(
        &boardMutex_
    );
}

// --------------------------------------------------
// MOVIMIENTOS CONTROLADOS PARA TESTS
// --------------------------------------------------

bool Table::attemptDogMove(
    int dogId,
    Board::Node destination
) {

    if (
        dogId < 0 ||
        dogId >= 12
    ) {
        return false;
    }

    pthread_mutex_lock(
        &boardMutex_
    );

    if (finished_) {

        pthread_mutex_unlock(
            &boardMutex_
        );

        return false;
    }

    Board::Node& current =
        dogNodes_[
            static_cast<std::size_t>(
                dogId
            )
        ];

    if (current < 0) {

        pthread_mutex_unlock(
            &boardMutex_
        );

        return false;
    }

    const MoveResult move =
        Rules::applyDogMove(
            board_,
            current,
            destination
        );

    if (move.success) {

        current =
            destination;

        pthread_cond_signal(
            &stateChanged_
        );
    }

    pthread_mutex_unlock(
        &boardMutex_
    );

    return move.success;
}

bool Table::attemptPumaMove(
    Board::Node destination
) {

    pthread_mutex_lock(
        &boardMutex_
    );

    if (finished_) {

        pthread_mutex_unlock(
            &boardMutex_
        );

        return false;
    }

    const MoveResult move =
        Rules::applyPumaMove(
            board_,
            pumaNode_,
            destination
        );

    if (move.success) {

        pumaNode_ =
            destination;

        if (move.capture) {

            ++capturedDogs_;

            if (
                move.capturedDogId >= 0 &&
                move.capturedDogId < 12
            ) {
                dogNodes_[
                    static_cast<std::size_t>(
                        move.capturedDogId
                    )
                ] = -1;
            }
        }

        pthread_cond_signal(
            &stateChanged_
        );
    }

    pthread_mutex_unlock(
        &boardMutex_
    );

    return move.success;
}

Piece Table::pieceAt(
    Board::Node node
) const {

    pthread_mutex_lock(
        &boardMutex_
    );

    const Piece piece =
        board_.at(node);

    pthread_mutex_unlock(
        &boardMutex_
    );

    return piece;
}

// --------------------------------------------------
// LOGS
// --------------------------------------------------

void Table::logEvent(
    const std::string& message
) const {

    pthread_mutex_lock(
        &consoleMutex
    );

    std::cout
        << "[Mesa "
        << tableId_
        << "] "
        << message
        << std::endl;

    pthread_mutex_unlock(
        &consoleMutex
    );
}

void Table::printBoardLocked() const {

    pthread_mutex_lock(
        &consoleMutex
    );

    std::cout
        << "[Mesa "
        << tableId_
        << "] Estado del tablero:\n"
        << board_.toString()
        << std::endl;

    pthread_mutex_unlock(
        &consoleMutex
    );
}

// --------------------------------------------------
// ENTRADAS DE THREADS
// --------------------------------------------------

void* Table::pumaThreadEntry(
    void* arg
) {

    Table* table =
        static_cast<Table*>(
            arg
        );

    table->pumaLoop();

    return nullptr;
}

void* Table::dogThreadEntry(
    void* arg
) {

    DogThreadData* data =
        static_cast<DogThreadData*>(
            arg
        );

    data->table->dogLoop(
        data->dogId
    );

    return nullptr;
}

void* Table::judgeThreadEntry(
    void* arg
) {

    Table* table =
        static_cast<Table*>(
            arg
        );

    table->judgeLoop();

    return nullptr;
}

// --------------------------------------------------
// HILO PUMA
// --------------------------------------------------

void Table::pumaLoop() {

    std::mt19937 generator(
        seed_ +
        1000U +
        static_cast<unsigned int>(
            tableId_
        )
    );

    std::uniform_int_distribution<int>
        delayDistribution(
            delayMinMs_,
            delayMaxMs_
        );

    while (!isFinished()) {

        std::this_thread::sleep_for(
            std::chrono::milliseconds(
                delayDistribution(
                    generator
                )
            )
        );

        pthread_mutex_lock(
            &boardMutex_
        );

        if (finished_) {

            pthread_mutex_unlock(
                &boardMutex_
            );

            break;
        }

        const auto moves =
            Rules::validPumaMoves(
                board_,
                pumaNode_
            );

        if (!moves.empty()) {

            std::uniform_int_distribution<
                std::size_t
            > moveDistribution(
                0,
                moves.size() - 1
            );

            const Board::Node destination =
                moves[
                    moveDistribution(
                        generator
                    )
                ];

            const MoveResult move =
                Rules::applyPumaMove(
                    board_,
                    pumaNode_,
                    destination
                );

            if (move.success) {

                const Board::Node origin =
                    pumaNode_;

                pumaNode_ =
                    destination;

                std::ostringstream event;

                event
                    << "PUMA "
                    << board_.nodeName(
                        origin
                    )
                    << " -> "
                    << board_.nodeName(
                        destination
                    );

                if (move.capture) {

                    ++capturedDogs_;

                    if (
                        move.capturedDogId >= 0 &&
                        move.capturedDogId < 12
                    ) {

                        dogNodes_[
                            static_cast<std::size_t>(
                                move.capturedDogId
                            )
                        ] = -1;
                    }

                    event
                        << " | CAPTURA DOG "
                        << move.capturedDogId
                        << " en "
                        << board_.nodeName(
                            move.capturedNode
                        );
                }

                logEvent(
                    event.str()
                );

                printBoardLocked();

                pthread_cond_signal(
                    &stateChanged_
                );
            }
        }

        pthread_mutex_unlock(
            &boardMutex_
        );
    }
}

// --------------------------------------------------
// HILOS PERRO
// --------------------------------------------------

void Table::dogLoop(
    int dogId
) {

    std::mt19937 generator(
        seed_ +
        2000U +
        static_cast<unsigned int>(
            tableId_
        ) * 100U +
        static_cast<unsigned int>(
            dogId
        )
    );

    std::uniform_int_distribution<int>
        delayDistribution(
            delayMinMs_,
            delayMaxMs_
        );

    while (!isFinished()) {

        std::this_thread::sleep_for(
            std::chrono::milliseconds(
                delayDistribution(
                    generator
                )
            )
        );

        pthread_mutex_lock(
            &boardMutex_
        );

        if (finished_) {

            pthread_mutex_unlock(
                &boardMutex_
            );

            break;
        }

        Board::Node& dogNode =
            dogNodes_[
                static_cast<std::size_t>(
                    dogId
                )
            ];

        // Si el Perro fue capturado,
        // su hilo termina.
        if (dogNode < 0) {

            pthread_mutex_unlock(
                &boardMutex_
            );

            break;
        }

        const auto moves =
            Rules::validDogMoves(
                board_,
                dogNode
            );

        if (!moves.empty()) {

            std::uniform_int_distribution<
                std::size_t
            > moveDistribution(
                0,
                moves.size() - 1
            );

            const Board::Node destination =
                moves[
                    moveDistribution(
                        generator
                    )
                ];

            const MoveResult move =
                Rules::applyDogMove(
                    board_,
                    dogNode,
                    destination
                );

            if (move.success) {

                const Board::Node origin =
                    dogNode;

                dogNode =
                    destination;

                std::ostringstream event;

                event
                    << "DOG "
                    << dogId
                    << " "
                    << board_.nodeName(
                        origin
                    )
                    << " -> "
                    << board_.nodeName(
                        destination
                    );

                logEvent(
                    event.str()
                );

                printBoardLocked();

                pthread_cond_signal(
                    &stateChanged_
                );
            }
        }

        pthread_mutex_unlock(
            &boardMutex_
        );
    }
}

// --------------------------------------------------
// JUEZ
// --------------------------------------------------

void Table::judgeLoop() {

    pthread_mutex_lock(
        &boardMutex_
    );

    while (!finished_) {

        if (
            Rules::pumaWins(
                capturedDogs_
            )
        ) {

            logEvent(
                "JUEZ: victoria del Puma"
            );

            finishGame(
                GameResult::PumaWin
            );

            break;
        }

        if (
            Rules::dogsWin(
                board_,
                pumaNode_
            )
        ) {

            logEvent(
                "JUEZ: victoria de los Perros"
            );

            finishGame(
                GameResult::DogsWin
            );

            break;
        }

        pthread_cond_wait(
            &stateChanged_,
            &boardMutex_
        );
    }

    pthread_mutex_unlock(
        &boardMutex_
    );
}

// --------------------------------------------------
// EJECUCION COMPLETA DE LA MESA
// --------------------------------------------------

GameResult Table::run() {

    for (
        int dogId = 0;
        dogId < 12;
        ++dogId
    ) {

        dogThreadData_[
            static_cast<std::size_t>(
                dogId
            )
        ].table = this;

        dogThreadData_[
            static_cast<std::size_t>(
                dogId
            )
        ].dogId = dogId;
    }

    // Crear Juez.
    if (
        pthread_create(
            &judgeThread_,
            nullptr,
            &Table::judgeThreadEntry,
            this
        ) != 0
    ) {

        requestStop();

        return result();
    }

    // Crear Puma.
    if (
        pthread_create(
            &pumaThread_,
            nullptr,
            &Table::pumaThreadEntry,
            this
        ) != 0
    ) {

        requestStop();

        pthread_join(
            judgeThread_,
            nullptr
        );

        return result();
    }

    int dogsCreated = 0;

    // Crear 12 Perros.
    for (
        int dogId = 0;
        dogId < 12;
        ++dogId
    ) {

        if (
            pthread_create(
                &dogThreads_[
                    static_cast<std::size_t>(
                        dogId
                    )
                ],
                nullptr,
                &Table::dogThreadEntry,
                &dogThreadData_[
                    static_cast<std::size_t>(
                        dogId
                    )
                ]
            ) != 0
        ) {

            requestStop();

            break;
        }

        ++dogsCreated;
    }

    // Esperar Puma.
    pthread_join(
        pumaThread_,
        nullptr
    );

    // Esperar Perros.
    for (
        int dogId = 0;
        dogId < dogsCreated;
        ++dogId
    ) {

        pthread_join(
            dogThreads_[
                static_cast<std::size_t>(
                    dogId
                )
            ],
            nullptr
        );
    }

    // Despertar Juez por seguridad.
    pthread_mutex_lock(
        &boardMutex_
    );

    pthread_cond_broadcast(
        &stateChanged_
    );

    pthread_mutex_unlock(
        &boardMutex_
    );

    // Esperar Juez.
    pthread_join(
        judgeThread_,
        nullptr
    );

    return result();
}
