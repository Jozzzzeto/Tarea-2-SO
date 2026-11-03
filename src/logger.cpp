#include "logger.hpp"

#include <cstdarg>

Logger::Logger(FILE* out) : out_(out), t0_(std::chrono::steady_clock::now()) {
    pthread_mutex_init(&mu_, nullptr);
}

Logger::~Logger() {
    if (file_) std::fclose(file_);
    pthread_mutex_destroy(&mu_);
}

//abrir el archivo donde se guardaran los mensajes
bool Logger::openFile(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) return false;
    pthread_mutex_lock(&mu_);
    if (file_) std::fclose(file_);
    file_ = f;
    pthread_mutex_unlock(&mu_);
    return true;
}

//escribir mientras los otros hilos esperan su turno
void Logger::write(const std::string& text) {
    pthread_mutex_lock(&mu_);
    if (out_) {
        std::fputs(text.c_str(), out_);
        std::fflush(out_);
    }
    if (file_) {
        std::fputs(text.c_str(), file_);
        std::fflush(file_);
    }
    pthread_mutex_unlock(&mu_);
}

//poner el tiempo y escribir el mensaje
void Logger::log(const char* fmt, ...) {
    char msg[512];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - t0_).count();
    char head[32];
    std::snprintf(head, sizeof head, "[%6lld ms] ", static_cast<long long>(ms));
    write(std::string(head) + msg + "\n");
}

void Logger::block(const std::string& text) { write(text); }
