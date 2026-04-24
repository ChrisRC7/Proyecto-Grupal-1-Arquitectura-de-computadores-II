#ifndef SIM_INTERCONNECT_H
#define SIM_INTERCONNECT_H

#include <systemc.h>

#include <cstdint>
#include <vector>

#include "types.h"

class L1Cache;

class Interconnect : public sc_module {
public:
    struct TimingConfig {
        std::uint64_t bus_latency_ns = 1;
        std::uint64_t memory_latency_ns = 5;
        std::uint64_t shared_latency_ns = 2;
        std::uint64_t coherence_latency_ns = 2;
    };

    sc_in<bool> clk;

    // Senales para waveform VCD
    sc_dt::sc_uint<64> wf_last_addr = 0;
    sc_dt::sc_uint<2>  wf_last_op = 0;        // 0=none, 1=READ, 2=WRITE
    sc_dt::sc_uint<3>  wf_last_tx_type = 0;   // 0=none,1=BusRead,2=BusReadX,3=BusUpgrade,4=BusUpdate,5=BusInvalidate
    sc_dt::sc_uint<3>  wf_requester_id = 0;
    sc_dt::sc_uint<64> wf_total_tx = 0;
    sc_dt::sc_uint<64> wf_invalidations = 0;
    sc_dt::sc_uint<64> wf_updates = 0;
    sc_dt::sc_uint<1>  wf_shared_access = 0;

    Protocol protocol;
    TimingConfig timing;
    BusStats stats;
    std::vector<L1Cache*> caches;

    Interconnect(sc_module_name name, Protocol selected_protocol,
                 const TimingConfig& timing_config);

    void register_cache(L1Cache* cache);

    // Metodo principal: la cache dice exactamente que transaccion necesita
    void issue_transaction(BusTransaction tx, int requester_id, std::uint64_t addr);

    // Metodos internos para manejar cada tipo de transaccion
    void handle_read(int requester_id, std::uint64_t addr);
    void handle_write(int requester_id, std::uint64_t addr);

    void print_stats() const;

private:
    // Implementaciones internas por tipo de transaccion
    void do_bus_read(int requester_id, std::uint64_t addr);
    void do_bus_read_x(int requester_id, std::uint64_t addr);
    void do_bus_upgrade(int requester_id, std::uint64_t addr);
    void do_bus_update(int requester_id, std::uint64_t addr);

    void set_wf_tx_type(BusTransaction tx);
};

#endif