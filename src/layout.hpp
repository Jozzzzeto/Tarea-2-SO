#pragma once
//elige donde comienzan el puma y los perros
#include <array>
#include <string>

#include "board.hpp"
#include "config.hpp"

//sortea 12 posiciones distintas para los perros
//repite el sorteo si el puma queda atrapado
bool generateLayout(unsigned int seed,
                    Board::Node pumaNode,
                    std::array<Board::Node, 12>& dogsOut);

//usa las posiciones del archivo o genera nuevas
bool buildLayout(const Config& cfg,
                 unsigned int seed,
                 Board::Node& pumaOut,
                 std::array<Board::Node, 12>& dogsOut,
                 std::string& err);
