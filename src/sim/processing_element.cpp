#include "processing_element.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "l1_cache.h"
#include "types.h"

ProcessingElement::ProcessingElement(sc_module_name name, int id, const std::string& trace)
    : sc_module(name), pe_id(id), trace_file(trace), local_cache(nullptr) {
    SC_THREAD(process_trace);
    sensitive << clk.pos();
}

void ProcessingElement::process_trace() {
    std::ifstream file(trace_file);
    if (!file.is_open()) {
        std::cerr << "Error: no se pudo abrir " << trace_file << " para PE" << pe_id << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        int target_pe = -1;
        std::string op;
        std::string addr;
        ss >> target_pe >> op >> addr;

        if (target_pe != pe_id) {
            continue;
        }

        wait(1, SC_NS);
        local_cache->access(parse_op(op), parse_addr_hex(addr));
    }
}
