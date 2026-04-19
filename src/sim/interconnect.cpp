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
    wf_requester_id = 0;
    wf_total_tx = 0;
    wf_invalidations = 0;
    wf_updates = 0;
    wf_shared_access = 0;
}

void Interconnect::register_cache(L1Cache* cache) {
    caches.push_back(cache);
}

void Interconnect::handle_read(int requester_id, std::uint64_t addr) {
    stats.total++;
    stats.reads++;
    wf_last_op = 1;
    wf_requester_id = static_cast<sc_dt::sc_uint<3>>(requester_id);
    wf_last_addr = static_cast<sc_dt::sc_uint<64>>(addr);
    wf_total_tx = static_cast<sc_dt::sc_uint<64>>(stats.total);

    std::cout << "@" << sc_time_stamp() << " | Bus: READ de PE" << requester_id << " en 0x"
              << std::hex << addr << std::dec << std::endl;

    bool shared_copy_exists = false;
    for (L1Cache* cache : caches) {
        if (cache->cache_id == requester_id) {
            continue;
        }

        CacheState st = cache->get_state(addr);
        if (st == CacheState::M) {
            cache->set_state(addr, CacheState::S, "READ remoto: downgrade M->S");
            stats.downgrades_m_to_s++;
            shared_copy_exists = true;
        } else if (st == CacheState::S) {
            shared_copy_exists = true;
        }
    }

    for (L1Cache* cache : caches) {
        if (cache->cache_id == requester_id) {
            cache->set_state(addr, CacheState::S,
                             shared_copy_exists ? "READ miss compartido" : "READ miss memoria");
            break;
        }
    }

    std::uint64_t total_latency = timing.bus_latency_ns;
    total_latency += shared_copy_exists ? timing.shared_latency_ns : timing.memory_latency_ns;
    wf_shared_access = shared_copy_exists ? 1 : 0;
    wait(static_cast<double>(total_latency), SC_NS);
}

void Interconnect::handle_write(int requester_id, std::uint64_t addr) {
    stats.total++;
    stats.writes++;
    wf_last_op = 2;
    wf_requester_id = static_cast<sc_dt::sc_uint<3>>(requester_id);
    wf_last_addr = static_cast<sc_dt::sc_uint<64>>(addr);
    wf_total_tx = static_cast<sc_dt::sc_uint<64>>(stats.total);

    std::cout << "@" << sc_time_stamp() << " | Bus: WRITE de PE" << requester_id << " en 0x"
              << std::hex << addr << std::dec << std::endl;

    std::vector<L1Cache*> sharers;
    L1Cache* requester = nullptr;

    for (L1Cache* cache : caches) {
        if (cache->cache_id == requester_id) {
            requester = cache;
            continue;
        }
        if (cache->has_copy(addr)) {
            sharers.push_back(cache);
        }
    }

    if (requester == nullptr) {
        return;
    }

    if (protocol == Protocol::MSI) {
        for (L1Cache* sharer : sharers) {
            sharer->set_state(addr, CacheState::I, "WRITE remoto: invalidacion MSI");
            stats.invalidations++;
            wf_invalidations = static_cast<sc_dt::sc_uint<64>>(stats.invalidations);
        }
        requester->set_state(addr, CacheState::M, "WRITE propietario MSI");
    } else {
        if (!sharers.empty()) {
            for (L1Cache* sharer : sharers) {
                sharer->set_state(addr, CacheState::S, "WRITE update Firefly");
                stats.updates++;
                wf_updates = static_cast<sc_dt::sc_uint<64>>(stats.updates);
            }
            requester->set_state(addr, CacheState::S, "WRITE compartido Firefly");
        } else {
            requester->set_state(addr, CacheState::M, "WRITE privado Firefly");
        }
    }

    std::uint64_t total_latency = timing.bus_latency_ns + timing.coherence_latency_ns;
    if (sharers.empty()) {
        total_latency += timing.memory_latency_ns;
        wf_shared_access = 0;
    } else {
        total_latency += timing.shared_latency_ns;
        wf_shared_access = 1;
    }
    wait(static_cast<double>(total_latency), SC_NS);
}

void Interconnect::print_stats() const {
    std::cout << "\n=== ESTADISTICAS INTERCONNECT (" << protocol_to_string(protocol) << ") ===" << std::endl;
    std::cout << "Transacciones totales: " << stats.total << std::endl;
    std::cout << "Lecturas: " << stats.reads << std::endl;
    std::cout << "Escrituras: " << stats.writes << std::endl;
    std::cout << "Invalidaciones: " << stats.invalidations << std::endl;
    std::cout << "Updates: " << stats.updates << std::endl;
    std::cout << "Downgrades M->S: " << stats.downgrades_m_to_s << std::endl;
}
