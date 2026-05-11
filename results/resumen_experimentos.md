# Resumen de experimentos reproducibles

Este resumen se genera a partir de `results/interconnect_stats.csv` y `results/l1_stats.csv`.

| Propuesta | Protocolo | Tx bus | Invalidaciones | Updates | Downgrades M->S | Tiempo sim (ns) |
|---|---|---:|---:|---:|---:|---:|
| propuesta_1_matmul | MSI | 44 | 39 | 0 | 0 | 215 |
| propuesta_1_matmul | FIREFLY | 44 | 0 | 114 | 0 | 213 |
| propuesta_2_pipeline | MSI | 20 | 10 | 0 | 5 | 92 |
| propuesta_2_pipeline | FIREFLY | 20 | 0 | 10 | 5 | 90 |
| extra_migratorio | MSI | 57 | 48 | 0 | 7 | 187 |
| extra_migratorio | FIREFLY | 36 | 0 | 96 | 0 | 172 |

## Conjunto formal de transacciones de bus

- `BusRead`: un PE lee un bloque que no tiene localmente; el bloque puede compartirse en estado `S`.
- `BusReadX`: un PE solicita el bloque para escribir sin copia previa; invalida copias remotas y obtiene exclusividad.
- `BusUpgrade`: un PE ya tiene el bloque en `S` y quiere escribir; solo invalida a los demas para pasar a `M`.
- `BusUpdate`: politica Firefly; una escritura propaga el nuevo valor a todas las copias remotas.
- `BusInvalidate`: transaccion interna para invalidaciones remotas cuando corresponde al protocolo MSI.
