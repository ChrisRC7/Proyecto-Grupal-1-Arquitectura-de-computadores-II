#ifndef SIM_L1_CACHE_H
#define SIM_L1_CACHE_H

#include <systemc.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "types.h"

class Interconnect;

class L1Cache : public sc_module {
public:
    struct CacheConfig {
        std::size_t size_bytes = 1024;
        std::size_t block_size_bytes = 64;
        std::size_t associativity = 2;
        ReplacementPolicy replacement_policy = ReplacementPolicy::LRU;
        std::uint64_t l1_hit_latency_ns = 1;
        std::uint64_t l1_miss_detect_latency_ns = 1;
    };

    sc_in<bool> clk;

    // Senales para waveform VCD
    sc_dt::sc_uint<64> wf_last_addr = 0;
    sc_dt::sc_uint<2> wf_last_op = 0;  // 0=none, 1=READ, 2=WRITE
    sc_dt::sc_uint<2> wf_last_state = 0;  // 0=I, 1=S, 2=M
    sc_dt::sc_uint<64> wf_read_hits = 0;
    sc_dt::sc_uint<64> wf_read_misses = 0;
    sc_dt::sc_uint<64> wf_write_hits = 0;
    sc_dt::sc_uint<64> wf_write_misses = 0;
    sc_dt::sc_uint<64> wf_evictions = 0;
    sc_dt::sc_uint<64> wf_conflict_misses = 0;

    int cache_id;
    Interconnect* bus;

    std::size_t num_sets = 1;
    CacheConfig config;

    std::uint64_t read_hits = 0;
    std::uint64_t read_misses = 0;
    std::uint64_t write_hits = 0;
    std::uint64_t write_misses = 0;
    std::uint64_t evictions = 0;
    std::uint64_t conflict_misses = 0;

    L1Cache(sc_module_name name, int id, const CacheConfig& cache_config);

    bool has_copy(std::uint64_t addr) const;
    CacheState get_state(std::uint64_t addr) const;
    void set_state(std::uint64_t addr, CacheState next_state, const std::string& reason);
    void access(OpType op, std::uint64_t addr);
    void print_stats() const;

private:
    struct CacheLine {
        bool valid = false;
        std::uint64_t tag = 0;
        CacheState state = CacheState::I;
        std::uint64_t inserted_at = 0;
        std::uint64_t last_access_at = 0;
    };

    std::vector<std::vector<CacheLine>> sets;
    std::uint64_t access_counter = 0;

    std::uint64_t to_block_addr(std::uint64_t addr) const;
    std::size_t set_index_for_block(std::uint64_t block_addr) const;
    std::uint64_t tag_for_block(std::uint64_t block_addr) const;

    const CacheLine* find_line(std::size_t set_index, std::uint64_t tag) const;
    CacheLine* find_line(std::size_t set_index, std::uint64_t tag);

    CacheLine* allocate_line(std::size_t set_index, std::uint64_t tag, std::uint64_t block_addr,
                             bool count_conflict_miss);
    std::size_t victim_index(std::size_t set_index) const;
    std::uint64_t block_addr_from_line(std::size_t set_index, const CacheLine& line) const;
    void touch_line(CacheLine& line);
};

#endif
