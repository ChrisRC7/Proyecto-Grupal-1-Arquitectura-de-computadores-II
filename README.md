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
- Las trazas se generan localmente con `trace_generator` en C++; no dependen de Python.
- Para la presentacion, la caché de referencia se mantiene con tamaño definido y
	configuracion 2-way por defecto.

## 2. Instalacion de dependencias (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y build-essential libsystemc-dev
sudo apt install gtkwave
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

El generador de trazas es local y puede producir patrones deterministas o variables
sin salir del repositorio. Si se quiere repetir exactamente una corrida, basta con
conservar la misma salida generada por `trace_generator`.

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

Para ejecutar el lote reproducible y dejar un resumen agregado en Markdown:

```bash
make experiments
```

El lote genera:

- `results/interconnect_stats.csv`
- `results/l1_stats.csv`
- `results/logs/*.log`
- `results/resumen_experimentos.md`

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

### 4.3 Conjunto formal de transacciones de bus

El modelo usa el siguiente conjunto formal de transacciones:

- `BusRead`: lectura de un bloque sin copia local, con posible comparticion en `S`.
- `BusReadX`: lectura con intencion de escritura desde `I`; invalida copias remotas y concede exclusividad.
- `BusUpgrade`: escritura desde `S`; solo invalida a los demas y evita traer el bloque de memoria.
- `BusUpdate`: escritura con politica Firefly; actualiza todas las copias remotas.
- `BusInvalidate`: invalidacion interna usada por la ruta MSI.

La invalidacion no se trata como un dato utilizable: una linea marcada en `I` se
considera invalida y cualquier acceso futuro debe volver a traer o actualizar el
bloque antes de usarlo.

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
Para generar y revisar los VCD de MSI y Firefly, usa estas corridas directas:

```bash
./sim_proyecto traces/trace_pipeline.txt msi 1024 64 2 lru 1 1 1 5 2 2 results/waveforms/pipeline_msi
./sim_proyecto traces/trace_pipeline.txt firefly 1024 64 2 lru 1 1 1 5 2 2 results/waveforms/pipeline_firefly
```

Luego abre cada uno en GTKWave:

```bash
gtkwave results/waveforms/pipeline_msi.vcd
gtkwave results/waveforms/pipeline_firefly.vcd
```

Si quieres generar primero todo el lote y después revisar el VCD de MSI por defecto:

```bash
make waveforms
```

Eso te deja pipeline_msi.vcd.

Si además quieres regenerar los CSV y logs antes de mirar el waveform:

```bash
make test_csv
```

**Cómo debería comportarse bus_last_tx_type**

La señal está trazada en main.cpp y se codifica en interconnect.cpp:

- `000` o `0`: sin transacción / estado inicial
- `001` o `1`: BusRead
- `010` o `2`: BusReadX
- `011` o `3`: BusUpgrade
- `100` o `4`: BusUpdate

**En MSI deberías ver:**
- `001` en lecturas con miss
- `010` cuando un PE escribe sin copia previa
- `011` cuando un PE ya tenía la línea en S y quiere escribir

**En Firefly deberías ver:**
- `001` en lecturas
- `100` en escrituras, porque Firefly usa BusUpdate

**Cómo leerlo en GTKWave**
Agrega estas señales:

- `bus_last_tx_type`
- `bus_requester_id`
- `bus_last_addr`
- `bus_total_tx`
- `bus_invalidations`
- `bus_updates`
- `l1_0_last_state`
- `l1_1_last_state`
- `l1_2_last_state`
- `l1_3_last_state`

**Patrón esperado**
- En MSI: `001 -> 010 -> 011` según ocurran lecturas, write misses y upgrades.
- En Firefly: `001 -> 100 -> 001 -> 100`, alternando lecturas con actualizaciones.
- Entre transacciones puede quedarse en `000` si no hubo evento nuevo todavía.

Si quieres, te paso también un orden exacto de señales para que en GTKWave te quede legible y no tengas que buscarlas una por una.