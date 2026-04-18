#include "types.h"

#include <algorithm>
#include <cctype>

std::string state_to_string(CacheState state) {
    switch (state) {
        case CacheState::I:
            return "I";
        case CacheState::S:
            return "S";
        case CacheState::M:
            return "M";
    }
    return "?";
}

std::string protocol_to_string(Protocol protocol) {
    return protocol == Protocol::MSI ? "MSI" : "Firefly";
}

OpType parse_op(const std::string& op) {
    return op == "READ" ? OpType::READ : OpType::WRITE;
}

std::uint64_t parse_addr_hex(const std::string& addr) {
    return static_cast<std::uint64_t>(std::stoull(addr, nullptr, 16));
}

Protocol parse_protocol_arg(const std::string& protocol_text) {
    std::string normalized = protocol_text;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (normalized == "firefly") {
        return Protocol::FIREFLY;
    }
    return Protocol::MSI;
}
