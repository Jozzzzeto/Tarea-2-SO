CXX      ?= g++
CXXFLAGS ?= -Wall -Wextra -std=c++17 -pthread -g

SRC_ALL := $(wildcard src/*.cpp)
LIB     := $(filter-out src/main.cpp,$(SRC_ALL))
TESTS   := $(wildcard tests/test_*.cpp)
TEST_BINS := $(patsubst tests/%.cpp,bin/%,$(TESTS))

all: torneo

torneo: $(SRC_ALL)
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $^

bin/%: tests/%.cpp $(LIB)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -Isrc -o $@ $^

# Compila y ejecuta todos los tests (la salida de las partidas se oculta).
test: $(TEST_BINS)
	@for t in $(TEST_BINS); do \
	  printf "%-28s" $$t; \
	  if ./$$t > /dev/null 2>&1; then echo OK; else echo FALLO; ./$$t | tail -15; exit 1; fi; \
	done; echo "todos los tests OK"

# Verificacion de concurrencia / memoria (usar antes de entregar).
tsan: $(SRC_ALL)
	$(CXX) -std=c++17 -pthread -g -fsanitize=thread -Isrc -o torneo_tsan $^

asan: $(SRC_ALL)
	$(CXX) -std=c++17 -pthread -g -fsanitize=address,undefined -Isrc -o torneo_asan $^

clean:
	rm -rf torneo torneo_tsan torneo_asan bin

.PHONY: all test tsan asan clean
