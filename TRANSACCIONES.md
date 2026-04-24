# Transacciones Formales del Bus

## Definición

Una transacción de bus es el mensaje que una caché L1 emite al interconnect
para coordinar el acceso a un bloque de memoria compartida. Cada transacción
tiene un tipo específico que determina cómo el interconnect modifica el estado
de las demás cachés del sistema.

---

## Conjunto de Transacciones Soportadas

### BusRead
- **Transición:** I → S
- **Emisor:** Caché con miss de lectura (no tiene copia del bloque)
- **Acción del bus:** Si alguna caché tiene el bloque en M, la baja a S
  (downgrade). El bloque se entrega al solicitante en estado S.
- **Costo:** latencia_bus + latencia_memoria (si viene de RAM)
  o latencia_bus + latencia_compartida (si viene de otra caché)
- **Protocolos:** MSI y Firefly

---

### BusReadX
- **Transición:** I → M
- **Emisor:** Caché con miss de escritura (no tiene copia del bloque)
- **Acción del bus:** Invalida todas las copias remotas del bloque.
  El solicitante obtiene el bloque en estado M (propietario exclusivo).
- **Costo:** latencia_bus + latencia_coherencia + latencia_memoria/compartida
- **Protocolos:** MSI
- **Diferencia con BusUpgrade:** BusReadX transfiere el bloque desde
  memoria o desde otra caché. BusUpgrade no transfiere datos.

---

### BusUpgrade
- **Transición:** S → M
- **Emisor:** Caché que ya tiene el bloque en S y quiere escribir
- **Acción del bus:** Invalida las copias remotas del bloque.
  El solicitante pasa a M sin necesidad de transferir datos.
- **Costo:** latencia_bus + latencia_coherencia (sin transferencia de datos)
- **Protocolos:** MSI
- **Ventaja:** Más eficiente que BusReadX cuando el PE ya tiene el bloque,
  ya que evita la transferencia de datos innecesaria.

---

### BusUpdate
- **Transición:** cualquier → S o I → M (si no hay otras copias)
- **Emisor:** Caché que escribe un bloque (protocolo Firefly)
- **Acción del bus:** En lugar de invalidar, propaga el nuevo valor
  a todas las cachés que tienen copia. Todas quedan en estado S.
  Si nadie más tiene copia, el solicitante pasa a M.
- **Costo:** latencia_bus + latencia_coherencia + latencia_compartida
- **Protocolos:** Firefly
- **Diferencia con BusReadX/BusUpgrade:** No invalida. Actualiza.
  Reduce latencias futuras de lectura pero consume más ancho de banda.

---

### BusInvalidate
- **Transición:** S/M → I (en cachés remotas)
- **Emisor:** El interconnect lo genera internamente como efecto de
  BusReadX o BusUpgrade
- **Acción del bus:** Fuerza el estado I en las cachés remotas que
  tienen copia del bloque.
- **Protocolos:** MSI (generado internamente, no emitido directamente por la caché)

---

## Comparación por Protocolo

| Transacción  | MSI | Firefly | Transfiere datos | Invalida remotos |
|---|:---:|:---:|:---:|:---:|
| BusRead      | ✓   | ✓       | Sí               | No               |
| BusReadX     | ✓   | ✗       | Sí               | Sí               |
| BusUpgrade   | ✓   | ✗       | No               | Sí               |
| BusUpdate    | ✗   | ✓       | Sí (broadcast)   | No               |
| BusInvalidate| ✓   | ✗       | No               | Sí (interno)     |

---

## Evidencia Experimental

Los siguientes resultados fueron obtenidos con la simulación SystemC
del proyecto bajo los dos workloads oficiales.

### Propuesta 1: Multiplicación de Matrices (Alta Contención)

| Métrica       | MSI | Firefly |
|---|---:|---:|
| Transacciones | 44  | 44      |
| BusRead       | 4   | 4       |
| BusReadX      | 40  | 0       |
| BusUpgrade    | 0   | 0       |
| BusUpdate     | 0   | 40      |
| Invalidaciones| 39  | 0       |
| Updates       | 0   | 114     |

**Observación:** En alta contención todos los writes son BusReadX en MSI
porque cada PE invalida al anterior y siempre escribe desde estado I.
Firefly evita todas las invalidaciones a costa de 114 updates de broadcast.

### Propuesta 2: Pipeline Productor-Consumidor

| Métrica       | MSI | Firefly |
|---|---:|---:|
| Transacciones | 18  | 14      |
| BusRead       | 8   | 4       |
| BusReadX      | 6   | 0       |
| BusUpgrade    | 4   | 0       |
| BusUpdate     | 0   | 10      |
| Invalidaciones| 12  | 0       |
| Updates       | 0   | 17      |
| Downgrades M→S| 6   | 1       |

**Observación:** En el pipeline aparecen 4 BusUpgrade en MSI porque
PE1 lee primero (S) y luego escribe el mismo bloque, evitando la
transferencia de datos. Firefly usa menos transacciones totales (14 vs 18)
porque los updates proactivos evitan relecturas posteriores.

---

## Referencias

- Hennessy & Patterson. *Computer Architecture: A Quantitative Approach*. Elsevier, 2017.
- Nagarajan et al. *A Primer on Memory Consistency and Cache Coherence*. Morgan & Claypool, 2020.
- Archibald & Baer. *Cache Coherence Protocols: Evaluation Using a Multiprocessor Simulation Model*. ACM TOCS, 1986.
- Thacker et al. *Firefly: A Multiprocessor Workstation*. IEEE Transactions on Computers, 1988.