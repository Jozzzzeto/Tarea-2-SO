#include "layout.hpp"

#include <random>
#include <utility>
#include <vector>

#include "rules.hpp"

//sortear las posiciones iniciales
bool generateLayout(unsigned int seed,
                    Board::Node pumaNode,
                    std::array<Board::Node, 12>& dogsOut) {
    if (!Board::validNode(pumaNode)) return false;

    //usar la semilla para repetir el mismo sorteo
    std::mt19937 rng(seed);
    std::vector<Board::Node> pool;
    for (Board::Node n = 0; n < Board::GRID_NODES; ++n)
        if (n != pumaNode) pool.push_back(n);

    Board board;
    //intentar varias veces hasta conseguir una partida valida
    for (int attempt = 0; attempt < 1000; ++attempt) {
        //mezclar las posiciones para elegir los 12 perros
        for (std::size_t i = 0; i < 12; ++i) {
            const std::size_t j = i + static_cast<std::size_t>(rng() % (pool.size() - i));
            std::swap(pool[i], pool[j]);
        }
        board.clear();
        board.place(pumaNode, Piece{PieceType::Puma, 0});
        for (int d = 0; d < 12; ++d) {
            dogsOut[static_cast<std::size_t>(d)] = pool[static_cast<std::size_t>(d)];
            board.place(pool[static_cast<std::size_t>(d)], Piece{PieceType::Dog, d});
        }
        if (!Rules::dogsWin(board, pumaNode)) return true;  // partida no terminada de entrada
    }
    return false;
}

//preparar el tablero de una partida
bool buildLayout(const Config& cfg,
                 unsigned int seed,
                 Board::Node& pumaOut,
                 std::array<Board::Node, 12>& dogsOut,
                 std::string& err) {
    pumaOut = cfg.pumaStart;
    if (cfg.dogs.size() == 12) {
        //usar las posiciones del archivo cuando vienen indicadas
        Board board;
        board.place(pumaOut, Piece{PieceType::Puma, 0});
        for (std::size_t i = 0; i < 12; ++i) {
            dogsOut[i] = cfg.dogs[i];
            board.place(dogsOut[i], Piece{PieceType::Dog, static_cast<int>(i)});
        }
        //evitar que la partida empiece con el puma encerrado
        if (Rules::dogsWin(board, pumaOut)) {
            err = "la disposicion inicial ya es victoria de los perros";
            return false;
        }
        return true;
    }
    if (!cfg.dogs.empty()) {
        err = "dogs: deben ser 12 (o ninguno)";
        return false;
    }
    if (!generateLayout(seed, cfg.pumaStart, dogsOut)) {
        err = "no se pudo generar la disposicion inicial desde la semilla";
        return false;
    }
    return true;
}
