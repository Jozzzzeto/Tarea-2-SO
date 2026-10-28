#include "config.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <climits>

namespace {

// Quita comentarios, pasa a mayusculas y trata '=' y ',' como espacios.
std::vector<std::string> tokenize(std::string line) {
    const std::size_t hash = line.find('#');
    if (hash != std::string::npos) line.erase(hash);
    for (char& ch : line) {
        if (ch == '=' || ch == ',') ch = ' ';
        else ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    std::vector<std::string> t;
    std::size_t i = 0;
    while (i < line.size()) {
        if (std::isspace(static_cast<unsigned char>(line[i]))) { ++i; continue; }
        std::size_t j = i;
        while (j < line.size() && !std::isspace(static_cast<unsigned char>(line[j]))) ++j;
        t.push_back(line.substr(i, j - i));
        i = j;
    }
    return t;
}

struct Cursor {
    const std::vector<std::string>& t;
    std::size_t i = 0;
    explicit Cursor(const std::vector<std::string>& tok) : t(tok) {}

    bool done() const { return i >= t.size(); }
    bool word(std::string& out) {
        if (done()) return false;
        out = t[i++];
        return true;
    }
    bool integer(int& out) {
        if (done()) return false;
        char* end = nullptr;
        errno = 0;
        const long v = std::strtol(t[i].c_str(), &end, 10);
        if (errno != 0 || end == t[i].c_str() || *end != '\0' ||
            v < INT_MIN || v > INT_MAX) return false;
        out = static_cast<int>(v);
        ++i;
        return true;
    }
    // Nodo: "TRONO_*" o dos enteros "x y". Si falla, no consume nada.
    bool node(Board::Node& out) {
        if (done()) return false;
        if (t[i] == "TRONO_TOP")   { out = Board::THRONE_TOP;   ++i; return true; }
        if (t[i] == "TRONO_LEFT")  { out = Board::THRONE_LEFT;  ++i; return true; }
        if (t[i] == "TRONO_RIGHT") { out = Board::THRONE_RIGHT; ++i; return true; }
        const std::size_t save = i;
        int x = 0, y = 0;
        if (!integer(x) || !integer(y) || !Board::validGridCoord(x, y)) {
            i = save;
            return false;
        }
        out = Board::gridNode(x, y);
        return true;
    }
};

}  // namespace

bool loadConfig(const std::string& path, Config& cfg, std::string& err) {
    std::ifstream in(path);
    if (!in) {
        err = "no se pudo abrir " + path;
        return false;
    }
    std::string line;
    int lineNo = 0;
    auto fail = [&](const std::string& msg) {
        err = "linea " + std::to_string(lineNo) + ": " + msg;
        return false;
    };

    while (std::getline(in, line)) {
        ++lineNo;
        const auto tok = tokenize(line);
        Cursor c(tok);
        std::string key;
        while (c.word(key)) {
            int v = 0;
            if (key == "CONFIG") {
                continue;  // prefijo opcional
            } else if (key == "SEED" || key == "DELAY_MIN" || key == "DELAY_MAX") {
                if (!c.integer(v)) return fail("valor invalido para " + key);
                if (key == "SEED") {
                    //la semilla no puede ser negativa
                    if (v < 0) return fail("SEED debe ser mayor o igual a 0");
                    cfg.seed = static_cast<unsigned int>(v);
                }
                else if (key == "DELAY_MIN") cfg.delayMin = v;
                else cfg.delayMax = v;
            } else if (key == "PUMA_INIT" || key == "PUMA") {
                if (!c.node(cfg.pumaStart)) return fail("PUMA_INIT invalido (x y o TRONO_*)");
            } else if (key == "DOGS") {
                //leer las posiciones de los 12 perros
                std::vector<Board::Node> dogs;
                Board::Node n = 0;
                while (c.node(n)) dogs.push_back(n);
                cfg.dogs = dogs;
            } else {
                return fail("clave desconocida '" + key + "'");
            }
        }
    }
    //revisar que toda la configuracion sea valida
    return validateConfig(cfg, err);
}

bool validateConfig(const Config& cfg, std::string& err) {
    //revisar que los retardos tengan sentido
    if (cfg.delayMin < 0 || cfg.delayMax < cfg.delayMin) {
        err = "delay_min/delay_max invalidos";
        return false;
    }
    if (!Board::validNode(cfg.pumaStart)) {
        err = "posicion del puma fuera de rango";
        return false;
    }
    if (!cfg.dogs.empty() && cfg.dogs.size() != 12) {
        err = "dogs: deben ser exactamente 12 (o ninguno para usar la semilla)";
        return false;
    }
    std::vector<Board::Node> all = cfg.dogs;
    all.push_back(cfg.pumaStart);
    for (Board::Node n : all) {
        if (!Board::validNode(n)) {
            err = "posicion fuera de rango: " + std::to_string(n);
            return false;
        }
    }
    //ninguna ficha puede empezar en la misma posicion
    std::sort(all.begin(), all.end());
    if (std::adjacent_find(all.begin(), all.end()) != all.end()) {
        err = "posiciones repetidas entre puma y perros";
        return false;
    }
    return true;
}
