#pragma once
//recibe jugadores, arma parejas y les asigna una mesa
//nunca deja que se ocupen mas de M mesas
#include <pthread.h>
#include <semaphore.h>

#include <list>
#include <deque>
#include <vector>

#include "config.hpp"
#include "logger.hpp"
#include "table.hpp"

struct TournamentStats {
    int games = 0;               // partidas terminadas (incluye abortadas)
    int pumaWins = 0;
    int dogsWins = 0;
    int aborted = 0;             // detenidas por cierre o que no pudieron iniciar
    int unpaired = 0;            // jugadores que quedaron sin pareja
    int neverArrived = 0;        // no llegaron (cierre o fallo al crear su hilo)
    int maxConcurrentTables = 0; // maximo de mesas jugando a la vez
    bool interrupted = false;    // hubo cierre por senal
};

class Matchmaker {
public:
    //ajusta la cantidad de mesas segun el numero de parejas
    Matchmaker(int nPlayers, int maxTables, const Config& cfg, Logger& log);
    ~Matchmaker();
    Matchmaker(const Matchmaker&) = delete;
    Matchmaker& operator=(const Matchmaker&) = delete;

    bool ok() const { return ok_; }

    //espera a que termine todo el torneo
    TournamentStats run();

    //detiene el torneo sin dejar hilos pendientes
    bool shutdown();

private:
    struct Job {
        Matchmaker* mm = nullptr;
        int tableId = 0;
        int pairIndex = 0;
        int pumaPlayer = 0;
        int dogsPlayer = 0;
        pthread_t thread{};
        bool done = false;       // protegido por mu_
    };
    struct ClientArg {
        Matchmaker* mm;
        int id;
    };
    //guarda las mesas que estan jugando
    class ActiveTable;

    static void* clientEntry(void* arg);
    static void* tableEntry(void* arg);

    void clientMain(int id);
    void tableMain(Job* job);
    void startClients();
    void joinClients();
    void mainLoop();
    void reapJobs(bool all);

    int n_;
    int m_;
    Config cfg_;
    Logger& log_;
    bool ok_ = false;

    pthread_mutex_t mu_;
    pthread_cond_t queueCv_;   // el matchmaker espera jugadores
    pthread_cond_t stopCv_;    // los clientes esperan su hora de llegada / el cierre
    sem_t freeTables_;         // cupo de mesas: arranca en M

    // --- protegido por mu_ ---
    bool stopping_ = false;
    std::deque<int> waiting_;           // jugadores esperando pareja
    int accounted_ = 0;                 // jugadores ya llegados (o dados por perdidos)
    std::vector<int> freeIds_;          // ids de mesa libres
    std::vector<Table*> active_;        // mesas jugando ahora
    int running_ = 0;
    TournamentStats stats_;

    // --- solo hilo del matchmaker ---
    std::list<Job> jobs_;
    std::vector<ClientArg> clientArgs_;
    std::vector<pthread_t> clientThreads_;
    std::vector<char> clientStarted_;
};
