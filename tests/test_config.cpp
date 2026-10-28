// Config (formatos de lectura y validacion) y disposicion inicial por semilla.
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>

#include "board.hpp"
#include "config.hpp"
#include "layout.hpp"
#include "rules.hpp"

static int failures = 0;
#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("FALLO linea %d: %s\n", __LINE__, #cond);        \
            ++failures;                                                  \
        }                                                                \
    } while (0)

static bool loadText(const char* text, Config& cfg, std::string& err) {
    char path[] = "/tmp/cfgXXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) return false;
    close(fd);
    { std::ofstream(path) << text; }
    bool ok = loadConfig(path, cfg, err);
    unlink(path);
    return ok;
}

int main() {
    const auto N = [](int x, int y) { return Board::gridNode(x, y); };
    std::string err;

    // --- Formato del ejemplo del profesor ---
    Config a;
    CHECK(loadText("CONFIG SEED 42 DELAY_MIN 50 DELAY_MAX 300 PUMA_INIT 2 2\n", a, err));
    CHECK(a.seed == 42 && a.delayMin == 50 && a.delayMax == 300 && a.pumaStart == N(2, 2));

    // --- Formato clave=valor, comentarios y nombres del trono ---
    Config b;
    CHECK(loadText("seed=7\ndelay_min=1\ndelay_max=1  # sin espera\npuma_init=TRONO_TOP\n", b, err));
    CHECK(b.seed == 7 && b.delayMin == 1 && b.delayMax == 1 && b.pumaStart == Board::THRONE_TOP);
    Config b2;
    CHECK(loadText("PUMA_INIT TRONO_LEFT\n", b2, err) && b2.pumaStart == Board::THRONE_LEFT);

    // --- Errores ---
    Config e;
    CHECK(!loadText("SEED 1 DELAY_MIN 300 DELAY_MAX 50\n", e, err));   // min > max
    CHECK(!loadText("COLOR rojo\n", e, err));                          // clave desconocida
    CHECK(!loadText("PUMA_INIT 9 9\n", e, err));                       // fuera del tablero
    CHECK(!loadText("SEED abc\n", e, err));                            // no numerico
    CHECK(!loadConfig("/no/existe.txt", e, err));

    // --- Perros explicitos ---
    Config f;
    CHECK(loadText("DOGS 0 1  1 1  2 1  3 1  4 1  0 2  1 2  2 2  3 2  4 2  0 3  1 3\n", f, err));
    CHECK(f.dogs.size() == 12 && f.dogs[0] == N(0, 1));
    Config g;
    CHECK(!loadText("DOGS 0 1  1 1\n", g, err));                       // ni 0 ni 12
    Config h;
    CHECK(!loadText("PUMA_INIT 0 1\nDOGS 0 1  1 1  2 1  3 1  4 1  0 2  1 2  2 2  3 2  4 2  0 3  1 3\n", h, err));  // puma pisa un perro

    // --- Disposicion por semilla ---
    std::array<Board::Node, 12> d1{}, d2{}, d3{};
    CHECK(generateLayout(42, N(2, 2), d1) && generateLayout(42, N(2, 2), d2));
    CHECK(d1 == d2);                                       // reproducible
    CHECK(generateLayout(43, N(2, 2), d3) && d3 != d1);    // otra semilla, otra disposicion

    bool allOk = true;
    Board bd;
    for (unsigned s = 0; s < 500 && allOk; ++s) {
        for (Board::Node puma : {0, 2, 12, 20, 24, Board::THRONE_TOP}) {
            std::array<Board::Node, 12> d{};
            if (!generateLayout(s, puma, d)) { allOk = false; break; }
            bd.clear();
            bd.place(puma, Piece{PieceType::Puma, 0});
            for (int i = 0; i < 12; ++i)
                if (!bd.place(d[static_cast<std::size_t>(i)], Piece{PieceType::Dog, i}))
                    allOk = false;                            // 12 distintos y sin pisar al puma
            for (Board::Node n : d) if (!Board::isGridNode(n)) allOk = false;
            if (Rules::dogsWin(bd, puma)) allOk = false;      // no terminada de entrada
        }
    }
    CHECK(allOk);

    // buildLayout: explicito manda sobre la semilla
    Board::Node puma = 0;
    std::array<Board::Node, 12> dd{};
    CHECK(buildLayout(f, 99, puma, dd, err) && dd[0] == N(0, 1) && puma == f.pumaStart);
    Config empty;
    CHECK(buildLayout(empty, 99, puma, dd, err) && puma == Board::THRONE_TOP);

    // --- Logger basico ---
    std::puts(failures == 0 ? "test_config OK" : "test_config: HAY FALLOS");
    return failures == 0 ? 0 : 1;
}
