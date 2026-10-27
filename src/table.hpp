#pragma once

#include <string>

#include "board.hpp"

#include <array>
#include <pthread.h>

enum class GameResult {
    Running,
    PumaWin,
    DogsWin,
    Aborted
};

enum class Side {
    Puma,
    Dogs
};

class Table {
public:
    // Constructor normal.
    Table(
        int tableId,
        int delayMinMs,
        int delayMaxMs,
        unsigned int seed
    );

    // Constructor con posiciones iniciales.
    // Nos sirve para tests y luego para Config.
    Table(
        int tableId,
        int delayMinMs,
        int delayMaxMs,
        unsigned int seed,
        Board::Node pumaStart,
        const std::array<Board::Node, 12>& dogStarts
    );

    ~Table();

    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;

    GameResult run();

    void requestStop();

    void disconnect(Side side);

// Permiten ejecutar movimientos controlados.
// Son utiles para pruebas y escenarios.
bool attemptDogMove(
    int dogId,
    Board::Node destination
);

bool attemptPumaMove(
    Board::Node destination
);

Piece pieceAt(
    Board::Node node
) const;

    GameResult result() const;

    int 

capturedDogs() const;

private:

void logEvent(
    const std::string& message
) const;

void printBoardLocked() const;

    struct DogThreadData {
        Table* table;
        int dogId;
    };

    int tableId_;

    int delayMinMs_;
    int delayMaxMs_;

    unsigned int seed_;

    Board board_;

    Board::Node initialPumaNode_;

    std::array<
        Board::Node,
        12
    > initialDogNodes_;

    Board::Node pumaNode_;

    std::array<
        Board::Node,
        12
    > dogNodes_;

    int capturedDogs_;

    bool finished_;

    GameResult result_;

    mutable pthread_mutex_t boardMutex_;

    pthread_cond_t stateChanged_;

    pthread_t pumaThread_;

    pthread_t judgeThread_;

    std::array<
        pthread_t,
        12
    > dogThreads_;

    std::array<
        DogThreadData,
        12
    > dogThreadData_;

    static void* pumaThreadEntry(
        void* arg
    );

    static void* dogThreadEntry(
        void* arg
    );

    static void* judgeThreadEntry(
        void* arg
    );

    void pumaLoop();

    void dogLoop(
        int dogId
    );

    void judgeLoop();

    void initializeBoard();

    bool isFinished();

    void finishGame(
        GameResult result
    );
};
