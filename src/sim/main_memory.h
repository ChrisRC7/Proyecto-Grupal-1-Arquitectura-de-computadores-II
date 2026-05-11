#ifndef SIM_MAIN_MEMORY_H
#define SIM_MAIN_MEMORY_H

#include <systemc.h>

#include <cstdint>

class MainMemory : public sc_module {
public:
    sc_in<bool> clk;

    explicit MainMemory(sc_module_name name, std::uint64_t latency_ns = 5)
        : sc_module(name), latency_ns_(latency_ns) {}

    void set_latency_ns(std::uint64_t latency_ns) { latency_ns_ = latency_ns; }

    // Modelo simple: una "transaccion" a memoria cuesta latency_ns_.
    // Se invoca desde el contexto de un SC_THREAD (e.g., PE) via el Interconnect.
    void access(std::uint64_t /*addr*/, bool /*is_write*/) {
        wait(static_cast<double>(latency_ns_), SC_NS);
    }

private:
    std::uint64_t latency_ns_;
};

#endif
