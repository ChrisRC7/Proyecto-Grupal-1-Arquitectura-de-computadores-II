# Estado de Tareas del Proyecto

Fecha: 28 de abril de 2026

## Resumen de la sesion de hoy

- Se reforzo el flujo completo de experimentacion: `make`, `make test_csv`, `make experiments`, `make plots` y `make waveforms`.
- Se dejo el modelo de hardware exclusivamente en SystemC/TLM (C++), prohibiendo Python en `src/sim`.
- Se agrego modelado realista de L1 con cache parametrizable, evictions, conflict misses y latencias diferenciadas.
- Se incorporo exportacion de CSV, bitacoras por corrida, graficas PNG y comparativos MSI vs Firefly por propuesta.
- Se habilito la generacion de waveforms VCD desde SystemC para inspeccion visual en GTKWave.
- Se formalizo el conjunto de transacciones de bus y se documento en el codigo y la guia del proyecto.
- Se agrego un resumen reproducible en Markdown para las corridas por lote.
- Se dejo constancia de que las trazas se generan localmente en C++ y que la cache de
  referencia para la presentacion queda definida con configuracion 2-way.
- Se aclaro que una invalidacion corresponde a una linea no utilizable hasta que vuelva
  a ser traida o actualizada por el protocolo.

## Tareas Hechas

- Se consolidaron 2 propuestas separadas como base oficial del proyecto:
  - Propuesta 1: Multiplicacion de Matrices Bloqueada (Alta Contencion)
  - Propuesta 2: Pipeline de Procesamiento de Criptografia/Red (Productor-Consumidor)

- Se agrego trazabilidad de transiciones de estado en cache L1.
- Se incorporaron estadisticas de interconnect y de cada cache.
- Se agrego una tercera traza (`trace_migratorio`) como escenario opcional de exploracion.
- Se incorporo comparativo MSI vs Firefly por propuesta oficial en tabla separada para reporte.
- Se modelo cache L1 parametrizable con:
  - tamaño total
  - tamaño de bloque
  - asociatividad
  - politica de reemplazo (LRU/FIFO)
- Se agrego exportacion automatica de resultados a CSV (`interconnect_stats` y `l1_stats`).
- Se agregaron bitacoras por corrida para trazabilidad de transacciones de bus.
- Se definio formalmente el conjunto de transacciones de bus: `BusRead`, `BusReadX`, `BusUpgrade`, `BusUpdate` y `BusInvalidate`.
- Se agrego script de Python para generar graficas PNG a partir de los CSV.
- Se agrego un resumen agregado en Markdown para experimentos reproducibles.
- Se modelaron `evictions` y `conflict misses` de forma explicita en L1.
- Se agregaron latencias diferenciadas para hit L1, deteccion de miss L1, bus, memoria,
  acceso compartido y costo de coherencia.
- Se agrego generacion de waveforms VCD desde SystemC para inspeccion temporal
  del comportamiento de interconnect y caches.


## Lo que Falta

- Preparar material de entrega final:
  - paper IEEE (max 4 paginas)
  - video de 3-5 minutos
  - evidencia de comparacion de ancho de banda/rendimiento
- Ordenar flujo de ramas y PR segun la estrategia acordada del equipo.

## Riesgos Actuales

- El modelo de coherencia es funcional pero simplificado; aun no representa todos los detalles de una jerarquia real.
- Falta automatizar la generacion de tablas/figuras para el paper.
