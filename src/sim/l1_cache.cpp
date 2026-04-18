#include "l1_cache.h"

#include <iostream>

#include "interconnect.h"

L1Cache::L1Cache(sc_module_name name, int id) : sc_module(name), cache_id(id), bus(nullptr) {}

bool L1Cache::has_copy(std::uint64_t addr) const {
    auto it = lines.find(addr);
    return it != lines.end() && it->second != CacheState::I;
}

CacheState L1Cache::get_state(std::uint64_t addr) const {
    auto it = lines.find(addr);
    if (it == lines.end()) {
        return CacheState::I;
    }
    return it->second;
}

void L1Cache::set_state(std::uint64_t addr, CacheState next_state, const std::string& reason) {
    CacheState previous = get_state(addr);
    lines[addr] = next_state;
    if (previous != next_state) {
        std::cout << "@" << sc_time_stamp() << " | L1_" << cache_id << " : "
                  << "0x" << std::hex << addr << std::dec << " "
                  << state_to_string(previous) << " -> " << state_to_string(next_state)
                  << " (" << reason << ")" << std::endl;
    }
}

void L1Cache::access(OpType op, std::uint64_t addr) {
    CacheState current = get_state(addr);
    bool has_local_copy = current != CacheState::I;

    if (op == OpType::READ) {
        if (has_local_copy) {
            read_hits++;
            return;
        }
        read_misses++;
        bus->handle_read(cache_id, addr);
        return;
    }

    if (current == CacheState::M) {
        write_hits++;
        return;
    }

    if (has_local_copy) {
        write_hits++;
    } else {
        write_misses++;
    }
    bus->handle_write(cache_id, addr);
}

void L1Cache::print_stats() const {
    std::cout << "L1_" << cache_id << " | RH=" << read_hits << " RM=" << read_misses
              << " WH=" << write_hits << " WM=" << write_misses << std::endl;
}
