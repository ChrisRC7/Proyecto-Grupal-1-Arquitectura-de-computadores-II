# Estado de Tareas del Proyecto

Fecha: 18 de abril de 2026

## Tareas Hechas

- Se consolidaron 2 propuestas separadas como base oficial del proyecto:
  - Propuesta 1: Multiplicacion de Matrices Bloqueada (Alta Contencion)
  - Propuesta 2: Pipeline de Procesamiento de Criptografia/Red (Productor-Consumidor)

- Se agrego trazabilidad de transiciones de estado en cache L1.
- Se incorporaron estadisticas de interconnect y de cada cache.
- Se agrego una tercera traza (`trace_migratorio`) como escenario opcional de exploracion.


## Lo que Falta

- Mantener analisis y reporte final centrados en las 2 propuestas oficiales por separado.
- Preparar comparativos MSI vs Firefly por propuesta (tabla por propuesta, no mezcladas).

- Modelar cache L1 con parametros reales:
  - tamaño total
  - tamaño de bloque
  - asociatividad
  - politica de reemplazo
- Modelar evictions/conflict misses de forma explicita.
- Agregar latencias diferenciadas (hit L1, miss, memoria compartida, bus).
- Definir y documentar un conjunto formal de transacciones de bus.
- Exportar resultados a CSV para comparacion automatica MSI vs Firefly.
- Construir scripts/targets para experimentos reproducibles (corridas por lote con resumen).
- Preparar material de entrega final:
  - paper IEEE (max 4 paginas)
  - video de 3-5 minutos
  - evidencia de comparacion de ancho de banda/rendimiento
- Ordenar flujo de ramas y PR segun la estrategia acordada del equipo.

## Riesgos Actuales

- El modelo de coherencia es funcional pero simplificado; aun no representa todos los detalles de una jerarquia real.
- Falta automatizar la generacion de tablas/figuras para el paper.
