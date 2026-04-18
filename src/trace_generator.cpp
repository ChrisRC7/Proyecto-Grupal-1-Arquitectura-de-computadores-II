#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

void generarMultiplicacion(int num_pes) {
    std::ofstream archivo("traces/trace_matmul.txt");
    archivo << "# Trace: Multiplicacion de Matrices Bloqueada\n";
    for (int i = 0; i < 10; ++i) {
        for (int pe = 0; pe < num_pes; ++pe) {
            archivo << pe << " READ " << std::hex << 0x1000 + (pe * 4) << "\n";
            archivo << pe << " WRITE " << std::hex << 0x2000 << "\n";
        }
    }
    archivo.close();
    std::cout << "Generado: traces/trace_matmul.txt\n";
}

void generarPipeline(int num_pes) {
    std::ofstream archivo("traces/trace_pipeline.txt");
    archivo << "# Trace: Pipeline Criptografia (Productor-Consumidor)\n";
    for (int i = 0; i < 5; ++i) {
        int base_addr = 0x5000 + (i * 0x10);
        archivo << "0 WRITE " << std::hex << base_addr << "\n"; // PE0 escribe (Produce)
        archivo << "1 READ "  << std::hex << base_addr << "\n"; // PE1 lee (Consume) y modifica
        archivo << "1 WRITE " << std::hex << base_addr << "\n";
        archivo << "2 READ "  << std::hex << base_addr << "\n"; // PE2 lee
    }
    archivo.close();
    std::cout << "Generado: traces/trace_pipeline.txt\n";
}

void generarMigratorio() {
    std::ofstream archivo("traces/trace_migratorio.txt");
    archivo << "# Trace: Patron Migratorio (token compartido)\n";

    int token_addr = 0x7000;
    for (int ronda = 0; ronda < 8; ++ronda) {
        for (int pe = 0; pe < 4; ++pe) {
            archivo << pe << " READ " << std::hex << token_addr << "\n";
            archivo << pe << " WRITE " << std::hex << token_addr << "\n";
        }
    }

    archivo.close();
    std::cout << "Generado: traces/trace_migratorio.txt\n";
}

int main() {
    std::filesystem::create_directories("traces");

    int PEs = 4;
    generarMultiplicacion(PEs);
    generarPipeline(PEs);
    generarMigratorio();
    return 0;
}