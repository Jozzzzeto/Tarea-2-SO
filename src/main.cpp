#include <pthread.h>
#include <signal.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "config.hpp"
#include "layout.hpp"
#include "logger.hpp"
#include "matchmaker.hpp"

namespace {

struct SignalCtx {
    Matchmaker* mm;
    Logger* log;
    sigset_t set;
};

void* signalThread(void* p) {
    SignalCtx* c = static_cast<SignalCtx*>(p);
    for (;;) {
        int sig = 0;
        if (sigwait(&c->set, &sig) != 0) return nullptr;
        if (sig == SIGUSR1) return nullptr;        // fin normal: main nos despierta
        if (c->mm->shutdown())
            c->log->log("senal %d: cierre ordenado (sin jugadores nuevos; se detienen las "
                        "partidas activas)", sig);
        else
            c->log->log("cierre ya en curso, un momento...");
    }
}

bool parseLong(const char* s, long minVal, long maxVal, long& out) {
    char* end = nullptr;
    errno = 0;
    const long v = std::strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || v < minVal || v > maxVal) return false;
    out = v;
    return true;
}

}  

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        std::fprintf(stderr, "Uso: %s N M [config.txt]\n", argv[0]);
        return 1;
    }
    long n = 0, m = 0;
    if (!parseLong(argv[1], 0, 100000, n)) {
        std::fprintf(stderr, "N invalido: '%s' (entero entre 0 y 100000)\n", argv[1]);
        return 1;
    }
    if (!parseLong(argv[2], 1, 100000, m)) {
        std::fprintf(stderr, "M invalido: '%s' (entero entre 1 y 100000)\n", argv[2]);
        return 1;
    }

    Config cfg;
    std::string err;
    if (argc == 4 && !loadConfig(argv[3], cfg, err)) {
        std::fprintf(stderr, "config invalida: %s\n", err.c_str());
        return 1;
    }
    if (!validateConfig(cfg, err)) {
        std::fprintf(stderr, "config invalida: %s\n", err.c_str());
        return 1;
    }

    //preparar las senales antes de crear hilos
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &set, nullptr);

    Logger log;
    log.log("torneo: N=%ld jugadores, M=%ld mesas maximas", n, m);
    log.log("config: seed=%u delay=[%d,%d] ms, perros %s", cfg.seed, cfg.delayMin, cfg.delayMax,
            cfg.dogs.empty() ? "por semilla" : "explicitos");
    if (n % 2 != 0) log.log("aviso: N impar, un jugador quedara sin pareja");

    Matchmaker mm(static_cast<int>(n), static_cast<int>(m), cfg, log);
    if (!mm.ok()) {
        std::fprintf(stderr, "no se pudieron inicializar las primitivas de sincronizacion\n");
        return 1;
    }

    SignalCtx ctx{&mm, &log, set};
    pthread_t sigThread;
    if (pthread_create(&sigThread, nullptr, &signalThread, &ctx) != 0) {
        std::fprintf(stderr, "no se pudo crear el hilo de senales\n");
        return 1;
    }

    const TournamentStats s = mm.run();

    pthread_kill(sigThread, SIGUSR1);   //terminar el hilo de senales
    pthread_join(sigThread, nullptr);

    log.log("resumen: partidas=%d (puma=%d, perros=%d, abortadas=%d), sin pareja=%d, "
            "no llegaron=%d, max mesas simultaneas=%d%s",
            s.games, s.pumaWins, s.dogsWins, s.aborted, s.unpaired, s.neverArrived,
            s.maxConcurrentTables, s.interrupted ? "  [INTERRUMPIDO]" : "");
    return 0;
}
