#include "interconnect.h"

#include <iostream>
#include <string>
#include <vector>

#include "l1_cache.h"

Interconnect::Interconnect(sc_module_name name, Protocol selected_protocol,
                           const TimingConfig& timing_config)
    : sc_module(name), protocol(selected_protocol), timing(timing_config) {
    wf_last_addr = 0;
    wf_last_op = 0;
    wf_last_tx_type = 0;
    wf_requester_id = 0;
    wf_total_tx = 0;
    wf_invalidations = 0;
    wf_updates = 0;
    wf_shared_access = 0;
}

void Interconnect::register_cache(L1Cache* cache) {
    caches.push_back(cache);
}

// ─────────────────────────────────────────────
// Metodo auxiliar: actualiza la señal de waveform
// segun el tipo de transaccion
// ─────────────────────────────────────────────
void Interconnect::set_wf_tx_type(BusTransaction tx) {
    switch (tx) {
        case BusTransaction::BusRead:       wf_last_tx_type = 1; break;
        case BusTransaction::BusReadX:      wf_last_tx_type = 2; break;
        case BusTransaction::BusUpgrade:    wf_last_tx_type = 3; break;
        case BusTransaction::BusUpdate:     wf_last_tx_type = 4; break;
        case BusTransaction::BusInvalidate: wf_last_tx_type = 5; break;
    }
}

// ─────────────────────────────────────────────
// Metodo principal: decide que transaccion ejecutar
// ─────────────────────────────────────────────
void Interconnect::issue_transaction(BusTransaction tx, int requester_id, std::uint64_t addr) {
    stats.total++;
    wf_requester_id = static_cast<sc_dt::sc_uint<3>>(requester_id);
    wf_last_addr    = static_cast<sc_dt::sc_uint<64>>(addr);
    wf_total_tx     = static_cast<sc_dt::sc_uint<64>>(stats.total);
    set_wf_tx_type(tx);

    std::cout << "@" << sc_time_stamp()
              << " | Bus: " << transaction_to_string(tx)
              << " de PE" << requester_id
              << " en 0x" << std::hex << addr << std::dec
              << std::endl;

    switch (tx) {
        case BusTransaction::BusRead:
            do_bus_read(requester_id, addr);
            break;
        case BusTransaction::BusReadX:
            do_bus_read_x(requester_id, addr);
            break;
        case BusTransaction::BusUpgrade:
            do_bus_upgrade(requester_id, addr);
            break;
        case BusTransaction::BusUpdate:
            do_bus_update(requester_id, addr);
            break;
        case BusTransaction::BusInvalidate:
            // usado internamente, no deberia llegar aqui directamente
            break;
    }
}

// ─────────────────────────────────────────────
// BusRead: I -> S
// El PE no tiene copia. Pide el bloque para leer.
// Si alguien lo tiene en M, hace downgrade a S primero.
// ─────────────────────────────────────────────
void Interconnect::do_bus_read(int requester_id, std::uint64_t addr) {
    stats.reads++;
    wf_last_op = 1;

    bool shared_copy_exists = false;

    for (L1Cache* cache : caches) {
        if (cache->cache_id == requester_id) continue;

        CacheState st = cache->get_state(addr);
        if (st == CacheState::M) {
            // El dueño exclusivo debe bajar a S antes de compartir
            cache->set_state(addr, CacheState::S, "BusRead remoto: downgrade M->S");
            stats.downgrades_m_to_s++;
            shared_copy_exists = true;
        } else if (st == CacheState::S) {
            shared_copy_exists = true;
        }
    }

    // El requester recibe el bloque en S
    for (L1Cache* cache : caches) {
        if (cache->cache_id == requester_id) {
            cache->set_state(addr, CacheState::S,
                shared_copy_exists ? "BusRead: datos de cache compartida"
                                   : "BusRead: datos de memoria");
            break;
        }
    }

    wf_shared_access = shared_copy_exists ? 1 : 0;
    std::uint64_t latency = timing.bus_latency_ns;
    latency += shared_copy_exists ? timing.shared_latency_ns : timing.memory_latency_ns;
    wait(static_cast<double>(latency), SC_NS);
}

// ─────────────────────────────────────────────
// BusReadX: I -> M  (MSI)
// El PE no tiene copia pero quiere escribir.
// Invalida todas las copias remotas.
// ─────────────────────────────────────────────
void Interconnect::do_bus_read_x(int requester_id, std::uint64_t addr) {
    stats.writes++;
    stats.bus_read_x++;
    wf_last_op = 2;

    bool had_copies = false;

    for (L1Cache* cache : caches) {
        if (cache->cache_id == requester_id) continue;

        if (cache->has_copy(addr)) {
            cache->set_state(addr, CacheState::I, "BusReadX: invalidacion remota");
            stats.invalidations++;
            wf_invalidations = static_cast<sc_dt::sc_uint<64>>(stats.invalidations);
            had_copies = true;
        }
    }

    // El requester pasa a M: es el unico dueño
    for (L1Cache* cache : caches) {
        if (cache->cache_id == requester_id) {
            cache->set_state(addr, CacheState::M, "BusReadX: propietario exclusivo");
            break;
        }
    }

    wf_shared_access = had_copies ? 1 : 0;
    std::uint64_t latency = timing.bus_latency_ns + timing.coherence_latency_ns;
    latency += had_copies ? timing.shared_latency_ns : timing.memory_latency_ns;
    wait(static_cast<double>(latency), SC_NS);
}

// ─────────────────────────────────────────────
// BusUpgrade: S -> M  (MSI)
// El PE YA tiene copia en S y quiere escribir.
// No necesita traer el bloque, solo invalida a los demas.
// Es mas barato que BusReadX porque no hay transferencia de datos.
// ─────────────────────────────────────────────
void Interconnect::do_bus_upgrade(int requester_id, std::uint64_t addr) {
    stats.writes++;
    stats.bus_upgrades++;
    wf_last_op = 2;

    for (L1Cache* cache : caches) {
        if (cache->cache_id == requester_id) continue;

        if (cache->has_copy(addr)) {
            cache->set_state(addr, CacheState::I, "BusUpgrade: invalidacion remota");
            stats.invalidations++;
            wf_invalidations = static_cast<sc_dt::sc_uint<64>>(stats.invalidations);
        }
    }

    // El requester pasa a M
    for (L1Cache* cache : caches) {
        if (cache->cache_id == requester_id) {
            cache->set_state(addr, CacheState::M, "BusUpgrade: propietario exclusivo");
            break;
        }
    }

    // Solo costo de coherencia, no hay transferencia de datos
    wf_shared_access = 1;
    std::uint64_t latency = timing.bus_latency_ns + timing.coherence_latency_ns;
    wait(static_cast<double>(latency), SC_NS);
}

// ─────────────────────────────────────────────
// BusUpdate: Firefly
// El PE escribio un bloque. En lugar de invalidar,
// actualiza todas las copias remotas con el nuevo valor.
// ─────────────────────────────────────────────
void Interconnect::do_bus_update(int requester_id, std::uint64_t addr) {
    stats.writes++;
    stats.bus_updates++; 
    wf_last_op = 2;

    std::vector<L1Cache*> sharers;
    L1Cache* requester = nullptr;

    for (L1Cache* cache : caches) {
        if (cache->cache_id == requester_id) {
            requester = cache;
        } else if (cache->has_copy(addr)) {
            sharers.push_back(cache);
        }
    }

    if (requester == nullptr) return;

    if (!sharers.empty()) {
        // Hay otros con copia: actualiza a todos, todos quedan en S
        for (L1Cache* sharer : sharers) {
            sharer->set_state(addr, CacheState::S, "BusUpdate: copia actualizada Firefly");
            stats.updates++;
            wf_updates = static_cast<sc_dt::sc_uint<64>>(stats.updates);
        }
        requester->set_state(addr, CacheState::S, "BusUpdate: escritura compartida Firefly");
        wf_shared_access = 1;
        std::uint64_t latency = timing.bus_latency_ns + timing.coherence_latency_ns
                              + timing.shared_latency_ns;
        wait(static_cast<double>(latency), SC_NS);
    } else {
        // Nadie mas tiene copia: el requester es dueno exclusivo
        requester->set_state(addr, CacheState::M, "BusUpdate: escritura privada Firefly");
        wf_shared_access = 0;
        std::uint64_t latency = timing.bus_latency_ns + timing.memory_latency_ns;
        wait(static_cast<double>(latency), SC_NS);
    }
}

// ─────────────────────────────────────────────
// handle_read y handle_write: ahora son puentes
// que llaman a issue_transaction con el tipo correcto
// ─────────────────────────────────────────────
void Interconnect::handle_read(int requester_id, std::uint64_t addr) {
    issue_transaction(BusTransaction::BusRead, requester_id, addr);
}

void Interconnect::handle_write(int requester_id, std::uint64_t addr) {
    // Decide que transaccion usar segun el estado actual del bloque
    // en la cache del requester y el protocolo activo
    CacheState current = CacheState::I;
    for (L1Cache* cache : caches) {
        if (cache->cache_id == requester_id) {
            current = cache->get_state(addr);
            break;
        }
    }

    if (protocol == Protocol::MSI) {
        if (current == CacheState::S) {
            // Ya tiene copia, solo necesita invalidar
            issue_transaction(BusTransaction::BusUpgrade, requester_id, addr);
        } else {
            // No tiene copia, necesita traer el bloque e invalidar
            issue_transaction(BusTransaction::BusReadX, requester_id, addr);
        }
    } else {
        // Firefly siempre usa BusUpdate
        issue_transaction(BusTransaction::BusUpdate, requester_id, addr);
    }
}

void Interconnect::print_stats() const {
    std::cout << "\n=== ESTADISTICAS INTERCONNECT ("
              << protocol_to_string(protocol) << ") ===" << std::endl;
    std::cout << "Transacciones totales : " << stats.total << std::endl;
    std::cout << "  BusRead             : " << stats.reads << std::endl;
    std::cout << "  BusReadX            : " << stats.bus_read_x << std::endl;
    std::cout << "  BusUpgrade          : " << stats.bus_upgrades << std::endl;
    std::cout << "  BusUpdate           : " << stats.bus_updates << std::endl;
    std::cout << "  Invalidaciones      : " << stats.invalidations << std::endl;
    std::cout << "  Updates             : " << stats.updates << std::endl;
    std::cout << "  Downgrades M->S     : " << stats.downgrades_m_to_s << std::endl;
}