# Proyecto Grupal 1 - Arquitectura de Computadores II

Guia rapida para instalar dependencias, compilar y ejecutar el proyecto en Linux.

## 1. Requisitos

- Linux (probado en Ubuntu 24.04)
- `g++` con soporte C++17
- `make`
- Biblioteca de desarrollo de SystemC

## Restriccion de Implementacion de Hardware

- El modelo de hardware (`interconnect`, caches y PEs en `src/sim`) se implementa
	exclusivamente en SystemC/TLM (C++).
- Queda prohibido usar Python para el modelo de hardware.
- Python en este repositorio se usa solo para automatizacion/analisis de resultados
	(scripts en `scripts/`).

## 2. Instalacion de dependencias (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y build-essential libsystemc-dev
```

Verificar instalacion de SystemC:

```bash
apt-cache policy libsystemc-dev
ls -l /usr/include/systemc /usr/lib/x86_64-linux-gnu/libsystemc.so
```

## 3. Compilar y probar

Desde la raiz del repositorio:

```bash
make clean
make
```

Con esta configuracion, `make` realiza dos cosas:

1. Compila los binarios.
2. Ejecuta una bateria de pruebas (`trace_generator` + simulaciones).

Esto genera:

- `trace_generator` (generador de trazas)
- `sim_proyecto` (simulador con SystemC)

Las pruebas automaticas por defecto (`make`) ejecutan las 2 propuestas del equipo, cada una con 2 protocolos:

- `traces/trace_matmul.txt` (alta contencion)
- `traces/trace_pipeline.txt` (productor-consumidor)
- Protocolos: `msi` y `firefly`

Existe un escenario adicional opcional (`trace_migratorio`) para pruebas exploratorias:

```bash
make test_extra
```

Si solo quieren compilar sin correr pruebas:

```bash
make build
```

Para ejecutar las 3 traces contra ambos protocolos y exportar resultados en CSV + bitacoras:

```bash
make test_csv
```

Para ademas generar graficas PNG desde esos CSV:

```bash
make plots
```

Para generar waveforms VCD (visualizacion temporal de hardware):

```bash
make waveforms
```

Esto produce por defecto:

- `results/waveforms/pipeline_msi.vcd`

Para abrirlo con GTKWave:

```bash
gtkwave results/waveforms/pipeline_msi.vcd
```

## 4. Ejecutar

### 4.1 Generar trazas

```bash
./trace_generator
```

### 4.2 Ejecutar simulador

```bash
./sim_proyecto
```

Tambien puedes especificar traza y protocolo:

```bash
./sim_proyecto traces/trace_pipeline.txt msi
./sim_proyecto traces/trace_pipeline.txt firefly
```

Tambien puedes especificar parametros de cache L1 (bytes):

```bash
./sim_proyecto <trace> <protocolo> \
	<l1_size_bytes> <block_size_bytes> <associativity> <replacement> \
	<l1_hit_ns> <l1_miss_ns> <bus_ns> <mem_ns> <shared_ns> <coherence_ns>

# Ejemplo: L1 de 4KB, bloque de 64B, 4-way, reemplazo LRU
./sim_proyecto traces/trace_pipeline.txt msi 4096 64 4 lru

# Ejemplo: misma cache pero FIFO
./sim_proyecto traces/trace_pipeline.txt firefly 4096 64 4 fifo

# Ejemplo con latencias diferenciadas
# l1_hit=1ns l1_miss=1ns bus=1ns mem=5ns shared=2ns coherence=2ns
./sim_proyecto traces/trace_pipeline.txt msi 4096 64 4 lru 1 1 1 5 2 2

# Ejemplo indicando nombre base del waveform VCD de salida
./sim_proyecto traces/trace_pipeline.txt msi 4096 64 4 lru 1 1 1 5 2 2 results/waveforms/mi_corrida
```

Si no se pasan esos argumentos, se usan defaults:

- L1 size = 1024 bytes
- block size = 64 bytes
- associativity = 2
- replacement = LRU
- l1_hit = 1 ns
- l1_miss = 1 ns
- bus = 1 ns
- memoria = 5 ns
- compartida = 2 ns
- coherencia = 2 ns

El simulador imprime:

- transacciones de bus
- transiciones de estado por cache L1
- estadisticas finales por interconnect y por cache
- evictions y conflict misses de L1 (explicitos)

Con `make test_csv` se generan automaticamente:

- `results/interconnect_stats.csv`
- `results/l1_stats.csv`
- bitacoras por corrida en `results/logs/*.log`

Con `make plots` se generan graficas en:

- `results/plots/*.png`

## 5. Limpiar binarios y trazas

```bash
make clean
```

## 6. Problemas comunes

### Error: `fatal error: systemc.h: No existe el archivo o el directorio`

Instala la dependencia faltante:

```bash
sudo apt install -y libsystemc-dev
```

### Error de enlace con `sc_api_version_..._cxx201402L`

Ese error aparece cuando el codigo se compila con un estandar C++ distinto al de la biblioteca instalada.
Este repositorio ya esta configurado para compilar con **C++17**, que coincide con `libsystemc-dev` de Ubuntu reciente.

## 7. Nota sobre rutas de SystemC

El `Makefile` detecta automaticamente dos esquemas:

1. Instalacion local en `/usr/local/systemc-2.3.3`
2. Instalacion de paquete del sistema (`/usr/include` y `/usr/lib/x86_64-linux-gnu`)

Si usan una instalacion local distinta, pueden sobrescribir la ruta al compilar:

```bash
make SYSTEMC_HOME=/ruta/a/systemc
```
