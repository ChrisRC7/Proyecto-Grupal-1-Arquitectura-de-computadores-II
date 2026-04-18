#include <systemc.h>

// 1. Módulo PE (Trace Player)
SC_MODULE(ProcessingElement) {
    sc_in<bool> clk;
    int pe_id;

    void process_traces() {
        while (true) {
            // Aquí irá la lógica para leer el .txt y enviar transacciones TLM
            wait();
        }
    }

    SC_HAS_PROCESS(ProcessingElement);
    ProcessingElement(sc_module_name name, int id) : sc_module(name), pe_id(id) {
        SC_THREAD(process_traces);
        sensitive << clk.pos();
    }
};

// 2. Módulo Caché L1 (Base para Demo 2)
SC_MODULE(L1Cache) {
    sc_in<bool> clk;
    // Aquí deberán investigar y definir el tamaño, bloque y asociatividad
    
    SC_CTOR(L1Cache) {
        // Constructor
    }
};

// 3. Módulo Interconnect (Cache Coherent Interconnect)
SC_MODULE(Interconnect) {
    sc_in<bool> clk;
    // Aquí implementarán el arbitraje y contadores de desempeño
    
    SC_CTOR(Interconnect) {
        // Constructor
    }
};

// Top-Level: Uniendo todo
int sc_main(int argc, char* argv[]) {
    sc_clock clk("clk", 10, SC_NS); // Reloj del sistema a 100MHz

    // Instanciación de los 4 PEs y sus Cachés L1
    ProcessingElement* pe[4];
    L1Cache* l1[4];
    
    for (int i = 0; i < 4; i++) {
        std::string pe_name = "PE_" + std::to_string(i);
        pe[i] = new ProcessingElement(pe_name.c_str(), i);
        pe[i]->clk(clk);

        std::string l1_name = "L1_" + std::to_string(i);
        l1[i] = new L1Cache(l1_name.c_str());
        l1[i]->clk(clk);
    }

    // Instanciación del Interconnect
    Interconnect bus("Interconnect_System");
    bus.clk(clk);

    cout << "Iniciando simulador con 4 PEs..." << endl;
    sc_start(1000, SC_NS); // Simular 1000 nanosegundos
    cout << "Simulacion terminada." << endl;

    // Limpieza de memoria
    for (int i = 0; i < 4; i++) {
        delete pe[i];
        delete l1[i];
    }

    return 0;
}