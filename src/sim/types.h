#ifndef SIM_TYPES_H
#define SIM_TYPES_H

#include <cstdint>
#include <string>

enum class OpType { READ, WRITE };
enum class CacheState { I, S, M };
enum class Protocol { MSI, FIREFLY };

struct BusStats {
    std::uint64_t total = 0;
    std::uint64_t reads = 0;
    std::uint64_t writes = 0;
    std::uint64_t invalidations = 0;
    std::uint64_t updates = 0;
    std::uint64_t downgrades_m_to_s = 0;
};

std::string state_to_string(CacheState state);
std::string protocol_to_string(Protocol protocol);
OpType parse_op(const std::string& op);
std::uint64_t parse_addr_hex(const std::string& addr);
Protocol parse_protocol_arg(const std::string& protocol_text);

#endif
