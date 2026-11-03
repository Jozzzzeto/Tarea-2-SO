#pragma once
//guarda mensajes sin que varios hilos los mezclen
#include <pthread.h>

#include <chrono>
#include <cstdio>
#include <string>

class Logger {
public:
    explicit Logger(FILE* out = stdout);
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    //tambien puede guardar los mensajes en un archivo
    bool openFile(const std::string& path);

    //escribe un mensaje con el tiempo transcurrido
    void log(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

    //escribe un tablero completo sin mezclarlo con otros mensajes
    void block(const std::string& text);

private:
    void write(const std::string& text);

    FILE* out_;
    FILE* file_ = nullptr;
    pthread_mutex_t mu_;
    std::chrono::steady_clock::time_point t0_;
};
