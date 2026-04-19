# Comparativos por Propuesta Oficial

Este documento separa los resultados de MSI vs Firefly por propuesta oficial, sin mezclar escenarios.

## Propuesta 1: Multiplicacion de Matrices Bloqueada (Alta Contencion)

| Protocolo | Transacciones | Lecturas | Escrituras | Invalidaciones | Updates | Downgrades M->S |
|---|---:|---:|---:|---:|---:|---:|
| MSI | 43 | 4 | 39 | 38 | 0 | 0 |
| Firefly | 44 | 4 | 40 | 0 | 114 | 0 |

Observacion breve: bajo alta contencion, MSI concentra trafico en invalidaciones, mientras Firefly cambia ese costo por updates.

## Propuesta 2: Pipeline Criptografia/Red (Productor-Consumidor)

| Protocolo | Transacciones | Lecturas | Escrituras | Invalidaciones | Updates | Downgrades M->S |
|---|---:|---:|---:|---:|---:|---:|
| MSI | 20 | 10 | 10 | 11 | 0 | 4 |
| Firefly | 20 | 10 | 10 | 0 | 16 | 0 |

Observacion breve: en pipeline, ambos protocolos mantienen el mismo total de transacciones, con distinta estrategia de coherencia (invalidar vs actualizar).
