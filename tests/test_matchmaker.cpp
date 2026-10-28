// Matchmaker con Tables REALES (retardos chicos para que sea rapido).
#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>

#include "config.hpp"
#include "logger.hpp"
#include "matchmaker.hpp"

static int failures = 0;
#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FALLO linea %d: %s\n", __LINE__, #cond);        \
            ++failures;                                                  \
        }                                                                \
    } while (0)

static FILE* nul = nullptr;

static Config makeCfg(int dmin, int dmax, unsigned seed = 7) {
    Config c;
    c.delayMin = dmin;
    c.delayMax = dmax;
    c.seed = seed;
    return c;
}

static TournamentStats runTournament(int n, int m, int dmin, int dmax) {
    Logger log(nul);
    Matchmaker mm(n, m, makeCfg(dmin, dmax), log);
    CHECK(mm.ok());
    return mm.run();
}

int main() {
    nul = std::fopen("/dev/null", "w");
    std::cout.setstate(std::ios::failbit);   // silencia el log de las Tables

    // N par, mas parejas que mesas: todos juegan, nunca mas de M mesas a la vez.
    {
        TournamentStats s = runTournament(20, 2, 1, 3);
        CHECK(s.games == 10 && s.aborted == 0 && s.unpaired == 0 && s.neverArrived == 0);
        CHECK(s.pumaWins + s.dogsWins == 10);
        CHECK(s.maxConcurrentTables >= 1 && s.maxConcurrentTables <= 2);
        CHECK(!s.interrupted);
    }
    // Con partidas mas largas las 2 mesas se llenan de verdad (saturacion).
    {
        TournamentStats s = runTournament(8, 2, 20, 40);
        CHECK(s.games == 4 && s.maxConcurrentTables == 2);
    }
    // M = 1: partidas estrictamente una tras otra.
    {
        TournamentStats s = runTournament(6, 1, 1, 3);
        CHECK(s.games == 3 && s.maxConcurrentTables == 1);
    }
    // N impar: un jugador sin pareja.
    {
        TournamentStats s = runTournament(7, 3, 1, 3);
        CHECK(s.games == 3 && s.unpaired == 1);
    }
    // Casos borde.
    {
        TournamentStats s0 = runTournament(0, 2, 1, 3);
        CHECK(s0.games == 0 && s0.unpaired == 0);
        TournamentStats s1 = runTournament(1, 2, 1, 3);
        CHECK(s1.games == 0 && s1.unpaired == 1);
        TournamentStats s2 = runTournament(4, 50, 1, 3);      // M mucho mayor que N/2
        CHECK(s2.games == 2 && s2.maxConcurrentTables <= 2);
    }
    // M invalido.
    {
        Logger log(nul);
        Matchmaker bad(4, 0, makeCfg(1, 3), log);
        CHECK(!bad.ok());
    }

    // Cierre en pleno torneo: las mesas activas se abortan, no entran mas parejas.
    {
        Logger log(nul);
        Matchmaker mm(40, 2, makeCfg(50, 100), log);
        std::thread killer([&mm] {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            mm.shutdown();
        });
        const auto t0 = std::chrono::steady_clock::now();
        TournamentStats s = mm.run();
        const long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - t0).count();
        killer.join();
        CHECK(s.interrupted);
        CHECK(s.aborted >= 1 && s.games < 20);
        CHECK(ms < 2000);                       // cierre rapido, sin esperar el torneo
        CHECK(!mm.shutdown());                  // idempotente
    }
    // Cierre ANTES de empezar: nadie llega, nada juega.
    {
        Logger log(nul);
        Matchmaker mm(10, 2, makeCfg(1, 3), log);
        CHECK(mm.shutdown());
        TournamentStats s = mm.run();
        CHECK(s.interrupted && s.games == 0 && s.neverArrived == 10);
    }

    std::puts(failures == 0 ? "test_matchmaker OK" : "test_matchmaker: HAY FALLOS");
    std::fclose(nul);
    return failures == 0 ? 0 : 1;
}
