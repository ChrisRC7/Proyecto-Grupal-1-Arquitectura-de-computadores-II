#ifndef SIM_TYPES_H
#define SIM_TYPES_H

#include <cstdint>
#include <string>

enum class OpType { READ, WRITE };
enum class CacheState { I, S, M };
enum class Protocol { MSI, FIREFLY };
enum class ReplacementPolicy { LRU, FIFO };

struct BusStats {
    std::uint64_t total = 0;
    std::uint64_t reads = 0;          // BusRead
    std::uint64_t writes = 0;         // total de escrituras al bus
    std::uint64_t bus_read_x = 0;     // I -> M
    std::uint64_t bus_upgrades = 0;   // S -> M
    std::uint64_t bus_updates = 0;    // Firefly updates
    std::uint64_t invalidations = 0;
    std::uint64_t updates = 0;
    std::uint64_t downgrades_m_to_s = 0;
};

std::string state_to_string(CacheState state);
std::string protocol_to_string(Protocol protocol);
std::string replacement_policy_to_string(ReplacementPolicy policy);
OpType parse_op(const std::string& op);
std::uint64_t parse_addr_hex(const std::string& addr);
Protocol parse_protocol_arg(const std::string& protocol_text);
ReplacementPolicy parse_replacement_policy_arg(const std::string& policy_text);

enum class BusTransaction {
    BusRead,       // I -> S: leer bloque sin copia local
    BusReadX,      // I -> M: leer bloque con intencion de escribir
    BusUpgrade,    // S -> M: ya tengo copia, solo invalida a los demas (MSI)
    BusUpdate,     // Firefly: propagar escritura a todas las copias
    BusInvalidate  // Invalidar copias remotas (usado internamente por el bus)
};
std::string transaction_to_string(BusTransaction tx);



#endif
