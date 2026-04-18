#ifndef SIM_L1_CACHE_H
#define SIM_L1_CACHE_H

#include <systemc.h>

#include <cstdint>
#include <string>
#include <unordered_map>

#include "types.h"

class Interconnect;

class L1Cache : public sc_module {
public:
    sc_in<bool> clk;

    int cache_id;
    Interconnect* bus;
    std::unordered_map<std::uint64_t, CacheState> lines;

    std::uint64_t read_hits = 0;
    std::uint64_t read_misses = 0;
    std::uint64_t write_hits = 0;
    std::uint64_t write_misses = 0;

    L1Cache(sc_module_name name, int id);

    bool has_copy(std::uint64_t addr) const;
    CacheState get_state(std::uint64_t addr) const;
    void set_state(std::uint64_t addr, CacheState next_state, const std::string& reason);
    void access(OpType op, std::uint64_t addr);
    void print_stats() const;
};

#endif
