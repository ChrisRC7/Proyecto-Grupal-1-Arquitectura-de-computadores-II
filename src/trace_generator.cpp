#include <iostream>
#include <fstream>
#include <string>

// Formato de salida: [PE_ID] [OPERACION] [DIRECCION_HEX] [DATO]
void generarMultiplicacionBasica(int num_pes) {
    std::ofstream archivo("trace_matmul.txt");
    if (!archivo.is_open()) return;

    archivo << "# Trace: Multiplicacion de Matrices Bloqueada (Alta Contención)\n";
    
    // Generar un patrón falso de accesos intercalados para forzar contención
    for (int i = 0; i < 20; ++i) {
        for (int pe = 0; pe < num_pes; ++pe) {
            // Todos intentan leer direcciones muy cercanas (posible False Sharing)
            archivo << pe << " READ " << std::hex << 0x1000 + (pe * 4) << " 0\n";
            // Escritura en una dirección compartida
            archivo << pe << " WRITE " << std::hex << 0x2000 << " " << std::dec << (i * pe) << "\n";
        }
    }
    archivo.close();
    std::cout << "Trace generado: trace_matmul.txt\n";
}

int main() {
    // El sistema base debe simular un mínimo de 4 PEs
    int PEs = 4; 
    generarMultiplicacionBasica(PEs);
    return 0;
}