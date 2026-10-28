#include "matchmaker.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <ctime>
#include <exception>
#include <random>
#include <string>

#include "layout.hpp"

namespace {

timespec deadlineAfterMs(long ms) {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
    return ts;
}

const char* resultName(GameResult r) {
    switch (r) {
        case GameResult::PumaWin: return "gana el PUMA";
        case GameResult::DogsWin: return "ganan los PERROS";
        case GameResult::Aborted: return "ABORTADA";
        default:                  return "sin resultado";
    }
}

}  // namespace

// Mientras vive, la Table esta en active_ y el cierre puede detenerla
// Se construye despues de la table, asi se destruye antes y no queda un puntero colgante en active_.
class Matchmaker::ActiveTable {
public:
    ActiveTable(Matchmaker& mm, Table* t) : mm_(mm), t_(t) {
        pthread_mutex_lock(&mm_.mu_);
        mm_.active_.push_back(t_);
        ++mm_.running_;
        mm_.stats_.maxConcurrentTables =
            std::max(mm_.stats_.maxConcurrentTables, mm_.running_);
        stopRequested = mm_.stopping_;
        pthread_mutex_unlock(&mm_.mu_);
    }
    ~ActiveTable() {
        pthread_mutex_lock(&mm_.mu_);
        mm_.active_.erase(std::remove(mm_.active_.begin(), mm_.active_.end(), t_),
                          mm_.active_.end());
        --mm_.running_;
        pthread_mutex_unlock(&mm_.mu_);
    }
    ActiveTable(const ActiveTable&) = delete;
    ActiveTable& operator=(const ActiveTable&) = delete;

    bool stopRequested = false;

private:
    Matchmaker& mm_;
    Table* t_;
};

Matchmaker::Matchmaker(int nPlayers, int maxTables, const Config& cfg, Logger& log)
    : n_(nPlayers < 0 ? 0 : nPlayers), m_(0), cfg_(cfg), log_(log) {
    if (maxTables < 1) return;
    m_ = std::min(maxTables, std::max(1, n_ / 2));

    pthread_condattr_t at;
    pthread_condattr_init(&at);
    pthread_condattr_setclock(&at, CLOCK_MONOTONIC);   // para timedwait
    bool mutexOk = pthread_mutex_init(&mu_, nullptr) == 0;
    bool qOk = mutexOk && pthread_cond_init(&queueCv_, nullptr) == 0;
    bool sOk = qOk && pthread_cond_init(&stopCv_, &at) == 0;
    pthread_condattr_destroy(&at);
    bool semOk = sOk && sem_init(&freeTables_, 0, static_cast<unsigned int>(m_)) == 0;
    if (!semOk) {
        if (sOk) pthread_cond_destroy(&stopCv_);
        if (qOk) pthread_cond_destroy(&queueCv_);
        if (mutexOk) pthread_mutex_destroy(&mu_);
        return;
    }
    for (int id = m_ - 1; id >= 0; --id) freeIds_.push_back(id);
    ok_ = true;
}

Matchmaker::~Matchmaker() {
    if (!ok_) return;
    sem_destroy(&freeTables_);
    pthread_cond_destroy(&stopCv_);
    pthread_cond_destroy(&queueCv_);
    pthread_mutex_destroy(&mu_);
}

// cada jugador llega tras un retardo aleatorio y se encola.


void* Matchmaker::clientEntry(void* arg) {
    ClientArg* a = static_cast<ClientArg*>(arg);
    a->mm->clientMain(a->id);
    return nullptr;
}

void Matchmaker::clientMain(int id) {
    std::mt19937 rng(cfg_.seed * 7919u + static_cast<unsigned int>(id));
    //elegir cuanto demora este jugador en llegar
    std::uniform_int_distribution<int> arrival(0, cfg_.delayMax);
    const long delay = arrival(rng);
    const timespec deadline = deadlineAfterMs(delay);

    pthread_mutex_lock(&mu_);
    // Espera pasiva hasta su hora de llegada o hasta el cierre.
    while (!stopping_) {
        if (pthread_cond_timedwait(&stopCv_, &mu_, &deadline) == ETIMEDOUT) break;
    }
    if (stopping_) {                       // ya no se aceptan jugadores nuevos
        ++stats_.neverArrived;
        pthread_mutex_unlock(&mu_);
        return;
    }
    waiting_.push_back(id);
    ++accounted_;
    log_.log("jugador %d llega", id);   // dentro del lock: el log sigue el orden real
    pthread_cond_signal(&queueCv_);
    pthread_mutex_unlock(&mu_);
}

void Matchmaker::startClients() {
    clientArgs_.assign(static_cast<std::size_t>(n_), ClientArg{this, 0});
    clientThreads_.assign(static_cast<std::size_t>(n_), pthread_t{});
    clientStarted_.assign(static_cast<std::size_t>(n_), 0);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    //usar una pila pequena para no gastar tanta memoria por jugador
    pthread_attr_setstacksize(&attr, 256 * 1024);

    //crear un hilo para cada jugador
    for (int i = 0; i < n_; ++i) {
        const std::size_t k = static_cast<std::size_t>(i);
        clientArgs_[k] = ClientArg{this, i};
        if (pthread_create(&clientThreads_[k], &attr, &Matchmaker::clientEntry,
                           &clientArgs_[k]) == 0) {
            clientStarted_[k] = 1;
        } else {
            log_.log("jugador %d: no se pudo crear su hilo, se descarta", i);
            pthread_mutex_lock(&mu_);
            ++accounted_;                  // para que el matchmaker no espere por el
            ++stats_.neverArrived;
            pthread_cond_signal(&queueCv_);
            pthread_mutex_unlock(&mu_);
        }
    }
    pthread_attr_destroy(&attr);
}

void Matchmaker::joinClients() {
    for (std::size_t k = 0; k < clientThreads_.size(); ++k)
        if (clientStarted_[k]) pthread_join(clientThreads_[k], nullptr);
}


// Mesas


void* Matchmaker::tableEntry(void* arg) {
    Job* job = static_cast<Job*>(arg);
    job->mm->tableMain(job);
    return nullptr;
}

void Matchmaker::tableMain(Job* job) {
    const unsigned int seed = cfg_.seed + static_cast<unsigned int>(job->pairIndex);
    GameResult res = GameResult::Aborted;
    int captures = 0;

    Board::Node puma = 0;
    std::array<Board::Node, 12> dogs{};
    std::string err;
    if (!buildLayout(cfg_, seed, puma, dogs, err)) {
        log_.log("mesa %d: %s", job->tableId, err.c_str());
    } else {
        try {
            Table table(job->tableId, cfg_.delayMin, cfg_.delayMax, seed, puma, dogs);
            ActiveTable registered(*this, &table);
            if (registered.stopRequested) table.requestStop();   // el cierre ya habia llegado
            res = table.run();
            captures = table.capturedDogs();
        } catch (const std::exception& e) {
            log_.log("mesa %d: error: %s", job->tableId, e.what());
            res = GameResult::Aborted;
        }
    }

    log_.log("mesa %d: termina (%s, capturas=%d) [jugador %d PUMA, jugador %d PERROS]",
             job->tableId, resultName(res), captures, job->pumaPlayer, job->dogsPlayer);

    pthread_mutex_lock(&mu_);
    ++stats_.games;
    if (res == GameResult::PumaWin) ++stats_.pumaWins;
    else if (res == GameResult::DogsWin) ++stats_.dogsWins;
    else ++stats_.aborted;
    freeIds_.push_back(job->tableId);   // primero devolver el id ...
    job->done = true;
    pthread_mutex_unlock(&mu_);
    sem_post(&freeTables_);             // ... y despues liberar el cupo
}

// Une los hilos de mesa terminados (all=false) o todos (all=true).
void Matchmaker::reapJobs(bool all) {
    for (;;) {
        Job* j = nullptr;
        pthread_t th{};
        pthread_mutex_lock(&mu_);
        for (Job& x : jobs_) {
            if (all || x.done) {
                j = &x;
                th = x.thread;
                break;
            }
        }
        pthread_mutex_unlock(&mu_);
        if (!j) break;
        pthread_join(th, nullptr);
        pthread_mutex_lock(&mu_);
        jobs_.remove_if([j](const Job& x) { return &x == j; });
        pthread_mutex_unlock(&mu_);
    }
}


// Bucle del matchmaker: pareja -> cupo de mesa -> abrir mesa

void Matchmaker::mainLoop() {
    int pairIndex = 0;
    for (;;) {
        reapJobs(false);

        //esperar sin gastar CPU hasta tener dos jugadores
        pthread_mutex_lock(&mu_);
        while (!stopping_ && waiting_.size() < 2 && accounted_ < n_)
            pthread_cond_wait(&queueCv_, &mu_);
        if (stopping_ || waiting_.size() < 2) {     // cierre, o ya no llegan mas
            pthread_mutex_unlock(&mu_);
            return;
        }
        const int a = waiting_.front(); waiting_.pop_front();
        const int b = waiting_.front(); waiting_.pop_front();
        pthread_mutex_unlock(&mu_);

        // Roles al azar, reproducibles con la semilla.
        std::mt19937 rng(cfg_.seed * 31u + static_cast<unsigned int>(pairIndex));
        const bool aIsPuma = (rng() & 1u) != 0;
        const int pumaPlayer = aIsPuma ? a : b;
        const int dogsPlayer = aIsPuma ? b : a;
        log_.log("pareja %d: jugador %d (PUMA) vs jugador %d (PERROS)", pairIndex,
                 pumaPlayer, dogsPlayer);

        //esperar una mesa libre sin superar el limite M
        int waitResult = sem_trywait(&freeTables_);
        if (waitResult != 0 && errno == EAGAIN) {
            log_.log("pareja %d: las %d mesas estan ocupadas, espera su turno", pairIndex, m_);
            do {
                waitResult = sem_wait(&freeTables_);
            } while (waitResult != 0 && errno == EINTR);
        }
        if (waitResult != 0) {
            //si falla el semaforo, cerrar sin usar un cupo inexistente
            log_.log("error al esperar una mesa libre");
            pthread_mutex_lock(&mu_);
            stats_.unpaired += 2;
            pthread_mutex_unlock(&mu_);
            shutdown();
            return;
        }

        // 3) Abrir la mesa (si no nos cerraron mientras esperabamos).
        pthread_mutex_lock(&mu_);
        if (stopping_) {
            stats_.unpaired += 2;
            pthread_mutex_unlock(&mu_);
            sem_post(&freeTables_); //devolver el cupo que ya se obtuvo
            log_.log("pareja %d descartada por el cierre", pairIndex);
            return;
        }
        //el cierre puede despertar al semaforo sin liberar una mesa real
        if (freeIds_.empty()) {
            stats_.unpaired += 2;
            pthread_mutex_unlock(&mu_);
            log_.log("pareja %d sin mesa disponible", pairIndex);
            shutdown();
            return;
        }
        const int tableId = freeIds_.back();
        freeIds_.pop_back();
        pthread_mutex_unlock(&mu_);

        jobs_.emplace_back();
        Job& job = jobs_.back();
        job.mm = this;
        job.tableId = tableId;
        job.pairIndex = pairIndex;
        job.pumaPlayer = pumaPlayer;
        job.dogsPlayer = dogsPlayer;
        log_.log("mesa %d: abre (jugador %d PUMA vs jugador %d PERROS)", tableId, pumaPlayer,
                 dogsPlayer);

        if (pthread_create(&job.thread, nullptr, &Matchmaker::tableEntry, &job) != 0) {
            log_.log("mesa %d: no se pudo crear el hilo de la mesa", tableId);
            jobs_.pop_back();
            pthread_mutex_lock(&mu_);
            freeIds_.push_back(tableId);
            ++stats_.games;
            ++stats_.aborted;
            pthread_mutex_unlock(&mu_);
            sem_post(&freeTables_);
        }
        ++pairIndex;
    }
}

TournamentStats Matchmaker::run() {
    TournamentStats empty;
    if (!ok_) return empty;

    startClients();
    mainLoop();
    reapJobs(true);       // esperar a que terminen (o se detengan) las mesas
    joinClients();        // todos los clientes ya terminaron o fueron despertados

    pthread_mutex_lock(&mu_);
    stats_.interrupted = stopping_;
    stats_.unpaired += static_cast<int>(waiting_.size());
    const std::deque<int> leftover = waiting_;
    waiting_.clear();
    const TournamentStats result = stats_;
    pthread_mutex_unlock(&mu_);

    for (int id : leftover) log_.log("jugador %d se queda sin pareja", id);
    return result;
}


// Cierre ordenado 


bool Matchmaker::shutdown() {
    if (!ok_) return false;
    pthread_mutex_lock(&mu_);
    if (stopping_) {
        pthread_mutex_unlock(&mu_);
        return false;
    }
    stopping_ = true;                              //no aceptar jugadores nuevos
    for (Table* t : active_) t->requestStop();     //pedir que terminen las partidas
    pthread_cond_broadcast(&queueCv_);             //despertar al emparejador
    pthread_cond_broadcast(&stopCv_);              //despertar a los jugadores
    pthread_mutex_unlock(&mu_);
    sem_post(&freeTables_);                        //despertar si esperaba una mesa
    return true;
}
