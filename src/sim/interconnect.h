#ifndef SIM_INTERCONNECT_H
#define SIM_INTERCONNECT_H

#include <systemc.h>

#include <cstdint>
#include <vector>

#include "types.h"

class L1Cache;

class Interconnect : public sc_module {
public:
    sc_in<bool> clk;

    Protocol protocol;
    BusStats stats;
    std::vector<L1Cache*> caches;

    Interconnect(sc_module_name name, Protocol selected_protocol);

    void register_cache(L1Cache* cache);
    void handle_read(int requester_id, std::uint64_t addr);
    void handle_write(int requester_id, std::uint64_t addr);
    void print_stats() const;
};

#endif
