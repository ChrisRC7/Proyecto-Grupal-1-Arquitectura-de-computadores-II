#ifndef SIM_PROCESSING_ELEMENT_H
#define SIM_PROCESSING_ELEMENT_H

#include <systemc.h>

#include <string>

class L1Cache;

class ProcessingElement : public sc_module {
public:
    sc_in<bool> clk;

    int pe_id;
    std::string trace_file;
    L1Cache* local_cache;

    void process_trace();

    SC_HAS_PROCESS(ProcessingElement);
    ProcessingElement(sc_module_name name, int id, const std::string& trace);
};

#endif
