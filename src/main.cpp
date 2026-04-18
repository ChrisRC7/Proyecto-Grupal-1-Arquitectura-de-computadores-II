#include <systemc.h>

#include <iostream>
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

    sc_clock clk("clk", 1, SC_NS);
    Interconnect bus("Global_Interconnect", protocol);
    bus.clk(clk);

    L1Cache* caches[4];
    ProcessingElement* pes[4];

    for (int i = 0; i < 4; i++) {
        std::string cache_name = "L1_" + std::to_string(i);
        caches[i] = new L1Cache(cache_name.c_str(), i);
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

    sc_start(1000, SC_NS);

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