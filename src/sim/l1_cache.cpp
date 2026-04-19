#include "l1_cache.h"

#include <algorithm>
#include <iostream>

#include "interconnect.h"

L1Cache::L1Cache(sc_module_name name, int id, const CacheConfig& cache_config)
    : sc_module(name), cache_id(id), bus(nullptr), config(cache_config) {
    std::size_t safe_block = std::max<std::size_t>(1, config.block_size_bytes);
    std::size_t safe_assoc = std::max<std::size_t>(1, config.associativity);
    std::size_t num_lines = std::max<std::size_t>(1, config.size_bytes / safe_block);
    if (num_lines < safe_assoc) {
        num_lines = safe_assoc;
    }

    num_sets = std::max<std::size_t>(1, num_lines / safe_assoc);
    sets.assign(num_sets, std::vector<CacheLine>(safe_assoc));

    wf_last_addr = 0;
    wf_last_op = 0;
    wf_last_state = 0;
    wf_read_hits = 0;
    wf_read_misses = 0;
    wf_write_hits = 0;
    wf_write_misses = 0;
    wf_evictions = 0;
    wf_conflict_misses = 0;
}

std::uint64_t L1Cache::to_block_addr(std::uint64_t addr) const {
    std::uint64_t block_size = static_cast<std::uint64_t>(std::max<std::size_t>(1, config.block_size_bytes));
    return addr / block_size;
}

std::size_t L1Cache::set_index_for_block(std::uint64_t block_addr) const {
    return static_cast<std::size_t>(block_addr % static_cast<std::uint64_t>(num_sets));
}

std::uint64_t L1Cache::tag_for_block(std::uint64_t block_addr) const {
    return block_addr / static_cast<std::uint64_t>(num_sets);
}

const L1Cache::CacheLine* L1Cache::find_line(std::size_t set_index, std::uint64_t tag) const {
    const std::vector<CacheLine>& set = sets[set_index];
    for (const CacheLine& line : set) {
        if (line.valid && line.tag == tag) {
            return &line;
        }
    }
    return nullptr;
}

L1Cache::CacheLine* L1Cache::find_line(std::size_t set_index, std::uint64_t tag) {
    std::vector<CacheLine>& set = sets[set_index];
    for (CacheLine& line : set) {
        if (line.valid && line.tag == tag) {
            return &line;
        }
    }
    return nullptr;
}

std::size_t L1Cache::victim_index(std::size_t set_index) const {
    const std::vector<CacheLine>& set = sets[set_index];

    for (std::size_t i = 0; i < set.size(); ++i) {
        if (!set[i].valid) {
            return i;
        }
    }

    std::size_t victim = 0;
    std::uint64_t best_metric =
        (config.replacement_policy == ReplacementPolicy::FIFO) ? set[0].inserted_at : set[0].last_access_at;

    for (std::size_t i = 1; i < set.size(); ++i) {
        std::uint64_t candidate_metric =
            (config.replacement_policy == ReplacementPolicy::FIFO) ? set[i].inserted_at : set[i].last_access_at;
        if (candidate_metric < best_metric) {
            best_metric = candidate_metric;
            victim = i;
        }
    }

    return victim;
}

std::uint64_t L1Cache::block_addr_from_line(std::size_t set_index, const CacheLine& line) const {
    return line.tag * static_cast<std::uint64_t>(num_sets) + static_cast<std::uint64_t>(set_index);
}

void L1Cache::touch_line(CacheLine& line) {
    access_counter++;
    line.last_access_at = access_counter;
    if (!line.valid) {
        line.inserted_at = access_counter;
    }
}

L1Cache::CacheLine* L1Cache::allocate_line(std::size_t set_index, std::uint64_t tag,
                                           std::uint64_t block_addr, bool count_conflict_miss) {
    std::vector<CacheLine>& set = sets[set_index];
    std::size_t index = victim_index(set_index);
    CacheLine& victim = set[index];

    if (victim.valid && victim.state != CacheState::I) {
        std::uint64_t evicted_block_addr = block_addr_from_line(set_index, victim);
        std::uint64_t evicted_addr =
            evicted_block_addr * static_cast<std::uint64_t>(std::max<std::size_t>(1, config.block_size_bytes));
        evictions++;
        wf_evictions = static_cast<sc_dt::sc_uint<64>>(evictions);
        if (count_conflict_miss) {
            conflict_misses++;
            wf_conflict_misses = static_cast<sc_dt::sc_uint<64>>(conflict_misses);
        }
        std::cout << "@" << sc_time_stamp() << " | L1_" << cache_id << " : "
                  << "0x" << std::hex << evicted_addr << std::dec << " "
                  << state_to_string(victim.state) << " -> I"
                  << " (Eviccion " << replacement_policy_to_string(config.replacement_policy) << ")"
                  << std::endl;
    }

    access_counter++;
    victim.valid = true;
    victim.tag = tag;
    victim.state = CacheState::I;
    victim.inserted_at = access_counter;
    victim.last_access_at = access_counter;
    (void)block_addr;
    return &victim;
}

bool L1Cache::has_copy(std::uint64_t addr) const {
    std::uint64_t block_addr = to_block_addr(addr);
    std::size_t set_index = set_index_for_block(block_addr);
    std::uint64_t tag = tag_for_block(block_addr);
    const CacheLine* line = find_line(set_index, tag);
    return line != nullptr && line->state != CacheState::I;
}

CacheState L1Cache::get_state(std::uint64_t addr) const {
    std::uint64_t block_addr = to_block_addr(addr);
    std::size_t set_index = set_index_for_block(block_addr);
    std::uint64_t tag = tag_for_block(block_addr);
    const CacheLine* line = find_line(set_index, tag);
    if (line == nullptr) {
        return CacheState::I;
    }
    return line->state;
}

void L1Cache::set_state(std::uint64_t addr, CacheState next_state, const std::string& reason) {
    std::uint64_t block_addr = to_block_addr(addr);
    std::size_t set_index = set_index_for_block(block_addr);
    std::uint64_t tag = tag_for_block(block_addr);

    CacheLine* line = find_line(set_index, tag);
    if (line == nullptr) {
        if (next_state == CacheState::I) {
            return;
        }
        line = allocate_line(set_index, tag, block_addr, false);
    }

    touch_line(*line);

    CacheState previous = get_state(addr);
    line->state = next_state;
    if (next_state == CacheState::I) {
        wf_last_state = 0;
    } else if (next_state == CacheState::S) {
        wf_last_state = 1;
    } else {
        wf_last_state = 2;
    }
    if (previous != next_state) {
        std::cout << "@" << sc_time_stamp() << " | L1_" << cache_id << " : "
                  << "0x" << std::hex << addr << std::dec << " "
                  << state_to_string(previous) << " -> " << state_to_string(next_state)
                  << " (" << reason << ")" << std::endl;
    }
}

void L1Cache::access(OpType op, std::uint64_t addr) {
    wf_last_addr = static_cast<sc_dt::sc_uint<64>>(addr);
    wf_last_op = op == OpType::READ ? 1 : 2;

    CacheState current = get_state(addr);
    bool has_local_copy = current != CacheState::I;

    if (op == OpType::READ) {
        if (has_local_copy) {
            read_hits++;
            wf_read_hits = static_cast<sc_dt::sc_uint<64>>(read_hits);
            wait(static_cast<double>(config.l1_hit_latency_ns), SC_NS);
            return;
        }
        read_misses++;
        wf_read_misses = static_cast<sc_dt::sc_uint<64>>(read_misses);
        std::uint64_t block_addr = to_block_addr(addr);
        std::size_t set_index = set_index_for_block(block_addr);
        std::uint64_t tag = tag_for_block(block_addr);
        if (find_line(set_index, tag) == nullptr) {
            std::vector<CacheLine>& set = sets[set_index];
            bool set_full = true;
            for (const CacheLine& line : set) {
                if (!line.valid) {
                    set_full = false;
                    break;
                }
            }

            if (set_full) {
                allocate_line(set_index, tag, block_addr, true);
            }
        }
        wait(static_cast<double>(config.l1_miss_detect_latency_ns), SC_NS);
        bus->handle_read(cache_id, addr);
        return;
    }

    if (current == CacheState::M) {
        write_hits++;
        wf_write_hits = static_cast<sc_dt::sc_uint<64>>(write_hits);
        wait(static_cast<double>(config.l1_hit_latency_ns), SC_NS);
        return;
    }

    if (has_local_copy) {
        write_hits++;
        wf_write_hits = static_cast<sc_dt::sc_uint<64>>(write_hits);
        wait(static_cast<double>(config.l1_hit_latency_ns), SC_NS);
    } else {
        write_misses++;
        wf_write_misses = static_cast<sc_dt::sc_uint<64>>(write_misses);
        std::uint64_t block_addr = to_block_addr(addr);
        std::size_t set_index = set_index_for_block(block_addr);
        std::uint64_t tag = tag_for_block(block_addr);
        if (find_line(set_index, tag) == nullptr) {
            std::vector<CacheLine>& set = sets[set_index];
            bool set_full = true;
            for (const CacheLine& line : set) {
                if (!line.valid) {
                    set_full = false;
                    break;
                }
            }

            if (set_full) {
                allocate_line(set_index, tag, block_addr, true);
            }
        }
        wait(static_cast<double>(config.l1_miss_detect_latency_ns), SC_NS);
    }
    bus->handle_write(cache_id, addr);
}

void L1Cache::print_stats() const {
    std::cout << "L1_" << cache_id << " | RH=" << read_hits << " RM=" << read_misses
              << " WH=" << write_hits << " WM=" << write_misses << " EV=" << evictions
              << " CM=" << conflict_misses
              << " | cfg(size=" << config.size_bytes << "B, block=" << config.block_size_bytes
              << "B, ways=" << config.associativity
              << ", repl=" << replacement_policy_to_string(config.replacement_policy)
              << ", l1_hit=" << config.l1_hit_latency_ns << "ns"
              << ", l1_miss=" << config.l1_miss_detect_latency_ns << "ns"
              << ")"
              << std::endl;
}
