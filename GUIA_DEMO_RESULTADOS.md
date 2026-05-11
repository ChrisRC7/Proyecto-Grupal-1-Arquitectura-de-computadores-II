# Guía de Resultados Esperados - Simulador de Coherencia de Caché

Este documento describe los resultados esperados al ejecutar las simulaciones y cómo interpretarlos.

---

## Ejecución General

### Comando para todo
```bash
make clean && make test_csv && make waveforms && make plots
```

Esto genera:
- **CSV** con estadísticas: `results/interconnect_stats.csv`, `results/l1_stats.csv`
- **VCD** para GTKWave: `results/waveforms/pipeline_msi.vcd`
- **PNG** con gráficas: `results/plots/*.png`
- **Bitácoras**: `results/logs/*.log`

---

## Resultados Esperados por Propuesta

### Propuesta 1: Multiplicación de Matrices Bloqueada (Alta Contención)

**Escenario:** 4 PEs escriben repetidamente en la misma dirección (0x2000).

| Métrica | MSI | Firefly | Interpretación |
|---------|-----|---------|-----------------|
| Transacciones totales | **44** | **44** | Mismo número de transacciones |
| Lecturas (BusRead) | **4** | **4** | MSI y Firefly leen igual |
| Escrituras | **40** | **40** | Misma cantidad de escrituras |
| **Invalidaciones** | **39** | **0** | ✓ MSI invalida agresivamente, Firefly no |
| **Updates** | **0** | **114** | ✓ Firefly propaga actualizaciones en broadcast |
| Downgrades M→S | **0** | **0** | Nadie tiene copia compartida |

**Conclusión:** En alta contención, MSI concentra tráfico en **invalidaciones**, mientras que Firefly concentra el costo en **updates broadcast**.

---

### Propuesta 2: Pipeline Productor-Consumidor

**Escenario:** PE0 escribe, PE1 lee/escribe, PE2 lee. Patrón de colaboración.

| Métrica | MSI | Firefly | Interpretación |
|---------|-----|---------|-----------------|
| **Transacciones totales** | **18** | **14** | ✓ Firefly usa menos transacciones (22% menos) |
| Lecturas (BusRead) | **8** | **4** | Firefly reduce relecturas por updates proactivos |
| Escrituras | **10** | **10** | Escrituras iguales |
| **Invalidaciones** | **12** | **0** | ✓ MSI invalida, Firefly no |
| **Updates** | **0** | **17** | ✓ Firefly actualiza copias |
| **Downgrades M→S** | **6** | **1** | ✓ Firefly reduce downgrade a 1 |

**Conclusión:** En patrón productor-consumidor, Firefly es más eficiente porque los updates proactivos reducen **relecturas futuras**.

---

### Propuesta 3: Patrón Migratorio (Extra)

**Escenario:** El bloque migra entre PEs sin contención simultánea.

| Métrica | MSI | Firefly | Interpretación |
|---------|-----|---------|-----------------|
| Transacciones totales | **61** | **36** | ✓ Firefly reduce 41% de transacciones |
| Invalidaciones | **55** | **0** | ✓ MSI invalida mucho en transferencia |
| Updates | **0** | **96** | Firefly prefiere actualizar que permitir exclusividad |
| Tiempo sim (ns) | **91** | **73** | ✓ Firefly termina 20% más rápido |

**Conclusión:** En migración, Firefly es significativamente más rápido porque no invalida copias intermedias.

---

## Cómo Verificar en GTKWave

### Abrir los waveforms
```bash
gtkwave results/waveforms/pipeline_msi.vcd  # MSI
gtkwave results/waveforms/pipeline_firefly.vcd  # Firefly
```

### Señales clave para seguir transacciones

Agrega en este orden:

1. **`bus_total_tx`** — contador de transacciones (debe subir 0 → 18 en pipeline MSI)
2. **`bus_last_tx_type`** — tipo de transacción:
   - `01` = BusRead
   - `10` = BusReadX (solo MSI)
   - `11` = BusUpgrade (solo MSI)
   - `100` = BusUpdate (solo Firefly)
3. **`bus_requester_id`** — PE que originó (0, 1, 2, 3)
4. **`bus_last_addr`** — dirección afectada
5. **`bus_invalidations`** — contador de invalidaciones (sube en MSI, NO en Firefly)
6. **`bus_updates`** — contador de updates (sube en Firefly, NO en MSI)
7. **`l1_0_last_state`** / **`l1_1_last_state`** / **`l1_2_last_state`** — estados de las L1:
   - `0` = I (Invalid)
   - `1` = S (Shared)
   - `2` = M (Modified)

### Patrón de lectura en MSI (pipeline)

Busca el primer `bus_last_tx_type = 01` (BusRead):
- PE hace lectura
- `bus_total_tx` sube a 1
- `bus_requester_id` = X (qué PE leyó)
- Alguna L1 cambia `last_state` a `1` (S)

### Patrón de escritura en MSI (pipeline)

Busca `bus_last_tx_type = 10` (BusReadX):
- PE quiere escribir sin copia
- `bus_total_tx` sube
- `bus_invalidations` sube (1 o más)
- Otras L1s cambian a `0` (I)
- La L1 del requester cambia a `2` (M)

### Patrón de upgrade en MSI (pipeline)

Busca `bus_last_tx_type = 11` (BusUpgrade):
- PE ya tenía en S, quiere escribir
- `bus_invalidations` sube
- Requester cambia S → M
- Otros cambian S → I

### Diferencia en Firefly

En lugar de ver `10` y `11`, verás `100` (BusUpdate):
- `bus_updates` sube en lugar de `bus_invalidations`
- Las otras L1s que tenían copia pasan a `1` (S) en lugar de `0` (I)
- En la siguiente lectura NO hay miss (porque Firefly ya actualizó)

---

## Resumen de CSV

### `results/interconnect_stats.csv`

Columnas importantes:
- `protocol` — MSI o FIREFLY
- `transacciones_totales` — total de tx del bus
- `lecturas` — conteo de BusRead
- `escrituras` — conteo de BusReadX + BusUpgrade + BusUpdate
- `invalidaciones` — solo en MSI
- `updates` — solo en Firefly
- `downgrades_m_to_s` — transiciones M→S

### `results/l1_stats.csv`

Columnas importantes por caché:
- `read_hits`, `read_misses` — accesos de lectura
- `write_hits`, `write_misses` — accesos de escritura
- `read_hit_rate`, `write_hit_rate` — tasas de acierto
- `evictions`, `conflict_misses` — problemas de capacidad

---

## Gráficas PNG Esperadas

### `results/plots/transacciones_totales.png`
- Muestra que MSI y Firefly tienen similar número de transacciones en matmul.
- Pero Firefly reduce transacciones en pipeline (18 vs 14).

### `results/plots/invalidaciones.png`
- MSI tiene picos de invalidaciones.
- Firefly tiene cero invalidaciones (por eso BusUpdate).

### `results/plots/updates.png`
- MSI: cero updates.
- Firefly: alto número de updates (broadcast).

### `results/plots/bus_tx_por_ns.png`
- Ancho de banda aproximado: transacciones por nanosegundo.
- Comparación MSI vs Firefly por trace.

---

## Checklist de Demo (5 minutos)

1. ✅ Compilación: `make clean && make build`
2. ✅ Ejecución: `make test_csv`
3. ✅ Mostrar CSV: abrir `results/interconnect_stats.csv` en librecalc/Excel
4. ✅ Mostrar VCD MSI: `gtkwave results/waveforms/pipeline_msi.vcd`
   - Señales: bus_total_tx, bus_last_tx_type, bus_requester_id, bus_invalidations, l1_X_last_state
   - Explicar: "ves cómo sube invalidations? Eso es MSI"
5. ✅ Mostrar VCD Firefly: `gtkwave results/waveforms/pipeline_firefly.vcd`
   - Mismas señales.
   - Explicar: "ves cómo NO sube invalidations sino updates? Eso es Firefly"
6. ✅ Mostrar gráficas: `eog results/plots/invalidaciones.png` o solo mostrar en pantalla
7. ✅ Resumen: leer `results/resumen_experimentos.md`

---

## Interpretación General

**MSI (Invalidate-Based):**
- Cada escritura **invalida copias remotas**.
- Siguiente lectura requiere nuevo BusRead.
- Más invalidaciones, menos ancho de banda en escritura, pero más misses después.

**Firefly (Update-Based):**
- Cada escritura **propaga valor a copias**.
- Siguiente lectura puede hitear (porque ya tiene valor actualizado).
- Menos transacciones totales en patrones con relectura, pero más broadcast.

**Conclusión:** Firefly gana en **pipeline y migración**, MSI es equivalente en **alta contención**.
