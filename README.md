# Torneo de Komikan

**Tarea 2 · Sistemas Operativos**  
**C++17 · POSIX Threads · Sincronización y concurrencia**

Simulador concurrente de un torneo de **Komikan**. Los jugadores llegan en distintos momentos, se emparejan y disputan partidas automatizadas en un conjunto limitado de mesas. El programa coordina los recursos compartidos, registra los eventos y permite finalizar ordenadamente ante una interrupción.

## Contenido

1. [Descripción y funcionalidades](#1-descripción-y-funcionalidades)
2. [Requisitos](#2-requisitos)
3. [Compilación](#3-compilación)
4. [Configuración](#4-configuración)
5. [Ejecución](#5-ejecución)
6. [Estructura del proyecto](#6-estructura-del-proyecto)
7. [Arquitectura y funcionamiento](#7-arquitectura-y-funcionamiento)
8. [Gestión de concurrencia](#8-gestión-de-concurrencia)
9. [Manejo de errores y recursos](#9-manejo-de-errores-y-recursos)
10. [Pruebas y validación](#10-pruebas-y-validación)
11. [Limitaciones](#11-limitaciones)

## 1. Descripción y funcionalidades

El proyecto implementa una simulación concurrente de un torneo del juego de estrategia **Komikan**, utilizando C++ y la API POSIX Threads (`pthread`). La cantidad máxima de mesas simultáneas se indica al ejecutar el programa.

El simulador permite:

- Simular la llegada de jugadores con retardos configurables.
- Formar parejas y asignar los roles de **Puma** y **Perros**.
- Administrar un número limitado de mesas de juego.
- Ejecutar partidas automatizadas concurrentemente.
- Aplicar las reglas del Komikan y determinar el resultado de cada partida.
- Registrar eventos, movimientos y estadísticas del torneo.
- Gestionar interrupciones y realizar un cierre controlado.

## 2. Requisitos

- **Sistema operativo:** compatible con POSIX; se recomienda Linux.
- **Compilador:** `g++` con soporte para **C++17**.
- **Bibliotecas:** POSIX Threads y semáforos POSIX.
- **Herramienta recomendada:** `make`.

## 3. Compilación

Desde la raíz del repositorio, ejecutar:

```bash
make
```

Se generará el ejecutable `torneo`.

**Compilación manual**, sin `make`:

```bash
g++ -Wall -Wextra -std=c++17 -pthread -g -Isrc -o torneo src/*.cpp
```

Otros comandos disponibles:

| Comando | Función |
|---|---|
| `make -B` | Fuerza la recompilación del ejecutable. |
| `make test` | Compila y ejecuta las pruebas automatizadas. |
| `make asan` | Genera `torneo_asan` con AddressSanitizer y UndefinedBehaviorSanitizer. |
| `make tsan` | Genera `torneo_tsan` con ThreadSanitizer. |
| `make clean` | Elimina ejecutables y binarios de pruebas generados. |

## 4. Configuración

El archivo `config.txt` define los parámetros de la simulación. El analizador acepta comentarios iniciados con `#` y admite `=` o espacios para separar claves y valores.

### 4.1. Parámetros

| Parámetro | Descripción |
|---|---|
| `SEED` | Semilla entera no negativa del generador pseudoaleatorio. |
| `DELAY_MIN` | Retardo mínimo de llegada de los jugadores, en milisegundos. |
| `DELAY_MAX` | Retardo máximo de llegada de los jugadores, en milisegundos. Debe ser mayor o igual que `DELAY_MIN`. |
| `PUMA_INIT` | Posición inicial del Puma. También se admite la clave `PUMA`. |
| `DOGS` | Posiciones iniciales explícitas de los **12 perros**. Si se omite, se utiliza la distribución pseudoaleatoria prevista por el programa. |

> **Importante:** `DELAY_MIN` y `DELAY_MAX` controlan la **llegada de jugadores**, no el tiempo entre movimientos de una partida. `SEED` no es una coordenada: determina la secuencia pseudoaleatoria utilizada por la simulación.

### 4.2. Ejemplo con posiciones automáticas

```text
CONFIG SEED 42 DELAY_MIN 50 DELAY_MAX 300 PUMA_INIT TRONO_TOP
```

`CONFIG` es un prefijo opcional. Al no especificar `DOGS`, el programa determina automáticamente las posiciones iniciales de los perros.

### 4.3. Posiciones manuales de los perros

Para establecer posiciones concretas, agregar la clave `DOGS` seguida de **12 nodos**. Cada nodo puede expresarse mediante un par de coordenadas `x y` o mediante uno de los identificadores especiales:

- `TRONO_TOP`
- `TRONO_LEFT`
- `TRONO_RIGHT`

Ejemplo de **sintaxis** (las posiciones deben elegirse de acuerdo con el tablero y sin ocupar el nodo del Puma):

```text
CONFIG SEED 42 DELAY_MIN 50 DELAY_MAX 300 PUMA_INIT TRONO_TOP
DOGS 0 0  1 0  2 0  3 0  4 0  0 1  1 1  2 1  3 1  4 1  0 2  1 2
```

La configuración se rechaza si contiene coordenadas inválidas, posiciones repetidas, superposición con el Puma o una cantidad de perros distinta de 12. Si se utiliza `DOGS`, deben declararse las doce posiciones.

## 5. Ejecución

La sintaxis es:

```bash
./torneo N M config.txt
```

| Argumento | Significado |
|---|---|
| `N` | Cantidad de jugadores del torneo. |
| `M` | Número máximo de mesas simultáneas. |
| `config.txt` | Ruta del archivo de configuración. |

Ejemplo con **10 jugadores** y **2 mesas**:

```bash
./torneo 10 2 config.txt
```

Para conservar la salida estándar y los mensajes de error en un archivo:

```bash
./torneo 50 5 config.txt > prueba.log 2>&1
```

La ejecución informa los eventos de llegada, emparejamiento, desarrollo de partidas y un resumen estadístico final.

## 6. Estructura del proyecto

```text
Tarea-2-SO/
├── Makefile
├── config.txt
├── src/
│   ├── main.cpp
│   ├── matchmaker.cpp / matchmaker.hpp
│   ├── table.cpp
│   ├── board.cpp
│   ├── rules.cpp
│   ├── config.cpp / config.hpp
│   ├── logger.cpp
│   └── layout.cpp
└── tests/
    └── test_*.cpp
```

Los archivos `.hpp` declaran las interfaces y estructuras correspondientes a los módulos que los utilizan.

| Módulo | Responsabilidad principal |
|---|---|
| `main.cpp` | Procesamiento de argumentos, inicio de la simulación y coordinación del cierre. |
| `matchmaker.cpp` | Llegadas, cola de emparejamiento, asignación de mesas y coordinación del torneo. |
| `table.cpp` | Desarrollo y control de las partidas. |
| `board.cpp` | Representación y operaciones sobre el tablero. |
| `rules.cpp` | Reglas, movimientos válidos y condiciones de victoria. |
| `config.cpp` | Lectura y validación de la configuración. |
| `logger.cpp` | Registro de eventos y resultados. |
| `layout.cpp` | Presentación del tablero. |
| `tests/` | Pruebas automatizadas de componentes y escenarios del sistema. |

## 7. Arquitectura y funcionamiento

El sistema se organiza alrededor de **`Matchmaker`**, responsable de coordinar los jugadores y la disponibilidad de mesas. La lógica del juego se mantiene separada de la gestión general del torneo.

### 7.1. Llegada de jugadores

Los jugadores esperan un retardo de llegada determinado por la configuración. Después de ese intervalo, su llegada se registra y pueden incorporarse al proceso de emparejamiento.

### 7.2. Emparejamiento

`Matchmaker` administra a los jugadores disponibles y forma parejas. En cada partida, uno de los participantes asume el rol de **Puma** y el otro el de **Perros**.

### 7.3. Asignación de mesas

Una pareja inicia su partida cuando existe una mesa disponible. Si todas están ocupadas, debe esperar a que alguna se libere. De esta manera, el sistema mantiene el límite de partidas simultáneas indicado mediante `M`.

### 7.4. Desarrollo de partidas

Cada partida administra su estado del tablero y ejecuta movimientos automatizados según las reglas del Komikan. Las condiciones de victoria son:

- **Puma:** captura seis perros.
- **Perros:** el Puma queda sin movimientos legales.

### 7.5. Finalización

Cuando termina una partida, se registra su resultado y se libera la mesa. Al concluir el torneo, se informa un resumen con partidas completadas, victorias por rol, partidas abortadas, jugadores sin pareja y máximo de mesas simultáneas utilizadas.

## 8. Gestión de concurrencia

La coordinación se implementa mediante mecanismos POSIX que cumplen funciones complementarias.

| Mecanismo | Uso en el proyecto |
|---|---|
| **Hilos (`pthread`)** | Permiten gestionar actividades concurrentes, incluidas llegadas de jugadores y partidas. |
| **Mutex (`pthread_mutex_t`)** | Protegen el estado compartido del coordinador, evitando modificaciones simultáneas no sincronizadas. |
| **Variables de condición (`pthread_cond_t`)** | Permiten esperar cambios en el estado de la cola y coordinar la detención sin realizar consultas continuas. |
| **Semáforos (`sem_t`)** | Restringen la cantidad de mesas que pueden estar ocupadas simultáneamente. |
| **Señales POSIX** | Permiten solicitar una interrupción y activar el procedimiento de cierre. |

### 8.1. Coordinación y exclusión mutua

`Matchmaker` mantiene información compartida relacionada con el emparejamiento y la disponibilidad de recursos. El uso de mutex permite serializar las operaciones críticas sobre ese estado. Las variables de condición permiten que los hilos esperen eventos relevantes en lugar de consumir CPU mediante espera activa.

### 8.2. Límite de mesas

El semáforo de mesas representa los recursos de juego disponibles. La adquisición de una mesa permite iniciar una partida; su liberación al finalizar posibilita que otra pareja continúe. Así se respeta el máximo de concurrencia configurado.

### 8.3. Esperas temporizadas

Una de las variables de condición de `Matchmaker` se configura con **`CLOCK_MONOTONIC`**. Este reloj resulta apropiado para medir intervalos de espera porque no depende de los cambios en la hora del sistema. Durante su inicialización se comprueban los resultados de `pthread_condattr_init()` y `pthread_condattr_setclock()`.

### 8.4. Interrupciones

El programa contempla señales como `SIGINT`. Ante una solicitud de interrupción, se inicia un cierre controlado que permite detener la ejecución, abortar las partidas activas cuando corresponde y registrar el estado final del torneo.

## 9. Manejo de errores y recursos

La implementación incluye validaciones de argumentos y configuración, además de comprobaciones en la inicialización de mecanismos de sincronización.

En particular, el constructor de `Matchmaker` comprueba la creación y configuración de atributos de condición antes de continuar. También se contempla la liberación de recursos inicializados durante el ciclo de vida del programa.

Estas comprobaciones mejoran la robustez, aunque no implican que toda operación del sistema esté exenta de posibles errores.

## 10. Pruebas y validación

### 10.1. Pruebas automatizadas

```bash
make test
```

Para recompilar antes de ejecutar:

```bash
make -B && make test
```

En las ejecuciones de validación realizadas se superaron **9 pruebas automatizadas**.

### 10.2. Escenarios de concurrencia

| Jugadores | Mesas máximas | Partidas completadas | Resultado observado |
|---:|---:|---:|---|
| 10 | 2 | 5 | Ejecución completa. |
| 50 | 5 | 25 | Sin partidas abortadas ni jugadores pendientes. |
| 1.000 | 10 | 500 | Sin partidas abortadas ni jugadores pendientes. |

En estos escenarios se respetó el máximo de mesas simultáneas configurado.

### 10.3. Interrupción mediante `SIGINT`

Se ejecutó una simulación de **100 jugadores y 5 mesas**, interrumpida mientras había partidas activas. El programa registró la señal, abortó las cinco partidas en curso y generó el resumen final sin bloqueos observados.

Un ejemplo para repetir la prueba es:

```bash
./torneo 100 5 config.txt > prueba_sigint.log 2>&1 &
PID=$!
sleep 2
kill -INT "$PID"
wait "$PID"
tail -n 15 prueba_sigint.log
```

### 10.4. Análisis de memoria y concurrencia

El `Makefile` incluye objetivos para generar versiones instrumentadas:

```bash
make asan
./torneo_asan 10 2 config.txt

make tsan
./torneo_tsan 10 2 config.txt
```

- **AddressSanitizer (ASan)** y **UndefinedBehaviorSanitizer (UBSan):** ayudan a detectar errores de memoria y ciertos comportamientos indefinidos.
- **ThreadSanitizer (TSan):** ayuda a identificar condiciones de carrera.

Las ejecuciones de validación analizadas no reportaron errores de estas herramientas. La ejecución de TSan puede depender de las características del entorno Linux y de la disposición de memoria del proceso.

> Las pruebas documentan el comportamiento observado en escenarios concretos; no demuestran la ausencia de errores para todas las entradas o planificaciones de hilos posibles.

## 11. Limitaciones

- Las partidas utilizan **jugadores automatizados**; no existe un modo interactivo para elegir movimientos manualmente.
- Los resultados pueden variar según la semilla, la configuración y las decisiones pseudoaleatorias de los jugadores.
- El rendimiento depende de la cantidad de jugadores, el número de mesas y los recursos del equipo.

---

Este repositorio contiene la implementación y las instrucciones de uso del simulador. El análisis detallado de las decisiones de diseño, los mecanismos de sincronización y los resultados experimentales se desarrolla en el **informe académico de la tarea**.
