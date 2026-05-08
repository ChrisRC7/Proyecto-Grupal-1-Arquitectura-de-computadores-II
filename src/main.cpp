#include <systemc.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "sim/interconnect.h"
#include "sim/l1_cache.h"
#include "sim/processing_element.h"
#include "sim/types.h"

int sc_main(int argc, char* argv[]) {
    std::string trace_path = "traces/trace_matmul.txt";
    if (argc >= 2) {
        trace_path = argv[1];
    }

    Protocol protocol = Protocol::MSI;
    if (argc >= 3) {
        protocol = parse_protocol_arg(argv[2]);
    }

    L1Cache::CacheConfig l1_config;
    if (argc >= 4) {
        try {
            l1_config.size_bytes = static_cast<std::size_t>(std::stoull(argv[3]));
        } catch (const std::exception&) {
            std::cerr << "Advertencia: tamaño L1 invalido, usando default " << l1_config.size_bytes
                      << " bytes." << std::endl;
        }
    }
    if (argc >= 5) {
        try {
            l1_config.block_size_bytes = static_cast<std::size_t>(std::stoull(argv[4]));
        } catch (const std::exception&) {
            std::cerr << "Advertencia: tamaño de bloque invalido, usando default "
                      << l1_config.block_size_bytes << " bytes." << std::endl;
        }
    }
    if (argc >= 6) {
        try {
            l1_config.associativity = static_cast<std::size_t>(std::stoull(argv[5]));
        } catch (const std::exception&) {
            std::cerr << "Advertencia: asociatividad invalida, usando default "
                      << l1_config.associativity << "." << std::endl;
        }
    }
    if (argc >= 7) {
        l1_config.replacement_policy = parse_replacement_policy_arg(argv[6]);
    }
    if (argc >= 8) {
        try {
            l1_config.l1_hit_latency_ns = static_cast<std::uint64_t>(std::stoull(argv[7]));
        } catch (const std::exception&) {
            std::cerr << "Advertencia: latencia hit L1 invalida, usando default "
                      << l1_config.l1_hit_latency_ns << " ns." << std::endl;
        }
    }
    if (argc >= 9) {
        try {
            l1_config.l1_miss_detect_latency_ns = static_cast<std::uint64_t>(std::stoull(argv[8]));
        } catch (const std::exception&) {
            std::cerr << "Advertencia: latencia miss L1 invalida, usando default "
                      << l1_config.l1_miss_detect_latency_ns << " ns." << std::endl;
        }
    }

    Interconnect::TimingConfig interconnect_timing;
    if (argc >= 10) {
        try {
            interconnect_timing.bus_latency_ns = static_cast<std::uint64_t>(std::stoull(argv[9]));
        } catch (const std::exception&) {
            std::cerr << "Advertencia: latencia bus invalida, usando default "
                      << interconnect_timing.bus_latency_ns << " ns." << std::endl;
        }
    }
    if (argc >= 11) {
        try {
            interconnect_timing.memory_latency_ns = static_cast<std::uint64_t>(std::stoull(argv[10]));
        } catch (const std::exception&) {
            std::cerr << "Advertencia: latencia memoria invalida, usando default "
                      << interconnect_timing.memory_latency_ns << " ns." << std::endl;
        }
    }
    if (argc >= 12) {
        try {
            interconnect_timing.shared_latency_ns = static_cast<std::uint64_t>(std::stoull(argv[11]));
        } catch (const std::exception&) {
            std::cerr << "Advertencia: latencia compartida invalida, usando default "
                      << interconnect_timing.shared_latency_ns << " ns." << std::endl;
        }
    }
    if (argc >= 13) {
        try {
            interconnect_timing.coherence_latency_ns = static_cast<std::uint64_t>(std::stoull(argv[12]));
        } catch (const std::exception&) {
            std::cerr << "Advertencia: latencia coherencia invalida, usando default "
                      << interconnect_timing.coherence_latency_ns << " ns." << std::endl;
        }
    }

    std::string waveform_base = "results/waveforms/sim_wave";
    if (argc >= 14) {
        waveform_base = argv[13];
    }

    if (waveform_base.size() > 4 && waveform_base.substr(waveform_base.size() - 4) == ".vcd") {
        waveform_base = waveform_base.substr(0, waveform_base.size() - 4);
    }

    std::filesystem::path waveform_path(waveform_base);
    std::filesystem::path waveform_dir = waveform_path.parent_path();
    if (!waveform_dir.empty()) {
        std::filesystem::create_directories(waveform_dir);
    }

    sc_clock clk("clk", 1, SC_NS);
    Interconnect bus("Global_Interconnect", protocol, interconnect_timing);
    bus.clk(clk);

    L1Cache* caches[4];
    ProcessingElement* pes[4];

    for (int i = 0; i < 4; i++) {
        std::string cache_name = "L1_" + std::to_string(i);
        caches[i] = new L1Cache(cache_name.c_str(), i, l1_config);
        caches[i]->clk(clk);
        caches[i]->bus = &bus;
        bus.register_cache(caches[i]);

        std::string pe_name = "PE_" + std::to_string(i);
        pes[i] = new ProcessingElement(pe_name.c_str(), i, trace_path);
        pes[i]->clk(clk);
        pes[i]->local_cache = caches[i];
    }

    std::cout << "--- Iniciando Simulacion HW con 4 PEs ---" << std::endl;
    std::cout << "Trace: " << trace_path << std::endl;
    std::cout << "Protocolo: " << protocol_to_string(protocol) << std::endl;
    std::cout << "L1 config: size=" << l1_config.size_bytes << "B, block="
              << l1_config.block_size_bytes << "B, ways=" << l1_config.associativity
              << ", replacement=" << replacement_policy_to_string(l1_config.replacement_policy)
              << ", l1_hit=" << l1_config.l1_hit_latency_ns << "ns"
              << ", l1_miss=" << l1_config.l1_miss_detect_latency_ns << "ns"
              << std::endl;
    std::cout << "Interconnect timing: bus=" << interconnect_timing.bus_latency_ns
              << "ns, mem=" << interconnect_timing.memory_latency_ns
              << "ns, shared=" << interconnect_timing.shared_latency_ns
              << "ns, coherence=" << interconnect_timing.coherence_latency_ns
              << std::endl;

    sc_trace_file* vcd = sc_create_vcd_trace_file(waveform_base.c_str());
    if (vcd != nullptr) {
        vcd->set_time_unit(1, SC_PS);
        sc_trace(vcd, clk, "clk");

        sc_trace(vcd, bus.wf_last_addr, "bus_last_addr");
        sc_trace(vcd, bus.wf_last_op, "bus_last_op");
        sc_trace(vcd, bus.wf_last_tx_type, "bus_last_tx_type");
        sc_trace(vcd, bus.wf_requester_id, "bus_requester_id");
        sc_trace(vcd, bus.wf_total_tx, "bus_total_tx");
        sc_trace(vcd, bus.wf_invalidations, "bus_invalidations");
        sc_trace(vcd, bus.wf_updates, "bus_updates");
        sc_trace(vcd, bus.wf_shared_access, "bus_shared_access");

        for (int i = 0; i < 4; i++) {
            const std::string prefix = "l1_" + std::to_string(i) + "_";
            sc_trace(vcd, caches[i]->wf_last_addr, prefix + "last_addr");
            sc_trace(vcd, caches[i]->wf_last_op, prefix + "last_op");
            sc_trace(vcd, caches[i]->wf_last_state, prefix + "last_state");
            sc_trace(vcd, caches[i]->wf_read_hits, prefix + "read_hits");
            sc_trace(vcd, caches[i]->wf_read_misses, prefix + "read_misses");
            sc_trace(vcd, caches[i]->wf_write_hits, prefix + "write_hits");
            sc_trace(vcd, caches[i]->wf_write_misses, prefix + "write_misses");
            sc_trace(vcd, caches[i]->wf_evictions, prefix + "evictions");
            sc_trace(vcd, caches[i]->wf_conflict_misses, prefix + "conflict_misses");
        }
    }

    sc_start(1000, SC_NS);

    if (vcd != nullptr) {
        sc_close_vcd_trace_file(vcd);
        std::cout << "Waveform VCD: " << waveform_base << ".vcd" << std::endl;
    }

    std::cout << "--- Simulacion Finalizada ---" << std::endl;
    bus.print_stats();
    std::cout << "\n=== ESTADISTICAS L1 ===" << std::endl;
    for (int i = 0; i < 4; i++) {
        caches[i]->print_stats();
    }

    for (int i = 0; i < 4; i++) {
        delete pes[i];
        delete caches[i];
    }

    return 0;
}