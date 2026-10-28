#pragma once
//datos que se pueden configurar para el torneo
#include <string>
#include <vector>

#include "board.hpp"

struct Config {
    int delayMin = 50;                              // ms
    int delayMax = 300;                             // ms
    unsigned int seed = 42;
    Board::Node pumaStart = Board::THRONE_TOP;
    //si esta vacio, las posiciones se generan con la semilla
    std::vector<Board::Node> dogs;
};

//el archivo acepta estas formas de escribir los datos:
//   - una o varias claves por linea, con o sin prefijo "CONFIG"
//   - separador espacio, '=' o ','; mayusculas/minusculas indistintas
//   - '#' inicia un comentario
// Claves: SEED, DELAY_MIN, DELAY_MAX,
//         PUMA_INIT (x y | TRONO_TOP / TRONO_LEFT / TRONO_RIGHT),
//         DOGS (12 nodos "x y x y ..."; si se omite, decide la semilla).
// Coordenadas: x = columna 0..4, y = fila 0..4 con y=4 pegada al trono
// (la misma convencion de Board::gridNode y de los ejemplos del profesor).
// Ejemplos validos:
//   CONFIG SEED 42 DELAY_MIN 50 DELAY_MAX 300 PUMA_INIT 2 2
//   seed=42
//   puma_init=TRONO_TOP
bool loadConfig(const std::string& path, Config& cfg, std::string& err);

//revisa que las posiciones y los tiempos sean validos
bool validateConfig(const Config& cfg, std::string& err);
